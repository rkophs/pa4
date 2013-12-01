#include "../../shared/network_.h"

struct Args {
    char *clientName;
    char *serverIP;
    char *serverPort;
};

void releaseArgs(struct Args *a){
    if(a == NULL){
        return;
    }
    if(a->clientName != NULL){
        free(a->clientName);
        a->clientName = NULL;
    }
    if(a->serverIP != NULL){
        free(a->serverIP);
        a->serverIP = NULL;
    }
    if(a->serverPort != NULL){
        free(a->serverPort);
        a->serverPort = NULL;
    }
    free(a);
    a = NULL;
}

struct Args *initArgs(int argc, char **argv){
    if(argc < 4){
        printf("Usage: client_PFS <Client Name> <Server IP> <Server Port>\n");
        return NULL;
    }
    
    struct Args *a;
    if((a = (struct Args *) malloc(sizeof(struct Args))) == NULL){
        releaseArgs(a);
        return NULL;
    }
    
    int nLen = strlen(argv[1]);
    int IPLen = strlen(argv[2]);
    int PLen = strlen(argv[3]);
    
    if((a->clientName = (char *)malloc(nLen + 1)) == NULL){
        releaseArgs(a);
        return NULL;
    }
    if((a->serverIP = (char *)malloc(IPLen + 1)) == NULL){
        releaseArgs(a);
        return NULL;
    }
    if((a->serverPort = (char *)malloc(PLen + 1)) == NULL){
        releaseArgs(a);
        return NULL;
    }
    
    bzero(a->clientName, sizeof(a->clientName));
    bzero(a->serverIP, sizeof(a->serverIP));
    bzero(a->serverPort, sizeof(a->serverPort));
    
    strncpy(a->clientName, argv[1], nLen);
    strncpy(a->serverIP, argv[2], IPLen);
    strncpy(a->serverPort, argv[3], PLen);
    
    return a;
}