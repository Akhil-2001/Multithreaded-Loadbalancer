/**
 * Basic Procedure for Socket Programming:
 * Create a socket with the socket() call
 * Bind the socket to an IP addr and a port#
 * Listen for incoming connections
 * accept() connection and send() or receive() data
 **/

// Socket --> IP addr, port#, protocol

/**
 * serverSocket (passive) is used to listen to incoming connection requests
 * accept() fetches the connection requests from the serverSocket
 * accept() creates a new socket (active) and returns a pointer to this socket
 **/

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <linux/in.h>

#define BACKLOG 100
#define SERVERS 3
#define PORT 8010

pthread_mutex_t mutex;
const char message[5000] = "Hello from server\n";
int server_ports[] = {8080, 8081, 8082};
int id = 0;

typedef struct {
    int sock;
    struct sockaddr *address;
    int addr_len;
} conn_t;

void* worker(void* ptr) {
    
    int sock = 0;
    conn_t *conn;
    int port;
    struct sockaddr_in lb_addr;
    char buffer[1024] = {0};
    char response[1024] = {0};

    if(!ptr) {
        perror("Error in pointer");
        exit(EXIT_FAILURE);
    }

    conn = (conn_t*) ptr;
    read(conn->sock , buffer, 30000);
    
    
    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error in socket thread");
        exit(EXIT_FAILURE);
    }

    pthread_mutex_lock(&mutex);
    port = server_ports[id];
    id = (id + 1) % SERVERS;
    pthread_mutex_unlock(&mutex);

    memset(&lb_addr, '0', sizeof(lb_addr));

    lb_addr.sin_family = AF_INET;
    lb_addr.sin_port = htons(port);

    if(inet_pton(AF_INET, "127.0.0.1", &lb_addr.sin_addr) <= 0) {
        perror("Error while translating IP address");
        exit(EXIT_FAILURE);
    }

    if(connect(sock, (struct sockaddr*) &lb_addr, sizeof(lb_addr)) < 0) {
        perror("Error while connecting to server");
        exit(EXIT_FAILURE);
    }

    send(sock, buffer, strlen(buffer), 0);
    printf("Load balancing initiated...\n");
    read(sock, response, 1024);
    printf("Load balancing complete\n");

    write(conn->sock, response, strlen(response));

    close(conn->sock);
    free(conn);
    pthread_exit(0);
}

int main(int argc, char const *argv[]) {

    int server_fd, new_socket;
    struct sockaddr_in address, address_in;
    conn_t *connection;
    pthread_t thread;

    pthread_mutex_init(&mutex, NULL);

    // Creating the socket
    // int server_fd = socket(domain, type, protocol);

    /**
     * Domain -> Communication domain such as IPv4, IPv6, Local Network
     * Type -> Communication semantics such as TCP, UDP
     * Protocol -> To indicate a specific protocol
    **/
    
    /**
     * AF_INET --> IPv4
     * SOCK_STREAM --> Reliable, Sequenced, Two-Way Connection (TCP)
     * 0 --> No protocol variations for the domain and type combination
    **/
   
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Cannot create socket");
        exit(EXIT_FAILURE);
    }

    // Creating the local address structure

    /**
     * sockaddr helps the OS identify the address family
     * sin_family --> Address family which was used in socket creation
     * sin_port --> port#
     * sin_addr --> IP address
    **/

   address.sin_family = AF_INET;
   address.sin_port = htons(PORT);
   address.sin_addr.s_addr = htonl(INADDR_LOOPBACK); //127.0.0.1

    // Binding the address to the socket
    // bind(int socket, struct sockaddr* address, socklen_t address_len)

    if(bind(server_fd, (struct sockaddr*) &address, sizeof(address)) < 0) {
        perror("Unable to bind socket");
        exit(EXIT_FAILURE);
    }

    // Listening on the specified port
    printf("Listening...\n");
    if(listen(server_fd, BACKLOG) < 0) {
        perror("Error while listening");
        exit(EXIT_FAILURE);
    }

    // Accept incoming connections which are queued up
    // Accept copies the incoming socket addr and addr_len to the function parameters passed as pointers
    
    while(1) {

        printf("Ready to accept...\n");
        connection = (conn_t*)malloc(sizeof(conn_t));
        connection->sock = accept(server_fd, (struct sockaddr*) &connection->address, &connection->addr_len);
        if(connection->sock < 0) {
            perror("Error while accepting connection");
            free(connection);
            exit(EXIT_FAILURE);
        }

        pthread_create(&thread, 0, worker, (void*)connection);
        pthread_detach(thread);

    }

    return 0;

}