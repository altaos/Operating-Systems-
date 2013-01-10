#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include "fsfileops.h"

/* Get file attributes.
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored.  The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given.
 */
static int my_getattr(const char *path, struct stat *stbuf)
{
    memset(stbuf, 0, sizeof(struct stat));

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

    stbuf->st_ino = temporary_catalog[i].first_cluster; /* номер первого кластера */
    stbuf->st_mode =  temporary_catalog[i].atribute;    /* атрибут */
    stbuf->st_size = temporary_catalog[i].file_size;    /* размер в байтах */
    stbuf->st_mtime =temporary_catalog[i].mtime;   /* дата последнего изменения */
    stbuf->st_ctime = temporary_catalog[i].ctime;   /* дата создания */

    return 0;
}

/* Read directory */
static int my_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
    long index = fi->fh;

    if (ReadCatalog(index)< 0)
        return -EIO;

    long i;
    for(i = 0; i < ELEMENT_COUNT_IN_CATALOG; i++)
    {
        if (temporary_catalog[i].first_cluster != 0)
            filler(buf, temporary_catalog[i].name, NULL, 0); //TODO: stat вместо NULL
    }
    return 0;
}

/* Create a file node
 * There is no create() operation, mknod() will be called for
 * creation of all non-directory, non-symlink nodes.
 */
int my_mknod(const char *path, mode_t mode, dev_t dev)
{
    if (S_ISREG(mode))
        return CreateFile(path, mode);
    return -EINVAL;
}

/* указатель на эту функцию будет передан модулю ядра FUSE в качестве
поля open структуры типа   fuse_operations  - эта функция определяет
имеет ли право пользователь открыть файл /hello нашей файловой системы -
путём анализа данных структуры типа fuse_file_info*/
static int my_open(const char *path, struct fuse_file_info *fi)
{
    fi->fh = GetNumberFirstClusterByPath(path);
    if((fi->fh) < 0)
        return -ENOENT;

    return 0;
}

/* Read data from file */
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
    if(ReadFileFromClusters(buf, cluster_number, size) < 0)
        return -EIO;

    //temporary_catalog[i].mtime = time(NULL);

    if(WriteCatalog(temporary_catalog, parent_cluster) < 0)
        return -EIO;

    return size;
}

int my_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    long cluster_number = fi->fh;
    int place_in_catalog;

    if((place_in_catalog = WriteFileToClusters((void *)buf, size, cluster_number)) < 0)
        return -1;

    if(ReadCatalog(cluster_number) < 0)
        return -EIO;

    temporary_catalog[place_in_catalog].ctime = time(NULL);
    temporary_catalog[place_in_catalog].mtime = time(NULL);
    temporary_catalog[place_in_catalog].atribute = 'a'; //?????????????????????????????????????????????????????
    //temporary_catalog[place_in_catalog].
    int pos = strlen(path) - 1;
    while(path[pos] != '/' && pos > -1)
        pos--;

    int start = ++pos;

    while(path[pos] != '.' && pos < strlen(path))
        pos++;

    if(pos == strlen(path) || pos == (strlen(path) - 1))
    {
        if((pos - start) > 7)
        {
            ReturnBackState(temporary_catalog[place_in_catalog].first_cluster);
            temporary_catalog[place_in_catalog].first_cluster = 0;
            return -1; //имя файла слишком длинное
        }
        strncpy(temporary_catalog[place_in_catalog].name, path + start, pos - start);
        strcpy(temporary_catalog[place_in_catalog].extension, "");
    }
    else
    {
        if((pos - start) > 7)
        {
            ReturnBackState(temporary_catalog[place_in_catalog].first_cluster);
            temporary_catalog[place_in_catalog].first_cluster = 0;
            return -1; //имя файла слишком длинное
        }

        strncpy(temporary_catalog[place_in_catalog].name, path + start, pos - start);
        strncpy(temporary_catalog[place_in_catalog].extension, path + pos + 1, 3);
    }
}

/* Open directory */
int my_opendir(const char *path, struct fuse_file_info *fi)
{
    long index = GetNumberFirstClusterByPath(path);

    if (ReadCatalog(index)< 0)
        return -EIO;

    fi->fh = index;

    return 0;
}

/* Create a directory */
int my_mkdir(const char *path, mode_t mode)
{
    return(CreateDirectory(path, mode));
}

/*Remove a directory*/
int my_rmdir(const char *path)
{
    return RemoveByPath(path);
}

/* Remove a file */
int my_unlink(const char *path)
{
    return RemoveByPath(path);
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

/* Rename a file */
int my_rename(const char *path, const char *newpath)
{
    return Rename(path, newpath);
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

    temporary_catalog[i].file_size = newsize;

    if(WriteCatalog(temporary_catalog, parent) < 0)
        return -EIO;

    return 0;
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

static struct fuse_operations my_operations;

int main(int argc, char *argv[])
{
    if(Load(FILE_PATH) < 0)
    {
        printf("Cann't load file system.\n");
        return -1;
    }
    /*my_operations.getattr = my_getattr;
    my_operations.readdir = my_readdir;
    my_operations.open = my_open;
    my_operations.opendir = my_opendir;
    my_operations.read = my_read;
    my_operations.mkdir = my_mkdir;
    my_operations.rmdir = my_rmdir;
    my_operations.statfs = my_statfs;
    my_operations.utime = my_utime;
    my_operations.rename = my_rename;
    my_operations.mknod = my_mknod;
    my_operations.unlink = my_unlink;
    my_operations.truncate = my_truncate;
    my_operations.read = my_read;
    my_operations.write = my_write;*/
    return fuse_main(argc, argv, &my_operations, 0);
}
