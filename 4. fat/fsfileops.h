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


#define LOG_PATH "/home/alina/Downloads/log.txt"
#define FILE_PATH "/home/alina/Downloads/file.dat"
//#define HOME "/home/alina/Documents/FS1"
/////////////////////////////////////////////////////////////////
void WriteToLog(const char *str);
long ReadFreeClusterCount();
long WriteFreeClusterCount(long value);
long WriteValueFat(long cluster_number, long value);
long FreeClusterIndex(long operation);
long FindFreeCluster();
long ReadFreeCluster();
long WriteFreeCluster(long value);
long ReadCatalog(struct File_Record *buf, long cluster_number);
long WriteCatalog(struct File_Record cat[ELEMENT_COUNT_IN_CATALOG], long cluster_number);
long Load(char *filename);
long ReadFileFromClusters(void *buf, long first_cluster, long file_size);
long CalcCountNeededClusters(long size);
long IsSpaceInCatalog(long cluster_number);
long WriteFileToClusters(void *buf, long size, long first_cluster);
long GetNumberFirstClusterByPath(const char *path);
long CreateFile(const char *path, mode_t mode);
long ReturnBackState(long cluster_number);
long GetParentFirstCluster(const char *path);
long Rename(const char *path, const char *newpath);
long CreateDirectory(const char *path, mode_t mode);
/////////////////////////////////////////////////////////////////

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

long lastNewCluster = -1;
//поиск свободного кластера
long FindFreeCluster()
{
    long i;

    for(i = lastNewCluster+1; i < CLUSTER_COUNT_FAT; i++)
    {
        if(ReadValueFAT(i) == NOT_USED)
        {
            free_cluster = i;
            WriteValueFat(i, EOC);
            FreeClusterIndex(-1);
            lastNewCluster = free_cluster;
            return free_cluster;
        }
    }
    for(i = 0; i < lastNewCluster+1; i++)
    {
        if(ReadValueFAT(i) == NOT_USED)
        {
            free_cluster = i;
            WriteValueFat(i, EOC);
            FreeClusterIndex(-1);
            lastNewCluster = free_cluster;
            return free_cluster;
        }
    }

    //свободных кластеров нет
    return -ENOMEM;
}

//освободить кластер
void FreeCluster(long cluster)
{
    printf("FREE CLUSTER %ld\n", cluster);
    WriteValueFat(cluster, NOT_USED);
    FreeClusterIndex(1);
}

long ReadCluster(long cluster_number, long size, void *buf, long offset)
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

long WriteCluster(long cluster_number, long size, void *buf, long offset)
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

//посчитать количество необходимых для файла кластреров
/*long CalcCountNeededClusters(long size)
{
    return size % CLUSTER_SIZE == 0 ? size/CLUSTER_SIZE : size/CLUSTER_SIZE + 1;
}*/

/*long IsSpaceInCatalog(long cluster_number)
{
    if(ReadCatalog(temporary_catalog, cluster_number) < 0)
        return -EIO;

    int i = 0;
    while(temporary_catalog[i].first_cluster != 0 && i < ELEMENT_COUNT_IN_CATALOG)
        i++;

    if(i == ELEMENT_COUNT_IN_CATALOG)
        return -1;

    return i;
}*/

long cachedFileFirstCluster = -1;
#define CACHE_SIZE 1000000
long cachedClusters[CACHE_SIZE];

long ReadFile(void *buf, long bytes_to_read, struct File_Record *fr, long offset)
{
    long pos = 0;
    long cluster = fr->first_cluster;
    long bytes_read = 0;
    long offs;
    long n;
    long i = 0;

    //проверяем, можно ли воспользоваться кэшем
    if(cachedFileFirstCluster == fr->first_cluster)
    {
        i = offset / CLUSTER_SIZE; //порядковый номер необходимого кластера
        if(i < CACHE_SIZE)
        {
            while(cachedClusters[i] <= 0 && i > 0)//ищем ближайший закэшированный кластер
                i--;
            if(i > 0)
            {
                cluster = cachedClusters[i];
                pos = CLUSTER_SIZE * i;
            }
        }
        else
        {
            cachedFileFirstCluster = -1;
            i = 0;
        }
    }

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

            //printf("READ FILE  cluster %ld  n = %ld   offs = %ld  pos = %ld, bytes_read = %ld\n", cluster, n, offs, pos, bytes_read);

            /*////sprintf(message, "READ FILE  cluster %ld  n = %ld   offs = %ld  pos = %ld", cluster, n, offs, pos);
            ////WriteToLog(message);*/

            ReadCluster(cluster, n, buf+bytes_read, offs);
            bytes_read += n;
        }
        if(cachedFileFirstCluster == fr->first_cluster)
        {
            cachedClusters[i] = cluster;
            i++;
        }
        cluster = ReadValueFAT(cluster);
        pos += CLUSTER_SIZE;
    }

    if(bytes_read < bytes_to_read)
        memset(buf+bytes_read, 0, bytes_to_read-bytes_read);
    fr->atime = time(NULL);

    return 0;
}

