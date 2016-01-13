#define CLUSTERSIZE 10
#define CLUSTERCOUNT 20
#define FILECOUNT 100

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
 
static const char *hello_str = "It`s work!\n";
static const char *hello_path = "/hello";

//------------------------------------------------------------------------------------------------------------------//
//----------------------------------------------   Claster & Node   ------------------------------------------------//
//------------------------------------------------------------------------------------------------------------------//

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

static LinkCl freeCluster;
static NodeType* files[FILECOUNT];
static LinkCl clusters[CLUSTERCOUNT];

//------------------------------------------------------------------------------------------------------------------//
//------------------------------------------   Нужно будет реализовать   -------------------------------------------//
//------------------------------------------------------------------------------------------------------------------//

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
//------------------------------------------------------------------------------------------------------------------//
//------------------------------------------   Методы и ф-ии для Fuse   --------------------------------------------//
//------------------------------------------------------------------------------------------------------------------//

static void *cl_init(struct fuse_conn_info * conn) {//initialize
    char *tmpFiles = " test 1 test1 2";
    NodeType *myDir = (NodeType *)malloc(sizeof(NodeType));
    myDir->isDir = 1; //говорим, что это дирректория
    myDir->idCluster = 0; //присваиваем 0 id кластера
    

    createFreeClusters();
    myDir->firstCl = freeCluster; //присваиваем пустой кластер
    writeToClusters(tmpFiles, myDir->firstCl); //проверка 
    files[0] = myDir;

    //забиваем всё оставшееся место файлами
    for(int i = 1; i<100; i++){
      NodeType *myFileTmp = (NodeType *)malloc(sizeof(NodeType));
      myFileTmp->isEmpty = 1;
      myFileTmp->isDir = 0;
      files[i] = myFileTmp;
    }

    NodeType *myFile = (NodeType *)malloc(sizeof(NodeType));
    myFile->idCluster = freeCluster->id;
    LinkCl tmpcluster = freeCluster;
    myFile->firstCl = freeCluster;
    myFile->index = 1;
    myFile->isEmpty = 0;
    files[1] = myFile;
    freeCluster = freeCluster->nextCluster;
    

    NodeType *myFile1 = (NodeType *)malloc(sizeof(NodeType));
    myFile1->idCluster = freeCluster->id;
    myFile1->firstCl = freeCluster;
    myFile1->index = 2;
    myFile1->isEmpty = 0;
    files[2] = myFile1;
    freeCluster = freeCluster->nextCluster;

    //tmpcluster->nextCluster = 0;
    files[1]->firstCl->nextCluster = 0;
    files[2]->firstCl->nextCluster = 0;

    printf("Filesystem has been initialized!\n");

    return NULL;
}

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


//------------------------------------------------------------------------------------------------------------------//
//---------------------------------------   Мои вспомогательне функции   -------------------------------------------//
//------------------------------------------------------------------------------------------------------------------//

void writeToClusters(const char* content, LinkCl cluster){
  int len = strlen(content);

  int j = 0;
  LinkCl tmpCluster = cluster;

  while(tmpCluster->isFull == 1)//пропускаем, если кластер заполнен, пока не нейдём свободный
  {
    tmpCluster = tmpCluster->nextCluster;
  }

  while(tmpCluster->content[j] != '\0')
  {
    j++;
  }

  int i, iCon = -1; //нужны для счётчика
  LinkCl tmpClusterWrite = tmpCluster;  //кластер, в который в данный момент записывают
  //добавление кластеров и дозапись
  while(iCon<len){
    for(i = j; i<CLUSTERSIZE; i++) //дописываем в кластер нужную информацию
    {
      if(iCon < len)
      {
        tmpClusterWrite->content[i] = content[++iCon];
      }
    }

    if(iCon < len)  //создаём новый кластер
    {
      tmpClusterWrite->isFull = 1;
      if(tmpClusterWrite->nextCluster == 0)
      {
        tmpClusterWrite->nextCluster = freeCluster;
      }

      tmpClusterWrite = tmpCluster->nextCluster;
    }
    j = 0;  //обнуляем счётчик, что бы записывалось в начало след. кластера
  }
  if(tmpClusterWrite->nextCluster != 0)//если у кластера остались "наследники", 
    //то программа обозначает след. наследника как пустой, что бы в след. раз всё писалось в него
  {
    freeCluster  = tmpClusterWrite->nextCluster;
    tmpClusterWrite->nextCluster = 0;//говорит, что наследников нет
  }
  tmpClusterWrite->content[i] = '\0'; //записываем окончание файла

}

void createFreeClusters()
{
  freeCluster = (LinkCl)malloc(sizeof(ClusterType));//выделяем память
  LinkCl tmpCluster = freeCluster;

  char str[10];
  clusters[0] = tmpCluster;
  for (int i = 1; i<CLUSTERCOUNT; i++){
    LinkCl cluster = (LinkCl)malloc(sizeof(ClusterType));
    
    sprintf(str, "%d", i);
    //проверка кластеров
    //strncpy(cluster->content, str, sizeof(cluster->content));
    tmpCluster->isFull = 0;
    tmpCluster->id = i;
    tmpCluster->nextCluster = cluster;
    clusters[i] = cluster;
    tmpCluster = cluster;
  }
}

