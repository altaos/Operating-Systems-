#include "fsfileops.h"
#include <string.h>
char message[500];

char file[16000];

long Create()
{
    long i;

    FILE *file;
    if((file=fopen(fsfilename,"wb"))==0)
    {
        puts ("Can't open output file.");
        return -1;
    }

    long free_cluster_count = CLUSTER_COUNT_FAT;
    long free_cluster = 0;

    fwrite(&free_cluster_count, sizeof(free_cluster_count), 1, file);
    fwrite(&free_cluster, sizeof(free_cluster), 1, file);

    // Заполняем таблицу fat
    WriteValueFat(0, NOT_USED);
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
    struct File_Record  root_catalog[ELEMENT_COUNT_IN_CATALOG];

    for (i=0; i<ELEMENT_COUNT_IN_CATALOG; i++)
    {
        //memset(root_catalog[i].name, 0, 256);
        strcpy(root_catalog[i].name, "aaa");
        root_catalog[i].first_cluster = 0;
        root_catalog[i].size = 0;
        root_catalog[i].atime = time (NULL);
        root_catalog[i].ctime = time(NULL);
        root_catalog[i].mtime = time(NULL);
        root_catalog[i].mode = 0;
        root_catalog[i].gen = 0;
        root_catalog[i].gid = 0;
        root_catalog[i].uid = 0;
    }
    printf("Root Caaaaaatalog \n");
    if(WriteFileToClusters(root_catalog, sizeof(struct File_Record)*ELEMENT_COUNT_IN_CATALOG, 0) < 0)
    {
        printf("Root Catalog is fail \n");
        return -EIO;
    }

    printf("Root Catalog is created \n");
}

void PrintRootCatalog(const char *path)
{
    long i;
    long index = GetNumberFirstClusterByPath(path);
    printf ("index %ld\n", index);
    ReadCatalog(index);

    for (i=0; i<10; i++)
    {
        printf("File Record in Catalog %ld\n", i);
        int j;
        printf("name: ");
        for (j=0; j<256; j++)
        {
            printf("%c", temporary_catalog[i].name[j]);
        }
        sprintf(message, "%ld Name, path = %s", i, temporary_catalog[i].name);
        WriteToLog(message);
        printf("\n");
        printf("first %ld\n", temporary_catalog[i].first_cluster);
        printf("size %ld\n", temporary_catalog[i].size);
        printf("mode %d\n", temporary_catalog[i].mode);
        printf("gen %ld\n", temporary_catalog[i].gen);
    }
}

int FillMass(char *fd)
{
    long i;
    for(i = 0; i < 16000 - 1; i++)
    {
        fd[i] = 'a';
    }
    fd[i] = '\0';
}

int main(int argc, char *argv[])
{
    //printf("%ld\n", sizeof(struct File_Record));
    //return 0;
    fsfilename = FILE_PATH;
    Create();
    CreateRootCatalog();
    int i = 0;
//    for(i = 0; i < 11; i++)
//        printf("№ %d       value : %ld\n", i, ReadValueFAT(i));
    CreateDirectory("/Examp", S_IFDIR);
    CreateDirectory("/Try", S_IFDIR);
    CreateDirectory("/Examp/Yes", S_IFDIR);
    //PrintRootCatalog("/");
//    PrintRootCatalog("/Examp/Yes");
//    CreateFile("/NewFile", S_IFREG);
//    printf("paren_f_cluster %ld\n", GetParentFirstCluster("/Examp/Yes"));
    PrintFileSystemInfo();
//    FillMass(file);
//    ReadCatalog(0);
//    CreateFile("/filesa", S_IFREG);
//    PrintRootCatalog("/");
//    long cl = GetNumberFirstClusterByPath("/filesa");
//    long j = 0;
//    while(temporary_catalog[j].first_cluster != cl && j < ELEMENT_COUNT_IN_CATALOG)
//    {
//        j++;
//    }

//    printf("index = %ld  Name = %s First_cluster = %ld\n", j, temporary_catalog[j].name, temporary_catalog[j].first_cluster);
//    WriteFile(file, 16000, &(temporary_catalog[j]), 0);
    PrintRootCatalog("/");
//    RemoveFile((long)0, (long)40);
//    RemoveByPath("/NewFile");
//    PrintRootCatalog("/");
    printf("=========================================\n");
//    PrintFileSystemInfo();
//    TruncFile((long)20, (long)40040);
    //RemoveDirectory((long)0, (long)20);
    for(i = 0; i < 60; i++)
        printf("№ %d       value : %ld\n", i, ReadValueFAT(i));
//    for(i = 18; i < 21; i++)
//         printf("№ %d       value : %ld\n", i, ReadValueFAT(i));
}
