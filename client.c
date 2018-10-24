//client
#include <sys/socket.h>
#include <msgpack.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>   /* for read */

#define PORT 1234

msgpack_sbuffer sbuf; /* buffer */
msgpack_packer pk;    /* packer */
msgpack_zone mempool;

int id_session, sock;

void send_helo();
void create_sheet();
void remove_sheet();
int get_val(int row, int col, float * res);
int set_val(int row, int col, float val);
int bye();

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

    printf("sending helo\n");
    send_helo();

    printf("creating sheet\n");
    create_sheet();

    printf("\nsetting value\n");
    set_val(0, 0, 12.4);

    //sleep(5);
    float f;
    int res = get_val(0, 0, &f);
    if (res == 0) printf("\ngetting value: %f.\n", f);

    printf("deleting sheet\n");
    remove_sheet();

    printf("\nsaying bye\n");
    bye();

    msgpack_zone_destroy(&mempool);
    msgpack_sbuffer_destroy(&sbuf);

    return 0;
}

int bye() {
    msgpack_pack_map(&pk, 2);

    msgpack_pack_str(&pk, 2);
    msgpack_pack_str_body(&pk, "id", 2);
    msgpack_pack_short(&pk, id_session);

    msgpack_pack_str(&pk, 6);
    msgpack_pack_str_body(&pk, "method", 6);
    msgpack_pack_str(&pk, 3);
    msgpack_pack_str_body(&pk, "bye", 3);

    send(sock, sbuf.data, sbuf.size, 0 );
    msgpack_sbuffer_clear(&sbuf);

    //get OK return status
    valread = read(sock, buffer, 1024);
    if (valread > 0) {
        msgpack_object p;

        //FIXME sizeof(..) may differ on sender and receiver machine
        msgpack_unpack(buffer, sizeof(buffer), NULL, &mempool, &p);

        msgpack_object_print(stdout, p);

        // return 0
        if ( p.type == MSGPACK_OBJECT_MAP && p.via.map.size == 2 &&
           ! strncmp(p.via.map.ptr->key.via.str.ptr, "ret", 3) &&
           ((int) p.via.map.ptr->val.via.u64 == 0)) {
            return 0;
        }
    } else if (valread < 0) {
        printf("nothing to read in socket\n");
        return -2;
    }
    return -1;
}

// TODO should return 0 when 0 return status is received from server
int set_val(int row, int col, float val) {
    msgpack_pack_map(&pk, 3);

    msgpack_pack_str(&pk, 2);
    msgpack_pack_str_body(&pk, "id", 2);
    msgpack_pack_short(&pk, id_session);

    msgpack_pack_str(&pk, 6);
    msgpack_pack_str_body(&pk, "method", 6);
    msgpack_pack_str(&pk, 7);
    msgpack_pack_str_body(&pk, "set_val", 7);

    msgpack_pack_str(&pk, 6);
    msgpack_pack_str_body(&pk, "params", 6);
    msgpack_pack_map(&pk, 3);

    msgpack_pack_str(&pk, 3);
    msgpack_pack_str_body(&pk, "row", 3);
    msgpack_pack_int(&pk, row);

    msgpack_pack_str(&pk, 3);
    msgpack_pack_str_body(&pk, "col", 3);
    msgpack_pack_int(&pk, col);

    msgpack_pack_str(&pk, 3);
    msgpack_pack_str_body(&pk, "val", 3);
    msgpack_pack_float(&pk, val);
    send(sock, sbuf.data, sbuf.size, 0 );
    msgpack_sbuffer_clear(&sbuf);

    //get OK return status
    valread = read(sock, buffer, 1024);
    if (valread > 0) {
        msgpack_object p;

        //FIXME sizeof(..) may differ on sender and receiver machine
        msgpack_unpack(buffer, sizeof(buffer), NULL, &mempool, &p);

        msgpack_object_print(stdout, p);

        // return 0
        if ( p.type == MSGPACK_OBJECT_MAP && p.via.map.size == 2 &&
           ! strncmp(p.via.map.ptr->key.via.str.ptr, "ret", 3) &&
           ((int) p.via.map.ptr->val.via.u64 == 0)) {
            return 0;
        }
    } else if (valread < 0) {
        printf("nothing to read in socket\n");
        return -2;
    }
    return -1;
}

