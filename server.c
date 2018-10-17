#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <msgpack.h>
#include <fcntl.h>
#include <pthread.h>

#include "session.h"
#include "sheet.h"
#include "rpsc.h"

#define PORT 1234
#define MAXCLIENTS 30

void * handle_new_connection(void * arg);
void process_msg(int msocket, msgpack_object o);

struct sockaddr_in address;
int server_fd;

msgpack_sbuffer sbuf;
msgpack_packer pk;
msgpack_zone mempool;

/* Used as argument to thread_start() */
struct thread_info {
       pthread_t thread_id;        /* ID returned by pthread_create() */
       int       msocket;          /* socket descriptor */
       };


// this is a dirty struct to handle a message
// will be removed later on
typedef struct {
    int id;
    char method[20];
    char name[20]; //sheet_name
    int row;
    int col;
    double val;
} msg;

void decompress_msg(msgpack_object o, msg * m);

int main(void) {
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
    while (1) {

        // check number of clients
        if (listen(server_fd, MAXCLIENTS) < 0) {
            printf("listen failure");
            return -1;
        }

        if ((msocket = accept(server_fd, (struct sockaddr *) &address, (socklen_t*) &addrlen))<0) {
            printf("accept");
            return -1;
        }
        printf("new connection - address:%d port:%d\n", address.sin_addr, address.sin_port);

        // Put the socket in non-blocking mode:
        if(fcntl(msocket, F_SETFL, fcntl(msocket, F_GETFL) | O_NONBLOCK) < 0) {
            printf("cannot set socket in non blocking mode");
            return -1;
        }
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
    return 0;

    // wait for threads to finish
    printf("waiting for threads to finish");
    for (i=0; i<intLanzados; i++) {
        stat = pthread_join(thLanzados[i].thread_id, NULL);
        if (0 != stat) return -1;
    }

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

    msgpack_sbuffer_init(&sbuf); /* msgpack::sbuffer */
    msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write); /* initialize packer */

    // FIXME is this used correctly here?
    slab_allocator_init();
    ExpressionInit();
    init_lib();

    // send server welcome
    char * helo = "ROMAN server v0.01";
    send(msocket, helo, strlen(helo), 0 );

    int l=0;
    while (1) {
        valread = read(msocket, buffer, 1024);

        if (valread > 0) {
            msgpack_object deserialized;

            //FIXME sizeof(..) may differ on sender and receiver machine
            msgpack_unpack(buffer, sizeof(buffer), NULL, &mempool, &deserialized);

            process_msg(msocket, deserialized);

            if (! strncmp(buffer, "BYE", 3)) break;
            //if (valread == 4) printf("msg from %d %d: %s %d\n", s.sin_addr, s.sin_port, buffer, sizeof(buffer));

        }
    }
    msgpack_zone_destroy(&mempool);
    msgpack_sbuffer_destroy(&sbuf);

    close(msocket);
    printf("closed socket: %d\n", msocket);
    return;
}

