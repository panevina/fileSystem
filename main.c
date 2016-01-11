#define FUSE_USE_VERSION  26
#define CLUSTERCOUNT 20
#define CLUSTERSIZE 10

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

static int cl_getattr(const char *path, struct stat *stbuf);
static int cl_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);
static int cl_open(const char *path, struct fuse_file_info *fi);
//static int cl_release(const char *path, struct fuse_file_info *fi);
static int cl_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info * fi);
static void *cl_init(struct fuse_conn_info * conn);
static void cl_destroy(void *a);
static int cl_truncate(const char *path, off_t size);
//static int cl_create(const char *path, mode_t mode, struct fuse_file_info *fi);
static int cl_write(const char *path, const char *content, size_t size, off_t offset, struct fuse_file_info *fi);
static int cl_mkdir(const char* path, mode_t mode);
static int cl_mknod(const char* path, mode_t mode, dev_t dev);
static int cl_create(const char *path, mode_t mode, struct fuse_file_info *fi);
static int cl_unlink(const char *path);

static struct fuse_operations oper = {
    .readdir = cl_readdir,
    //.create = cl_create,
    .open = cl_open,
    .read = cl_read,
    .mknod = cl_mknod,
    .write = cl_write,
    .mkdir = cl_mkdir,
    .truncate = cl_truncate,
    //.release = cl_release,
    //.flush = NULL,
    .getattr = cl_getattr,
    .destroy = cl_destroy,
    .init = cl_init,
    .unlink = cl_unlink,
    //.rename = cl_rename,
};

typedef struct Cluster* LinkCl; 
struct Cluster 
{
    int id;
    int isFull;
    char content[CLUSTERSIZE]; 
    LinkCl nextCluster; 
};

typedef struct Cluster ClusterType;

struct  Node
{
    int idCluster;
    LinkCl firstCl;
    int isDir;
    int isEmpty;
    int index;
    //int childCount;
};


typedef struct Node NodeType;

//мои функции
NodeType* seekFile(NodeType* node,char* name);
NodeType* seekConcreteFile(const char* path);
void createFreeClusters();
char* getName(const char* path);
char* getRootName(const char* path);
void writeToClusters(const char * content, LinkCl cluster);


static LinkCl freeCluster;
static NodeType* files[100];
static LinkCl clusters[CLUSTERCOUNT];


int main(int argc, char *argv[]) {

    return fuse_main(argc, argv, &oper, NULL);
}

static void *cl_init(struct fuse_conn_info * conn) {
  printf("----------------------\n");
    printf("initialize\n");

    char *tmpFiles = " test 1 test1 2";
    NodeType *myDir = (NodeType *)malloc(sizeof(NodeType));
    myDir->isDir = 1;
    myDir->idCluster = 0;
    

    createFreeClusters();
    myDir->firstCl = freeCluster;
    writeToClusters(tmpFiles, myDir->firstCl);
    //printf("%d\n",myDir->firstCl->nextCluster->isFull);
    //myDir->firstCl->nextCluster->isFull = 1;
    files[0] = myDir;
 

    //create files
    for(int i = 1; i<100; i++){
      NodeType *myFileTmp = (NodeType *)malloc(sizeof(NodeType));
      myFileTmp->isEmpty = 1;
      myFileTmp->isDir = 0;
      files[i] = myFileTmp;
    }

    //create clusters


    /*for(int i = 0; i<CLUSTERCOUNT-1; i++){
      printf("id = %d\n", clusters[i]->id);
      printf("content = %s\n", clusters[i]->content);
    }*/
    
    printf("id = %d\n", freeCluster->id);
    printf("content = %s\n", freeCluster->content);

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

static void cl_destroy(void *a) {
    /*
      Close filesystem 
      ...
    */
    printf("Filesystem has been destroyed!\n");
}

static int cl_getattr(const char *path, struct stat * stbuf) {
  printf("----------------------\n");
    printf("getattr: %s\n", path);
    int res = 0; 
    memset(stbuf, 0, sizeof(struct stat)); 

    NodeType* node = seekConcreteFile(path);
    printf("!create: %s\n", path);

    if (!node->isEmpty && node->isDir) { 
      stbuf->st_mode = S_IFDIR | 0755; 
      stbuf->st_nlink = 2; 
    } else if (node->isEmpty == 0) { 
      stbuf->st_mode = S_IFREG | 0444;
      stbuf->st_nlink = 1; 
      int len = 0;
      LinkCl tmpcluster = node->firstCl;
      while(tmpcluster != 0){
        len+=strlen(tmpcluster->content);
        tmpcluster = tmpcluster->nextCluster;
      }
      stbuf->st_size = len;//strlen(cluster->content);
    } else {
      printf("6666666\n");
      res = -ENOENT;
    } 
    return res; 

    //memset(stbuf, 0, sizeof (struct stat));
    //stbuf->st_mode = /*mode*/
    //stbuf->st_nlink = /*count of links*/
    //stbuf->st_size = /*file size*/

}

static int cl_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info * fi) {
   printf("----------------------\n");
    printf("cl_readdir%s\n",path);
  (void) offset; 
  (void) fi;
  //if (strcmp(path, "/") != 0) 
  //  return -ENOENT; 
  //filler(buf, ".", NULL, 0); 
  //filler(buf, "..", NULL, 0); 

  NodeType* node = seekConcreteFile(path);

  LinkCl tmpcluster = node->firstCl;

  char mybuf[100];
  strcpy(mybuf,tmpcluster->content);
  tmpcluster = tmpcluster->nextCluster;
  while(tmpcluster != 0){
    printf("%s\n", tmpcluster->content);
    strcat(mybuf, tmpcluster->content);
    printf("%s\n", mybuf);
    tmpcluster = tmpcluster->nextCluster;
   } 
   char sep[10]=" ";

   char *istr;
   char *tmp;
   istr = strtok (mybuf,sep);
   int flag = 1;
   printf("Files\n");
   printf("cl_readdir -->%s\n", mybuf);
   while (istr != NULL)
   {
      tmp=istr;
      printf("%s\n", tmp);
      if(flag > 0)
        filler(buf, tmp , NULL, 0);
      flag =-flag;
      istr = strtok (NULL,sep);
   }
  
  return 0;
}

