#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "./heartbeat.h"
#include "./utils.h"


#define M_PORT "3000"
#define B_PORT "4951"
#define H_PORT "49500"
#define Tracker_PORT "9034"

#define MAXBUFLEN 100

int server_exists;

void check_for_server(int signum)
{
   // if(server_exists==1){
   //  printf("%s\n","WELL!");
   // }else{
   //  printf("%s\n","BAD!");
   // }
   signal(SIGALRM, check_for_server);
   alarm(2);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void set_listener(struct addrinfo hints, struct addrinfo* ai, struct addrinfo* p, char* port, int* listener, int* rv, int* fdmax, fd_set* master, int mode){
    int yes=1;        // for setsockopt() SO_REUSEADDR, below
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    if(mode==1)
        hints.ai_socktype = SOCK_STREAM;
    else
        hints.ai_socktype = SOCK_DGRAM;

    hints.ai_flags = AI_PASSIVE;
    if ((*rv = getaddrinfo(NULL, port, &hints, &ai)) != 0) {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(*rv));
        exit(1);
    }

    for(p = ai; p != NULL; p = p->ai_next) {
        *listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (*listener < 0) {
            continue;
        }
       
       setsockopt(*listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

       if (bind(*listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(*listener);
            continue;
        }
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
    }
    FD_SET(*listener, master);
    *fdmax = *listener;
    freeaddrinfo(ai);
    
}

void connect_to_server(struct sockaddr_in server, int* sock){
    *sock = socket(AF_INET , SOCK_STREAM , 0);
    if (*sock == -1){
        printf("Could not create socket");
    }
    puts("Socket created");
     
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons( 9034 );
 
    //Connect to remote server
    if (connect(*sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("connect failed. Error");
        exit(1);
    }
    puts("Connected\n");
}


void send_data(char* message, int sock){
    char server_reply[2000];
     
    if( send(sock , message , strlen(message) , 0) < 0){
            puts("Send failed");
            exit(1);
        }
         
        if( recv(sock , server_reply , 2000 , 0) < 0)
        {
            puts("recv failed");
        }
        // printf("%d\n",strlen(server_reply));
        puts(server_reply);
}


int main(){
    fd_set master;   
    fd_set read_fds;
    int fdmax;  

    int M_listener, H_listener, B_listener;
    int newfd;        
    struct sockaddr_storage remoteaddr ,client_addr; 
    socklen_t addrlen;
    int* client_addr_size;
    int H_numbytes;
    char server_ip[INET6_ADDRSTRLEN];
    // char buf[256];    // buffer for client data
    int nbytes;

    char remoteIP[INET6_ADDRSTRLEN];
    char ip[INET_ADDRSTRLEN];

    int yes=1;
    int i, j, rv;

    struct addrinfo M_hints, *M_ai, *M_p;
    struct addrinfo T_hints, *T_ai, *T_p;
    struct addrinfo B_hints, *B_ai, *B_p;

    struct sockaddr_storage server_addr;
    char buf[MAXBUFLEN];
    socklen_t server_addr_len;

    int server_socket;
    struct sockaddr_in server;
    connect_to_server(server, &server_socket);

    set_listener(M_hints, M_ai, M_p, M_PORT, &M_listener, &rv, &fdmax, &master, 1);

    // if (listen(M_listener, 10) == -1) {
    //     perror("listen");
    //     exit(3);
    // }

    set_listener(T_hints, T_ai, T_p, H_PORT, &H_listener, &rv, &fdmax, &master, 0);
    set_listener(B_hints, B_ai, B_p, B_PORT, &B_listener, &rv, &fdmax, &master, 0);

    check_for_server(1);

    while(1) {
        read_fds = master;
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            server_exists=0;
            continue;
        }
        int n = read(0, buf, 20);
        send_data(buf,server_socket);
        for(i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) {
                if(i==0){
                }
                if (i == H_listener) {
                    server_addr_len = sizeof server_addr;
                    if ((H_numbytes = recvfrom(H_listener, buf, MAXBUFLEN-1 , 0,
                        (struct sockaddr *)&server_addr, &server_addr_len)) == -1) {
                        perror("recvfrom");
                        exit(1);
                        server_exists=0;
                    }else{
                        buf[H_numbytes] = '\0';
                        // printf("Heartbeat: \"%s\"\n", buf);
                        server_exists=1;
                    }
                }else{
                }
            }
        } 
    }
}