/* INCLUDES */
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <msgpack.h>
#include <fcntl.h> //open
#include <pthread.h>
#include <unistd.h> //close
#include <signal.h> // for SIGINT
#include "session.h"
#include "sheet.h"
#include "rpsc.h"

/* DEFINES */
#define PORT 1234
#define MAXCLIENTS 3

/* STRUCTS */
/* Used as argument to thread_start() */
struct thread_info {
    pthread_t thread_id;        /* ID returned by pthread_create() */
    int       msocket;          /* socket descriptor */
    int exit;                   /* set it to 1 to exit thread */
};

// this is a dirty struct to handle a message
// will be removed later on when code generator is ready
typedef struct {
    int * id;
    char * method;
    char * name;      // sheet_name
    int * row;
    int * col;
    int * to_row;
    int * to_col;
    short * bye;
    double * val;     // ent value
    char * label;     // ent label
    short * flag;     // ent flag
    short * formula;  // ent formula
} msg;

/* PROTOTYPES */
void * handle_new_connection(void * arg);
int process_msg(int msocket, msgpack_object o, msgpack_sbuffer * sbuf);
void initialize_msg(msg * m);
void free_msg(msg * m);
void decompress_msg(msgpack_object o, msg * m);
void debug_msg(msg * m);

/* GLOBAL VARIABLES */
msgpack_packer pk;
msgpack_zone mempool;
int got_SIGINT = 0;
struct sockaddr_in address;
int server_fd;


/************************************************/

void SIGINT_handler(int dummy) {
    got_SIGINT = 1;
    printf("Got SIGINT. Shutting down server\n");
}

int main(void) {
    signal(SIGINT, SIGINT_handler);
    int opt = 1;
    int addrlen = sizeof(address);
    int intLanzados = 0;
    int stat, msocket, i;
    struct thread_info thLanzados[MAXCLIENTS];

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        printf("socket failed");
        return -1;
    }

    // Forcefully attaching socket to the port PORT
    // prevents “address already in use”.
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        printf("setsockopt");
        return -1;
    }


    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );

    if (bind(server_fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
        printf("bind failed");
        return -1;
    }


    /* deserialize the buffer into msgpack_object instance. */
    /* deserialized object is valid during the msgpack_zone instance alive. */
    msgpack_zone_init(&mempool, 1024);

    // loop
    while (! got_SIGINT) {

        // check number of clients
        if (listen(server_fd, MAXCLIENTS) < 0) {
            printf("listen failure");
            return -1;
        }

        /*if(fcntl(server_fd, F_SETFL, O_NONBLOCK) < 0) {
            printf("!! cannot set socket in non blocking mode");
            return -1;
        }*/
        if ((msocket = accept(server_fd, (struct sockaddr *) &address, (socklen_t*) &addrlen))<0) {
            continue;
            //return -1;
        }
        printf("new connection - address:%d port:%d\n", address.sin_addr, address.sin_port);

        /* Put the socket in non-blocking mode:
        if(fcntl(msocket, F_SETFL, fcntl(msocket, F_GETFL) | O_NONBLOCK) < 0) {
            printf("cannot set socket in non blocking mode");
            return -1;
        }*/
        if (msocket < 0) {
            printf("ERROR on accept\n");
            continue;
        }

        thLanzados[intLanzados].msocket = msocket;

        // The pthread_create() call stores the thread ID into corresponding element of thread_info
        int stat = pthread_create(&thLanzados[intLanzados].thread_id, NULL, &handle_new_connection, &thLanzados[intLanzados]);

        // Error creating the thread
        if ( 0 != stat ) return -1;
        else intLanzados++;

    }


    // wait for threads to finish
    printf("waiting for threads to finish");
    for (i=0; i<intLanzados; i++) {
        stat = pthread_join(thLanzados[i].thread_id, NULL);
        if (0 != stat) {
            printf("error waiting threads to finish\n");
            return -1;
        }
    }

    msgpack_zone_destroy(&mempool);

    close(server_fd);
    return 0;
}

