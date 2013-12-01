#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct Args {
    char *clientName;   //Set to NULL and never used on Server implementation
    char *dstIP;        //Set to NULL and never used on Server implementation
    int srcPort;
    int dstPort;        //Set to 0 and never used on Server implementation
};

void releaseArgs(struct Args *a){
    if(a == NULL){
        return;
    }
    if(a->clientName != NULL){
        free(a->clientName);
        a->clientName = NULL;
    }
    if(a->dstIP != NULL){
        free(a->dstIP);
        a->dstIP = NULL;
    }
    a->srcPort = 0;
    a->dstPort = 0;
    
    free(a);
    a = NULL;
}

struct Args *initClientArgs(int argc, char **argv){
    if(argc < 5){
        printf("Usage: client_PFS <Client Name> <Client Port> <Server IP> <Server Port>\n");
        return NULL;
    }
    
    struct Args *a;
    if((a = (struct Args *) malloc(sizeof(struct Args))) == NULL){
        releaseArgs(a);
        return NULL;
    }
    
    int nLen = strlen(argv[1]);
    int IPLen = strlen(argv[3]);
    
    if((a->clientName = (char *)malloc(nLen + 1)) == NULL){
        releaseArgs(a);
        return NULL;
    }
    if((a->dstIP = (char *)malloc(IPLen + 1)) == NULL){
        releaseArgs(a);
        return NULL;
    }
    
    bzero(a->clientName, sizeof(a->clientName));
    bzero(a->dstIP, sizeof(a->dstIP));
    
    strncpy(a->clientName, argv[1], nLen);
    a->srcPort = atoi(argv[2]);
    strncpy(a->dstIP, argv[3], IPLen);
    a->dstPort = atoi(argv[4]);
    
    return a;
}

struct Args *initServerArgs(int argc, char **argv){
    if(argc < 2){
        printf("Usage: server_PFS <Server Port>\n");
        return NULL;
    }
    
    struct Args *a;
    if((a = (struct Args *) malloc(sizeof(struct Args))) == NULL){
        releaseArgs(a);
        return NULL;
    }
    
    a->clientName = NULL;
    a->dstIP = NULL;
    a->dstPort = 0;
    a->srcPort = atoi(argv[1]);    
    return a;
}