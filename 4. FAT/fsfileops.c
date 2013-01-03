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

long ReadValueFAT(long cluster_number)
{
    FILE *file;
    if((file=fopen(fsfilename,"rb"))==0)
    {
        return -1;
    }
    long fat;

    fseek(file, sizeof(long)*(2 + cluster_number), SEEK_SET);
    fread(&fat, sizeof(long), 1, file);
    fclose(file);

    return 0;
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

long FreeClusterIndex(long operation)
{
//    if (index < 1 || index >= CLUSTER_COUNT_FAT)
//        return -1;

    long count = ReadFreeClusterCount();

//    FILE *file;
//    if((file=fopen(fsfilename,"r+b"))==0)
//    {
//        return -1;
//    }

//    fseek(file, sizeof(long), SEEK_SET);
//    fwrite(&index, sizeof(index), 1, file);
//    fclose(file);
//    WriteFreeCluster(index);
    switch(operation)
    {
    case 1 : WriteFreeClusterCount(++count); break;
    case -1 : WriteFreeClusterCount(--count); break;
    }

    WriteFreeClusterCount(++count);

    return 0;
}

//поиск свободного кластера
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
long ReadCatalog(long cluster_number)
{
    FILE *file;
    if((file=fopen(fsfilename,"rb"))==0)
    {
        return -1;
    }

//    struct File_Record catalog[ELEMENT_COUNT_IN_CATALOG];
    fseek(file, sizeof(long)*(2+CLUSTER_COUNT_FAT)+CLUSTER_SIZE*cluster_number, SEEK_SET);
    fread(temporary_catalog, sizeof(struct File_Record [ELEMENT_COUNT_IN_CATALOG]), 1 , file);
    fclose(file);

    return 0;

}

//запись каталога
long WriteCatalog(struct File_Record cat[ELEMENT_COUNT_IN_CATALOG], long cluster_number)
{
    FILE *file;
    if((file=fopen(fsfilename,"r+b"))==0)
    {
        return -1;
    }

    fseek(file, sizeof(long)*(2+CLUSTER_COUNT_FAT)+CLUSTER_SIZE*cluster_number, SEEK_SET);
    fwrite(cat, sizeof(struct File_Record [ELEMENT_COUNT_IN_CATALOG]), 1, file);
    fclose(file);

/*    if (free_cluster = FindFreeCluster() < 0)//////////////////////////////////////////////////////////
    {
        printf("File is empty\n");
    }
    else
    {
        WriteFreeCluster(free_cluster);
        FreeClusterIndex(-1);///////////////////////////////////////////////////////////////
    }
*/
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
    //root_catalog = ReadCatalog(0);
    //fread(&root_catalog, sizeof(struct File_Record), ELEMENT_COUNT_IN_CATALOG, file);

    fclose(file);
    return 0;
}

//считать файл
long ReadFileFromClusters(void *buf, long first_cluster, long file_size)
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

                if (free_cluster = FindFreeCluster() < 0)
                {
                    printf("File is empty\n");
                }
                else
                {
                    WriteFreeCluster(free_cluster);
                    FreeClusterIndex(-1);//////////////////////////////////////////////////////////////////////////////
                }
            }
        }
        else
        {
            if(WriteCluster(free_cluster, byte_to_write, buf + written_byte) != -1)
            {
                written_byte += byte_to_write;
                byte_to_write = 0;

                if (free_cluster = FindFreeCluster() < 0)
                {
                    printf("File is empty\n");
                }
                else
                {
                    WriteFreeCluster(free_cluster);
                    FreeClusterIndex(-1);////////////////////////////////////////////////////////////////////
                }
            }
        }
    }
    while (byte_to_write != 0);

    return cluster_for_catalog;
}

long GetNumberFirstClusterByPath(const char *path)
{
    int pos = 0;
    int i = 0;
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

        if (ReadCatalog (cluster_number) != -1)
        {
            cluster_number = -ENOENT;
            for (i=0; i++; i<ELEMENT_COUNT_IN_CATALOG)
            {
                if(strcmp(temporary_catalog[i].name, name) == 0)
                {
                    cluster_number = temporary_catalog[i].first_cluster;
                    break;
                }
            }
        }
        else
        {
            return -EIO;
        }

        if (cluster_number == -ENOENT)
        {
            return -ENOENT;
        }
    }
    return cluster_number;
}

