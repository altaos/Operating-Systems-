#include "fsfileops.h"
#include <string.h>

long Create()
{
    long i;

    FILE *file;
    if((file=fopen(fsfilename,"wb"))==0)
    {
        puts ("Can't open output file.");
        return -1;
    }

    long free_cluster_count = CLUSTER_COUNT_FAT-1;
    long free_cluster = 1;

    fwrite(&free_cluster_count, sizeof(free_cluster_count), 1, file);
    fwrite(&free_cluster, sizeof(free_cluster), 1, file);

    // Заполняем таблицу fat
    WriteValueFat(0, EOC);
    for (i=1; i<CLUSTER_COUNT_FAT; i++)
    {
        WriteValueFat(i, NOT_USED);
    }

    printf("Fat is created \n");
    fclose(file);
    return 0;
}

void PrintFileSystemInfo()
{
    printf("========================\n");
    printf("File system info:\n");
    printf("free_cluster_count = %ld\n", ReadFreeClusterCount());
    printf("free_cluster = %ld\n", ReadFreeCluster());
    printf("cluster_count = %d\n", CLUSTER_COUNT_FAT);
    printf("cluster_size = %d\n", CLUSTER_SIZE);
    printf("========================\n");
}

long CreateRootCatalog()
{
    int i;
    for (i=0; i<ELEMENT_COUNT_IN_CATALOG; i++)
    {
        strcpy(root_catalog[i].name,"00000000");
        strcpy(root_catalog[i].extension, "000");
        root_catalog[i].first_cluster = 0;
        root_catalog[i].file_size = 0;
        root_catalog[i].ctime = time(NULL);
        root_catalog[i].mtime = time(NULL);
        root_catalog[i].atribute = '0';
        root_catalog[i].isCatalog = 0;
    }
    if(WriteCatalog(root_catalog, 0) < 0)
        return -EIO;

    printf("Root Catalog is created \n");
}

void PrintRootCatalog(const char *path)
{
    long i;
    long index = GetNumberFirstClusterByPath(path);
    printf ("index %ld\n", index);
    ReadCatalog(index);

    for (i=0; i<ELEMENT_COUNT_IN_CATALOG; i++)
    {
        printf("File Record in Catalog %ld\n", i);
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
}

int main(int argc, char *argv[])
{
    fsfilename = FILE_PATH;
    Create();
    CreateRootCatalog();
    PrintRootCatalog("/");
    PrintFileSystemInfo();
}
