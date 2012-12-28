#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include "struct.h"

char *fsfilename;
long free_cluster;//номер первого свободного кластера
long count_free_cluster;//количество свободных кластеров

//long fat[CLUSTER_COUNT_FAT];
struct File_Record root_catalog[ELEMENT_COUNT_IN_CATALOG];
struct File_Record temporary_catalog[ELEMENT_COUNT_IN_CATALOG];

//считывание количества свободных кластеров
long ReadFreeClusterCount()
{
    FILE *file;
    if((file=fopen(fsfilename,"rb"))==0)
    {
        return -1;
    }

    fseek(file, 0, SEEK_SET);
    fread(&count_free_cluster, sizeof(long), 1, file);
    fclose(file);

    return count_free_cluster;
}

//запись числа свободных кластеров
long WriteFreeClusterCount(long value)
{
    FILE *file;
    if((file=fopen(fsfilename,"r+b"))==0)
    {
        return -1;
    }

    fseek(file, 0, SEEK_SET);
    fwrite(&value, sizeof(value), 1, file);
    fclose(file);

    return 0;
}

//считывание номера первого свободного кластера
long ReadFreeCluster()
{
    FILE *file;
    if((file=fopen(fsfilename,"rb"))==0)
    {
        return -1;
    }

    fseek(file, sizeof(long), SEEK_SET);
    fread(&free_cluster, sizeof(long), 1, file);
    fclose(file);

    return free_cluster;
}

//запись номера первого свободного кластера
long WriteFreeCluster(long value)
{
    FILE *file;
    if((file=fopen(fsfilename,"r+b"))==0)
    {
        return -1;
    }

    fseek(file, sizeof(long), SEEK_SET);
    fwrite(&value, sizeof(value), 1, file);
    fclose(file);

    return 0;
}

//считывание file allocation table
/*long* ReadFAT()
{
    FILE *file;
    if((file=fopen(fsfilename,"rb"))==0)
    {
        return NULL;
    }

    fseek(file, sizeof(long)*2, SEEK_SET);
    fread(fat, sizeof(long), CLUSTER_COUNT_FAT, file);
    fclose(file);

    return fat;
}*/

long ReadValueFAT(long cluster_number)
{
	FILE *file;
    if((file=fopen(fsfilename,"rb"))==0)
    {
        return NULL;
    }
	long fat;

	fseek(file, sizeof(long)*(2 + cluster_number), SEEK_SET);
    fread(&fat, sizeof(long), 1, file);
    fclose(file);

	return fat;
}

long WriteValueFat(long cluster_number, long value)
{
	FILE *file;
    if((file=fopen(fsfilename,"r+b"))==0)
    {
        return -1;
    }

	fseek(file, sizeof(long)*(2+cluster_number), SEEK_SET);
	fwrite(&value, sizeof(long), 1, file);
    fclose(file);

    return 0;
}

//запись file allocation table
/*long WriteFAT(long fat[CLUSTER_COUNT_FAT])
{
    FILE *file;
    if((file=fopen(fsfilename,"r+b"))==0)
    {
        return -1;
    }

    fseek(file, sizeof(long)*2, SEEK_SET);
    fwrite(fat, sizeof(long), CLUSTER_COUNT_FAT, file);
    fclose(file);

    return 0;
}
*/

//считывание каталога
struct File_Record *ReadCatalog(long cluster_number)
{
    FILE *file;
    if((file=fopen(fsfilename,"rb"))==0)
    {
        return NULL;
    }

    struct File_Record catalog;
    fseek(file, sizeof(long)*(2+CLUSTER_COUNT_FAT)+CLUSTER_SIZE*cluster_number, SEEK_SET);
    fread(&catalog, sizeof(struct File_Record), ELEMENT_COUNT_IN_CATALOG, file);
    fclose(file);

    return &catalog;

}

//запись каталога
long WriteCatalog(struct File_Record cat, long cluster_number)
{
    FILE *file;
    if((file=fopen(fsfilename,"r+b"))==0)
    {
        return -1;
    }

    fseek(file, sizeof(long)*(2+CLUSTER_COUNT_FAT)+CLUSTER_SIZE*cluster_number, SEEK_SET);
    fwrite(&cat, sizeof(struct File_Record), ELEMENT_COUNT_IN_CATALOG, file);
    fclose(file);

    return 0;
}

