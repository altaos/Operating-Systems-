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
        ////sprintf(message, "GETATTR: index = %ld, path = %s", stbuf->st_ino, path);
        ////WriteToLog(message);
    }
    else
    {
        struct File_Record temporary_catalog[ELEMENT_COUNT_IN_CATALOG];
        //получаем кластер каталога, в котором хранится файл
        long index_catalog = GetParentFirstCluster(path);



        //sprintf(message, "GETATTR: parent = %ld, path = %s", index_catalog, path);
        //WriteToLog(message);

        if(index_catalog < 0)
            return -ENOENT;

        //читаем каталог
        if (ReadCatalog(temporary_catalog, index_catalog)< 0)
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
        stbuf->st_size = temporary_catalog[i].size;    /* размер в байтах */
        stbuf->st_mtime =temporary_catalog[i].mtime;   /* дата последнего изменения */
        stbuf->st_ctime = temporary_catalog[i].ctime;   /* дата создания */
    }

    return 0;
}

static int my_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
    long index = GetNumberFirstClusterByPath(path);
//    //sprintf(message, "READDIR: index = %ld", index);
//    //WriteToLog(message);

    struct File_Record temporary_catalog[ELEMENT_COUNT_IN_CATALOG];
    if (ReadCatalog(temporary_catalog, index)< 0)
        return -EIO;

    long i;
    for(i = 0; i < ELEMENT_COUNT_IN_CATALOG; i++)
    {
//        //sprintf(message, "READDIR: first_cluster = %ld", temporary_catalog[i].first_cluster);
//        //WriteToLog(message);

        if (temporary_catalog[i].first_cluster != 0)
            filler(buf, temporary_catalog[i].name, NULL, 0); //TODO: stat вместо NULL
    }
    return 0;
}

int my_opendir(const char *path, struct fuse_file_info *fi)
{
    long index = GetNumberFirstClusterByPath(path);

    /*struct File_Record temporary_catalog[ELEMENT_COUNT_IN_CATALOG];
    if (ReadCatalog(temporary_catalog, index)< 0)
        return -EIO;*/
    if(index < 0)
        return -ENOENT;

    fi->fh = index;

    return 0;
}

int my_mkdir(const char *path, mode_t mode)
{
    return(CreateDirectory(path, mode | S_IFDIR));
}

int my_mknod(const char *path, mode_t mode, dev_t dev)
{
    //sprintf(message, "MKNOD: path = %s, mode = %o", path, mode);
    //WriteToLog(message);
    if (S_ISREG(mode))
        return CreateFile(path,  mode | S_IFREG);
    return -EINVAL;
}
 
int my_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    long cluster_number = fi->fh;
    long parent_cluster = GetParentFirstCluster(path);

    struct File_Record temporary_catalog[ELEMENT_COUNT_IN_CATALOG];
    if(ReadCatalog(temporary_catalog, parent_cluster) < 0)
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

int my_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    long cluster_number_file = fi->fh;
    long cluster_number_catalog = GetParentFirstCluster(path);

    struct File_Record temporary_catalog[ELEMENT_COUNT_IN_CATALOG];
    if(ReadCatalog(temporary_catalog, cluster_number_catalog) < 0)
        return -EIO;

    int i=0;
    for (;i<ELEMENT_COUNT_IN_CATALOG; i++)
    {
        if (temporary_catalog[i].first_cluster == cluster_number_file)
            break;
    }

    if(WriteFile((void *)(buf), size, &(temporary_catalog[i]), (long) offset) < 0)
        return -1;

    //temporary_catalog[i].size =  offset + size;

    if (WriteCatalog(temporary_catalog, cluster_number_catalog)< 0)
        return -EIO;

    return size;
}


static int my_open(const char *path, struct fuse_file_info *fi)
{
    //sprintf(message, "OPEN: path = %s", path);
    //WriteToLog(message);
    fi->fh = GetNumberFirstClusterByPath(path);
    if((fi->fh) < 0)
        return -ENOENT;

    cachedFileFirstCluster = fi->fh;//теперь кэшируем кластеры этого файла
    memset(cachedClusters, 0, sizeof(cachedClusters));//очищаем старый кэш

    return 0;
}

int my_truncate(const char *path, off_t newsize)
{
    long index = GetNumberFirstClusterByPath(path);
    long parent = GetParentFirstCluster(path);
    if(index < 0 || parent < 0)
        return -ENOENT;

    struct File_Record temporary_catalog[ELEMENT_COUNT_IN_CATALOG];
    if(ReadCatalog(temporary_catalog, parent) < 0)
        return -EIO;

    int i = 0;
    while(temporary_catalog[i].first_cluster != index)
        i++;
    if(i == ELEMENT_COUNT_IN_CATALOG)
        return -ENOENT;

    if(TruncFile(&(temporary_catalog[i]), (long)newsize) < 0)
    {
        return -EIO;
    }

    if(WriteCatalog(temporary_catalog, parent) < 0)
        return -EIO;

    return 0;
}

/* Remove a file */
int my_unlink(const char *path)
{
    //sprintf(message, "UNLINK: path = %s", path);
    //WriteToLog(message);
    return RemoveByPath(path);
}

int my_rename(const char *path, const char *newpath)
{
    return Rename(path, newpath);
}

/*Remove a directory*/
int my_rmdir(const char *path)
{
    //sprintf(message, "RMDIR: path = %s", path);
    //WriteToLog(message);
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

    struct File_Record temporary_catalog[ELEMENT_COUNT_IN_CATALOG];
    //читаем каталог
    if (ReadCatalog(temporary_catalog, index_catalog)< 0)
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
