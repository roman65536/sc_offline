#include <stdlib.h>
#include "rpsc.h"

#include "session.h"

// create a session
// returns an id_session when session is created
// returns -1 on error
struct roman * sessions;

int create_session() {
    pthread_mutex_t lock;

    if (pthread_mutex_init(&lock, NULL) != 0) {
        printf("\ncreate_session: mutex init has failed\n");
        return -1;
    }
    struct roman * p = (struct roman *) malloc(sizeof(struct roman));
    p->name=NULL;
    p->open=0; // we are not loading a file here..
    p->first_sh=p->last_sh=p->cur_sh=0;
    p->cache=0;
    p->cache_nr=0;
    pthread_mutex_lock(&lock);
    p->id = sessions == NULL ? 1 : sessions->id + 1;
    p->next = sessions;
    sessions = p;
    pthread_mutex_unlock(&lock);
    pthread_mutex_destroy(&lock);
    return p->id;
}

// close a session. freeing roman struct memory.
// return 0 OK, -1 on error
int close_session (int id) {
    pthread_mutex_t lock;
    int ret = -1;

    if (pthread_mutex_init(&lock, NULL) != 0) {
        printf("\nclose_session: mutex init has failed\n");
        return ret;
    }
    pthread_mutex_lock(&lock);
    struct roman * r = sessions;
    struct roman * prev = NULL;

    if (sessions && sessions->id == id) {
        free(r);
        sessions = NULL;
        printf("session removed1\n");
        ret = 0;
    }
    while (r != NULL && ret == -1) {
        if (r->id == id) {
            prev = sessions->next;
            free(prev);
            printf("session removed2\n");
            ret = 0;
            break;
        }
        prev = r;
        r = r->next;
    }
    pthread_mutex_unlock(&lock);
    pthread_mutex_destroy(&lock);
    return ret;
}

struct roman * get_session (int id) {
    pthread_mutex_t lock;
    struct roman * ret = NULL;
    if (pthread_mutex_init(&lock, NULL) != 0) {
        printf("\nclose_session: mutex init has failed\n");
        return NULL;
    }

    pthread_mutex_lock(&lock);
    if (sessions && sessions->id == id) {
        ret = sessions;
    }

    struct roman * r = sessions;
    while (r != NULL && ret == NULL) {
        if (r->id == id) ret = r;
        r = r->next;
    }
    pthread_mutex_unlock(&lock);
    pthread_mutex_destroy(&lock);
    return ret;
}
