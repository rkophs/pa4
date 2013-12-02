#include "dependencies/engine_.h"

int main(int argc, char** argv) {

    struct Engine *e;
    if((e = initEngine(argc, argv)) == NULL){
        releaseEngine(e); //OK if called twice by NULL check
        return(EXIT_FAILURE);
    }
    
    printf("Interpreted args: name: %s, IP: %s, src: %i, dst: %i\n", e->args->clientName, e->args->dstIP, e->args->srcPort, e->args->dstPort);
    
    if(engineStart(e) < 0 ){
        releaseEngine(e);
        return(EXIT_FAILURE);
    }
    
    engineRun(e);
    
    
    releaseEngine(e);
    return (EXIT_SUCCESS);
}

