#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

int main(){

    // Set basic params
    int listening_port = 4200;

    // Create socket
    int sock;
    if((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("opening TCP listening socket");
        exit(EXIT_FAILURE);
    }

    // Bind port?!
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(listening_port);
    sin.sin_addr.s_addr = htonl (INADDR_ANY);
    if(bind(sock, (sockaddr *)&sin, sizeof(sin)) == -1){
        perror("TCP bind");
        exit(EXIT_FAILURE);
    }

    std::cout << "Listening to port: " << listening_port << std::endl;
    listen(sock, 100);
    unsigned int clientLength = sizeof(sockaddr_in);
    accept(sock, (sockaddr *)&sin, &clientLength);

}