void * handle_new_connection(void * arg) {
    struct thread_info * ti = arg;
    int tid = ti->thread_id;
    int msocket = ti->msocket;
    char buffer[1024] = {0};
    printf("new   thread: %d\n", msocket);
    int valread;
    msgpack_sbuffer sbuf;

    msgpack_sbuffer_init(&sbuf); /* msgpack::sbuffer */
    msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write); /* initialize packer */

    // FIXME is this used correctly here?
    slab_allocator_init();
    ExpressionInit();
    init_lib();

    // send server welcome
    char * helo = "ROMAN server v0.01";
    send(msocket, helo, strlen(helo), 0 );

    int res = 0;
    while (res == 0) {
        valread = read(msocket, buffer, 1024);

        if (valread > 0) {
            msgpack_object deserialized;

            //FIXME sizeof(..) may differ on sender and receiver machine
            msgpack_unpack(buffer, sizeof(buffer), NULL, &mempool, &deserialized);

            res = process_msg(msocket, deserialized, &sbuf);

            //if (valread == 4) printf("msg from %d %d: %s %d\n", s.sin_addr, s.sin_port, buffer, sizeof(buffer));

        }
    }

    close(msocket);
    printf("closed socket: %d\n", msocket);
    msgpack_sbuffer_destroy(&sbuf);
    return NULL;
}

