#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>

struct Connection {
    int srcPort;
    int dstPort;
    char *srcAddr;
    char *dstAddr;
    char *alias;
};

void releaseConnection(struct Connection* c) {
    if (c == NULL) {
        return;
    }

    if (c->srcAddr != NULL) {
        free(c->srcAddr);
        c->srcAddr = NULL;
    }
    if (c->dstAddr != NULL) {
        free(c->dstAddr);
        c->dstAddr = NULL;
    }
    if (c->alias != NULL) {
        free(c->alias);
        c->alias = NULL;
    }
    c->srcPort = 0;
    c->srcPort = 0;

    free(c);
    c = NULL;
}

struct Connection *P2P(int srcPort, int dstPort,
        char *srcAddr, int srcAddrLen,
        char *dstAddr, int dstAddrLen) {

    struct Connection *c;
    if ((c = (struct Connection *) malloc(sizeof (struct Connection))) == NULL) {
        releaseConnection(c);
        return NULL;
    }
    c->srcPort = srcPort;
    c->dstPort = dstPort;

    if ((c->srcAddr = (char *) malloc(srcAddrLen + 1)) == NULL) {
        releaseConnection(c);
        return NULL;
    }
    bzero(c->srcAddr, sizeof (c->srcAddr));
    strncpy(c->srcAddr, srcAddr, srcAddrLen);

    if ((c->dstAddr = (char *) malloc(dstAddrLen + 1)) == NULL) {
        releaseConnection(c);
        return NULL;
    }
    bzero(c->dstAddr, sizeof (c->dstAddr));
    strncpy(c->dstAddr, dstAddr, dstAddrLen);

    return c;
}

struct Connection *P2S(int srcPort, int dstPort,
        char *srcAddr, int srcAddrLen,
        char *dstAddr, int dstAddrLen,
        char *alias, int aliasLen) {

    struct Connection *c;
    if ((c = P2P(srcPort, dstPort, srcAddr, srcAddrLen,
            dstAddr, dstAddrLen)) == NULL) {
        return NULL;
    }

    if ((c->alias = (char *) malloc(aliasLen + 1)) == NULL) {
        releaseConnection(c);
        return NULL;
    }
    bzero(c->alias, sizeof (c->alias));
    strncpy(c->alias, alias, aliasLen);
    return c;
}

/*
 * Bind a TCP listener that listens on any address via the connection src port
 */
int bindListener(struct Connection *c) {

    //Open socket:
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return -1;
    }

    //Reuse address so OS doesn't disable after program exit:
    int val = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof (val)) < 0) {
        return -1;
    }

    //Set up connection information to listen for any address on srcPort:
    struct sockaddr_in srcAddr;
    bzero(&srcAddr, sizeof (srcAddr));
    srcAddr.sin_family = AF_INET;
    srcAddr.sin_port = c->srcPort;
    srcAddr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sockfd, (struct sockaddr *) &srcAddr, sizeof (srcAddr)) < 0) {
        return -1;
    }
    if (listen(sockfd, 20) < 0) { //Test this after all is working
        return -1;
    }
    return sockfd;
}

/* 
 * Connect with the connection destination address and port
 */
int bindConnector(struct Connection *c) {

    //Open socket:
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return -1;
    }

    //Reuse address so OS doesn't disable after program exit:
    int val = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof (val)) < 0) {
        return -1;
    }

    //Set up connection information to bind to specified address and port:
    struct sockaddr_in destAddr;
    bzero(&destAddr, sizeof (destAddr));
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = c->dstPort;
    destAddr.sin_addr.s_addr = inet_addr(c->dstAddr);

    if (connect(sockfd, (struct sockaddr *) &destAddr, sizeof (destAddr)) < 0) {
        return -1;
    }
    return sockfd;
}
