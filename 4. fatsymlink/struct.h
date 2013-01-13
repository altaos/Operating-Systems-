#define CLUSTER_SIZE 8192//8192 //байты
#define CLUSTER_COUNT_FAT 1048576
//#define FAT_SIZE (CLUSTER_COUNT_FAT * sizeof(long))//4194304
#define EOC 0x0FFFFFFF //конец цепочки кластеров
#define NOT_USED 0 //кластер свободен
#define ELEMENT_COUNT_IN_CATALOG 256

struct File_Record
{
    char name[256]; //название
    long first_cluster; //первый кластер
    long size;//размер в байтах
    ushort mode;//режим и тип файла
    time_t ctime;//дата создания
    time_t mtime;//дата последнего изменения
    time_t atime;//дата послденего доступа
    ushort uid;//user id
    ushort gid;//group id
    long gen;//file generation number
};