/* process a message
   return 0 when message was handled
   return 1 on exit (close session)
*/
int process_msg(int msocket, msgpack_object o, msgpack_sbuffer * sbuf) {
    printf("\nprocess msg from: %d - type:%d\n\\\t->", msocket, o.type);
    //msgpack_object_print(stdout, o);
    //printf("\n\n");

    // HANDLE HELO and create a new session
    if (o.type == MSGPACK_OBJECT_STR && ! strcmp(o.via.str.ptr, "HELO")) {
        //printf("got: %s\n", o);
        // create new session
        int id_session = create_session();
        printf("created session #: %d\n", id_session);

        msgpack_pack_map(&pk, 1);
        msgpack_pack_str(&pk, 2);
        msgpack_pack_str_body(&pk, "id", 2);
        msgpack_pack_int(&pk, id_session);
        send(msocket, sbuf->data, sbuf->size, 0 );
        msgpack_sbuffer_clear(sbuf);

    // handle a message that comes in a map
    } else if (o.type == MSGPACK_OBJECT_MAP) {

        int size = o.via.map.size;

        msgpack_object_kv * p = o.via.map.ptr;
        msgpack_object_kv * pend = o.via.map.ptr + o.via.map.size;

        msg m;
        initialize_msg(&m);
        decompress_msg(o, &m);
        debug_msg(&m);


        if (! strncmp(m.method, "create_sheet", 12)) {
            struct roman * cur_sesn = get_session(*(m.id));
            struct Sheet * sh = new_sheet(cur_sesn, m.name);
            /* TODO: should return -1 if session not found */
            //growtbl(sh, GROWNEW, 0, 0);       /* ONLY if OLD is defined */
            cur_sesn->cur_sh = sh;
            msgpack_pack_map(&pk, 1);
            msgpack_pack_str(&pk, 3);
            msgpack_pack_str_body(&pk, "ret", 3); /* return an OK msg to client */
            msgpack_pack_short(&pk, 0);
            send(msocket, sbuf->data, sbuf->size, 0 );
            msgpack_sbuffer_clear(sbuf);

        } else if (! strncmp(m.method, "delete_sheet", 12)) {
            struct roman * cur_sesn = get_session (*(m.id));
            struct Sheet * sh = Search_sheet(cur_sesn, m.name);
            /* TODO: should return -1 if session not found
                     should return -2 if sheet not found */
            delete_sheet(cur_sesn, sh);

            // return an OK msg to client
            msgpack_pack_map(&pk, 1);
            msgpack_pack_str(&pk, 3);
            msgpack_pack_str_body(&pk, "ret", 3);
            msgpack_pack_short(&pk, 0);
            send(msocket, sbuf->data, sbuf->size, 0 );
            msgpack_sbuffer_clear(sbuf);

        } else if (! strncmp(m.method, "bye", 3)) {
            int res = close_session (*(m.id));
            // return an OK msg to client
            msgpack_pack_map(&pk, 1);
            msgpack_pack_str(&pk, 3);
            msgpack_pack_str_body(&pk, "ret", 3);
            msgpack_pack_short(&pk, res);
            send(msocket, sbuf->data, sbuf->size, 0 );
            msgpack_sbuffer_clear(sbuf);
            return 1;

        } else if (! strncmp(m.method, "set_val", 7)) {
            printf("\nin server set_val\n");
            struct roman * cur_sesn = get_session (*(m.id));
            struct Ent * t = lookat(cur_sesn->cur_sh, *(m.row), *(m.col)); // USES CURRENT SHEET
            /* TODO: should return -1 if session not found
                     should return -2 if sheet not found */
            t->flag |= VAL;
            t->val = *(m.val);

            // return an OK msg to client
            msgpack_pack_map(&pk, 1);
            msgpack_pack_str(&pk, 3);
            msgpack_pack_str_body(&pk, "ret", 3);
            msgpack_pack_short(&pk, 0);

            send(msocket, sbuf->data, sbuf->size, 0 );
            msgpack_sbuffer_clear(sbuf);
            printf("\nfin server set_val\n");

        } else if (! strncmp(m.method, "get_val", 7)) {
            printf("\nin server get_val\n");
            struct roman * cur_sesn = get_session (*(m.id));
            struct Ent * e1 = lookat(cur_sesn->cur_sh, *(m.row), *(m.col));
            /* TODO: should return -1 if session not found
                     should return -2 if sheet not found */
            printf("e1->val: %f", e1->val);

            msgpack_pack_map(&pk, 2);
            msgpack_pack_str(&pk, 3);
            msgpack_pack_str_body(&pk, "ret", 3);
            msgpack_pack_short(&pk, 0);
            msgpack_pack_str(&pk, 3);
            msgpack_pack_str_body(&pk, "val", 3);
            msgpack_pack_float(&pk, e1->val);

            send(msocket, sbuf->data, sbuf->size, 0 );
            msgpack_sbuffer_clear(sbuf);
            printf("\nfin server get_val\n");

        } else if (! strncmp(m.method, "get_cells", 9)) {
            printf("\nin server get_cells\n");
            struct roman * cur_sesn = get_session (*(m.id));
            /* TODO: should return -1 if session not found
                     should return -2 if sheet not found */

            msgpack_pack_map(&pk, 3);

            msgpack_pack_str(&pk, 3);
            msgpack_pack_str_body(&pk, "ret", 3);
            msgpack_pack_short(&pk, 0);

            /* count the number of ents to be returned */
            int ri = *(m.row);
            int ci = *(m.col);
            int rf = *(m.to_row);
            int cf = *(m.to_col);
            int r,c;
            register struct Ent ** pp;
            struct Ent * e1;
            int count = 0; // we need to count the cells we are going to send
            for (r=ri; r<rf; r++) {
                for (c=ci; c<cf; c++) {
                    pp = ATBL(cur_sesn->cur_sh, cur_sesn->cur_sh->tbl, r, c);
                    if ((pp !=0 ) && (*pp != 0)) count++;
                }
            }

            msgpack_pack_str(&pk, 5);
            msgpack_pack_str_body(&pk, "count", 5);
            msgpack_pack_short(&pk, count);

            msgpack_pack_str(&pk, 5);
            msgpack_pack_str_body(&pk, "cells", 5);
            msgpack_pack_map(&pk, count);

            int attr_count; /* we need to send the number of (key,values) in the first place. row and col are already counted*/

            for (r=ri; r<rf; r++) {
                for (c=ci; c<cf; c++) {
                    pp = ATBL(cur_sesn->cur_sh, cur_sesn->cur_sh->tbl, r, c);
                    if ((pp !=0 ) && (*pp != 0)) {
                        e1 = *pp;
                        attr_count = 2;
                        if (e1->val)     attr_count++;
                        if (e1->label)   attr_count++;
                        if (e1->formula) attr_count++;
                        if (e1->flag)    attr_count++;
                        msgpack_pack_str(&pk, 4);
                        msgpack_pack_str_body(&pk, "cell", 4);
                        msgpack_pack_map(&pk, attr_count);

                        // only pack whats is needed
                        msgpack_pack_str(&pk, 3);
                        msgpack_pack_str_body(&pk, "row", 3);
                        msgpack_pack_float(&pk, e1->row);
                        msgpack_pack_str(&pk, 3);
                        msgpack_pack_str_body(&pk, "col", 3);
                        msgpack_pack_float(&pk, e1->col);
                        if (e1->val) {
                            msgpack_pack_str(&pk, 3);
                            msgpack_pack_str_body(&pk, "val", 3);
                            msgpack_pack_float(&pk, e1->val);
                        }

                        if (e1->label) {
                            msgpack_pack_str(&pk, 5);
                            msgpack_pack_str_body(&pk, "label", 5);
                            msgpack_pack_str(&pk, e1->label != NULL ? strlen(e1->label) : 0);
                            msgpack_pack_str_body(&pk, e1->label != NULL ? e1->label : "", e1->label != NULL ? strlen(e1->label) : 0);
                        }

                        if (e1->formula) {
                            msgpack_pack_str(&pk, 7);
                            msgpack_pack_str_body(&pk, "formula", 7);
                            msgpack_pack_str(&pk, e1->formula != NULL ? strlen(e1->formula) : 0);
                            msgpack_pack_str_body(&pk, e1->formula != NULL ? e1->formula : "", e1->formula != NULL ? strlen(e1->formula) : 0);
                        }

                        if (e1->flag) {
                            msgpack_pack_str(&pk, 4);
                            msgpack_pack_str_body(&pk, "flag", 4);
                            msgpack_pack_short(&pk, e1->flag);
                        }
                    }
                }
            }

            send(msocket, sbuf->data, sbuf->size, 0 );
            msgpack_sbuffer_clear(sbuf);

            printf("\nfin server get_cells\n");

        } else if (! strncmp(m.method, "get_cell", 8)) {
            printf("\nin server get_cell\n");
            struct roman * cur_sesn = get_session (*(m.id));
            /* TODO: should return -1 if session not found
                     should return -2 if sheet not found */


            int attr_count; /* we need to send the number of (key,values) in the first place. row and col are already counted*/
            int row = *(m.row);
            int col = *(m.col);

            register struct Ent ** pp;
            struct Ent * e1;
            pp = ATBL(cur_sesn->cur_sh, cur_sesn->cur_sh->tbl, row, col);
            if ((pp !=0 ) && (*pp != 0)) {
                msgpack_pack_map(&pk, 2);

                msgpack_pack_str(&pk, 3);
                msgpack_pack_str_body(&pk, "ret", 3);
                msgpack_pack_short(&pk, 0);
                e1 = *pp;
                attr_count = 2;
                if (e1->val)     attr_count++;
                if (e1->label)   attr_count++;
                if (e1->formula) attr_count++;
                if (e1->flag)    attr_count++;
                msgpack_pack_str(&pk, 4);
                msgpack_pack_str_body(&pk, "cell", 4);
                msgpack_pack_map(&pk, attr_count);

                // only pack whats is needed
                msgpack_pack_str(&pk, 3);
                msgpack_pack_str_body(&pk, "row", 3);
                msgpack_pack_int(&pk, e1->row);
                msgpack_pack_str(&pk, 3);
                msgpack_pack_str_body(&pk, "col", 3);
                msgpack_pack_int(&pk, e1->col);
                if (e1->val) {
                    msgpack_pack_str(&pk, 3);
                    msgpack_pack_str_body(&pk, "val", 3);
                    msgpack_pack_float(&pk, e1->val);
                }

                if (e1->label) {
                    msgpack_pack_str(&pk, 5);
                    msgpack_pack_str_body(&pk, "label", 5);
                    msgpack_pack_str(&pk, e1->label != NULL ? strlen(e1->label) : 0);
                    msgpack_pack_str_body(&pk, e1->label != NULL ? e1->label : "", e1->label != NULL ? strlen(e1->label) : 0);
                }

                if (e1->formula) {
                    msgpack_pack_str(&pk, 7);
                    msgpack_pack_str_body(&pk, "formula", 7);
                    msgpack_pack_str(&pk, e1->formula != NULL ? strlen(e1->formula) : 0);
                    msgpack_pack_str_body(&pk, e1->formula != NULL ? e1->formula : "", e1->formula != NULL ? strlen(e1->formula) : 0);
                }

                if (e1->flag) {
                    msgpack_pack_str(&pk, 4);
                    msgpack_pack_str_body(&pk, "flag", 4);
                    msgpack_pack_short(&pk, e1->flag);
                }
            } else {
                msgpack_pack_map(&pk, 1);

                msgpack_pack_str(&pk, 3);
                msgpack_pack_str_body(&pk, "ret", 3);
                msgpack_pack_short(&pk, -1);
            }
            send(msocket, sbuf->data, sbuf->size, 0 );
            msgpack_sbuffer_clear(sbuf);
            printf("\nfin server get_cell\n");

        } else if (! strncmp("recalc", m.method, 6)) {
            // TODO
        }
        free_msg(&m);
    }
    return 0;
}

