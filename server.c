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

#define PORT "9034"   // port we're listening on

struct client
{
    char* ip;
    char* port;
    char username[256];
};

void add_client(struct client* clients, int i, char* ip, char* port, char* username){
                                
    clients[i].ip = ip;
    clients[i].port = port;
    strcpy(clients[i].username, username);
}

struct client* find(struct client* online_clients, char* username , int numofclients){
    int i;
    if(username == NULL){
        return NULL;
    }
    for(i = 0; i < numofclients+2; i++) {
        // printf("%d : %s : %s : %d : %d \n",i,online_clients[i].username,  username, strlen(online_clients[i].username), strlen(username));
        if(strcmp(online_clients[i].username, username) == 0){
            return &online_clients[i];
        }
    }
    return NULL;
}

void remove_client(struct client* clients, int i){
    // clients[i].username = NULL;
    strcpy(clients[i].username, "");
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{
    struct client online_clients[100];
    
    set_heartbeat();
    send_heartbeat(0);

    fd_set master;
    fd_set read_fds;
    int fdmax;

    int listener;
    int newfd;
    struct sockaddr_storage remoteaddr ,client_addr;
    socklen_t addrlen;

    int* client_addr_size;

    char buf[256];
    int nbytes;

    char remoteIP[INET6_ADDRSTRLEN];
    char ip[INET_ADDRSTRLEN];

    int yes=1;
    int i, j, rv;

    struct addrinfo hints, *ai, *p;

    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }
    
    for(p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) { 
            continue;
        }
        
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
    }

    freeaddrinfo(ai);

    if (listen(listener, 10) == -1) {
        perror("listen");
        exit(3);
    }

    FD_SET(listener, &master);

    fdmax = listener;

    while(1) {
        read_fds = master;
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            continue;
        }
        for(i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == listener) {
                    addrlen = sizeof remoteaddr;
                    newfd = accept(listener,
                        (struct sockaddr *)&remoteaddr,
                        &addrlen);
                    if (newfd == -1) {
                        perror("accept");
                    } else {
                        FD_SET(newfd, &master);
                        if (newfd > fdmax) {
                            fdmax = newfd;
                        }
                        printf("selectserver: new connection from %s on "
                            "socket %d\n",
                            inet_ntop(remoteaddr.ss_family,
                                get_in_addr((struct sockaddr*)&remoteaddr),
                                remoteIP, INET6_ADDRSTRLEN),
                            newfd);
                    }
                } else {
                   if ((nbytes = recvfrom(i, buf, sizeof buf, 0, (struct sockaddr*) &client_addr, &addrlen)) <= 0) {
                        if (nbytes == 0) {
                            printf("selectserver: socket %d hung up\n", i);
                            remove_client(online_clients, i);
                        } else {
                            perror("recv");
                        }
                        close(i);
                        FD_CLR(i, &master);
                    } else {
                            buf[nbytes]='\0';
                            char **tokens;
                            size_t numtokens;
                            tokens = strsplit(buf, ", \t\n\x0A\x0D", &numtokens);
                            if(strcmp(tokens[0],"login")==0){
                                char* port = tokens[2];
                                char* username = tokens[1];
                                printf("User registered:");
                                printf("IP: %s\n", remoteIP);
                                printf("PORT: %s\n", port);
                                printf("Username: %s\n", username);
                                add_client(online_clients, i, remoteIP, port , username);
                                if (send(i, "Registerd!\n", 11, 0) == -1) {
                                   perror("send");
                                }
                            }
                            else if(strcmp(tokens[0],"send")==0 || strcmp(tokens[0],"send_file")==0){
                                printf("query: %s\n",tokens[0]);
                                struct client* client =  find(online_clients, tokens[1], fdmax);
                                    
                                if(client==NULL){
                                    if (send(i,"N\n", 2, 0) == -1) {
                                        perror("send");
                                    }
                                }else{
                                    char reciever_port[4] ;
                                    strcpy(reciever_port, client->port);
                                    char reciever_ip[sizeof client->ip];
                                    strcpy(reciever_ip, client->ip);
                                    
                                    printf("%s\n","User Found!");
                                    char data[50];
                                    strcpy(data,  "F: ");
                                    strcat(data,reciever_port);
                                    strcat(data," ");
                                    strcat(data,reciever_ip);
                                    strcat(data,"\n");
                                    if (send(i, data, strlen(data) , 0) == -1) {
                                        perror("send");
                                    }
                                }
                            }
                            else{
                                // if (send(i, buf, nbytes, 0) == -1) {
                                //     perror("send");
                                // }
                            }   
        
                    }
                }
            }
        } 
    }
    return 0;
}