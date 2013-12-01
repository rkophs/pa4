#include "dependencies/engine_.h"

int main(int argc, char** argv) {

    struct Engine *e;
    if((e = initEngine(argc, argv)) == NULL){
        return(EXIT_FAILURE);
    }
        
    releaseEngine(e);
    return (EXIT_SUCCESS);
}