//при написании функции truncate не забыть изменить размер файла в каталоге!!!!
long TruncFile(long first_cluster_number, long offset, long size)
{
    long index = offset % CLUSTER_SIZE;
    index = index == 0 ? offset / CLUSTER_SIZE : offset / CLUSTER_SIZE + 1;
    long curr_cluster_number = first_cluster_number;

    long i = 0;
    while (++i != index)
    {
        curr_cluster_number = ReadValueFAT(curr_cluster_number);
    }

    //если кластер, после которого остальные должны освобождаться, не является последним
    if (curr_cluster_number != EOC)
    {
        long tmp = ReadValueFAT(curr_cluster_number);
        WriteValueFat(curr_cluster_number, EOC);
        curr_cluster_number = tmp;
        WriteFreeCluster(curr_cluster_number);

        while(curr_cluster_number != EOC)
        {
            //запомнили номер текущего кластера
            tmp = curr_cluster_number;
            //считали номер следующего
            curr_cluster_number = ReadValueFAT(curr_cluster_number);
            //отметили, что текущий кластер не занят
            WriteValueFat(tmp, NOT_USED);
            FreeClusterIndex(1);////////////////////////////////////////////////////////////////////////////////
        }
        WriteValueFat(curr_cluster_number, NOT_USED);
        FreeClusterIndex(1);///////////////////////////////////////////////////////////////////////////////////////
    }

    return 0;

}

int main()
{
/*
    fsfilename = "test.dat";   
    printf("%s\n", fsfilename);
    int i;

    printf("free cluster count \n");
    WriteFreeClusterCount(CLUSTER_COUNT_FAT-1);

    printf("free cluster \n");
    WriteFreeCluster(1);

    WriteValueFat(0, EOC);
    for (i=1; i<CLUSTER_COUNT_FAT; i++)
    {
        WriteValueFat(i, NOT_USED);
        printf("element fat: %d\n", i);
    }

    printf("root catalog \n");
    for (i=0; i<ELEMENT_COUNT_IN_CATALOG; i++)
    {
        int j=0;
        for (j=0; j<8; j++)
        {
            root_catalog[i].name[j] = 'a';
        }
        for (j=0; j<3; j++)
        {
            root_catalog[i].extension[j] = 'b';
        }

        root_catalog[i].first_cluster = 0;
        root_catalog[i].file_size = 0;
        root_catalog[i].ctime = time(NULL);
        root_catalog[i].mtime = time(NULL);
        root_catalog[i].atribute = '0';
        root_catalog[i].isCatalog = 0;
    }
    WriteCatalog(root_catalog, 0);

    char buf [CLUSTER_SIZE];
    buf[0]='b';
    for (i=1; i<CLUSTER_SIZE; i++)
    {
        buf[i]='0';
    }
    buf[CLUSTER_SIZE-1]='e';
    printf("clusters \n");

   for (i=1; i<CLUSTER_COUNT_FAT; i++)
    {
        WriteCluster(i, CLUSTER_SIZE, buf);
        printf("cluster: %d\n", i);
    }

//////////Чтение из файла/////////////
    long a;
    a = ReadFreeClusterCount();
    printf("free cluster count: %ld\n", a);

    a = ReadFreeCluster();
    printf("free cluster: %ld\n", a);


    for (i=0; i<CLUSTER_COUNT_FAT; i++)
    {
        a = ReadValueFAT(i);
        printf("fat %d value: %ld\n",i,a);
    }

    ReadCatalog(0);
    for (i=0; i<ELEMENT_COUNT_IN_CATALOG; i++)
    {
        printf("File Record %d\n", i);
        int j;
        printf("name: ");
        for (j=0; j<8; j++)
        {
            printf("%c", temporary_catalog[i].name[j]);
        }
        printf("\n");
        printf("extension: ");
        for (j=0; j<3; j++)
        {
            printf("%c", temporary_catalog[i].extension[j]);
        }
        printf("\n");
        printf("first %ld\n", temporary_catalog[i].first_cluster);
        printf("size %ld\n", temporary_catalog[i].file_size);
        printf("attribute %c\n", temporary_catalog[i].atribute);
        printf("is Catalog %d\n", temporary_catalog[i].isCatalog);
    }

    printf("first claster: \n");
    char res [CLUSTER_SIZE];
    ReadCluster(1, CLUSTER_SIZE, res);
    for (i=0; i<CLUSTER_SIZE; i++)
    {
        printf("%c", res[i]);
    }
    printf("\n");
*/
    return 0;
}
