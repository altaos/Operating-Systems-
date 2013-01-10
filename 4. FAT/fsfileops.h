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
char message[500];

struct File_Record temporary_catalog[ELEMENT_COUNT_IN_CATALOG];

#define LOG_PATH "/home/alina/Documents/FS1/log.txt"
#define FILE_PATH "/home/alina/Documents/FS1/file.dat"
#define HOME "/home/alina/Documents/FS1"
/////////////////////////////////////////////////////////////////
void WriteToLog(const char *str);
long ReadFreeClusterCount();
long WriteFreeClusterCount(long value);
long WriteValueFat(long cluster_number, long value);
long WriteValueFat(long cluster_number, long value);
long FreeClusterIndex(long operation);
long FindFreeCluster();
long ReadFreeCluster();
long WriteFreeCluster(long value);
long ReadCatalog(long cluster_number);
long WriteCatalog(struct File_Record cat[ELEMENT_COUNT_IN_CATALOG], long cluster_number);
long ReadCluster(long cluster_number, long size, void *buf);
long WriteCluster(long cluster_number, long size, void *buf);
long Load(char *filename);
long ReadFileFromClusters(void *buf, long first_cluster, long file_size);
long CalcCountNeededClusters(long size);
long IsSpaceInCatalog(long cluster_number);
long WriteFileToClusters(void *buf, long size, long first_cluster);
long GetNumberFirstClusterByPath(const char *path);
long TruncFile(long first_cluster_number, long offset);
long CreateFile(const char *path, mode_t mode);
long ReturnBackState(long cluster_number);
long GetParentFirstCluster(const char *path);
long RemoveDirectory(long parent_catalog_first_cluster, long child_catalog_first_cluster);
long RemoveFile(long parent_first_cluster, long child_first_cluster);
long RemoveByPath(const char *path);
long Rename(const char *path, const char *newpath);
long CreateDirectory(const char *path, mode_t mode);
/////////////////////////////////////////////////////////////////


void PrintCatalog(struct File_Record *str)
{
    long i;
    //long index = GetParentFirstClusterForNewPath();//GetNumberFirstClusterByPath(path);
    printf ("index %d\n", 0);
    ReadCatalog(0);

    for (i=0; i<10; i++)
    {
        printf("File Record in Catalog %ld\n", i);
        int j;
        printf("name: ");
        for (j=0; j<256; j++)
        {
            printf("%c", str[i].name[j]);
        }
        sprintf(message, "%ld Name, path = %s", i, str[i].name);
        WriteToLog(message);
        printf("\n");
        printf("first %ld\n", str[i].first_cluster);
        sprintf(message, "%ld First cluster = %ld", i, str[i].first_cluster);
        WriteToLog(message);
        printf("size %ld\n", str[i].size);
        sprintf(message, "%ld Size = %ld", i, str[i].size);
        WriteToLog(message);
        printf("mode %d\n", str[i].mode);
        sprintf(message, "%ld Mode = %d", i, str[i].mode);
        WriteToLog(message);
        printf("gen %ld\n", str[i].gen);
    }
}

void WriteToLog(const char *str)
{
    FILE *output;

    if((output=fopen(LOG_PATH, "a+"))==0)
    {
        puts ("Can't open output file.");
        exit(-1);
    }
    fprintf(output, "%s\n", str);
    fclose(output);
}

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

long FreeClusterIndex(long operation)
{

    count_free_cluster = ReadFreeClusterCount();

    switch(operation)
    {
    case 1 : WriteFreeClusterCount(++count_free_cluster); break;
    case -1 : WriteFreeClusterCount(--count_free_cluster); break;
    }

    //WriteFreeClusterCount(++count_free_cluster);

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
            printf("I'm freeeeeeeeeeeeeeeeeee\n");
            free_cluster = i;
            return free_cluster;
        }
    }

    for(i = 1; i < CLUSTER_COUNT_FAT; i++)
    {
        if(ReadValueFAT(i) == NOT_USED)
        {
            free_cluster = i;
            return free_cluster;
        }
    }

    //свободных кластеров нет
    return -ENOMEM;
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