long ReadCluster(long cluster_number, long size, void *buf)
{
    FILE *file;
    if((file=fopen(fsfilename,"rb"))==0)
    {
        return -1;
    }

    fseek(file, sizeof(long)*(2+CLUSTER_COUNT_FAT)+CLUSTER_SIZE*cluster_number, SEEK_SET);
    fread(buf, size, 1, file);
    fclose(file);

    return 0;
}

long WriteCluster(long cluster_number, long size, void *buf)
{
    FILE *file;
    if((file=fopen(fsfilename,"r+b"))==0)
    {
        return -1;
    }

    fseek(file, sizeof(long)*(2+CLUSTER_COUNT_FAT)+CLUSTER_SIZE*cluster_number, SEEK_SET);
    fwrite(buf, size, 1, file);
    fclose(file);

    return 0;
}

long Load(char *filename)
{
    FILE *file;
    if((file=fopen(filename,"rb"))==0)
    {
        return -1;
    }
    fsfilename = filename;

    fread(&count_free_cluster, sizeof(long), 1, file);
    fread(&free_cluster, sizeof(long), 1, file);
    //fread(fat, sizeof(long), CLUSTER_COUNT_FAT, file);
	root_catalog = ReadCatalog(0);
    fread(&root_catalog, sizeof(struct File_Record), ELEMENT_COUNT_IN_CATALOG, file);

    fclose(file);
    return 0;
}

//считать файл
int ReadFileFromClusters(void *buf, long first_cluster, long file_size)
{
    long current_cluster = first_cluster;
    long read_byte = 0;
    long byte_to_read = file_size;

    while(byte_to_read > 0)
    {
        if(current_cluster != EOC)
        {
            //if(byte_to_read >= CLUSTER_SIZE)
            //{
                if(ReadCluster(current_cluster, CLUSTER_SIZE, buf + read_byte) != -1)
                {
                    read_byte += CLUSTER_SIZE;
                    byte_to_read -= CLUSTER_SIZE;
					current_cluster = ReadValueFAT(current_cluster);
                }
            //}
        }
        else
        {
            if(ReadCluster(current_cluster, byte_to_read, buf + read_byte) != -1)
            {
                read_byte += byte_to_read;
                byte_to_read = 0;
            }
        }
    }

    return 0;
}

//посчитать количество необходимых для файла кластреров
long CalcCountNeededClusters(long size)
{
    return size % CLUSTER_SIZE == 0 ? size/CLUSTER_SIZE : size/CLUSTER_SIZE + 1;
}

long FindFreeCluster()
{
    long i;
    if (free_cluster == CLUSTER_COUNT_FAT -1)
    {
        i = 1;
    }
    else
    {
        i = free_cluster + 1;
    }

    for(; i < CLUSTER_COUNT_FAT; i++)
    {
		if(ReadValueFAT(i) == NOT_USED)
        {
            return free_cluster = i;
        }
    }

    for(i = 1; i < CLUSTER_COUNT_FAT; i++)
    {
		if(ReadValueFAT(i) == NOT_USED)
        {
            return free_cluster = i;
        }
    }

    //свободных кластеров нет
    return -1;
}

long WriteFileToClusters(void *buf, long size)
{
    //проверка на достаточность места в файловой системе
    if(CalcCountNeededClusters(size) > count_free_cluster)
    {
        return -1;
    }

    long written_byte = 0;
    long byte_to_write = size;
    long cluster_for_catalog = free_cluster;

    do
    {
        if(byte_to_write > CLUSTER_SIZE)
        {
            if(WriteCluster(free_cluster, CLUSTER_SIZE, buf + written_byte) != -1)
            {
                written_byte += CLUSTER_SIZE;
                byte_to_write -= CLUSTER_SIZE;
                FindFreeCluster();
            }
        }
        else
        {
            if(WriteCluster(free_cluster, byte_to_write, buf + written_byte) != -1)
            {
                written_byte += byte_to_write;
                byte_to_write = 0;
                FindFreeCluster();
            }
        }
    }
    while (byte_to_write != 0);

    return cluster_for_catalog;
}

long GetNumberFirstClusterByPath(const char *path)
{
	int pos = 0;
	long cluster_number;
	long start = 0;
    if(path[pos] == '/')
    {
		cluster_number = 0;
        start = pos+1;
        pos++;
    }
    else
        return -ENOENT;
    while(pos < strlen(path))
    {
        while(pos < strlen(path) && path[pos] != '/')
            pos++;
        char name[pos - start +1];
        strncpy(name, path+start, pos - start);
        name[pos - start] = '\0';
        start = pos+1;
        pos++;
}

int main()
{
    return 0;
}
