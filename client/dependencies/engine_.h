#include "../../shared/network_.h"

struct Engine {
    struct Args *args;
    int sockL;
    int sockServer;
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
    if((len = recv(e->sockServer, buff, buffSize, 0)) > 0){
        if(!strncmp(buff, "ERROR   ", 8)){
            printf("Name already exists in server's director...Closing...\n");
            return 0;
        }
        return 1;
    }
    return -1;
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

int engineRun(struct Engine *e) {


}
