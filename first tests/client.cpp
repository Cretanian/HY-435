#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

int main(){

    // Set basic params
    const char *server_ip = "147.52.19.28";
    int listening_port = 4200;

    // Create socket
    int sock;
    if((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("opening TCP listening socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(listening_port);
    inet_aton (server_ip, &sin.sin_addr);

    if(connect(sock, (sockaddr *)&sin, sizeof(sockaddr_in)) < 0){
        std::cout << "Malakia paixthke\n";
        exit(EXIT_FAILURE);
    }

    std::cout << "Connected\n";
}