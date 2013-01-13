#include "fsfileops1.h"
#include <string.h>
char message[500];

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
    for (i=0; i<CLUSTER_COUNT_FAT; i++)
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
        root_catalog[i].gen = i;
        root_catalog[i].gid = 0;
        root_catalog[i].uid = 0;
    }
    printf("Root Caaaaaatalog \n");
    long cluster = FindFreeCluster();
    printf("clusterrrrrrr = %ld\n", cluster);
    //printf("clusterrrrrrr = %ld\n", ReadValueFAT(cluster));
    if(WriteCatalog(root_catalog, cluster) < 0)
    {
        printf("Root Catalog is fail \n");
        return -EIO;
    }

    printf("Root Catalog is created \n");
}

void PrintCatalogByPath(const char *path)
{
    printf("@=========================================\n");
    long i;
    long index = GetNumberFirstClusterByPath(path);
    printf("%s: index %ld\n", path, index);
    struct File_Record temporary_catalog[ELEMENT_COUNT_IN_CATALOG];
    ReadCatalog(temporary_catalog, index);

    for (i=0; i<ELEMENT_COUNT_IN_CATALOG; i++)
    {
        if(temporary_catalog[i].first_cluster > 0)
        {
            /*sprintf(message, "%ld Name, path = %s", i, temporary_catalog[i].name);
            WriteToLog(message);*/
            printf("#%ld %s: first cluster = %ld, size = %ld, mode = %o, gen %ld\n", i, temporary_catalog[i].name, temporary_catalog[i].first_cluster, temporary_catalog[i].size, temporary_catalog[i].mode, temporary_catalog[i].gen);
        }
    }
    printf("@=========================================\n");
}

int FillMass(char *fd)
{
    long i;
    char arr[5] = {'a', 'b', 'c', 'd', 'e'};
    for(i = 0; i < 160000 - 1; i++)
    {
        fd[i] = arr[i%(sizeof(arr)/sizeof(arr[0]))];
    }
    fd[i] = '\0';
}

int main(int argc, char *argv[])
{
    //printf("%ld\n", sizeof(struct File_Record));
    //return 0;
    fsfilename = FILE_PATH;
    Create();
    Load(FILE_PATH);
    CreateRootCatalog();
    Load(FILE_PATH);
    int i = 0;

    //CreateDirectory("/Untitled Folder", S_IFDIR);
    for(i; i < 5; i++)
    {
    /*CreateDirectory("/Untitled Folder/Untitled Folder", S_IFDIR);
    CreateDirectory("/Untitled Folder/Untitled Folder/Untitled Folder", S_IFDIR);
    CreateDirectory("/Untitled Folder/Untitled Folder/Untitled Folder/Untitled Folder", S_IFDIR);*/
        /*CreateDirectory("/Examp", S_IFDIR);
        CreateDirectory("/Yes", S_IFDIR);*/

    /*CreateFile("/Examp/Текстовый документ", S_IFREG | 0777);
    CreateFile("/Examp/Yes/Текстовый документ 1", S_IFREG | 0777);*/

    //printf("%s\n", file);
    //RemoveByPath("/Untitled Folder/Untitled Folder");
    /*PrintCatalogByPath("/Examp");
    RemoveByPath("/Try");
    PrintCatalogByPath("/");
    RemoveByPath("/Examp/Yes/Текстовый документ 1");
    PrintCatalogByPath("/Examp/Yes");
    RemoveByPath("/Examp");*/
    //PrintCatalogByPath("/");
    }
    /*CreateFile("/Текстовый документ", S_IFREG | 0777);
    char file[160000];
    FillMass(file);

    char buf[CLUSTER_SIZE];

    struct File_Record temporary_catalog[ELEMENT_COUNT_IN_CATALOG];
    ReadCatalog(temporary_catalog, 0);

    WriteFile(file, sizeof(file), &(temporary_catalog[0]), 0);
    memset(file,'g', sizeof(file));
    WriteFile(file, sizeof(file), &(temporary_catalog[0]), sizeof(file));
    //ReadFile(file, sizeof(file), &(temporary_catalog[i]), 0);
    WriteCatalog(temporary_catalog, 0);*/
    PrintCatalogByPath("/");

    PrintFileSystemInfo();
//    PrintFileSystemInfo();
//    TruncFile((long)20, (long)40040);
    //RemoveDirectory((long)0, (long)20);
//    for(i = 0; i < 60; i++)
//        printf("№ %d       value : %ld\n", i, ReadValueFAT(i));
//    for(i = 18; i < 21; i++)
//         printf("№ %d       value : %ld\n", i, ReadValueFAT(i));
}
