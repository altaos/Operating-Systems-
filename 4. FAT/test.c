#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "fsfileops.h"
 
static const char *hello_str = "Hello World!\n";
static const char *hello_path = "/hello";
char message[500];
 
static int my_getattr(const char *path, struct stat *stbuf)
{
    memset(stbuf, 0, sizeof(struct stat));
    //WriteToLog(path);

    if(strcmp(path, "/") == 0) {
        stbuf->st_ino = 0;
        stbuf->st_mode = S_IFDIR | 0755;
        //sprintf(message, "GETATTR: index = %ld, path = %s", stbuf->st_ino, path);
        //WriteToLog(message);
    }
    else
    {

        //получаем кластер каталога, в котором хранится файл
        long index_catalog = GetParentFirstCluster(path);


//        sprintf(message, "GETATTR: index = %ld, path = %s", index_catalog, path);
//        WriteToLog(message);

        if(index < 0)
            return -ENOENT;

        //читаем каталог
        if (ReadCatalog(index_catalog)< 0)
            return -EIO;

        //получаем первый кластер файла
        long index_file = GetNumberFirstClusterByPath(path);
        //WriteToLog("!!!!!!!!!!!!!!!!!!");

        //ищем нужную строку в записях каталога
        int i=0;
        for (;i<ELEMENT_COUNT_IN_CATALOG; i++)
        {
            if (temporary_catalog[i].first_cluster == index_file)
                break;
        }

        if(i >= ELEMENT_COUNT_IN_CATALOG)
            return -ENOENT;

        stbuf->st_ino = temporary_catalog[i].first_cluster; /* номер первого кластера */
        stbuf->st_mode =  temporary_catalog[i].mode;    /* атрибут */
        //stbuf->st_size = temporary_catalog[i].file_size;    /* размер в байтах */
        stbuf->st_mtime =temporary_catalog[i].mtime;   /* дата последнего изменения */
        stbuf->st_ctime = temporary_catalog[i].ctime;   /* дата создания */
    }

    return 0;
}
 

static int my_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
    long index = fi->fh;
//    sprintf(message, "READDIR: index = %ld", index);
//    WriteToLog(message);

    if (ReadCatalog(index)< 0)
        return -EIO;

    long i;
    for(i = 0; i < ELEMENT_COUNT_IN_CATALOG; i++)
    {
//        sprintf(message, "READDIR: first_cluster = %ld", temporary_catalog[i].first_cluster);
//        WriteToLog(message);

        if (temporary_catalog[i].first_cluster != 0)
            filler(buf, temporary_catalog[i].name, NULL, 0); //TODO: stat вместо NULL
    }
    return 0;
}

int my_opendir(const char *path, struct fuse_file_info *fi)
{
    long index = GetNumberFirstClusterByPath(path);

    if (ReadCatalog(index)< 0)
        return -EIO;

    fi->fh = index;

    return 0;
}

int my_mkdir(const char *path, mode_t mode)
{
    return(CreateDirectory(path, mode | S_IFDIR));
}

int my_mknod(const char *path, mode_t mode, dev_t dev)
{
    if (S_ISREG(mode))
        return CreateFile(path,  S_IFREG);
    return -EINVAL;
}
 
static int hello_open(const char *path, struct fuse_file_info *fi)
{
    if(strcmp(path, hello_path) != 0)
        return -ENOENT;
 
    if((fi->flags & 3) != O_RDONLY)
        return -EACCES;
 
    return 0;
}
 
int my_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    long cluster_number = fi->fh;
    long parent_cluster = GetParentFirstCluster(path);

    if(ReadCatalog(parent_cluster) < 0)
        return -EIO;

    int i = 0;
    while(temporary_catalog[i].first_cluster != cluster_number && i < ELEMENT_COUNT_IN_CATALOG)
        i++;

    if(i == ELEMENT_COUNT_IN_CATALOG)
        return -EIO;

    //long size = temporary_catalog[i].file_size;
    if(ReadFile(buf, size, &(temporary_catalog[i]), offset) < 0)
        return -EIO;

    //temporary_catalog[i].mtime = time(NULL);

    if(WriteCatalog(temporary_catalog, parent_cluster) < 0)
        return -EIO;

    return size;
}

//int my_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
//{

//    long cluster_number = fi->fh;
//    int place_in_catalog = 0;
//    long parent = GetParentFirstClusterForNewPath(path);
//    sprintf(message, "MY WRITE parent = %ld", parent);
//    WriteToLog(message);

//    long j = 0;
//    for(j; j < 50; j++)
//    {
//        sprintf(message, "BEFORE %ld cluster, value = %ld", j, ReadValueFAT(j));
//        WriteToLog(message);
//    }

//    if(ReadCatalog(parent) < 0)
//        return -EIO;

//    PrintCatalog(temporary_catalog);

//    while(temporary_catalog[place_in_catalog].first_cluster != cluster_number && place_in_catalog < ELEMENT_COUNT_IN_CATALOG)
//        place_in_catalog++;

//    sprintf(message, "MY WRITE place in cat = %d", place_in_catalog);
//    WriteToLog(message);

//    if(place_in_catalog == ELEMENT_COUNT_IN_CATALOG)
//        return -ENOENT;

