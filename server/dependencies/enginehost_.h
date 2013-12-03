#include "../../shared/network_.h"
#include <pthread.h>

#define HOSTMAX 100

struct Host {
    pthread_t tid;
    char *name;
    int sockfd;
    //int fileCount;
    //struct file *files;
    char *files;
};

struct Engine {
    struct Args *args;
    struct Host socks[HOSTMAX];
    int threadCount;
    int change;
};

struct EngineArgs {
    int newSock;
    struct Engine *e;
    char *addr;
};

int nameExists(struct Engine *e, char *name, int size) {
    int i;
    for (i = 0; i < 100; i++) {
        if (e->socks[i].name != NULL) {
            if (strncmp(e->socks[i].name, name, size) == 0) {
                return 1;
            }
        }
    }
    return 0;
}

void emptyHost(struct Engine *e, int it) {
    if (e == NULL || it < 0 || it >= HOSTMAX) {
        return;
    }
    if (e->socks[it].name != NULL) {
        free(e->socks[it].name);
    }

    //emptyHostFiles(&(e->socks[it]));
    if(e->socks[it].files != NULL){
        free(e->socks[it].files);
        e->socks[it].files = NULL;
    }
    e->socks[it].name = NULL;
    e->socks[it].sockfd = 0;
    e->socks[it].tid = -1;
    //e->socks[it].fileCount = 0;
}

int initHost(struct Engine *e, int it, char * name, int nameSize, int sockfd, pthread_t tid) {
    if (e == NULL || it < 0 || it >= HOSTMAX) {
        return -1;
    }
    e->socks[it].name = NULL;
    e->socks[it].tid = -1;
    e->socks[it].sockfd = 0;
    e->socks[it].files = NULL;

    int add = 0;
    if (nameSize > 0) {
        if ((e->socks[it].name = (char *) malloc(nameSize + 1)) == NULL) {
            return -1;
        }
        bzero(e->socks[it].name, sizeof (e->socks[it].name));
        strncpy(e->socks[it].name, name, nameSize);
        add++;
        e->change = 1;
    }

    if (sockfd > 0) {
        e->socks[it].sockfd = sockfd;
        add++;
    }

    if (tid > 0) {
        e->socks[it].tid = tid;
        add++;
    }

    if (add) {
        e->threadCount++;
        return 1;
    }

    return -1;
}

int addSock(struct Engine *e, char *name, int size, int sockfd, pthread_t tid) {
    if (nameExists(e, name, size)) {
        return -1; //Name already exists
    }
    int i;
    for (i = 0; i < HOSTMAX; i++) {
        if (e->socks[i].name == NULL && e->socks[i].sockfd == -1
                && e->socks[i].tid == 0) {
            initHost(e, i, name, size, sockfd, tid);
            return 1;
        }
    }
    return -2; //Filled array
}

int addThread(struct Engine *e, pthread_t tid) {
    int i;
    for (i = 0; i < HOSTMAX; i++) {
        if (e->socks[i].name == NULL && e->socks[i].sockfd == -1
                && e->socks[i].tid == 0) {
            initHost(e, i, NULL, 0, -1, tid);
            return 1;
        }
    }
    return -2; //Filled array
}

int delSock(struct Engine *e, int sockfd) {
    if (e == NULL) {
        return -1;
    }
    if (e->socks == NULL) {
        return -1;
    }
    int i;
    for (i = 0; i < HOSTMAX; i++) {
        if (e->socks[i].sockfd == sockfd) {
            e->change = 1;
            emptyHost(e, i);
            close(sockfd);
            return 1;
        }
    }
    return -1;
}

int delThread(struct Engine *e, pthread_t tid) {
    int i;
    for (i = 0; i < HOSTMAX; i++) {
        if (e->socks[i].tid == tid) {
            emptyHost(e, i);
            return 1;
        }
    }
    return -1;
}

int overwriteHostFiles(struct Host *h, char *buff, int size){
    if(h == NULL){
        return -1;
    }
    
    if(h->files != NULL){
        free(h->files);
        h->files = NULL;
    }
    
    if((h->files = (char *) malloc (size + 1)) == NULL){
        return -1;
    }
    bzero(h->files, sizeof(h->files));
    
    strncpy(h->files, buff, size);
    return 1;
}

int overwriteHostFilesBySockFD(struct Engine *e, int sockfd, char *buff, int size) {
    if (e == NULL) {
        return -1;
    }
    int i;
    for (i = 0; i < HOSTMAX; i++) {
        if (e->socks[i].sockfd == sockfd) {
            return overwriteHostFiles(&(e->socks[i]), buff, size);
        }
    }
    return -1;
}

void printFiles(struct Engine *e){
    if(e == NULL){
        return;
    }
    int i;
    printf("----------------------File Ledger-------------------------------------\n");
    printf("File name || File size KB || File owner || Owner IP || Owner Port\n");
    for(i = 0; i < HOSTMAX; i++){
        if(e->socks[i].files != NULL){
            printf("%s", e->socks[i].files);
        }
    }
    printf("----------------------End Of File Ledger------------------------------\n");
}