static int cl_open(const char *path, struct fuse_file_info * fi) {
  printf("----------------------\n");
    printf("open: %s\n", path);
    /*if (strcmp(path, files[file_ind].path) != 0) 
      return -ENOENT;
      if ((fi->flags & 3) != O_RDONLY)
        return -EACCES; */
      //return 0; 
    //fileInfo->fileInode = // set inode;
    //fileInfo->fullfileName = // specify file name;
    //fileInfo->isLocked = false; // set is file locked
    
    printf("open: Opened successfully\n");

    return 0;
}

/*static int cl_release(const char *path, struct fuse_file_info * fi) {
    printf("release: %s\n", path);
    
      Unlock file
      ...
    
    return 1;
}
*/
static int cl_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info * fi) {
printf("----------------------\n");
    printf("cl_read: %s\n", path);
    /*
      byte *fileContent = readFile(fileInode);
      memcpy(buf, fileContent, size);
    */  
    printf("cl_read: %s\n", path); 
    size_t len; (void) fi; 
    NodeType* mFile = seekConcreteFile(path);
    //size = 0;
    if(mFile->isEmpty == 1)//(strcmp(path, mFile->path) != 0) 
      return -ENOENT; 
 
    LinkCl tmpcluster = mFile->firstCl;

    *buf = '\0';
    while(tmpcluster != 0){
      printf("%s\n", tmpcluster->content);
      strcat(buf, tmpcluster->content);
      printf("%s\n", buf);
      tmpcluster = tmpcluster->nextCluster;
      //len = strlen(tmpcluster->content);
      //size += len;
    }
    strcat(buf, "\0");

    return size; 
}

static int cl_truncate(const char *path, off_t size) {
  printf("----------------------\n");

  printf("truncate: Truncated successfully\n");
  return 0;
}

/*static int cl_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    printf("----------------------\n");
    printf("cl_create: %s\n", path);
  return 0;
}*/

static int cl_mknod(const char* path, mode_t mode, dev_t dev)
{
  printf("----------------------\n");
    //char* rootPath = getRootName(path);
    //NodeType *dirNode = seekConcreteFile(rootPath);

    printf("createNode: %s\n", path);
    char* rootPath = getRootName(path);
    NodeType *dirNode = seekConcreteFile(rootPath);
    int i = 0;
    for(i =0; i<100; i++){
      if(files[i]->isEmpty == 1)
        break;
    }

    files[i]->isEmpty = 0;
    files[i]->firstCl = freeCluster;
    freeCluster = freeCluster->nextCluster;
    files[i]->firstCl->nextCluster = 0;
    char tmpName[30];
    char str[10];
    sprintf(str, "%d", i);
    strncpy(tmpName," ", sizeof(tmpName));
    strcat(tmpName,getName(path));
    strcat(tmpName," ");
    strcat(tmpName,str);
    printf("%s\n", tmpName);
    writeToClusters(tmpName,dirNode->firstCl);
    return 0;
}

