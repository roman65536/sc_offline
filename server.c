#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <msgpack.h>
#include <fcntl.h>

#include "session.h"

#define PORT 1234
#define MAXCLIENTS 3

void handle_new_connection(struct sockaddr_in s);
void process_msg(msgpack_object o);

int msocket;
struct sockaddr_in address;

msgpack_sbuffer sbuf;
msgpack_packer pk;
msgpack_zone mempool;

int main(void) {

    int server_fd;
    int opt = 1;
    int addrlen = sizeof(address);

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

    msgpack_sbuffer_init(&sbuf); /* msgpack::sbuffer */
    msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write); /* initialize packer */

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

        if ((msocket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
            printf("accept");
            return -1;
        }
        printf("new connection - address:%d port:%d\n", address.sin_addr, address.sin_port);

        // Put the socket in non-blocking mode:
        if(fcntl(msocket, F_SETFL, fcntl(msocket, F_GETFL) | O_NONBLOCK) < 0) {
            printf("cannot set socket in non blocking mode");
            return -1;
        }
        if (msocket < 0) printf("ERROR on accept\n");

        int pid = fork();

        if (pid < 0) printf("ERROR on fork\n");

        if (pid == 0)  {
            close(server_fd);
            handle_new_connection(address);
            return 0;
        } else close(msocket);
    }

    return 0;
}

void handle_new_connection(struct sockaddr_in s) {
    char buffer[1024] = {0};
    char * helo = "ROMAN server v0.01";
    int valread;

    // send helo
    send(msocket, helo, strlen(helo), 0 );

    while (1) {
        valread = read(msocket, buffer, 1024);

        if (valread != -1) {
            msgpack_object deserialized;

            //FIXME sizeof(..) may differ on sender and receiver machine
            msgpack_unpack(buffer, sizeof(buffer), NULL, &mempool, &deserialized);

            process_msg(deserialized);


            //if (! strncmp(buffer, "BYE", 3)) return;
            //if (valread == 4) printf("msg from %d %d: %s %d\n", s.sin_addr, s.sin_port, buffer, sizeof(buffer));

        }

        break;
    }
    msgpack_zone_destroy(&mempool);
    msgpack_sbuffer_destroy(&sbuf);
    return;
}

void process_msg(msgpack_object o) {
    msgpack_object_print(stdout, o);
    printf("\n");

    // HANDLE HELO and create a new session
    if (o.type == MSGPACK_OBJECT_STR && ! strcmp(o.via.str.ptr, "HELO")) {
        printf("got: %s\n", o);
        // create new session
        int id_session = create_session();
        printf("created session #: %d\n", id_session);

        msgpack_pack_int(&pk, id_session);
        send(msocket, sbuf.data, sbuf.size, 0 );

    } else if (o.type == MSGPACK_OBJECT_ARRAY) {
        printf("size: %d\n", o.via.array.size);
        printf("obj1: %d\n", (int) o.via.array.ptr[0].via.u64);
        printf("obj2: %s\n", o.via.array.ptr[1]);
    }
}