//    WriteFile((void *)buf, size, &(temporary_catalog[place_in_catalog]), offset);
////    WriteFileToClusters((void *)buf, size, cluster_number);
////    temporary_catalog[place_in_catalog].size = size;


//    sprintf(message, "temporary_catalog  name = %s    size = %ld  first_cluster = %ld", temporary_catalog[place_in_catalog].name, temporary_catalog[place_in_catalog].size, temporary_catalog[place_in_catalog].first_cluster);
//    WriteToLog(message);

//    if(WriteCatalog(temporary_catalog, parent) < 0)
//        return -EIO;

//    PrintCatalog(temporary_catalog);
//    for(j = 0; j < 50; j++)
//    {
//        sprintf(message, "AFTER %ld cluster, value = %ld", j, ReadValueFAT(j));
//        WriteToLog(message);
//    }

//    PrintCatalog(temporary_catalog);

////    temporary_catalog[place_in_catalog].ctime = time(NULL);
////    temporary_catalog[place_in_catalog].mtime = time(NULL);
//    //temporary_catalog[place_in_catalog].
//}

int my_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    long cluster_number_file = fi->fh;
    long cluster_number_catalog = GetParentFirstCluster(path);

    if(WriteFileToClusters((void *)buf, size, cluster_number_file) < 0)
        return -1;

    if(ReadCatalog(cluster_number_catalog) < 0)
        return -EIO;

    int i=0;
    for (;i<ELEMENT_COUNT_IN_CATALOG; i++)
    {
        if (temporary_catalog[i].first_cluster == cluster_number_file)
            break;
    }

    temporary_catalog[i].size =  offset + size;

    if (WriteCatalog(temporary_catalog, cluster_number_catalog)< 0)
        return -EIO;
}


static int my_open(const char *path, struct fuse_file_info *fi)
{
    fi->fh = GetNumberFirstClusterByPath(path);
    if((fi->fh) < 0)
        return -ENOENT;

    return 0;
}

int my_truncate(const char *path, off_t newsize)
{
    long index = GetNumberFirstClusterByPath(path);
    long parent = GetParentFirstCluster(path);

    if(TruncFile(index, (long)newsize) < 0)
    {
        return -EIO;
    }

    if(ReadCatalog(parent) < 0)
        return -EIO;

    int i = 0;
    while(temporary_catalog[i].first_cluster != index)
        i++;

    temporary_catalog[i].size = newsize;

    if(WriteCatalog(temporary_catalog, parent) < 0)
        return -EIO;

    return 0;
}

/* Remove a file */
int my_unlink(const char *path)
{
    return RemoveByPath(path);
}

int my_rename(const char *path, const char *newpath)
{
    return Rename(path, newpath);
}

/*Remove a directory*/
int my_rmdir(const char *path)
{
    return RemoveByPath(path);
}

int my_statfs(const char *path, struct statvfs *statv)
{
    int retstat = 0;

    statv->f_bsize = CLUSTER_SIZE;
    statv->f_blocks = CLUSTER_COUNT_FAT;
    statv->f_bfree = ReadFreeClusterCount();
    statv->f_bavail = statv->f_bfree;
    /*statv->f_files = ReadMaxInodesCount();
    statv->f_ffree = ReadFreeInodesCount();
    statv->f_favail = statv->f_ffree;*/

    return retstat;
}

/*Change the access and/or modification times of a file */
int my_utime(const char *path, struct utimbuf *ubuf)
{
    //получаем кластер каталога, в котором хранится файл
    long index_catalog =GetParentFirstCluster(path);

    //читаем каталог
    if (ReadCatalog(index_catalog)< 0)
        return -EIO;

    //получаем первый кластер файла
    long index_file = GetNumberFirstClusterByPath(path);

    //ищем нужную строку в записях каталога
    int i=0;
    for (;i<ELEMENT_COUNT_IN_CATALOG; i++)
    {
        if (temporary_catalog[i].first_cluster == index_file)
            break;
    }

    if(i >= ELEMENT_COUNT_IN_CATALOG)
        return -ENOENT;


    temporary_catalog[i].mtime = ubuf->modtime;   /* дата последнего изменения */

    if(WriteCatalog(temporary_catalog, index_catalog) < 0)
        return -EIO;
    return 0;
}
 
static struct fuse_operations hello_oper;
 
int main(int argc, char *argv[])
{
    if(Load(FILE_PATH) < 0)
    {
            printf("Cann't load file system.\n");
            return -1;
    }

    hello_oper.getattr = my_getattr;
    hello_oper.readdir = my_readdir;
    //hello_oper.read = hello_read;
    hello_oper.opendir = my_opendir;
    hello_oper.mkdir = my_mkdir;
    hello_oper.mknod = my_mknod;
    hello_oper.open = my_open;
    hello_oper.read = my_read;
    hello_oper.unlink = my_unlink;
    hello_oper.write = my_write;
    hello_oper.truncate = my_truncate;
    hello_oper.rename = my_rename;
    hello_oper.rmdir = my_rmdir;
    hello_oper.utime = my_utime;
 
    return fuse_main(argc, argv, &hello_oper, 0);
}
