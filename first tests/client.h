#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <cassert>
#include <arpa/inet.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "Message.h"
#include "Parameters.h"


int Client(Parameters *params){
    // Set basic params
    const char *server_ip = "147.52.19.61";
    int sending_port = 4201;

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

    if(inet_aton (server_ip, &sin.sin_addr) == 0){
        perror("Error converting IP to Network Byte Order");
        exit(EXIT_FAILURE);
    }

    if(connect(sock, (struct sockaddr *)&sin, sizeof(struct sockaddr_in)) < 0){
        perror("Connection Error in socket ");
        exit(EXIT_FAILURE);
    }

    Header *test_header = (Header *)malloc(sizeof(Header));

    int data_len = sizeof(Header);
    int data_sent = send(sock, test_header, data_len, 0);
    std::cout << "Total data sent: " << data_sent << std::endl;
   
    int udp_sock;
    if((udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
        perror("Opening UDP listening socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in udp_sin;
    memset(&udp_sin, 0, sizeof(sockaddr_in));
    udp_sin.sin_family = AF_INET;
    udp_sin.sin_port = htons(4000);
    

    if(inet_aton (server_ip, &udp_sin.sin_addr) == 0){
        perror("Error converting IP to Network Byte Order");
        exit(EXIT_FAILURE);
    }

    void * echoString = (void*)malloc(sizeof(int));
    
    *((int*)echoString) = 5;
    int echoStringLen= sizeof(echoString);

    if (sendto(udp_sock, echoString, echoStringLen, 0, (struct sockaddr *) &udp_sin,sizeof(udp_sin)) 
            != echoStringLen)
        perror("send() sent a different number of bytes than expected");

    std::cout << "Connected\n";
    close(sock);
    close(udp_sock);

    return -1;
}