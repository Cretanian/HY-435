#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <cassert>
#include <arpa/inet.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

int main(){

    // Set basic params
    const char *server_ip = "147.52.19.28";
    int sending_port = 4200;

    // Create socket
    int sock;
    if((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1){
        perror("Opening TCP listening socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(sending_port);
    assert(inet_aton (server_ip, &sin.sin_addr) != 0);

    if(inet_aton (server_ip, &sin.sin_addr) != 0){
        perror("Error converting IP to Network Byte Order");
        exit(EXIT_FAILURE);
    }


    if(connect(sock, (struct sockaddr *)&sin, sizeof(struct sockaddr_in)) < 0){
        perror("Connection Error in socket ");
        exit(EXIT_FAILURE);
    }

    int data_sent = send(sock,0);

    int data_recieved = recv(sock,0); 
    
    std::cout << "Connected\n";

    close(sock);
}