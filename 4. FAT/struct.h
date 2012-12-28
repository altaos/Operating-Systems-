#define CLUSTER_SIZE 8192 //байты
#define ELEMENT_COUNT_IN_CATALOG 256
#define CLUSTER_COUNT_FAT 1048576
#define FAT_SIZE 4194304
#define EOC 0x0FFFFFFF //конец цепочки кластеров
#define NOT_USED 0 //кластер свободен

struct File_Record
{
    char name[8]; //название
    char extension[3]; //расширение
    long first_cluster; //первый кластер
    long file_size; //размер файла в байтах
    time_t ctime; //дата создания
    time_t mtime;//дата последнего изменения
    char atribute; //атрибуты(r - read only, h - hidden, s - system, a - archive)
    int isCatalog; //каталог или нет
};