void process_msg(int msocket, msgpack_object o) {
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
        send(msocket, sbuf.data, sbuf.size, 0 );
        msgpack_sbuffer_clear(&sbuf);

    // handle a message that comes in a map
    } else if (o.type == MSGPACK_OBJECT_MAP) {

        int size = o.via.map.size;

        msgpack_object_kv * p = o.via.map.ptr;
        msgpack_object_kv * pend = o.via.map.ptr + o.via.map.size;

        msg m;
        m.method[0]='\0';
        m.name[0]='\0';
        m.id  = -1;
        m.row = -1;
        m.col = -1;
        m.val = -1; // val should be int * type

        decompress_msg(o, &m);

        printf("debug:\n");
        if (m.id != -1)       printf("m.id: %d\n", m.id);
        if (m.row != -1)      printf("m.row: %d\n", m.row);
        if (m.col != -1)      printf("m.col: %d\n", m.col);
        if (strlen(m.method)) printf("m.method: %s\n", m.method);
        if (strlen(m.method)) printf("m.name: %s\n", m.name);
        printf("m.val: %f\n", m.val);

        if (! strncmp(m.method, "create_sheet", 12)) {
            struct roman * cur_sesn = get_session (m.id);
            struct Sheet * sh = new_sheet(cur_sesn, m.name);
            growtbl(sh, GROWNEW, 0, 0);
            cur_sesn->cur_sh = sh;
            // return an OK msg to client
            msgpack_pack_map(&pk, 1);
            msgpack_pack_str(&pk, 3);
            msgpack_pack_str_body(&pk, "ret", 3);
            msgpack_pack_short(&pk, 0);
            send(msocket, sbuf.data, sbuf.size, 0 );
            msgpack_sbuffer_clear(&sbuf);
            /* TODO: should return -1 if session not found
                     should return -2 if sheet not found */

        } else if (! strncmp(m.method, "set_val", 7)) {
            printf("\nin server set_val\n");
            struct roman * cur_sesn = get_session (m.id);
            struct Ent * t = lookat(cur_sesn->cur_sh, m.row, m.col); // USES CURRENT SHEET
            t->flag |= VAL;
            t->val = m.val;

            // return an OK msg to client
            msgpack_pack_map(&pk, 1);
            msgpack_pack_str(&pk, 3);
            msgpack_pack_str_body(&pk, "ret", 3);
            msgpack_pack_short(&pk, 0);

            send(msocket, sbuf.data, sbuf.size, 0 );
            msgpack_sbuffer_clear(&sbuf);
            /* TODO: should return -1 if session not found
                     should return -2 if sheet not found */
            printf("\nfin server set_val\n");

        } else if (! strncmp(m.method, "get_val", 7)) {
            printf("\nin server get_val\n");
            struct roman * cur_sesn = get_session (m.id);
            struct Ent * e1 = lookat(cur_sesn->cur_sh, m.row, m.col);
            printf("e1->val: %f", e1->val);

            msgpack_pack_map(&pk, 2);
            msgpack_pack_str(&pk, 3);
            msgpack_pack_str_body(&pk, "ret", 3);
            msgpack_pack_short(&pk, 0);
            msgpack_pack_str(&pk, 3);
            msgpack_pack_str_body(&pk, "val", 3);
            msgpack_pack_float(&pk, e1->val);

            send(msocket, sbuf.data, sbuf.size, 0 );
            msgpack_sbuffer_clear(&sbuf);
            /* TODO: should return -1 if session not found
                     should return -2 if sheet not found */
            printf("\nfin server get_val\n");

        } else if (! strncmp("recalc", m.method, 6)) {
            // TODO
        }
    }
}

// decompress message
// will be removed later
void decompress_msg(msgpack_object o, msg * m) {
        int size = o.via.map.size;

        msgpack_object_kv * p = o.via.map.ptr;
        msgpack_object_kv * pend = o.via.map.ptr + o.via.map.size;

        while (p < pend) {
            if (p->key.type == MSGPACK_OBJECT_STR) {
                //printf("key: %.*s\n", p->key.via.str.size, p->key.via.str.ptr);

                if (! strncmp(p->key.via.str.ptr, "id", p->key.via.str.size)) m->id = p->val.via.u64;
                else if (! strncmp(p->key.via.str.ptr, "row", p->key.via.str.size)) m->row = p->val.via.u64;
                else if (! strncmp(p->key.via.str.ptr, "col", p->key.via.str.size)) m->col = p->val.via.u64;
                else if (! strncmp(p->key.via.str.ptr, "val", p->key.via.str.size)) m->val = p->val.via.f64;
                else if (! strncmp(p->key.via.str.ptr, "method", p->key.via.str.size)) sprintf(m->method, "%.*s", p->val.via.str.size, p->val.via.str.ptr);
                else if (! strncmp(p->key.via.str.ptr, "name", p->key.via.str.size)) sprintf(m->name, "%.*s", p->val.via.str.size, p->val.via.str.ptr);

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