// decompress message
// will be removed later when the code generator is ready
void decompress_msg(msgpack_object o, msg * m) {
        int size = o.via.map.size;

        msgpack_object_kv * p = o.via.map.ptr;
        msgpack_object_kv * pend = o.via.map.ptr + o.via.map.size;

        while (p < pend) {
            if (p->key.type == MSGPACK_OBJECT_STR) {
                printf("key: %.*s\n", p->key.via.str.size, p->key.via.str.ptr);

                if (m->id == NULL && ! strncmp(p->key.via.str.ptr, "id", p->key.via.str.size)) {
                    m->id = (int *) malloc(sizeof(int));
                    *(m->id) = p->val.via.u64;
                } else if (m->row == NULL && ! strncmp(p->key.via.str.ptr, "row", p->key.via.str.size)) {
                    m->row = (int *) malloc(sizeof(int));
                    *(m->row) = p->val.via.u64;
                } else if (m->col == NULL && ! strncmp(p->key.via.str.ptr, "col", p->key.via.str.size)) {
                    m->col = (int *) malloc(sizeof(int));
                    *(m->col) = p->val.via.u64;
                } else if (m->to_row == NULL && ! strncmp(p->key.via.str.ptr, "to_row", p->key.via.str.size)) {
                    m->to_row = (int *) malloc(sizeof(int));
                    *(m->to_row) = p->val.via.u64;
                } else if (m->to_col == NULL && ! strncmp(p->key.via.str.ptr, "to_col", p->key.via.str.size)) {
                    m->to_col = (int *) malloc(sizeof(int));
                    *(m->to_col) = p->val.via.u64;
                } else if (m->bye == NULL && ! strncmp(p->key.via.str.ptr, "bye", p->key.via.str.size)) {
                    m->bye = (short *) malloc(sizeof(short));
                    *(m->bye) = p->val.via.u64;
                } else if (m->val == NULL && ! strncmp(p->key.via.str.ptr, "val", p->key.via.str.size)) {
                    m->val = (double *) malloc(sizeof(double));
                    *(m->val) = p->val.via.f64;
                } else if (m->method == NULL && ! strncmp(p->key.via.str.ptr, "method", p->key.via.str.size)) {
                    m->method = (char *) malloc(sizeof(char) * (p->val.via.str.size + 1));
                    sprintf(m->method, "%.*s", p->val.via.str.size, p->val.via.str.ptr);
                } else if (m->name == NULL && ! strncmp(p->key.via.str.ptr, "name", p->key.via.str.size)) {
                    m->name = (char *) malloc(sizeof(char) * (p->val.via.str.size + 1));
                    sprintf(m->name, "%.*s", p->val.via.str.size, p->val.via.str.ptr);
                }

            } else if (p->key.type == MSGPACK_OBJECT_POSITIVE_INTEGER) {
                //printf("key: %d\n", (int) p->key.via.u64);
            }

            if (p->val.type == MSGPACK_OBJECT_STR) {
                //printf("val: %.*s\n", p->val.via.str.size, p->val.via.str.ptr);
            } else if (p->val.type == MSGPACK_OBJECT_POSITIVE_INTEGER) {
                //printf("val: %d\n", (int) p->val.via.u64);
            } else if (p->val.type == MSGPACK_OBJECT_MAP) {
                //printf("val: ");
                //msgpack_object_print(stdout, p->val);
                decompress_msg(p->val, m);
                //printf("\n");
            }
            p++;
        }
}