static int cl_mkdir(const char* path, mode_t mode)
{   
  printf("----------------------\n");
  printf("createDir: %s\n", path);
    char* rootPath = getRootName(path);
    NodeType *dirNode = seekConcreteFile(rootPath);

    int i = 0;
    for(i =0; i<100; i++){
      if(files[i]->isEmpty == 1)
        break;
    }
    files[i]->firstCl = freeCluster;
    freeCluster = freeCluster->nextCluster;
    files[i]->firstCl->nextCluster = 0;
    files[i]->isEmpty = 0;
    files[i]->isDir = 1;
    char tmpName[30];
    char str[10];
    sprintf(str, "%d", i);
    strncpy(tmpName," ", sizeof(tmpName));
    strcat(tmpName,getName(path));
    strcat(tmpName," ");
    strcat(tmpName,str);
    printf("%s\n", tmpName);
    writeToClusters(tmpName,dirNode->firstCl);
    return 0;       
}


static int cl_write(const char *path, const char *content, size_t size, off_t offset, struct fuse_file_info *fi) {
    printf("write: %s\n", path);
    printf("%s\n", content);
    NodeType* node = seekConcreteFile(path);
    writeToClusters(content,node->firstCl);
    return 0; // Num of bytes written
}
static int cl_unlink(const char *path){
  printf("----------------------\n");
  printf("unLink: %s\n", path);
  //удаление файла
  NodeType *node = seekConcreteFile(path);
  node->isEmpty = 1;
  LinkCl tmpcluster = node->firstCl;

  while(tmpcluster->nextCluster != 0){
    tmpcluster = tmpcluster->nextCluster;
  }
  tmpcluster->nextCluster = freeCluster;
  freeCluster = node->firstCl;

  char *name = getName(path);
  char *tmpName = (char * )malloc(sizeof(char)*strlen(name));
  strcpy(tmpName,name);

  //удаление из папки
  char* rootPath = getRootName(path);
  NodeType *dirNode = seekConcreteFile(rootPath);

  tmpcluster = dirNode->firstCl;
  int len = 0;
  while(tmpcluster->nextCluster != 0){
    tmpcluster->isFull = 0;
    tmpcluster = tmpcluster->nextCluster;
    len++;
  }

  char *dirData = (char * )malloc(sizeof(char)*len*CLUSTERSIZE);
  char *newDirData = (char * )malloc(sizeof(char)*(len*CLUSTERSIZE - strlen(tmpName)));
  tmpcluster = dirNode->firstCl;
  strcpy(dirData,tmpcluster->content);
  tmpcluster = tmpcluster->nextCluster;
  while(tmpcluster->nextCluster != 0){
    strcat(dirData, tmpcluster->content);
    tmpcluster = tmpcluster->nextCluster;
  } 
  char sep[10]=" ";
  char *istr;
  char *tmp;
  istr = strtok (dirData,sep);
  int flag = 1; int del = 0;
  while (istr != NULL)
  {
    tmp=istr;
    printf("%s\n", tmp);
    //if(flag > 0)
    //  filler(buf, tmp , NULL, 0);

    if(!del && strcmp(tmpName,tmp) != 0){
      strcat(newDirData," ");
      strcat(newDirData,tmp);
    }else{
      if(del)
        del = 0;
      else
        del = 1;
    }
    flag =-flag;
    istr = strtok (NULL,sep);
  }
  printf("NNEW%s\n", newDirData);
  dirNode->firstCl->content[0] = '\0';
  //tmpcluster->nextCluster = freeCluster;
  //freeCluster = dirNode->firstCl->nextCluster;
  if(newDirData != 0)
    writeToClusters(newDirData,dirNode->firstCl);
  return 0;
}

static int cl_rename(const char* old, const char* new){
  printf("rename \n");
  return 0;
}

