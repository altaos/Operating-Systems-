#include "fsfileops.h"
#include <string.h>
char message[500];


void PrintFileSystemInfo()
{
    printf("========================\n");
    printf("File system info:\n");
    printf("free_cluster_count = %ld\n", ReadFreeClusterCount());
    printf("cluster_count = %d\n", CLUSTER_COUNT_FAT);
    printf("cluster_size = %d\n", CLUSTER_SIZE);
    printf("========================\n");
}

void PrintCatalogByPath(const char *path)
{
    printf("@=========================================\n");
    long i;
    long index = GetNumberFirstClusterByPath(path);
    if(index < 0)
        return;
    printf("%s: index %ld\n", path, index);
    struct File_Record temporary_catalog[ELEMENT_COUNT_IN_CATALOG];
    if(ReadCatalog(temporary_catalog, index) < 0)
        return;

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
    for(i = 0; i < 16000 - 1; i++)
    {
        fd[i] = arr[i%(sizeof(arr)/sizeof(arr[0]))];
    }
    fd[i] = '\0';
}

int main(int argc, char *argv[])
{
    Load(FILE_PATH);
    PrintCatalogByPath("/");
//    PrintCatalogByPath("/Untitled Folder");
//    PrintCatalogByPath("/Untitled Folder/Untitled Folder");
//    PrintCatalogByPath("/Untitled Folder/Untitled Folder/Untitled Folder");
//    PrintCatalogByPath("/Untitled Folder/Untitled Folder/Untitled Folder/Untitled Folder");
//    PrintCatalogByPath("/Untitled Folder/Untitled Folder/Untitled Folder/Untitled Folder/Untitled Folder");
    printf("=========================================\n");
    PrintFileSystemInfo();
//    PrintFileSystemInfo();
//    TruncFile((long)20, (long)40040);
    //RemoveDirectory((long)0, (long)20);
//    for(i = 0; i < 60; i++)
//        printf("№ %d       value : %ld\n", i, ReadValueFAT(i));
//    for(i = 18; i < 21; i++)
//         printf("№ %d       value : %ld\n", i, ReadValueFAT(i));
}