long ReadCatalog(long cluster_number)
{
    FILE *file;
    if((file=fopen(fsfilename,"rb"))==0)
    {
        return -1;
    }

    ReadFileFromClusters(temporary_catalog, cluster_number, ELEMENT_COUNT_IN_CATALOG * sizeof(struct File_Record));

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

    long written_byte = 0;
    long byte_to_write = ELEMENT_COUNT_IN_CATALOG * sizeof(struct File_Record);
    long cluster = cluster_number;

    while(byte_to_write > 0)
    {
        if(byte_to_write > CLUSTER_SIZE)
        {
            if(WriteCluster(cluster, CLUSTER_SIZE, cat + written_byte) != -1)
            {
                written_byte += CLUSTER_SIZE;
                byte_to_write -= CLUSTER_SIZE;
                cluster = ReadValueFAT(cluster);
            }
        }
        else
        {
            if(WriteCluster(cluster, byte_to_write, cat + written_byte) != -1)
            {
                written_byte += byte_to_write;
                byte_to_write = 0;
            }
        }
    }

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

long ReadClusterWithOffset(long cluster_number, long size, void *buf, long offset)
{
    if(offset + size > CLUSTER_SIZE || cluster_number < 0 || cluster_number >= CLUSTER_COUNT_FAT)
        return -1;

    FILE *file;
    if((file=fopen(fsfilename,"rb"))==0)
    {
        return -1;
    }

    fseek(file, sizeof(long)*(2+CLUSTER_COUNT_FAT)+CLUSTER_SIZE*cluster_number+offset, SEEK_SET);
    fread(buf, size, 1, file);
    fclose(file);

    return 0;
}

long WriteClusterWithOffset(long cluster_number, long size, void *buf, long offset)
{
    if(offset + size > CLUSTER_SIZE || cluster_number < 0 || cluster_number >= CLUSTER_COUNT_FAT)
        return -1;

    FILE *file;
    if((file=fopen(fsfilename,"r+b"))==0)
    {
        return -1;
    }

    fseek(file, sizeof(long)*(2+CLUSTER_COUNT_FAT)+CLUSTER_SIZE*cluster_number + offset, SEEK_SET);
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
        //printf("byte_to_read : %ld\n", byte_to_read);
        if(current_cluster != EOC)
        {
            if(ReadCluster(current_cluster, CLUSTER_SIZE, buf + read_byte) != -1)
            {
                read_byte += CLUSTER_SIZE;
                byte_to_read -= CLUSTER_SIZE;
                current_cluster = ReadValueFAT(current_cluster);
                //printf("current_cluster : %ld\n", current_cluster);
             }
        }
        else
        {
            //printf("current_cluster last: %ld\n", current_cluster);
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

long IsSpaceInCatalog(long cluster_number)
{
    if(ReadCatalog(cluster_number) < 0)
        return -EIO;

    int i = 0;
    while(temporary_catalog[i].first_cluster != 0 && i < ELEMENT_COUNT_IN_CATALOG)
        i++;

    if(i == ELEMENT_COUNT_IN_CATALOG)
        return -1;

    return i;
}

//long ReadFile(void *buf, long first_cluster, long file_size, long offset)
//{
//    long current_cluster = first_cluster;
//    long read_byte = 0;
//    long byte_to_read = file_size;

//    long byte_to_offset = offset;
//    while(byte_to_offset - CLUSTER_SIZE > 0)
//    {
//        if(current_cluster != EOC)
//        {
//            byte_to_offset -= CLUSTER_SIZE;
//            current_cluster = ReadValueFAT(current_cluster);
//        }
//    }


//    while(byte_to_read > 0)
//    {
//        if(current_cluster != EOC)
//        {
//            if(ReadClusterWithOffset(current_cluster, CLUSTER_SIZE, buf + read_byte, byte_to_offset) != -1)
//            {
//                byte_to_offset = 0;
//                read_byte += CLUSTER_SIZE;
//                byte_to_read -= CLUSTER_SIZE;
//                current_cluster = ReadValueFAT(current_cluster);
//             }
//        }
//        else
//        {
//            if(ReadClusterWithOffset(current_cluster, byte_to_read, buf + read_byte, byte_to_offset) != -1)
//            {
//                read_byte += byte_to_read;
//                byte_to_read = 0;
//            }
//        }
//    }
//    return 0;
//}


long ReadFile(void *buf, long bytes_to_read, struct File_Record *fr, long offset)
{
    long pos = 0;
    long cluster = fr->first_cluster;
    long previous_cluster = cluster;
    long bytes_read = 0;
    long offs;
    long n;

    while(bytes_read < bytes_to_read && pos < fr->size)
    {
        if(cluster == EOC)
        {
            break;
        }

        if(pos + CLUSTER_SIZE > offset)
        {
            offs = offset - pos > 0 ? offset - pos : 0; //отступ внутри кластера
            n = CLUSTER_SIZE - offs < bytes_to_read - bytes_read ? CLUSTER_SIZE - offs : bytes_to_read - bytes_read;//сколько байт пишем в кластер
            if(pos + n > fr->size)
                n = fr->size - pos;

            sprintf(message, "WRITE FILE  cluster %ld  n = %ld   offs = %ld  pos = %ld", cluster, n, offs, pos);
            WriteToLog(message);

            ReadClusterWithOffset(cluster, n, buf, offs);
            bytes_read += n;
        }

        previous_cluster = cluster;
        cluster = ReadValueFAT(cluster);
        pos += CLUSTER_SIZE;
    }

    if(bytes_read < bytes_to_read)
        memset(buf+bytes_read, 0, bytes_to_read-bytes_read);
    fr->atime = time(NULL);

    return 0;
}

//long WriteFile(void *buf, long size, struct File_Record *fr, long offset)
//{
//    sprintf(message, "WRITE FILE size = %ld   file_size = %ld   offset = %ld", size, fr->size, offset);
//    WriteToLog(message);
//    //пишем в конец файла
//    if(fr->size == offset)
//    {
//        long cur_cluster, previous_cluster = fr->first_cluster;
//        cur_cluster = previous_cluster;
//        //ищем кластер указывающий на конец файла
//        while(cur_cluster != EOC)
//        {
//            previous_cluster = cur_cluster;
//            cur_cluster = ReadValueFAT(cur_cluster);
//        }

//        ReadFreeCluster();

//        long next_cluster = free_cluster;

//        if (FindFreeCluster() < 0)
//        {
//            printf("Error in reading free cluster\n");
//        }
//        else
//        {
//            WriteFreeCluster(free_cluster);
//            FreeClusterIndex(-1);
//        }

//        //если запись файла прошла успешно, убираем предыдущее EOC
//        if(WriteFileToClusters(buf, size, next_cluster) == 0)
//        {
//            WriteValueFat(previous_cluster, next_cluster);
//        }
//        fr->size = fr->size + size;
//        fr->atime = time(NULL);
//        fr->mtime = time(NULL);
//    }
//    //перезаписываем часть файла
//    if(offset < fr->size)
//    {
//        long b_for_write = size - fr->size + offset;
//        //проверка на достаточность места в файловой системе
//        if(b_for_write > 0 && CalcCountNeededClusters(size) > ReadFreeClusterCount())
//        {
//            return -ENOMEM; //недостаточно памяти
//        }

//        //порядковый номер кластера файла
//        long index = offset / CLUSTER_SIZE + 1;
//        //смещение в кластере
//        long new_off = offset % CLUSTER_SIZE == 0 ? 0 : offset % CLUSTER_SIZE;

//        long cur_cluster, previous_cluster = fr->first_cluster;
//        cur_cluster = previous_cluster;

//        long i = 0;
//        while (++i < index)
//        {
//            cur_cluster = ReadValueFAT(cur_cluster);
//        }

//        long written_byte = 0;
//        long byte_to_write = size;

//        if(WriteClusterWithOffset(cur_cluster, CLUSTER_SIZE - new_off, buf, new_off) <0)
//            return -EIO;

//        written_byte += CLUSTER_SIZE - new_off;
//        byte_to_write -= CLUSTER_SIZE - new_off;
//        cur_cluster = ReadValueFAT(cur_cluster);

//        while(cur_cluster != EOC && byte_to_write > 0)
//        {
//            if(byte_to_write > CLUSTER_SIZE)
//            {
//                if(WriteCluster(cur_cluster, CLUSTER_SIZE, buf + written_byte) != -1)
//                {
//                    previous_cluster = cur_cluster;
//                    cur_cluster = ReadValueFAT(cur_cluster);
//                    written_byte += CLUSTER_SIZE;
//                    byte_to_write -= CLUSTER_SIZE;
//                }
//            }
//            else
//            {
//                if(WriteCluster(cur_cluster, byte_to_write, buf + written_byte) != -1)
//                {
//                    //запоминаем для последующего присоединения
//                    previous_cluster = cur_cluster;
//                    cur_cluster = ReadValueFAT(cur_cluster);
//                    written_byte += byte_to_write;
//                    byte_to_write = 0;

//                    fr->atime = time(NULL);
//                    fr->mtime = time(NULL);
//                }
//            }
//        }

//        //перезаписал уже существующие кластеры файла, нужно записывать новые
//        if(byte_to_write > 0)
//        {
//            cur_cluster = ReadFreeCluster();
//            if(WriteFileToClusters(buf, size, cur_cluster) == 0)
//            {
//                //присоединяем дописанную часть
//                WriteValueFat(previous_cluster, cur_cluster);
//                fr->size = offset + size;
//                fr->atime = time(NULL);
//                fr->mtime = time(NULL);
//            }
//            else
//            {
//                return -EIO;
//            }
//        }
//    }
//    //необходимо добавить пустые кластеры
//    if(offset > fr->size)
//    {
//        //long new_off = fr->size - offset;
//        long ost = fr->size % CLUSTER_SIZE;
//        long new_off = offset - fr->size - ost;
//        //проверка на достаточность места в файловой системе
//        if(CalcCountNeededClusters(offset - fr->size - ost + size) > ReadFreeClusterCount())
//        {
//            return -ENOMEM; //недостаточно памяти
//        }

//        ost = offset - ost;
//        long blank_clusters = ost / CLUSTER_SIZE;
//        long off_in_claster = ost % CLUSTER_SIZE;//смещение в кластере после отступа
//        long previous_cluster, cur_cluster = fr->first_cluster;
//        previous_cluster = cur_cluster;

//        while(cur_cluster != EOC)
//        {
//            previous_cluster = cur_cluster;
//            cur_cluster = ReadValueFAT(cur_cluster);
//        }

//        long i;
//        ReadFreeCluster();
//        //дописываем пустые кластеры
//        for(i = 0; i < blank_clusters ; i++)
//        {
//            previous_cluster = free_cluster;
//            WriteValueFat(previous_cluster, ReadFreeCluster());

//            if (FindFreeCluster() < 0)
//            {
//                printf("Error in searching free cluster\n");
//            }
//            else
//            {
//                WriteFreeCluster(free_cluster);
//                FreeClusterIndex(-1);
//            }
//        }

//        cur_cluster = free_cluster;

//        if(WriteClusterWithOffset(cur_cluster, CLUSTER_SIZE - off_in_claster, buf, off_in_claster) < 0)
//        {
//            return -EIO;
//        }

//        if (FindFreeCluster() < 0)
//        {
//            printf("Error in searching free cluster\n");
//            return -EIO;
//        }
//        else
//        {
//            WriteFreeCluster(free_cluster);
//            FreeClusterIndex(-1);
//            cur_cluster = free_cluster;
//        }

//        if(WriteFileToClusters(buf + off_in_claster, size - CLUSTER_SIZE + off_in_claster, cur_cluster) == 0)
//        {
//            //присоединяем дописанную часть
//            WriteValueFat(previous_cluster, cur_cluster);
//            fr->size = offset + size;
//            fr->atime = time(NULL);
//            fr->mtime = time(NULL);
//        }
//        else
//        {
//            return -EIO;
//        }

//    }

//    return 0;
//}

long WriteFileToClusters(void *buf, long size, long first_cluster)
{

    //char message[500];
    if(ReadFreeClusterCount() < 0)
        return -1;

    //проверка на достаточность места в файловой системе
    if(CalcCountNeededClusters(size) > count_free_cluster)
    {
        return -ENOMEM;
    }

    long written_byte = 0;
    long byte_to_write = size;
    long previous_cluster = first_cluster;
    free_cluster = first_cluster;

    while (byte_to_write > 0)
    {
//        printf("byte_to_write : %ld\n", byte_to_write);
        if(byte_to_write > CLUSTER_SIZE)
        {
            if(WriteCluster(free_cluster, CLUSTER_SIZE, buf + written_byte) != -1)
            {
                WriteValueFat(previous_cluster, free_cluster);
//                printf("previous_cluster: %ld\n", previous_cluster);
//                printf("free cluster: %ld\n", free_cluster);
                previous_cluster = free_cluster;

                written_byte += CLUSTER_SIZE;
                byte_to_write -= CLUSTER_SIZE;

                if (/*free_cluster = */FindFreeCluster() < 0)
                {
                    printf("File is empty\n");
                }
                else
                {
                    WriteFreeCluster(free_cluster);
                    FreeClusterIndex(-1);
                }
            }
        }
        else
        {
            if(WriteCluster(free_cluster, byte_to_write, buf + written_byte) != -1)
            {
//                printf("previous cluster: %ld\n", previous_cluster);
                WriteValueFat(previous_cluster, free_cluster);
                WriteValueFat(free_cluster, EOC);
//                printf("free cluster: %ld\n", free_cluster);
//                printf("Cluster: %ld, value : %ld\n", previous_cluster, ReadValueFAT(free_cluster));


                written_byte += byte_to_write;
                byte_to_write = 0;

                if (FindFreeCluster() < 0)
                {
                    printf("File is empty\n");
                }
                else
                {
                    WriteFreeCluster(free_cluster);
                    FreeClusterIndex(-1);
                }
            }
        }
    }

//    printf("free cluster: %ld\n", free_cluster);

    return 0;
}

//long WriteFileToClusters(void *buf, long size, long first_cluster)
//{

//    if(ReadFreeClusterCount() < 0)
//        return -1;

//    //проверка на достаточность места в файловой системе
//    if(CalcCountNeededClusters(size) > count_free_cluster)
//    {
//        return -1;
//    }

//    long written_byte = 0;
//    long byte_to_write = size;
//    long previous_cluster = first_cluster;
//    free_cluster = first_cluster;

//    while (byte_to_write > 0)
//    {
//        if(byte_to_write > CLUSTER_SIZE)
//        {
//            if(WriteCluster(free_cluster, CLUSTER_SIZE, buf + written_byte) != -1)
//            {
//                WriteValueFat(previous_cluster, free_cluster);
//                previous_cluster = free_cluster;

//                written_byte += CLUSTER_SIZE;
//                byte_to_write -= CLUSTER_SIZE;

//                if (FindFreeCluster() < 0)
//                {
//                    printf("File is empty\n");
//                }
//                else
//                {
//                    WriteFreeCluster(free_cluster);
//                    FreeClusterIndex(-1);
//                }
//            }
//        }
//        else
//        {
//            if(WriteCluster(free_cluster, byte_to_write, buf + written_byte) != -1)
//            {
//                WriteValueFat(previous_cluster, free_cluster);
//                WriteValueFat(free_cluster, EOC);

//                written_byte += byte_to_write;
//                byte_to_write = 0;

//                if (FindFreeCluster() < 0)
//                {
//                    printf("File is empty\n");
//                }
//                else
//                {
//                    WriteFreeCluster(free_cluster);
//                    FreeClusterIndex(-1);
//                }
//            }
//        }
//    }

//    return 0;
//}

//на входе: структура с перым кластером и filesize, buf, offset, bytes_to_write
//pos = 0
//cluster = первый кластер
//bytes_wrote = 0;
//while (/*pos < offset + bytes_to_write*/bytes_wrote < bytes_to_write)
//{
//	если кластер не существует
//		cluster = выделить новый кластер()
//	if(pos + CLUSTER_SIZE > offset)
//	{
//		offs = offset - pos > 0 ? offset - pos : 0;//какой отступ внутри кластера?
//		n = CLUSTER_SIZE - offs > bytes_to_write - bytes_wrote ? CLUSTER_SIZE - offs : bytes_to_write - bytes_wrote;//сколько байт пишем в кластер?
//		записать кластер(кластер, offs, buf+bytes_wrote, n);
//		bytes_wrote+=n;
//	}
//	cluster = прочитать следующий кластер();
//	pos+=CLUSTER_SIZE;//перешли на следующий кластер
//}
//filesize = filesize > offset + bytes_wrote ? filesize : offset + bytes_wrote

long WriteFile(void *buf, long bytes_to_write, struct File_Record *fr, long offset)
{
    long pos = 0;
    long cluster = fr->first_cluster;
    long previous_cluster = cluster;
    long bytes_wrote = 0;
    long offs;
    long n;

    while(bytes_wrote < bytes_to_write)
    {
        if(cluster == EOC)
        {
            cluster = ReadFreeCluster();
            FindFreeCluster();
            WriteValueFat(previous_cluster, cluster);
            FreeClusterIndex(-1);
            WriteValueFat(cluster, EOC);
        }

        if(pos + CLUSTER_SIZE > offset)
        {
            offs = offset - pos > 0 ? offset - pos : 0; //отступ внутри кластера
            n = CLUSTER_SIZE - offs < bytes_to_write - bytes_wrote ? CLUSTER_SIZE - offs : bytes_to_write - bytes_wrote;//сколько байт пишем в кластер
            printf("WRITE FILE  cluster %ld  n = %ld   offs = %ld  pos = %ld\n", cluster, n, offs, pos);
            printf("Current cluster = %ld\n", cluster);

            //sprintf(message, "WRITE FILE  cluster %ld  n = %ld   offs = %ld  pos = %ld", cluster, n, offs, pos);
//            WriteToLog(message);


            WriteClusterWithOffset(cluster, n, buf, offs);
            bytes_wrote += n;
        }

        previous_cluster = cluster;
        cluster = ReadValueFAT(cluster);
        pos += CLUSTER_SIZE;
    }

    fr->size = (fr->size > offset + bytes_wrote) ? fr->size : (offset + bytes_wrote);
    sprintf(message, "WRITE FILE file_size = %ld", fr->size);
    WriteToLog(message);
    fr->atime = time(NULL);
    fr->mtime = time(NULL);

    if(cluster == EOC)
    {
        FindFreeCluster();
        WriteFreeCluster(free_cluster);
    }

    return 0;
}

long GetNumberFirstClusterByPath(const char *path)
{
    int pos = 0;
    int i;
    long cluster_number = -ENOENT;
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

        if (ReadCatalog(cluster_number) != -1)
        {
            cluster_number = -ENOENT;
            for (i=0; i<ELEMENT_COUNT_IN_CATALOG;  i++)
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
            return -ENOENT;
        }
    }
    return cluster_number;
}

//при написании функции truncate не забыть изменить размер файла в каталоге!!!!
long TruncFile(long first_cluster_number, long offset)
{
    long num = offset % CLUSTER_SIZE;
//    printf("num = %ld\n", num);
    long index = num == 0 ? (offset / CLUSTER_SIZE) : (offset / CLUSTER_SIZE + 1);
//    printf("index = %ld\n", index);
    long curr_cluster_number = first_cluster_number;

    long i = 0;
    while (++i < index)
    {
        curr_cluster_number = ReadValueFAT(curr_cluster_number);
    }
//    printf("!!!!!!!!!curr_cluster = %ld\n", curr_cluster_number);

//    printf("curr_cluster = %ld\n", curr_cluster_number);
    /*long tmp = ReadValueFAT(curr_cluster_number);
    WriteValueFat(curr_cluster_number, NOT_USED);
    curr_cluster_number = tmp;*/
    if(index == 0)
    {
        WriteValueFat(curr_cluster_number, NOT_USED);
        WriteFreeCluster(curr_cluster_number);
//        printf("!!!!!!!!!curr_cluster value = %ld\n", ReadValueFAT(curr_cluster_number));
        FreeClusterIndex(1);
        return 0;
    }

    //если кластер, после которого остальные должны освобождаться, не является последним
    if (curr_cluster_number != EOC)
    {
//        printf("curr_cluster = %ld\n", curr_cluster_number);
        long tmp = ReadValueFAT(curr_cluster_number);
        WriteValueFat(curr_cluster_number, EOC);
        curr_cluster_number = tmp;

        WriteFreeCluster(curr_cluster_number);

        while(curr_cluster_number != EOC)
        {
            //WriteFreeCluster(curr_cluster_number);
            //запомнили номер текущего кластера
            tmp = curr_cluster_number;
            //считали номер следующего
            curr_cluster_number = ReadValueFAT(curr_cluster_number);
            //отметили, что текущий кластер не занят
            WriteValueFat(tmp, NOT_USED);
            FreeClusterIndex(1);
        }
        WriteValueFat(curr_cluster_number, NOT_USED);
        WriteFreeCluster(curr_cluster_number);
        FreeClusterIndex(1);
    }
//    else
//    {
//        WriteValueFat(curr_cluster_number, NOT_USED);
//        FreeClusterIndex(1);
//    }
//    printf("------------curr_cluster = %ld\n", curr_cluster_number);

    return 0;

}

long CreateFile(const char *path, mode_t mode)
{
    printf("CREATE FILE path = %s mode = %d", path, mode);
    if(GetNumberFirstClusterByPath(path) >= 0)
    {
        return -EEXIST; //файл с таким именем уже существует
    }
    int l = strlen(path);
    while(path[l] != '/' && l >= 0)
        l--;
    if(l<0)
        return -ENOENT;

    char catalog_path[l+2];
    char name[strlen(path) - l];
    if(l==0)
    {
        strncpy(catalog_path, "/", 1);
        catalog_path[1]='\0';
    }
    else
    {
        strncpy(catalog_path, path, l);
        catalog_path[l]='\0';
    }
    printf("CREATE FILE parent_path = %s", path);

    strncpy(name, path+l+1, strlen(path) - l - 1);
    name[strlen(path) - l - 1]='\0';
    printf("CREATE FILE name = %s", name);

    long catalog_cluster_number = GetNumberFirstClusterByPath(catalog_path);
    printf("CREATE FILE catalog_cluster_number = %ld", catalog_cluster_number);

    ReadCatalog(catalog_cluster_number);
    int i;
    for (i=0; i<ELEMENT_COUNT_IN_CATALOG; i++)
    {
        if (temporary_catalog[i].first_cluster == 0)
        {
            break;
        }
    }

    struct File_Record fr;
    strcpy(fr.name, name);
    fr.first_cluster = free_cluster;
    fr.size = 0;
    fr.mode = mode;
    fr.atime = time(NULL);
    fr.ctime = time(NULL);
    fr.mtime = time(NULL);
    fr.uid = 0;
    fr.gid = 0;
    fr.size = 0;

    temporary_catalog[i] = fr;
    WriteValueFat(free_cluster, EOC);
    WriteCatalog(temporary_catalog, catalog_cluster_number);

//    sprintf(message, "CREATE FILE");
//    WriteToLog(message);
//    PrintCatalog(temporary_catalog);

    FindFreeCluster();
    WriteFreeCluster(free_cluster);
    FreeClusterIndex(-1);

    return 0;
}

//при неудачной записи освободить кластеры в фат
long ReturnBackState(long cluster_number)
{
    long cluster = cluster_number;
    long old_cluster = cluster_number;

    while(cluster != EOC)
    {
        while(cluster = ReadValueFAT(cluster) < 0) {}//////////////////////////////
        while(WriteValueFat(old_cluster, NOT_USED) < 0) {}/////////////////////////
    }

    while(WriteValueFat(old_cluster, NOT_USED) < 0) {}

    return 0;
}

//получение номера кластера родительского каталога
long GetParentFirstCluster(const char *path)
{
//    sprintf(message, "GET path = %s", path);
//    WriteToLog(message);
    int l = strlen(path);
    while(path[l] != '/' && l >= 0)
            l--;

//    sprintf(message, "GET l = %d", l);
//    WriteToLog(message);

    if(l < 0)
        return -1;

//    WriteToLog("ggggggggggg");
    char parent_path[l+2];
    if(l == 0)
    {
        strncpy(parent_path, "/", 1);
        parent_path[l+1] = '\0';
    }
    else
    {
        strncpy(parent_path, path, l);
        parent_path[l]='\0';
    }


//    sprintf(message, "GET PAR par_path = %s", parent_path);
//    WriteToLog(message);
    long cur_cluster = GetNumberFirstClusterByPath(path);
    printf("cur_path = %ld\n", cur_cluster);

//    printf("parent_path = %s\n", parent_path);

    long parent_cluster = GetNumberFirstClusterByPath(parent_path);
    if(ReadCatalog(parent_cluster) < 0)
        return -EIO;

    long i = 0;
    while(temporary_catalog[i].first_cluster != cur_cluster && i < ELEMENT_COUNT_IN_CATALOG)
        i++;

    if(i == ELEMENT_COUNT_IN_CATALOG)
        return -ENOENT;

    return parent_cluster;
}

void NullRecord(struct File_Record r)
{
    strcpy(r.name, "\0");
    r.first_cluster = 0;
    r.size = 0;
    r.atime = time (NULL);
    r.ctime = time(NULL);
    r.mtime = time(NULL);
    r.mode = 0;
    r.gen = 0;
    r.gid = 0;
    r.uid = 0;
}

long RemoveDirectory(long parent_catalog_first_cluster, long child_catalog_first_cluster)
{
    if (ReadCatalog(child_catalog_first_cluster) < 0)
        return -EIO;

    //удаляем все файлы и папки из каталога, который будем удалять
    long i = 0;
    for (; i<ELEMENT_COUNT_IN_CATALOG; i++)
    {
        if ((temporary_catalog[i].mode & S_IFDIR) && temporary_catalog[i].first_cluster != 0)
        {   //если хранится директория, то удаляем ее, но сначала ее содержимое
            printf("Temporary_catalog[%ld].mode = [%d] \n Temporary_catalog[%ld].fist_cluster = %ld\n", i, temporary_catalog[i].mode, i, temporary_catalog[i].first_cluster);
            RemoveDirectory(child_catalog_first_cluster,temporary_catalog[i].first_cluster);
            WriteValueFat(temporary_catalog[i].first_cluster, NOT_USED);
            NullRecord(temporary_catalog[i]);
        }
        else if(temporary_catalog[i].first_cluster != 0) //если это файл,то необходимо удалить ее содержимое
        {
            printf("Temporary_catalog[%ld].mode = [%d] \n Temporary_catalog[%ld].fist_cluster = %ld\n", i, temporary_catalog[i].mode, i, temporary_catalog[i].first_cluster);
            RemoveFile(child_catalog_first_cluster, temporary_catalog[i].first_cluster);
        }
    }

    if(WriteCatalog(temporary_catalog, child_catalog_first_cluster) < 0)
        return -EIO;


    // удаляем запись об удаляемом каталоге в родительском каталоге
    if (ReadCatalog(parent_catalog_first_cluster) < 0)
        return -EIO;

    i = -1;
    while(child_catalog_first_cluster != temporary_catalog[i].first_cluster)
        i++;

    printf("Number in parent catalog %ld\n", i);

    WriteValueFat(temporary_catalog[i].first_cluster, NOT_USED);
    NullRecord(temporary_catalog[i]);

    if(WriteCatalog(temporary_catalog, parent_catalog_first_cluster) < 0)
        return -EIO;

    return 0;
}

long RemoveFile(long parent_first_cluster, long child_first_cluster)
{
//    long size = temporary_catalog[i].file_size;


    //индекс записи в каталоге для удаляемого файла найден, теперь затираем эту запись

    if(TruncFile(child_first_cluster, 0) < 0)
        return -EIO;

    printf("parent_firsr_cluster = %ld\n", parent_first_cluster);
    printf("child_firsr_cluster = %ld\n", child_first_cluster);
    if (ReadCatalog(parent_first_cluster) < 0)
        return -EIO;

    int i = 0;
    while(child_first_cluster != temporary_catalog[i].first_cluster && i < ELEMENT_COUNT_IN_CATALOG)
        i++;

    printf("i file in catalog i = %d\n", i);

    if(i == ELEMENT_COUNT_IN_CATALOG)
        return -1;


    printf("Value in Fat %ld value : %ld", child_first_cluster, ReadValueFAT(child_first_cluster));

    temporary_catalog[i].size = 0;
    temporary_catalog[i].first_cluster = 0;
    strcpy(temporary_catalog[i].name, "");
    temporary_catalog[i].mode = 0;
    temporary_catalog[i].atime = 0;
    temporary_catalog[i].ctime = 0;
    temporary_catalog[i].mtime = 0;
    temporary_catalog[i].gen = 0;
    temporary_catalog[i].gid = 0;
    temporary_catalog[i].uid = 0;

    printf("parent_firsr_cluster = %d\n", i);
    if(WriteCatalog(temporary_catalog, parent_first_cluster) < 0)
        return -EIO;

    return 1;
}

long RemoveByPath(const char *path)
{
    //printf("REMOVE BY PATH : bb path = %s", path);
    long parent_cluster = GetParentFirstCluster(path);
    printf("REMOVE BY PATH : parent cluster = %ld\n", parent_cluster);
    if(ReadCatalog(parent_cluster) < 0)
        return -EIO;

    long cluster_number = GetNumberFirstClusterByPath(path);
    //printf("REMOVE BY PATH : path = %s", path);
    //printf("REMOVE BY PATH : cluster = %ld", cluster_number);

/*    if(index < 0)
        return -1*/;

    int i = 0;

    //ищем запись в каталоге
    while(cluster_number != temporary_catalog[i].first_cluster) i++;
    //printf("REMOVE BY PATH : cluster = %d", i);

    //если каталог
    if(temporary_catalog[i].mode == S_IFDIR)
    {
        if(RemoveDirectory(parent_cluster, cluster_number) < 0)//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            return -EIO;
    }
    else
    {
        if(RemoveFile(parent_cluster, cluster_number) < 0)
            return -EIO;
    }

    if(WriteCatalog(temporary_catalog, parent_cluster) < 0)
        return -EIO;
}

//получение номера кластера родительского каталога для файла который еще не создан
long GetParentFirstClusterForNewPath(const char *path)
{
    int l = strlen(path);
    while(path[l] != '/' && l >=0)
            l--;

    if(l < 0)
        return -1;

    char parent_path[l+2];
    if(l == 0)
    {
        strncpy(parent_path, "/", 1);
        parent_path[l+1] = '\0';
    }
    else
    {
        strncpy(parent_path, path, l);
        parent_path[l]='\0';
    }

    long parent_cluster = GetNumberFirstClusterByPath(parent_path);

    return parent_cluster;
}

long Rename(const char *path, const char *newpath)
{
    int l = strlen(newpath);
    while(newpath[l] != '/' && l >= 0)
        l--;

    if(l < 0)
        return -ENOENT;

    //выделение нового имени файла
    char name[strlen(newpath) - l];
    strncpy(name, newpath+l+1, strlen(newpath) - l-1);
    name[strlen(newpath) - l-1]='\0';

    long cluster_number = GetNumberFirstClusterByPath(path);
    printf("cluster_number %ld\n", cluster_number);

    long old_parent_cluster = GetParentFirstCluster(path);
    printf("old_parent_cluster %ld\n", old_parent_cluster);

    long new_parent_cluster = GetParentFirstClusterForNewPath(newpath);
    printf("new_parent_cluster %ld\n", new_parent_cluster);


    //считываем информацию о файле из каталога, где он хранился
    if(ReadCatalog(old_parent_cluster) < 0)
        return -EIO;

    long old_index = 0;//получаем место расположения в старом каталоге
    while(cluster_number != temporary_catalog[old_index].first_cluster)
        old_index++;
    printf("old_index %ld\n", old_index);

    //если каталог родительский и новый совпадают, то только меняем имя в старом каталоге
    if (old_parent_cluster == new_parent_cluster)
    {
        strcpy(temporary_catalog[old_index].name, name);
        if(WriteCatalog(temporary_catalog, old_parent_cluster) < 0)
            return -EIO;
        return 0;
    }

    struct File_Record old_record = temporary_catalog[old_index]; //получаем запись о файле
    //стираем из старого каталога запись о файле
    strcpy(temporary_catalog[old_index].name, "\0");
    temporary_catalog[old_index].first_cluster = 0;
    temporary_catalog[old_index].size = 0;
    temporary_catalog[old_index].atime = time (NULL);
    temporary_catalog[old_index].ctime = time(NULL);
    temporary_catalog[old_index].mtime = time(NULL);
    temporary_catalog[old_index].mode = 0;
    temporary_catalog[old_index].gen = 0;
    temporary_catalog[old_index].gid = 0;
    temporary_catalog[old_index].uid = 0;
    if(WriteCatalog(temporary_catalog, old_parent_cluster) < 0)
        return -EIO;

    // считываем новый каталог
    if(ReadCatalog(new_parent_cluster) < 0)
        return -EIO;
    //проверка:есть ли место в новом каталоге для записи
    //поиск незанятого места
    long index_in_new_catalog = 0;
    while(temporary_catalog[index_in_new_catalog].first_cluster != 0 && index_in_new_catalog < ELEMENT_COUNT_IN_CATALOG)
        index_in_new_catalog++;
    printf("index_in_new_catalog %ld\n", index_in_new_catalog);

    //если места для записи в новом каталоге нет, возвращаем все обратно
    if(index_in_new_catalog == ELEMENT_COUNT_IN_CATALOG)
    {
        if(ReadCatalog(old_parent_cluster) < 0)
            return -EIO;

        temporary_catalog[old_index] = old_record;

        if(WriteCatalog(temporary_catalog, old_parent_cluster) < 0)
            return -EIO;
    }
    else
    {
        strcpy(old_record.name, name);
        temporary_catalog[index_in_new_catalog] = old_record;
        if(WriteCatalog(temporary_catalog, new_parent_cluster) < 0)
            return -EIO;
    }
    return 0;
}

long CreateDirectory(const char *path, mode_t mode)
{
    if(GetNumberFirstClusterByPath(path) >= 0)
    {
        return -EEXIST; //файл с таким именем уже существует
    }

    int l = strlen(path);
    while(path[l] != '/' && l >= 0)
        l--;
    if(l<0)
        return -ENOENT;

    char catalog_path[l+2];
    char name[strlen(path) - l];
    if(l==0)
    {
        strncpy(catalog_path, "/", 1);
        catalog_path[1]='\0';
    }
    else
    {
        strncpy(catalog_path, path, l);
        catalog_path[l]='\0';
    }
    strncpy(name, path+l+1, strlen(path) - l-1);
    name[strlen(path) - l-1]='\0';
    printf("dir_name : %s\n", name);
    printf("cat_path : %s\n", catalog_path);

    long catalog_cluster_number = GetNumberFirstClusterByPath(catalog_path);
    printf("cat_cl_num: %ld\n", catalog_cluster_number);
    ReadCatalog(catalog_cluster_number);
    int i;
    for (i=0; i<ELEMENT_COUNT_IN_CATALOG; i++)
    {
        if (temporary_catalog[i].first_cluster == 0)
        {
            break;
        }
    }

    struct File_Record fr;
    strcpy(fr.name, name);
    fr.first_cluster = free_cluster;
    fr.size = 0;
    fr.atime = time(NULL);
    fr.ctime = time(NULL);
    fr.mtime = time(NULL);
    fr.mode = mode;
    fr.gen = 0;
    fr.gid = 0;
    fr.uid = 0;


    temporary_catalog[i] = fr;
    WriteValueFat(free_cluster, EOC);
    WriteCatalog(temporary_catalog, catalog_cluster_number);

    struct File_Record *mas = (struct File_Record *)malloc(sizeof(struct File_Record)*ELEMENT_COUNT_IN_CATALOG);
    WriteFileToClusters(mas, sizeof(struct File_Record)*ELEMENT_COUNT_IN_CATALOG, free_cluster);

    //FindFreeCluster();
    //WriteFreeCluster(free_cluster);
    //FreeClusterIndex(-1);

    return 0;
}


/*
int main()
{
    fsfilename = "fat.dat";
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
        printf("atribute %c\n", temporary_catalog[i].atribute);
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

    return 0;
}
*/
