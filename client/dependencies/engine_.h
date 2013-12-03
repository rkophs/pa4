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
    char *addr;
};

void releaseEngine(struct Engine *e) {
    if (e == NULL) {
        return;
    }
    if (e->args != NULL) {
        releaseArgs(e->args);
    }
    if (e->addr != NULL) {
        free(e->addr);
        e->addr = NULL;
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
    e->addr = NULL;

    return e;
};

int engineSendName(struct Engine *e) {
    int buffSize = 1024;
    char buff[buffSize];
    bzero(buff, buffSize);
    strncpy(buff, e->args->clientName, buffSize - 1);
    return sendEncrypt(e->sockServer, &buff, strlen(buff), 0);
}

int engineRecvOK(struct Engine *e, char *ip, int *ipLen) {

    int buffSize = 1024;
    char buff[buffSize];
    bzero(buff, buffSize);

    int len;
    if ((len = recvDecrypt(e->sockServer, buff, buffSize, 0)) > 0) {
        if (!strncmp(buff, "ERROR   ", 8)) {
            printf("Name already exists in server's director...Closing...\n");
            return 0;
        }
        if (*ipLen < INET_ADDRSTRLEN) {
            return -1;
        }
        *ipLen = len;
        strncpy(ip, buff, INET_ADDRSTRLEN);
        return len;
    }
    return -1;
}

void *engineThread(void *arg) {
    pthread_mutex_lock(&engineLock);
    struct Engine *e = arg;
    int sockfd = e->sockServer;
    pthread_mutex_unlock(&engineLock);
    
    int buffSize = 1024;
    char buff[buffSize];
    bzero(buff, buffSize);
    
    //Listen on server:
    int len;
    while (1) {
        bzero(buff, sizeof(buff));
        if ((len = recvDecrypt(sockfd, buff, sizeof (buff), 0)) > 0) {
            printf("\nNewly received file ledger: \n%s\n\n", buff);
        }
    }
}

int engineSendFileListing(int sockfd, char *ip, int len, int port) {
    int buffSize = 1024;
    char buff[buffSize];
    bzero(buff, buffSize);
    char p[8];
    bzero(p, 8);
    sprintf(p, "%i", port);

    FILE *fp;
    char cmd[1024];
    bzero(cmd, sizeof (cmd));
    strcpy(cmd, "/bin/ls -kl | /usr/bin/awk '{if(NR>1)print $9 \" || \" $5 \" KB || ");
    strncat(cmd, ip, len);
    strcat(cmd, " || ");
    strcat(cmd, p);
    strcat(cmd, " \" }'");
    if ((fp = popen(cmd, "r")) == NULL) {
        printf("Failed to parse file listing.\n");
        return -1;
    }
    if (fread(buff, sizeof (char), buffSize, fp) < 0) {
        printf("Failed to parse file listing.\n");
        pclose(fp);
        return -1;
    }
    pclose(fp);

    return sendEncrypt(sockfd, buff, strlen(buff), 0);
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
    char addr[INET_ADDRSTRLEN];
    int len = INET_ADDRSTRLEN;
    int status = engineRecvOK(e, addr, &len);
    if ((e->addr = (char*) malloc(INET_ADDRSTRLEN)) == NULL) {
        return -1;
    }
    strncpy(e->addr, addr, INET_ADDRSTRLEN);
    if (status < 0) {
        printf("Error getting response from server...Closing...\n");
        return -1;
    } else if (status == 0) {
        return -1;
    }

    if (engineSendFileListing(e->sockServer, e->addr, strlen(e->addr), e->args->srcPort) < 0) {
        printf("Error sending file listing to server...Closing...\n");
        return -1;
    }

    //Spawn a thread to listen on server;
    pthread_t tid;
    if (pthread_create(&tid, NULL, &engineThread, e) != 0) {
        printf("Error spawning new engine sync thread\n");
        return -1;
    }

    return 1;
}

void engineSendCmd(int sockfd, char *buff, int buffSize) {
    send(sockfd, buff, buffSize, 0);
}

int engineRun(struct Engine *e) {

    pthread_mutex_lock(&engineLock);
    int sockServer = e->sockServer;
    pthread_mutex_unlock(&engineLock);

    printf("Client is set up, please enter a command:\n");
    while (1) {
        int buffSize = 100;
        char buff[buffSize];
        bzero(buff, buffSize);
        fgets(buff, buffSize, stdin);
        if (!strncmp(buff, "ls", 2)) {
            printf(" ->LS being sent\n\n");
            sendEncrypt(sockServer, "LS      ", 8, 0);
        } else if (!strncmp(buff, "exit", 4)) {
            printf(" ->Sending command to kill server\n\n");
            sendEncrypt(sockServer, "QUIT    ", 8, 0);
            close(sockServer);
            pthread_mutex_lock(&engineLock);
            e->quit = 1;
            pthread_mutex_unlock(&engineLock);
            return 1;
        } else {
            printf(" ->Error: Command not found.\n\n", buff);
        }
    }

}
