#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <msgpack.h>

#define PORT 1234
#define MAXCLIENTS 3

void handle(int sock, struct sockaddr_in);

int main(void) {

    int server_fd, msocket;
    struct sockaddr_in address;
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

        if (msocket < 0) printf("ERROR on accept\n");

        int pid = fork();

        if (pid < 0) printf("ERROR on fork\n");

        if (pid == 0)  {
            close(server_fd);
            handle(msocket, address);
            return 0;
        } else close(msocket);
    }

    return 0;
}

void handle(int sock, struct sockaddr_in s) {
    char buffer[1024] = {0};
    char * helo = "ROMAN server v0.01";
    int valread;

    // send helo
    send(sock, helo, strlen(helo), 0 );

    /* msgpack::sbuffer is a simple buffer implementation. */
    msgpack_sbuffer sbuf;
    msgpack_sbuffer_init(&sbuf);

    /* deserialize the buffer into msgpack_object instance. */
    /* deserialized object is valid during the msgpack_zone instance alive. */
    msgpack_zone mempool;
    msgpack_zone_init(&mempool, 1024);

    while (1) {
        valread = read(sock, buffer, 2048);

        msgpack_object deserialized;
        //msgpack_unpack(sbuf.data, sbuf.size, NULL, &mempool, &deserialized);
        msgpack_unpack(buffer, sizeof(buffer), NULL, &mempool, &deserialized);
        msgpack_object_print(stdout, deserialized);

        msgpack_zone_destroy(&mempool);
        msgpack_sbuffer_destroy(&sbuf);

        //if (! strncmp(buffer, "BYE", 3)) return;
        //if (valread == 4) printf("msg from %d %d: %s %d\n", s.sin_addr, s.sin_port, buffer, sizeof(buffer));

        break;
    }
    return;
}
