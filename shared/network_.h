#include <stdarg.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>

#include "args_.h"

//struct Connection {
//    int srcPort; //Used by listeners
//    int dstPort; //Used by connectors
//    int sockfd; //Used by all
//    char *remoteIP; //Used by connectors
//    struct Connection *next;
//};

//void releaseConnection(struct Connection *c) {
//    if (c == NULL) {
//        return;
//    }
//    c->srcPort = -1;
//    c->srcPort = -1;
//    c->sockfd = -1;
//
//    if (c->remoteIP != NULL) {
//        free(c->remoteIP);
//        c->remoteIP = NULL;
//    }
//    free(c);
//    c = NULL;
//}
//
//void connectionRemove(struct Connection *c, int sockfd) {
//    if (c == NULL || sockfd < 0) {
//        return;
//    }
//
//    struct Connection *it1 = NULL;
//    struct Connection *it2 = c;
//
//    while (it2 != NULL) {
//        if (it2->sockfd == sockfd) { //match
//            if (it1 == NULL) { //Caught on first node
//                it1 = it2->next;
//                releaseConnection(it2);
//            } else { //Caught on future nodes
//                it1 = it2->next;
//                releaseConnection(it2);
//            }
//            break;
//        }
//        it1 = it2;
//        it2 = it2->next;
//    }
//    c = it1;
//}
//
//void releaseConnections(struct Connection *c) {
//    if (c == NULL) {
//        return;
//    }
//
//    struct Connection *tmp = NULL;
//    struct Connection *it = c;
//
//    while (it != NULL) {
//        tmp = it->next;
//        releaseConnection(it);
//        it = tmp;
//    }
//    c = NULL;
//}
//
//void connectionAdd(struct Connection *pool,
//        int srcPort, int dstPort, char *remoteIP, int remoteIPLen) {
//
//    struct Connection *c;
//    if ((c = (struct Connection *) malloc(sizeof (struct Connection))) == NULL) {
//        releaseConnection(c);
//        return NULL;
//    }
//    c->srcPort = srcPort;
//    c->dstPort = dstPort;
//    c->sockfd = -1;
//    c->remoteIP = NULL;
//    c->next = NULL;
//
//    if (remoteIPLen > 0) {
//        if ((c->remoteIP = (char *) malloc(remoteIPLen + 1)) == NULL) {
//            releaseConnection(c);
//            return NULL;
//        }
//        bzero(c->remoteIP, sizeof (c->remoteIP));
//        strncpy(c->remoteIP, remoteIP, remoteIPLen);
//    }
//
//    //Add new connection to the pool:
//    if (pool == NULL) {
//        pool = c;
//    } else {
//        struct Connection *it = pool;
//        while (it->next != NULL) {
//            it = it->next;
//        }
//        it->next = c;
//    }
//}
//
//int connectionRemoteIP(struct Connection* c, char *ip, int ipLen) {
//    if ((c->remoteIP = (char *) malloc(ipLen + 1)) == NULL) {
//        return -1;
//    }
//    bzero(c->remoteIP, sizeof (c->remoteIP));
//    strncpy(c->remoteIP, ip, ipLen);
//    return 0;
//}

/*
 * Bind a TCP listener that listens on any address via the connection src port
 */
int bindListener(int srcPort) {

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
    srcAddr.sin_port = srcPort;
    srcAddr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sockfd, (struct sockaddr *) &srcAddr, sizeof (srcAddr)) < 0) {
        return -1;
    }
    if (listen(sockfd, 20) < 0) {
        return -1;
    }
    return sockfd;
}

/* 
 * Connect with the destination address and port (non-blocking)
 */
int bindConnector(int dstPort, char *dstIP) {

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
    destAddr.sin_port = dstPort;
    if (dstIP != NULL) {
        destAddr.sin_addr.s_addr = inet_addr(dstIP);
    }

    if (connect(sockfd, (struct sockaddr *) &destAddr, sizeof (destAddr)) < 0) {
        return -1;
    }
    return sockfd;
}

ssize_t recvDecrypt(int sockfd, void *buf, size_t len, int flags){
    return recv(sockfd, buf, len, flags);
}

ssize_t sendEncrypt(int sockfd, void *buf, size_t len, int flags){
    return send(sockfd, buf, len, flags);
}

int BUFF(char *buffer, int size, const char *format, ...) {
    va_list args;
    va_start(args, format);
    bzero(buffer, size);
    int sizeUlt = vsprintf(buffer, format, args);
    va_end(args);
    return sizeUlt;
}