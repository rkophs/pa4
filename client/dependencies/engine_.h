#include "args_.h"

struct Engine {
    struct Args *args;
};

void releaseEngine(struct Engine *e){
    if(e == NULL){
        return;
    }
    if(e->args != NULL){
        releaseArgs(e->args);
    }
    
    free(e);
    e = NULL;
}

struct Engine *initEngine(int argc, char **argv) {
    struct Engine *e;
    if((e = (struct Engine *) malloc(sizeof(struct Engine))) == NULL){
        releaseEngine(e);
        return NULL;
    }
    
    if((e->args = initArgs(argc, argv)) == NULL){
        releaseEngine(e);
        return NULL;
    }
    
    return e;
};
