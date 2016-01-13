#define CLUSTERSIZE 10

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
 
static const char *hello_str = "It`s work!\n";
static const char *hello_path = "/hello";

typedef struct Cluster* LinkCl; //сылка на кластер
struct Cluster //кластер
{
    int id;
    int isFull;
    char content[CLUSTERSIZE]; //содержимое 1 кластера
    LinkCl nextCluster; //ссылка на следующий кластер
};

struct  Node
{
    int idCluster; //id первого кластера
    LinkCl firstCl; //ссылка на самый первый кластер
    int isDir;  //пояснение о дирректории
    int isEmpty;  //проверка существования
    int index;  
    //int childCount;
};

typedef struct Node NodeType;

//нужно будет реализовать:

//поиск файла в кластерах папки
NodeType* seekFile(NodeType* node,char* name);
//поиск файла путем парсинга path (будет вызывать seekFile)
NodeType* seekConcreteFile(const char* path);
//создание пустых кластеров
void createFreeClusters();
//получение имени файла
char* getName(const char* path);
//получение имени дириктории в которой находится файл
char* getRootName(const char* path);
//запись в кластеры
void writeToClusters(const char* content, LinkCl cluster);
//удаление файла из кластеров и массива файлов
void deleteFileFromCl(NodeType* node);
//получение последнего свободного кластера (для удаления)
LinkCl getEndCl();
//удаление информации из дириктории
void deleteFromDir(const char* path);

static int cl_getattr(const char *path, struct stat *stbuf)
{
    int res = 0;
 
    memset(stbuf, 0, sizeof(struct stat));
    if(strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    }
    else if(strcmp(path, hello_path) == 0) {
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = strlen(hello_str);
    }
    else
        res = -ENOENT;
 
    return res;
}
 
static int cl_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
    (void) offset;
    (void) fi;
 
    if(strcmp(path, "/") != 0)
        return -ENOENT;
 
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    filler(buf, hello_path + 1, NULL, 0);
 
    return 0;
}
 
static int cl_open(const char *path, struct fuse_file_info *fi)
{
    if(strcmp(path, hello_path) != 0)
        return -ENOENT;
 
    if((fi->flags & 3) != O_RDONLY)
        return -EACCES;
 
    return 0;
}
 
static int cl_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    size_t len;
    (void) fi;
    if(strcmp(path, hello_path) != 0)
        return -ENOENT;
 
    len = strlen(hello_str);
    if (offset < len) {
        if (offset + size > len)
            size = len - offset;
        memcpy(buf, hello_str + offset, size);
    } else
        size = 0;
 
    return size;
}
 
static struct fuse_operations operations;
 
int main(int argc, char *argv[])
{
    operations.getattr = cl_getattr;
    operations.readdir = cl_readdir;
    operations.open = cl_open;
    operations.read = cl_read;
 
    return fuse_main(argc, argv, &operations, 0);
}