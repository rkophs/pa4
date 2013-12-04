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

struct EngineArgs {
    int sockL;
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
            printf("Name already exists in server's directory...Closing...\n");
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

    int buffSize = 12000;
    char buff[buffSize];
    bzero(buff, buffSize);

    //Listen on server:
    int len;
    while (1) {
        bzero(buff, sizeof (buff));
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

void *engineListener(void *args) {
    struct EngineArgs *ea = (struct EngineArgs*) args;
    int sockL = ea->sockL;

    //Empty socket struct for incoming clients;
    struct sockaddr_in cli_addr;
    int cli_len = sizeof (cli_addr);
    bzero(&cli_addr, cli_len);

    //Wait for new connections and tend to them:
    while (1) {
        int newSock;
        bzero(&cli_addr, sizeof (cli_addr));
        if ((newSock = accept(sockL, (struct sockaddr*) &cli_addr, &cli_len)) < 0) {
            printf("Error accepting incoming connection\n");
            return;
        }

        //Now get the incoming GET request from connection
        int buffSize = 1024;
        char buff[buffSize];
        bzero(buff, buffSize);
        int len;
        if ((len = recvDecrypt(newSock, buff, sizeof (buff), 0)) > 0) {
            char *get = strtok(buff, " ");
            if (!strncmp(get, "GET", 3)) {
                char *fileName = strtok(NULL, " ");
                printf(" **Request for file %s\n", fileName);

                FILE *pFile;
                if ((pFile = fopen(fileName, "rb")) == NULL) {
                    printf(" **Error opening file... \n");
                    close(newSock);
                    continue;
                }

                // obtain file size:
                fseek(pFile, 0, SEEK_END);
                int lSize = ftell(pFile);
                rewind(pFile);

                // allocate memory to contain the whole file:
                char *buffer;
                if ((buffer = (char*) malloc(sizeof (char)*lSize)) == NULL) {
                    printf(" **Error allocating memory.\n");
                    fclose(pFile);
                    close(newSock);
                    return;
                }

                // copy the file into the buffer:
                if (fread(buffer, sizeof (char), lSize, pFile) != lSize) {
                    printf(" **Error reading file");
                    free(buffer);
                    fclose(pFile);
                    continue;
                }

                printf("Sending file of size: %i\n", lSize);
                sendEncrypt(newSock, buffer, lSize, 0);

                // terminate
                close(newSock);
                fclose(pFile);
                free(buffer);
            }
        }
        close(newSock);
    }

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

    pthread_t tid;
    struct EngineArgs ea;
    ea.sockL = e->sockL;
    if (pthread_create(&tid, NULL, &engineListener, &ea) != 0) {
        printf("Error spawning new engine thread\n");
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
    pthread_t tid2;
    if (pthread_create(&tid2, NULL, &engineThread, e) != 0) {
        printf("Error spawning new engine sync thread\n");
        return -1;
    }

    return 1;
}

void engineSendCmd(int sockfd, char *buff, int buffSize) {
    sendEncrypt(sockfd, buff, buffSize, 0);
}

int engineGet(char *cmd) {
    if (strncmp(cmd, "get", 3)) {
        return -1;
    }
    strtok(cmd, " ");
    char *file;
    file = strtok(NULL, " ");
    if (file == NULL) {
        printf("  ->Usage: get <filename> <ip> <port>\n");
        return -1;
    }
    char *ip;
    ip = strtok(NULL, " ");
    if (ip == NULL) {
        printf("  ->Usage: get <filename> <ip> <port>\n");
        return -1;
    }
    char *p;
    p = strtok(NULL, " ");
    if (p == NULL) {
        printf("  ->Usage: get <filename> <ip> <port>\n");
        return -1;
    }
    int port = atoi(p);

    FILE *fp;
    if ((fp = fopen(file, "rb")) != NULL) {
        printf(" ->This file already exists in your directory, select another one.\n");
        fclose(fp);
        return -1;
    }
    printf("Contacting %s:%i for file...\n", ip, port);

    int sock;
    if ((sock = bindConnector(port, ip)) < 0) {
        printf("Error contacting server...Please try again later.\n");
        return -1;
    }

    char sendCmd[128];
    bzero(sendCmd, sizeof (bzero));
    strcpy(sendCmd, "GET ");
    strcat(sendCmd, file);
    sendEncrypt(sock, sendCmd, strlen(sendCmd), 0);

    //Set up timeout:
    struct timeval tv;
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *) &tv, sizeof (struct timeval));

    int len;
    int buffSize = 1024;
    char buff[buffSize];
    int status = -1;
    do {
        bzero(buff, sizeof (buff));
        if ((len = recvDecrypt(sock, &buff, buffSize, 0)) > 0) {
            if (status == -1) { //Init
                if ((fp = fopen(file, "wb")) == NULL) {
                    printf(" **Error creating new file.\n");
                    break;
                }
            }
            status = 1;
            int res;
            if ((res = fwrite(buff, sizeof (char), len, fp)) != len) {
                printf(" **Error writing to file: %i != %i\n", res, len);
                break;
            }
        }
    } while (len > 0);

    if (status == 1) {
        fclose(fp);
    }
    return status;
}

int engineRun(struct Engine *e) {

    pthread_mutex_lock(&engineLock);
    int sockServer = e->sockServer;
    pthread_mutex_unlock(&engineLock);

    printf("Client is set up, please enter a command:\n");
    while (1) {
        int buffSize = 128;
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
        } else if (!strncmp(buff, "get", 3)) {
            if (engineGet(buff) > 0) {
                printf("  ->File download complete... sending updated file listing to server...\n\n");
                pthread_mutex_lock(&engineLock);
                if (engineSendFileListing(e->sockServer, e->addr, strlen(e->addr), e->args->srcPort) < 0) {
                    printf("  ->Error sending file listing to server...Closing...\n");
                    return -1;
                }
                pthread_mutex_unlock(&engineLock);
            } else {
                printf("  ->Error retrieving file... possibly because file doesn't exist on peer.\n\n");
            }
        } else {
            printf(" ->Error: Command not found.\n\n", buff);
        }
    }

}
