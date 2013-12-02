#include "../../shared/network_.h"
#include <pthread.h>

#define HOSTMAX 100

pthread_mutex_t engineLock;

struct Engine {
    struct Args *args;
    int sockL;
    int sockServer;
    pthread_t tid[HOSTMAX];
    int tidCount;
    int quit;
};

void releaseEngine(struct Engine *e) {
    if (e == NULL) {
        return;
    }
    if (e->args != NULL) {
        releaseArgs(e->args);
    }
    e->sockL = -1;
    e->sockServer = -1;
    e->tidCount = 0;
    e->quit = 0;

    free(e);
    e = NULL;
}

struct Engine *initEngine(int argc, char **argv) {
    struct Engine *e;
    if ((e = (struct Engine *) malloc(sizeof (struct Engine))) == NULL) {
        releaseEngine(e);
        return NULL;
    }

    if ((e->args = initClientArgs(argc, argv)) == NULL) {
        releaseEngine(e);
        return NULL;
    }

    e->sockL = -1;
    e->sockServer = -1;
    e->tidCount = 0;
    e->quit = 0;

    return e;
};

int engineSendName(struct Engine *e) {
    int buffSize = 1024;
    char buff[buffSize];
    bzero(buff, buffSize);
    strncpy(buff, e->args->clientName, buffSize - 1);
    return send(e->sockServer, &buff, strlen(buff), 0);
}

int engineRecvOK(struct Engine *e) {

    int buffSize = 32;
    char buff[buffSize];
    bzero(buff, buffSize);

    int len;
    if ((len = recv(e->sockServer, buff, buffSize, 0)) > 0) {
        printf("Server responded: %s\n", buff);
        if (!strncmp(buff, "ERROR   ", 8)) {
            printf("Name already exists in server's director...Closing...\n");
            return 0;
        }
        return 1;
    }
    return -1;
}

void *engineListenThread(void *arg) {
    struct Engine *e = arg;
}

int engineStart(struct Engine *e) {
    if (e == NULL) {
        return -1;
    }

    //Set up listener for other peers:
    if ((e->sockL = bindListener(e->args->srcPort)) < 0) {
        printf("Error listening for peers...Closing...\n");
        return -1;
    }

    //Connect to server:
    if ((e->sockServer = bindConnector(e->args->dstPort, e->args->dstIP)) < 0) {
        printf("Error connecting to server...Closing...\n");
        return -1;
    }

    //Send name to server:
    if (engineSendName(e) < 0) {
        printf("Error sending client name %s to server...Closing...\n", e->args->clientName);
        return -1;
    }

    //Receive response to server:
    int status = engineRecvOK(e);
    if (status < 0) {
        printf("Error getting response from server...Closing...\n");
        return -1;
    } else if (status == 0) {
        return -1;
    }

    return 1;
}

void engineSendCmd(int sockfd, char *buff, int buffSize) {
    send(sockfd, buff, buffSize, 0);
}


int engineRun(struct Engine *e) {
    
//    //Spawn listener thread:
//    pthread_mutex_lock(&engineLock);
//    if (pthread_create(&(e->tid[e->tidCount]), NULL, &engineListenThread, e) != 0) {
//        pthread_mutex_unlock(&engineLock);
//        printf("Error spawning new listener thread\n");
//        return -1;
//    }
//    e->tidCount++;
//    pthread_mutex_unlock(&engineLock);
    
    pthread_mutex_lock(&engineLock);
    int sockServer = e->sockServer;
    pthread_mutex_unlock(&engineLock);
    
    printf("Client is set up, please enter a command:\n");
    while(1){
        int buffSize = 100;
        char buff[buffSize];
        bzero(buff, buffSize);
        printf(">");
        fgets(buff, buffSize, stdin);
        if(!strncmp(buff, "ls", 2)){
            printf("LS being sent\n");
        } else if (!strncmp(buff, "kill p2p", 8)) {
            printf("Sending command to kill server\n");
            send(sockServer, "QUIT    ", 8, 0);
            close(sockServer);
            pthread_mutex_lock(&engineLock);
            e->quit = 1;
            pthread_mutex_unlock(&engineLock);
            return 1;
        } else {
            printf(" Error: Command not found.\n", buff);
        }
    }
    
}