void initialize_msg(msg * m) {
    m->method = NULL;
    m->name = NULL;
    m->id  = NULL;
    m->row = NULL;
    m->col = NULL;
    m->to_row = NULL;
    m->to_col = NULL;
    m->val = NULL;
    m->label = NULL;
    m->formula = NULL;
    m->flag = NULL;
    m->bye = NULL;
    return;
}

void free_msg(msg * m) {
    if (m->method != NULL)  free(m->method);
    if (m->name != NULL)    free(m->name);
    if (m->id  != NULL)     free(m->id);
    if (m->row != NULL)     free(m->row);
    if (m->col != NULL)     free(m->col);
    if (m->to_row != NULL)  free(m->to_row);
    if (m->to_col != NULL)  free(m->to_col);
    if (m->val != NULL)     free(m->val);
    if (m->label != NULL)   free(m->label);
    if (m->formula != NULL) free(m->formula);
    if (m->flag != NULL)   free(m->flag);
    if (m->bye != NULL)     free(m->bye);
    return;
}

void debug_msg(msg * m) {
    printf("debug:\n");
    if (m->id != NULL)       printf("m.id: %d\n", *(m->id));
    if (m->row != NULL)      printf("m.row: %d\n", *(m->row));
    if (m->col != NULL)      printf("m.col: %d\n", *(m->col));
    if (m->to_row != NULL)   printf("m.to_row: %d\n", *(m->to_row));
    if (m->to_col != NULL)   printf("m.to_col: %d\n", *(m->to_col));
    if (m->method != NULL)   printf("m.method: %s\n", (m->method));
    if (m->name != NULL)     printf("m.name: %s\n", (m->name));
    if (m->val != NULL)      printf("m.val: %f\n", *(m->val));
    if (m->label != NULL)    printf("m.label: %s\n", (m->label));
    if (m->flag != NULL)     printf("m.flag: %d\n", *(m->flag));
}