long WriteFile(void *buf, long bytes_to_write, struct File_Record *fr, long offset)
{
    long pos = 0;
    long cluster = fr->first_cluster;
    long bytes_wrote = 0;
    long offs;
    long n;
    long i = 0;

    //проверяем, можно ли воспользоваться кэшем
    if(cachedFileFirstCluster == fr->first_cluster)
    {
        i = offset / CLUSTER_SIZE; //порядковый номер необходимого кластера
        if(i < CACHE_SIZE)
        {
            while(cachedClusters[i] <= 0 && i > 0)//ищем ближайший закэшированный кластер
                i--;
            if(i > 0)
            {
                cluster = cachedClusters[i];
                pos = CLUSTER_SIZE * i;
            }
        }
        else
        {
            cachedFileFirstCluster = -1;
            i = 0;
        }
    }

    long previous_cluster = cluster;
    //sprintf(message, "START FROM cluster = %ld; first_cluster = %ld\n", cluster, fr->first_cluster);
    //WriteToLog(message);

    while(bytes_wrote < bytes_to_write)
    {
        if(cluster == EOC)
        {
            cluster = FindFreeCluster();
            WriteValueFat(previous_cluster, cluster);
            //WriteValueFat(cluster, EOC);
            printf("new cluster: %ld\n", cluster);
        }

        if(pos + CLUSTER_SIZE > offset)
        {
            offs = offset - pos > 0 ? offset - pos : 0; //отступ внутри кластера
            n = CLUSTER_SIZE - offs < bytes_to_write - bytes_wrote ? CLUSTER_SIZE - offs : bytes_to_write - bytes_wrote;//сколько байт пишем в кластер
            //printf("WRITE FILE  cluster %ld, prev_cluster = %ld,  n = %ld   offs = %ld  pos = %ld\n", cluster, previous_cluster, n, offs, pos);

            //////sprintf(message, "WRITE FILE  cluster %ld  n = %ld   offs = %ld  pos = %ld", cluster, n, offs, pos);
//            ////WriteToLog(message);

            WriteCluster(cluster, n, buf + bytes_wrote, offs);
            bytes_wrote += n;
        }
        if(cachedFileFirstCluster == fr->first_cluster)
        {
            cachedClusters[i] = cluster;
            i++;
        }
        previous_cluster = cluster;
        cluster = ReadValueFAT(cluster);
        pos += CLUSTER_SIZE;
    }

    fr->size = (fr->size > offset + bytes_wrote) ? fr->size : (offset + bytes_wrote);
    ////sprintf(message, "WRITE FILE file_size = %ld", fr->size);
    ////WriteToLog(message);
    fr->atime = time(NULL);
    fr->mtime = time(NULL);

    return 0;
}


//при написании функции truncate не забыть изменить размер файла в каталоге!!!!
long TruncFile(/*long first_cluster_number*/struct File_Record *fr, long offset)
{
    if(cachedFileFirstCluster == fr->first_cluster)
        cachedFileFirstCluster = -1;
    long pos = 0;
    long cluster = fr->first_cluster;
    long previous_cluster = cluster;
    long offs;

    while(cluster != EOC)
    {
        if(cluster == EOC)
        {
            break;
        }

        previous_cluster = cluster;
        cluster = ReadValueFAT(cluster);
        if(pos + CLUSTER_SIZE > offset)
        {
            offs = offset - pos > 0 ? offset - pos : 0; //отступ внутри кластера
            if(offs == 0 && pos > 0)
                FreeCluster(previous_cluster);
            else
                WriteValueFat(previous_cluster, EOC);
        }
        pos += CLUSTER_SIZE;
    }
    fr->size = offset;
    fr->atime = time(NULL);
    fr->mtime = time(NULL);

    return 0;

}

