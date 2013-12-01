#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

pthread_t tid[2];
pthread_mutex_t lock;


struct ARGS {
    int A;
};

void* doSomeThing(void  *arg)
{
    struct ARGS *c = arg;
    pthread_mutex_lock(&lock);

    unsigned long i = 0;
    c->A += 1;
    printf("\n Job %d started\n", c->A);

    for(i=0; i<100000;i++);

    printf("\n Job %d finished\n", c->A);

    pthread_mutex_unlock(&lock);

    return NULL;
}

int main(void)
{
    int i = 0;
    int err;
    
    struct ARGS *c;
    c->A = 13;

    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }

    while(i < 5)
    {
        err = pthread_create(&(tid[i]), NULL, &doSomeThing, c);
        if (err != 0)
            printf("\ncan't create thread :[%s]", strerror(err));
        i++;
    }

    pthread_join(tid[0], NULL);
    pthread_join(tid[1], NULL);
    pthread_mutex_destroy(&lock);

    return 0;
}