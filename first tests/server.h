#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>

#include "Parameters.h"
#include "Message.h"

int Server(Parameters *params){

    // Set basic params
    int listening_port = 4201;
    uint32_t bind_ip = INADDR_ANY;
    
    // Creating socket
    int sock;
    if((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1){
        perror("Opening TCP listening socket");
        exit(EXIT_FAILURE);
    }

    // Binding port with a specific interface (IPv4)
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sockaddr_in));
    sin.sin_family = AF_INET;

    // Set listening port 
    if(params->HasKey("-p"))
            listening_port = stoi(params->GetValue("-p"));
    sin.sin_port = htons(listening_port);

    // Set sin.sin_addr
    if(params->HasKey("-a")){
        if(inet_aton(params->GetValue("-a").c_str(), &sin.sin_addr) == 0){
            perror("Error converting IP to Network Byte Order");
            exit(EXIT_FAILURE);
        }
    }
    else
        sin.sin_addr.s_addr = htonl (INADDR_ANY);

    if(bind(sock, (struct sockaddr *)&sin, sizeof(sin)) == -1){
        perror("Erron in TCP bind");
        exit(EXIT_FAILURE);
    }

    //Setting port listening mode
    std::cout << "Listening to port: " << listening_port << std::endl;
    if (listen(sock, 10) == -1){
        perror("Error when marking the socket in listening mode");
        exit(EXIT_FAILURE);
    }

    //Accepting incoming connections
    struct sockaddr_in client_info;
    memset(&client_info, 0, sizeof(sockaddr_in));
    unsigned int clientLength = sizeof(sockaddr_in);

    int new_returned_client_socket = accept(sock, (struct sockaddr *)&client_info, &clientLength);
    
    if (new_returned_client_socket < 0){
        perror("Opening new TCP listening socket");
        exit(EXIT_FAILURE);
    }

    Header *test_header = (Header *)malloc(sizeof(Header));
    int data_length = sizeof(Header);
    int data_recv = recv(new_returned_client_socket, (void *)test_header, data_length, 0);

   

 

    int udp_sock;
    if((udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
        perror("Opening UDP listening socket");
        exit(EXIT_FAILURE);
    }
    
    

    
    struct sockaddr_in udp_sin;
    memset(&udp_sin, 0, sizeof(sockaddr_in));
    udp_sin.sin_family = AF_INET;
    udp_sin.sin_port = htons(4000);
    udp_sin.sin_addr.s_addr = htonl (INADDR_ANY);
    
    if (bind(udp_sock, (struct sockaddr *) &udp_sin, sizeof(udp_sin)) < 0)
        perror("bind() failed");


    void* echoBuffer =  (void*)malloc(sizeof(int));
    unsigned int ar = sizeof(udp_sin);
   
    int recvMsgSize;
    if ((recvMsgSize= recvfrom(udp_sock, echoBuffer, 50, 0, (struct sockaddr *) &udp_sin, &ar)) < 0)
        perror("recvfrom() failed");

    std::cout << *((int*)echoBuffer)  << std::endl;

    std::cout << "Connected\n";
    close(sock);
    close(udp_sock);

     return -1;
}