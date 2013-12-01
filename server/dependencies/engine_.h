#include "../../shared/network_.h"
#include <pthread.h>

#define HOSTMAX 100

pthread_mutex_t engineLock;

struct Host {
    char *name;
    int sockfd;
};

struct Engine {
    struct Args *args;
    struct Host socks[HOSTMAX];
    int sockCount;
    pthread_t tid[HOSTMAX];
    int tidCount;
    int quit;
    int change;
};

struct EngineArgs {
    int newSock;
    struct Engine *e;
};

void emptyHost(struct Engine *e, int it) {
    free(e->socks[it].name);
    e->socks[it].name = NULL;
    e->socks[it].sockfd = -1;
}

int initHost(struct Engine *e, int it, char * name, int nameSize, int sockfd) {

    if ((e->socks[it].name = (char *) malloc(nameSize + 1)) == NULL) {
        return -1;
    }

    bzero(e->socks[it].name, sizeof (e->socks[it].name));
    strncpy(e->socks[it].name, name, nameSize);

    e->socks[it].sockfd = sockfd;

    return 1;
}

int addSock(struct Engine *e, char *name, int size, int sockfd) {
    int i;
    for (i = 0; i < HOSTMAX; i++) {
        if (e->socks[i].name == NULL) {
            e->change = 1;
            initHost(e, i, name, size, sockfd);
            return 1;
        }
    }
    return -1;
}

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

void releaseEngine(struct Engine *e) {
    if (e == NULL) {
        return;
    }
    if (e->args != NULL) {
        releaseArgs(e->args);
    }

    int i;
    for (i = 0; i < HOSTMAX; i++) {
        emptyHost(e, i);
    }
    e->sockCount = 0;

    for (i = 0; i < e->tidCount; i++) {
        pthread_join(e->tid[i], NULL);
    }
    pthread_mutex_destroy(&engineLock);
    e->tidCount = 0;
    e->quit = 0;
    e->change = 0;

    free(e);
    e = NULL;
}

struct Engine *initEngine(int argc, char **argv) {
    struct Engine *e;
    if ((e = (struct Engine *) malloc(sizeof (struct Engine))) == NULL) {
        releaseEngine(e);
        return NULL;
    }

    if ((e->args = initServerArgs(argc, argv)) == NULL) {
        releaseEngine(e);
        return NULL;
    }

    int i;
    for (i = 0; i < HOSTMAX; i++) {
        e->socks[i].name = NULL;
        e->socks[i].sockfd = -1;
    }
    e->sockCount = 0;
    e->tidCount = 0;
    e->quit = 0;
    e->change = 0;

    return e;
};

void engineSendError(int sockfd) {
    int buffSize = 8;
    char buff[buffSize];
    bzero(buff, buffSize);
    strcpy(buff, "ERROR   ");
    send(sockfd, &buff, strlen(buff), 0);
    close(sockfd);
}

void *engineSync(void *arg) {
    printf("Entering sync thread\n");
    struct Engine *e = arg;
    while (!e->quit) {
        sleep(1);
        if (e->change) {
            printf("Change occurred!\n");
            //Send list out here;
            pthread_mutex_lock(&engineLock);
            e->change = 0;
            pthread_mutex_unlock(&engineLock);
        }
    }
    printf("Exiting engine sync\n");
    pthread_exit((void*) 0);
}

void *engineThread(void *args) {
    printf("Entering thread\n");
    struct EngineArgs *ea = args;
    struct Engine *e = ea->e;
    int newSock = ea->newSock;

    //Now get name and add appropriately to list of names:
    int buffSize = 32;
    char buff[buffSize];
    bzero(buff, buffSize);
    int len;
    if ((len = recv(newSock, buff, sizeof (buff), 0)) > 0) {
        pthread_mutex_lock(&engineLock);
        if (nameExists(e, buff, len)) {
            pthread_mutex_unlock(&engineLock);
            engineSendError(newSock);
            return;
        } else {
            if (addSock(e, buff, len, newSock) < 0) {
                pthread_mutex_unlock(&engineLock);
                engineSendError(newSock);
                return;
            }
        }
        pthread_mutex_unlock(&engineLock);
    }

    //Now setup interaction loop:
    pthread_mutex_lock(&engineLock);
    while (!e->quit) {
        pthread_mutex_unlock(&engineLock);
        bzero(buff, buffSize);
        if ((len = recv(newSock, buff, sizeof (buff), 0)) > 0) {
            if (!strncmp(buff, "LS        ", 8)) {
                printf("LS received\n");
            } else if (!strncmp(buff, "EXIT    ", 8)) {
                printf("EXIT received\n");
            } else if (!strncmp(buff, "GET     ", 8)) {
                printf("GET received\n");
            } else if (!strncmp(buff, "QUIT    ", 8)) {
                pthread_mutex_lock(&engineLock);
                e->quit = 1;
                pthread_mutex_unlock(&engineLock);
            }
        }
        pthread_mutex_lock(&engineLock);
    }
    printf("Exiting thread\n");
    pthread_exit((void*) 0);
}

int engineRun(struct Engine *e) {
    if (e == NULL) {
        return -1;
    }

    //Set up listener for other peers:
    int sockL = -1;
    if ((sockL = bindListener(e->args->srcPort)) < 0) {
        printf("Error listening for clients...Closing...\n");
        return -1;
    }

    //Array of future threads:
    pthread_t tid[HOSTMAX];
    int tidCount = 0;

    //Empty socket struct for incoming clients;
    struct sockaddr_in cli_addr;
    int cli_len = sizeof (cli_addr);
    bzero(&cli_addr, cli_len);

    //Spawn a thread to send out updates to each client;
    if (pthread_create(&(tid[tidCount]), NULL, &engineSync, e) != 0) {
        printf("Error spawning new engine sync thread\n");
        return -1;
    }
    tidCount++;

    while (1) {
        if (tidCount >= 100) {
            sleep(1);
            continue;
        }

        //Listen for new incoming connections:
        int newSock;
        bzero(&cli_addr, sizeof (cli_addr));
        if ((newSock = accept(sockL, (struct sockaddr*)
                &cli_addr, &cli_len)) < 0) {
            printf("Error accepting incoming connection\n");
            return -1;
        }

        //Spawn off a thread to deal with new connection:
        //Argument array for each thread
        struct EngineArgs *ea;
        ea->e = e;
        ea->newSock = newSock;
        if (pthread_create(&(tid[tidCount]), NULL, &engineThread, ea) != 0) {
            printf("Error spawning new engine thread\n");
            return -1;
        }
        tidCount++;
    }

    close(sockL);
    return 1;
}