long ReadCatalog(struct File_Record *buf, long cluster_number)
{
    struct File_Record temp;
    temp.first_cluster = cluster_number;
    temp.size = ELEMENT_COUNT_IN_CATALOG * sizeof(struct File_Record);
    return ReadFile(buf, ELEMENT_COUNT_IN_CATALOG * sizeof(struct File_Record), &temp, 0);
}

//запись каталога
long WriteCatalog(struct File_Record cat[ELEMENT_COUNT_IN_CATALOG], long cluster_number)
{
    struct File_Record temp;
    temp.first_cluster = cluster_number;
    temp.size = ELEMENT_COUNT_IN_CATALOG * sizeof(struct File_Record);
    return WriteFile(cat, ELEMENT_COUNT_IN_CATALOG * sizeof(struct File_Record), &temp, 0);
}

long GetNumberFirstClusterByPath(const char *path)
{
    printf("GET FIRST CLUSTER NUMBER: path = %s\n", path);
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

    struct File_Record temporary_catalog[ELEMENT_COUNT_IN_CATALOG];
    while(pos < strlen(path))
    {
        while(pos < strlen(path) && path[pos] != '/')
            pos++;
        char name[pos - start +1];
        strncpy(name, path+start, pos - start);
        name[pos - start] = '\0';
        start = pos+1;
        pos++;

        if (ReadCatalog(temporary_catalog, cluster_number) != -1)
        {
            cluster_number = -ENOENT;
            for (i=0; i<ELEMENT_COUNT_IN_CATALOG;  i++)
            {
                //printf("temporary_catalog[i].name = %s, name = %s\n", temporary_catalog[i].name, name);
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

long CreateFile(const char *path, mode_t mode)
{
    printf("CREATE FILE path = %s mode = %d;", path, mode);
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
    printf(" parent_path = %s;", catalog_path);

    strncpy(name, path+l+1, strlen(path) - l - 1);
    name[strlen(path) - l - 1]='\0';
    printf(" name = %s;", name);

    long catalog_cluster_number = GetNumberFirstClusterByPath(catalog_path);
    printf(" catalog_cluster_number = %ld;\n", catalog_cluster_number);

    struct File_Record temporary_catalog[ELEMENT_COUNT_IN_CATALOG];
    ReadCatalog(temporary_catalog, catalog_cluster_number);
    int i;
    for (i=0; i<ELEMENT_COUNT_IN_CATALOG; i++)
    {
        if (temporary_catalog[i].first_cluster == 0)
        {
            break;
        }
    }
    if(i >= ELEMENT_COUNT_IN_CATALOG)
        return -ENOMEM;

    struct File_Record fr;
    strcpy(fr.name, name);
    fr.first_cluster = FindFreeCluster();
    fr.size = 0;
    fr.mode = mode;
    fr.atime = time(NULL);
    fr.ctime = time(NULL);
    fr.mtime = time(NULL);
    fr.uid = 0;
    fr.gid = 0;
    fr.size = 0;
    printf("name = %s\n", fr.name);

    temporary_catalog[i] = fr;
    //WriteValueFat(free_cluster, EOC);
    //WriteCatalog(temporary_catalog, catalog_cluster_number);
    struct File_Record temp;
    temp.first_cluster = catalog_cluster_number;
    WriteFile(temporary_catalog, sizeof(temporary_catalog), &temp, 0);

//    ////sprintf(message, "CREATE FILE");
//    ////WriteToLog(message);
//    PrintCatalog(temporary_catalog);
    //WriteToLog("MKNOD SUCCESS");

    return 0;
}

//при неудачной записи освободить кластеры в фат -- по-моему это какой-то ад, а не метод, лучше удалить или переделать как-нибудь
/*long ReturnBackState(long cluster_number)
{
    long cluster = cluster_number;
    long old_cluster = cluster_number;

    while(cluster != EOC)
    {
        while(cluster = ReadValueFAT(cluster) < 0) {}//////////////////////////////
        while(WriteValueFat(old_cluster, NOT_USED) < 0) {}/////////////////////////
    }

    //while(WriteValueFat(old_cluster, NOT_USED) < 0) {}

    return 0;
}*/

//получение номера кластера родительского каталога
long GetParentFirstCluster(const char *path)
{
//    ////sprintf(message, "GET path = %s", path);
//    ////WriteToLog(message);
    int l = strlen(path);
    while(path[l] != '/' && l >= 0)
            l--;

//    ////sprintf(message, "GET l = %d", l);
//    ////WriteToLog(message);

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


    //sprintf(message, "GET PAR par_path = %s", parent_path);
    //WriteToLog(message);
    //long cur_cluster = GetNumberFirstClusterByPath(path);
    //printf("cur_path = %ld\n", cur_cluster);

//    printf("parent_path = %s\n", parent_path);

    //struct File_Record temporary_catalog[ELEMENT_COUNT_IN_CATALOG];
    long parent_cluster = GetNumberFirstClusterByPath(parent_path);
    /*if(ReadCatalog(temporary_catalog, parent_cluster) < 0)
        return -EIO;

    long i = 0;
    while(temporary_catalog[i].first_cluster != cur_cluster && i < ELEMENT_COUNT_IN_CATALOG)
        i++;

    if(i == ELEMENT_COUNT_IN_CATALOG)
        return -ENOENT;*/

    return parent_cluster;
}

void NullRecord(struct File_Record *r)
{
    strcpy(r->name, "\0");
    r->first_cluster = 0;
    r->size = 0;
    r->atime = time (NULL);
    r->ctime = time(NULL);
    r->mtime = time(NULL);
    r->mode = 0;
    r->gen = 0;
    r->gid = 0;
    r->uid = 0;
}

long RemoveFile(long parent_first_cluster, struct File_Record *file)
{
//    long size = temporary_catalog[i].file_size;


    //индекс записи в каталоге для удаляемого файла найден, теперь затираем эту запись

    if(TruncFile(file, 0) < 0)
        return -EIO;
    FreeCluster(file->first_cluster);

    /*struct File_Record temporary_catalog[ELEMENT_COUNT_IN_CATALOG];
    printf("parent_firsr_cluster = %ld\n", parent_first_cluster);
    printf("child_firsr_cluster = %ld\n", file->first_cluster);
    if (ReadCatalog(temporary_catalog, parent_first_cluster) < 0)
        return -EIO;

    int i = 0;
    while(file->first_cluster != temporary_catalog[i].first_cluster && i < ELEMENT_COUNT_IN_CATALOG)
        i++;

    printf("i file in catalog i = %d\n", i);

    if(i == ELEMENT_COUNT_IN_CATALOG)
        return -1;


    printf("Value in Fat %ld value : %ld", file->first_cluster, ReadValueFAT(file->first_cluster));

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

    if(WriteCatalog(temporary_catalog, parent_first_cluster) < 0)
        return -EIO;
    printf("parent_firsr_cluster = %ld\n", parent_first_cluster);*/

    return 1;
}

long RemoveDirectory(long parent_catalog_first_cluster, struct File_Record *dir)
{
    struct File_Record temporary_catalog[ELEMENT_COUNT_IN_CATALOG];
    if (ReadCatalog(temporary_catalog, dir->first_cluster) < 0)
        return -EIO;

    //удаляем все файлы и папки из каталога, который будем удалять
    long i = 0;
    for (; i<ELEMENT_COUNT_IN_CATALOG; i++)
    {
        if ((temporary_catalog[i].mode & S_IFDIR) && temporary_catalog[i].first_cluster != 0)
        {   //если хранится директория, то удаляем ее, но сначала ее содержимое
            printf("Temporary_catalog[%ld].mode = [%d] \n Temporary_catalog[%ld].fist_cluster = %ld\n", i, temporary_catalog[i].mode, i, temporary_catalog[i].first_cluster);
            RemoveDirectory(dir->first_cluster, &(temporary_catalog[i]));
            NullRecord(&(temporary_catalog[i]));
        }
        else if(temporary_catalog[i].first_cluster != 0) //если это файл,то необходимо удалить ее содержимое
        {
            printf("Temporary_catalog[%ld].mode = [%d] \n Temporary_catalog[%ld].fist_cluster = %ld\n", i, temporary_catalog[i].mode, i, temporary_catalog[i].first_cluster);
            RemoveFile(dir->first_cluster, &(temporary_catalog[i]));
        }
    }
    long child_first_cluster = dir->first_cluster;

    TruncFile(dir, 0);
    // удаляем запись об удаляемом каталоге в родительском каталоге
    /*if (ReadCatalog(temporary_catalog, parent_catalog_first_cluster) < 0)
        return -EIO;

    i = 0;
    while(child_first_cluster != temporary_catalog[i].first_cluster)
        i++;

    printf("Number in parent catalog %ld\n", i);

    NullRecord(&(temporary_catalog[i]));

    if(WriteCatalog(temporary_catalog, parent_catalog_first_cluster) < 0)
        return -EIO;*/
    FreeCluster(dir->first_cluster);

    return 0;
}

long RemoveByPath(const char *path)
{
    //printf("REMOVE BY PATH : bb path = %s", path);
    long parent_cluster = GetParentFirstCluster(path);
    printf("REMOVE BY PATH : parent cluster = %ld, path = %s\n", parent_cluster, path);

    struct File_Record temporary_catalog[ELEMENT_COUNT_IN_CATALOG];
    if(ReadCatalog(temporary_catalog, parent_cluster) < 0)
        return -EIO;

    long cluster_number = GetNumberFirstClusterByPath(path);
    //printf("REMOVE BY PATH : path = %s", path);
    printf("REMOVE BY PATH : parent_cluster = %ld, cluster = %ld", parent_cluster, cluster_number);

    if(cluster_number < 0)
        return -ENOENT;

    int i = 0;

    //ищем запись в каталоге
    while(cluster_number != temporary_catalog[i].first_cluster) i++;

    //если каталог
    if(temporary_catalog[i].mode & S_IFDIR)
    {
        if(RemoveDirectory(parent_cluster, &(temporary_catalog[i])) < 0)//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            return -EIO;
    }
    else
    {
        if(RemoveFile(parent_cluster, &(temporary_catalog[i])) < 0)
            return -EIO;
    }
    NullRecord(&(temporary_catalog[i]));

    printf("REMOVE BY PATH : WRITING CATALOG parent_cluster = %ld\n", parent_cluster);
    if(WriteCatalog(temporary_catalog, parent_cluster) < 0)
        return -EIO;
    return 0;
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


    struct File_Record temporary_catalog[ELEMENT_COUNT_IN_CATALOG];
    //считываем информацию о файле из каталога, где он хранился
    if(ReadCatalog(temporary_catalog, old_parent_cluster) < 0)
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
    if(ReadCatalog(temporary_catalog, new_parent_cluster) < 0)
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
        if(ReadCatalog(temporary_catalog, old_parent_cluster) < 0)
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
    if(catalog_cluster_number < 0)
        return -ENOENT;

    struct File_Record temporary_catalog[ELEMENT_COUNT_IN_CATALOG];
    ReadCatalog(temporary_catalog, catalog_cluster_number);
    int i;
    for (i=0; i<ELEMENT_COUNT_IN_CATALOG; i++)
    {
        if (temporary_catalog[i].first_cluster == 0)
        {
            break;
        }
    }
    if(i == ELEMENT_COUNT_IN_CATALOG)
        return -ENOMEM;

    struct File_Record fr;
    strcpy(fr.name, name);
    fr.first_cluster = FindFreeCluster();
    fr.size = 0;
    fr.atime = time(NULL);
    fr.ctime = time(NULL);
    fr.mtime = time(NULL);
    fr.mode = mode;
    fr.gen = 0;
    fr.gid = 0;
    fr.uid = 0;


    temporary_catalog[i] = fr;
    //WriteValueFat(free_cluster, EOC);
    WriteCatalog(temporary_catalog, catalog_cluster_number);

    struct File_Record mas[ELEMENT_COUNT_IN_CATALOG];
    memset(mas, 0, sizeof(mas));
    struct File_Record temp;
    temp.first_cluster = free_cluster;
    WriteFile(mas, sizeof(struct File_Record)*ELEMENT_COUNT_IN_CATALOG, &temp, 0);

    //FindFreeCluster();
    //WriteFreeCluster(free_cluster);
    //FreeClusterIndex(-1);

    return 0;
}