NodeType* seekFile(NodeType *node,char* name){

  //if(strcmp(path, "/") == 0)
  //  return files[0];
  printf("seekFile %s\n", name);

  LinkCl tmpcluster = node->firstCl;
  char mybuf[100];
  strcpy(mybuf,tmpcluster->content);
  tmpcluster = tmpcluster->nextCluster;
  int listOfInod[10];
  char listOfNames[10][30];
  while(tmpcluster != 0){
    strcat(mybuf, tmpcluster->content);
    tmpcluster = tmpcluster->nextCluster;
   } 
   char sep[10]=" ";

   char *istr;
   char *tmp;
   istr = strtok (mybuf,sep);
   int flag = 1;
   int index = -1;
   while (istr != NULL)
   {
      tmp=istr;
      if(flag>0){
        strcpy(listOfNames[++index],tmp);
      }
      else
        listOfInod[index] = atoi(tmp);
      flag =-flag;
      istr = strtok (NULL,sep);
   }
   
   for(int i = 0; i<index+1; i++)
      if(strcmp(name, listOfNames[i]) == 0){
        printf("seekFile--->Нашел %s\n", listOfNames[i]);
        return files[listOfInod[i]];
    }
   return files[99];
}

NodeType* seekConcreteFile(const char* path){
  printf("----------------------\n");
  if(strcmp(path, "/") == 0)
    return files[0];

  /*char* name = getName(path);
  char* name1 = (char*)malloc(sizeof(char)*strlen(name));
  strcpy(name1,name);
  printf("--seekConcreteFile %s\n", name1);
  NodeType* node = seekFile(0,name1);
  */

  char listOfFiles[10][30];
  int count = -1;
  
  char str[100];
  char sep [10]="/";
  strcpy(str,path);
  //strcat(str,"/");
  char *istr;
  char *tmp;
  istr = strtok (str,sep);
  printf("--seekConcreteFile %s\n", str);
  while (istr != NULL)
  {
    tmp=istr;
    strcpy(listOfFiles[++count],tmp);
    printf("--istr %s\n", tmp);
    istr = strtok (NULL,sep);
  }

  NodeType* node = files[0];
  NodeType* tmpNode = seekFile(node,listOfFiles[0]);
  node = tmpNode;
  
  for(int i = 1; i<=count; i++){
    //printf("id = %d\n",node->firstCl->id);
    printf("-- %s\n", listOfFiles[i]);
    tmpNode = seekFile(node,listOfFiles[i]);
    node = tmpNode;
  }
  

  return node;

}

void createFreeClusters(){
  printf("----------------------\n");
  freeCluster = (LinkCl)malloc(sizeof(ClusterType));
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

  //strncpy(freeCluster->content, "str\0", sizeof(freeCluster->content));
}

void writeToClusters(const char * content, LinkCl cluster){
  int len = strlen(content);
  printf("%d\n", len);
  int j = 0;
  LinkCl tmpCluster = cluster;
  while(tmpCluster->isFull == 1){
    printf("Пропустил\n");
    tmpCluster = tmpCluster->nextCluster;
  }
  while(tmpCluster->content[j] != '\0'){
    printf("%d\n", j++);
    //j++;
  }

  printf("%s\n", content);
  int i, iCon = -1;
  LinkCl tmpClusterWrite = tmpCluster;
  while(iCon<len){
    for(i = j; i<CLUSTERSIZE; i++){
      if(iCon < len)
        tmpClusterWrite->content[i] = content[++iCon];
    }

    if(iCon < len){
      tmpClusterWrite->isFull = 1;
      if(tmpClusterWrite->nextCluster == 0)
        tmpClusterWrite->nextCluster = freeCluster;

      tmpClusterWrite = tmpCluster->nextCluster;
    }
    j = 0;
  }
  if(tmpClusterWrite->nextCluster !=0){
    freeCluster  = tmpClusterWrite->nextCluster;
    tmpClusterWrite->nextCluster = 0;
  }
  tmpClusterWrite->content[i] = '\0';

}

char* getName(const char* path){
  char str[100];
  char sep [10]="/";

  strcpy(str,path);
  char *istr;
  char *tmp;
  istr = strtok (str,sep);
  while (istr != NULL)
  {
    tmp=istr;
    istr = strtok (NULL,sep);
  }
  return tmp;
}

char* getRootName(const char* path){
  printf("getRootName %s\n", path);
  char *tmpPath = (char *)malloc(sizeof(char)*strlen(path));
  strcpy(tmpPath,path);
  int i = 0;
  for(i = strlen(tmpPath); i>=0; i--){
    if(tmpPath[i] == '/')
      break;
    //else
      //printf("%s\n",tmpPath);
      //printf("%d\n", strlen(tmpPath));
      //printf("%c\n", tmpPath[i]);
  }
  printf("\n");
  tmpPath[i+1] = '\0';
  printf("getRootName %s\n", tmpPath);
  return tmpPath;
}