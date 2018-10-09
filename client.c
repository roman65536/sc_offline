//client
#include <sys/socket.h>
#include <msgpack.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <fcntl.h>

#define PORT 1234

msgpack_sbuffer sbuf; /* buffer */
msgpack_packer pk;    /* packer */
msgpack_zone mempool;

int id_session, sock;

void send_helo();
void create_sheet();
int valread;
char buffer[1024] = {0};

int main() {

    struct sockaddr_in address;
    struct sockaddr_in serv_addr;

    sock = 0;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket creation error \n");
        return -1;
    }

    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) {
        printf("Invalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Connection Failed \n");
        return -1;
    }

    /* Put the socket in non-blocking mode:
    if(fcntl(sock, F_SETFL, fcntl(sock, F_GETFL) | O_NONBLOCK) < 0) {
        printf("cannot set socket in non blocking mode");
        return -1;
    }*/

    valread = read(sock, buffer, 1024);
     if (valread != -1) {
         printf("got server inf.: %s\n", buffer);
         buffer[0]='\0';
    } else
         printf("valread :%d\n", valread);


    msgpack_sbuffer_init(&sbuf); /* initialize buffer */
    msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write); /* initialize packer */

    /* deserialize the buffer into msgpack_object instance. */
    /* deserialized object is valid during the msgpack_zone instance alive. */
    msgpack_zone_init(&mempool, 1024);

    send_helo();

    create_sheet();

    msgpack_sbuffer_destroy(&sbuf);

    return 0;
}


void send_helo() {
    // pack HELO
    msgpack_pack_str(&pk, 4);
    msgpack_pack_str_body(&pk, "HELO", 4); // this requests a new session

    //msgpack_pack_array(&pk, 2);
    //msgpack_pack_int(&pk, 16);
    //msgpack_pack_str(&pk, 4);
    //msgpack_pack_str_body(&pk, "HELO", 4);

    // send HELO
    send(sock, sbuf.data, sbuf.size, 0 );
    msgpack_sbuffer_clear(&sbuf);

    // get session ID
    valread = read(sock, buffer, 1024);
    if (valread > 0) {
        msgpack_object o;

        //FIXME sizeof(..) may differ on sender and receiver machine
        msgpack_unpack(buffer, sizeof(buffer), NULL, &mempool, &o);

        //msgpack_object_print(stdout, o);

        if (o.type == MSGPACK_OBJECT_MAP && o.via.map.size == 1 && ! strncmp(o.via.map.ptr->key.via.str.ptr, "id", 2)) {
            id_session = (int) o.via.map.ptr->val.via.u64;
            printf("got session #: %d.\n", id_session);
        }
    }
}

void create_sheet() {
    //msgpack_sbuffer_clear(&sbuf);
    msgpack_pack_map(&pk, 3);
    msgpack_pack_str(&pk, 2);
    msgpack_pack_str_body(&pk, "id", 2);
    msgpack_pack_int(&pk, id_session);
    msgpack_pack_str(&pk, 6);
    msgpack_pack_str_body(&pk, "method", 6);
    msgpack_pack_str(&pk, 12);
    msgpack_pack_str_body(&pk, "create_sheet", 12);
    msgpack_pack_str(&pk, 6);
    msgpack_pack_str_body(&pk, "params", 6);
    msgpack_pack_map(&pk, 1);
    msgpack_pack_str(&pk, 4);
    msgpack_pack_str_body(&pk, "name", 4);
    msgpack_pack_str(&pk, 6);
    msgpack_pack_str_body(&pk, "sheet1", 6);

    send(sock, sbuf.data, sbuf.size, 0 );
    //msgpack_sbuffer_clear(&sbuf);

}