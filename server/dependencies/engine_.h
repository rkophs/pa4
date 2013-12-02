
#include "enginehost_.h"

pthread_mutex_t engineLock;

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
    e->threadCount = 0;
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
        e->socks[i].tid = 0;
    }
    e->threadCount = 0;
    e->change = 0;

    return e;
};

void engineSendStatus(int sockfd, int ok) {
    int buffSize = 8;
    char buff[buffSize];
    bzero(buff, buffSize);
    ok ? strcpy(buff, "OK      ") : strcpy(buff, "ERROR   ");
    send(sockfd, &buff, strlen(buff), 0);
}

void *engineSync(void *arg) {
    pthread_mutex_lock(&engineLock);
    struct Engine *e = arg;
    addThread(e, pthread_self());
    printf("sock count: %i\n", e->threadCount);
    pthread_mutex_unlock(&engineLock);

    while (1) {
        sleep(1);
        pthread_mutex_lock(&engineLock);
        int change = e->change;
        pthread_mutex_unlock(&engineLock);
        if (change) {
            printf("Sending updated file ledger to clients\n");
            //Send list out here;
            pthread_mutex_lock(&engineLock);
            e->change = 0;
            pthread_mutex_unlock(&engineLock);
        }
    }
    
    pthread_mutex_lock(&engineLock);
    delThread(e, pthread_self());
    pthread_mutex_unlock(&engineLock);
    
    pthread_exit((void*) 0);
}

void *engineThread(void *args) {
    pthread_mutex_lock(&engineLock);
    struct EngineArgs *ea = args;
    struct Engine *e = ea->e;
    int newSock = ea->newSock;
    pthread_mutex_unlock(&engineLock);

    //Set up timeout:
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(newSock, SOL_SOCKET, SO_RCVTIMEO, (char *) &tv, sizeof (struct timeval));

    //Now get name and add appropriately to list of names:
    int buffSize = 32;
    char buff[buffSize];
    bzero(buff, buffSize);
    int len;
    if ((len = recv(newSock, buff, sizeof (buff), 0)) > 0) {
        pthread_mutex_lock(&engineLock);
        int status = addSock(e, buff, len, newSock, pthread_self());
        pthread_mutex_unlock(&engineLock);

        if (status > 0) { //Successful addition
            engineSendStatus(newSock, 1);
        } else {
            engineSendStatus(newSock, 0);
            pthread_exit((void*) 0);
            close(newSock);
            return;
        }
    }

    //Now setup interaction loop:
    printf("Client with socket ID %i has entered\n", newSock);
    while (1) {
        bzero(buff, buffSize);
        if ((len = recv(newSock, buff, buffSize, 0)) > 0) {
            if (!strncmp(buff, "LS        ", 8)) {
                printf("LS received\n");
            } else if (!strncmp(buff, "GET     ", 8)) {
                printf("GET received\n");
            } else if (!strncmp(buff, "QUIT    ", 8)) {
                break;
            }
        }
    }

    pthread_mutex_lock(&engineLock);
    delSock(e, newSock);
    pthread_mutex_unlock(&engineLock);
    printf("Client with socket ID %i has exited\n", newSock);
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

    //Empty socket struct for incoming clients;
    struct sockaddr_in cli_addr;
    int cli_len = sizeof (cli_addr);
    bzero(&cli_addr, cli_len);

    //Spawn a thread to send out updates to each client;
    pthread_t tid;
    if (pthread_create(&tid, NULL, &engineSync, e) != 0) {
        printf("Error spawning new engine sync thread\n");
        return -1;
    }

    while (1) {

        fd_set rfds;
        struct timeval tv;
        FD_ZERO(&rfds);
        FD_SET(sockL, &rfds);
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        //Wait for incoming connection
        if (select(32, &rfds, NULL, NULL, &tv) > 0) {
            int newSock;
            bzero(&cli_addr, sizeof (cli_addr));
            if ((newSock = accept(sockL, (struct sockaddr*) &cli_addr, &cli_len)) < 0) {
                printf("Error accepting incoming connection\n");
                return -1;
            }

            //Spawn off a thread to deal with new connection:
            pthread_mutex_lock(&engineLock);
            struct EngineArgs *ea;
            ea->e = e;
            ea->newSock = newSock;
            pthread_mutex_unlock(&engineLock);
            if (pthread_create(&tid, NULL, &engineThread, ea) != 0) {
                printf("Error spawning new engine thread\n");
                return -1;
            }
        }
    }

    close(sockL);
    return 1;
}
