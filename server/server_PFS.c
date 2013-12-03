#include "dependencies/engine_.h"

int main(int argc, char** argv) {

    struct Engine *e;
    if((e = initEngine(argc, argv)) == NULL){
        releaseEngine(e); //OK if called twice by NULL check
        return(EXIT_FAILURE);
    }
        
    engineRun(e);
    
    releaseEngine(e);
    return (EXIT_SUCCESS);
}

