#include <stdlib.h>
#include "rpsc.h"

#include "session.h"

// create a session
// returns an id_session
struct roman * sessions;

int create_session() {
    struct roman * p = (struct roman *) malloc(sizeof(struct roman));
    p->name=NULL;
    p->open=0; // we are not loading a file here..
    p->first_sh=p->last_sh=p->cur_sh=0;
    p->cache=0;
    p->cache_nr=0;
    //FIXME this is always 1 because variable is not shared between forks
    p->id = sessions == NULL ? 1 : sessions->id + 1;
    p->next = sessions;
    sessions = p;
    return p->id;
}

// close a session. freeing roman struct memory.
// return 0 OK, -1 on error
int close_session (int id) {
    struct roman * r = sessions;
    struct roman * prev = NULL;

    if (sessions && sessions->id == id) {
        free(r);
        sessions = NULL;
        return 0;
    }
    while (r != NULL) {
        if (r->id == id) {
            prev = sessions->next;
            free(prev);
            return 0;
        }
        prev = r;
        r = r->next;
    }
    return -1;
}
