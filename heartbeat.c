#include "heartbeat.h"


struct sockaddr_in their_addr;
int sockfd;

#define PORT 49500

void set_heartbeat(){
    struct hostent *he;
    int numbytes;
    int broadcast = 1;
    //char broadcast = '1'; // if that doesn't work, try this

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    // this call is what allows broadcast packets to be sent:
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast,
        sizeof broadcast) == -1) {
        perror("setsockopt (SO_BROADCAST)");
        exit(1);
    }

    if ((he=gethostbyname("255.255.255.255")) == NULL) {  // get the host info
        perror("gethostbyname");
        exit(1);
    }

    their_addr.sin_family = AF_INET;     // host byte order
    their_addr.sin_port = htons(PORT); // short, network byte order
    their_addr.sin_addr = *((struct in_addr *)he->h_addr);
    printf("Broadcasting Heartbeat on PORT: %d.\n", PORT);
    memset(their_addr.sin_zero, '\0', sizeof their_addr.sin_zero);

}

void send_heartbeat(int signum)
{
    if ((sendto(sockfd, "49500", strlen("49500"), 0,
             (struct sockaddr *)&their_addr, sizeof their_addr)) == -1) {
        perror("sendto");
        exit(1);
    }

    // printf("sent %d bytes to %s\n", numbytes,
    //     inet_ntoa(their_addr.sin_addr));

    // close(sockfd);

   signal(SIGALRM, send_heartbeat);
   alarm(1);
}

// int main(){
//     set_heartbeat();
//     send_heartbeat(0);
//     while(1);

// }