// TODO return 0 when ok. -1 when not found/error
int get_val(int row, int col, float * res) {
    int found=0;

    msgpack_pack_map(&pk, 3);

    msgpack_pack_str(&pk, 2);
    msgpack_pack_str_body(&pk, "id", 2);
    msgpack_pack_short(&pk, id_session);

    msgpack_pack_str(&pk, 6);
    msgpack_pack_str_body(&pk, "method", 6);
    msgpack_pack_str(&pk, 7);
    msgpack_pack_str_body(&pk, "get_val", 7);

    msgpack_pack_str(&pk, 6);
    msgpack_pack_str_body(&pk, "params", 6);
    msgpack_pack_map(&pk, 2);

    msgpack_pack_str(&pk, 3);
    msgpack_pack_str_body(&pk, "row", 3);
    msgpack_pack_int(&pk, row);

    msgpack_pack_str(&pk, 3);
    msgpack_pack_str_body(&pk, "col", 3);
    msgpack_pack_int(&pk, col);

    send(sock, sbuf.data, sbuf.size, 0 );
    msgpack_sbuffer_clear(&sbuf);

    msgpack_unpacker unp;
    bool result = msgpack_unpacker_init(&unp, 1024);
    if (result) printf("\nok init\n");

    // get value
    valread = read(sock, buffer, 1024);
    if (valread > 0) {
        memcpy(msgpack_unpacker_buffer(&unp), buffer, 1024);
        msgpack_unpacker_buffer_consumed(&unp, 1024);
        msgpack_unpacked und;
        msgpack_unpack_return ret;
        msgpack_unpacked_init(&und);
        ret = msgpack_unpacker_next(&unp, &und);
        if (ret == MSGPACK_UNPACK_SUCCESS) printf("ok unpacked\n");
        msgpack_object q = und.data;

        msgpack_object_print(stdout, q);

        if (
           q.type == MSGPACK_OBJECT_MAP && q.via.map.size == 2 &&
           ! strncmp(q.via.map.ptr->key.via.str.ptr, "ret", 3) &&
           ((int) q.via.map.ptr->val.via.u64 == 0) &&
           ! strncmp(((q.via.map.ptr)+1)->key.via.str.ptr, "val", 3)
           ) {
            *res = ((q.via.map.ptr)+1)->val.via.f64;
            found = 1;
        }
        msgpack_unpacked_destroy(&und);
    }
    msgpack_unpacker_destroy(&unp);
    return found ? 0 : -1;
}

// TODO return 0 when ok. -1 when not found/error
void send_helo() {
    // pack HELO
    msgpack_pack_str(&pk, 4);
    msgpack_pack_str_body(&pk, "HELO", 4); // this requests a new session

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

// TODO return 0 when ok. -1 when not found/error
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
    msgpack_sbuffer_clear(&sbuf);

    // get OK
    valread = read(sock, buffer, 1024);
    if (valread > 0) {
        msgpack_object o;
        msgpack_unpack(buffer, sizeof(buffer), NULL, &mempool, &o);
        msgpack_object_print(stdout, o);
    }
}

// TODO return 0 when ok. -1 when not found/error
void remove_sheet() {
    msgpack_pack_map(&pk, 3);
    msgpack_pack_str(&pk, 2);
    msgpack_pack_str_body(&pk, "id", 2);
    msgpack_pack_int(&pk, id_session);
    msgpack_pack_str(&pk, 6);
    msgpack_pack_str_body(&pk, "method", 6);
    msgpack_pack_str(&pk, 12);
    msgpack_pack_str_body(&pk, "delete_sheet", 12);
    msgpack_pack_str(&pk, 6);
    msgpack_pack_str_body(&pk, "params", 6);
    msgpack_pack_map(&pk, 1);
    msgpack_pack_str(&pk, 4);
    msgpack_pack_str_body(&pk, "name", 4);
    msgpack_pack_str(&pk, 6);
    msgpack_pack_str_body(&pk, "sheet1", 6);

    send(sock, sbuf.data, sbuf.size, 0 );
    msgpack_sbuffer_clear(&sbuf);

    // get OK
    valread = read(sock, buffer, 1024);
    if (valread > 0) {
        msgpack_object o;
        msgpack_unpack(buffer, sizeof(buffer), NULL, &mempool, &o);
        msgpack_object_print(stdout, o);
    }
    return;
}
