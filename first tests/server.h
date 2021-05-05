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

extern int listening_port;

int Server(Parameters *params){

    // Set basic params
    uint32_t bind_ip = INADDR_ANY;
    
    // // Creating socket
    // int sock;
    // if((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1){
    //     perror("Opening TCP listening socket");
    //     exit(EXIT_FAILURE);
    // }

    // // Binding port with a specific interface (IPv4)
    // struct sockaddr_in sin;
    // memset(&sin, 0, sizeof(sockaddr_in));
    // sin.sin_family = AF_INET;

    // // Set listening port 
    // if(params->HasKey("-p"))
    //         listening_port = stoi(params->GetValue("-p"));
    // sin.sin_port = htons(listening_port);

    // // Set sin.sin_addr
    // if(params->HasKey("-a")){
    //     if(inet_aton(params->GetValue("-a").c_str(), &sin.sin_addr) == 0){
    //         perror("Error converting IP to Network Byte Order");
    //         exit(EXIT_FAILURE);
    //     }
    // }
    // else
    //     sin.sin_addr.s_addr = htonl (INADDR_ANY);

    // if(bind(sock, (struct sockaddr *)&sin, sizeof(sin)) == -1){
    //     perror("Erron in TCP bind");
    //     exit(EXIT_FAILURE);
    // }

    // //Setting port listening mode
    // std::cout << "Listening to port: " << listening_port << std::endl;
    // if (listen(sock, 10) == -1){
    //     perror("Error when marking the socket in listening mode");
    //     exit(EXIT_FAILURE);
    // }

    // //Accepting incoming connections
    // struct sockaddr_in client_info;
    // memset(&client_info, 0, sizeof(sockaddr_in));
    // unsigned int clientLength = sizeof(sockaddr_in);

    // int new_returned_client_socket = accept(sock, (struct sockaddr *)&client_info, &clientLength);
    
    // if (new_returned_client_socket < 0){
    //     perror("Opening new TCP listening socket");
    //     exit(EXIT_FAILURE);
    // }

    // // TCP Communication
    // Header *test_header = (Header *)malloc(sizeof(Header));
    // int data_length = sizeof(Header);
    // int data_recv = recv(new_returned_client_socket, (void *)test_header, data_length, 0);
    // close(new_returned_client_socket);
    // close(sock);

    // std::cout << "TCP Done\n";

    // UDP Communication
    int udpSock;
    char buffer[500];
    const char *hello = "hello from server";
    struct sockaddr_in serverAddr, clientAddr;

    if((udpSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){
        perror("Socket indentifier failed");
        exit(EXIT_FAILURE);
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    memset(&clientAddr, 0, sizeof(clientAddr));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(3000);

    if(bind(udpSock, (const struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1){
        perror("Bind failed...");
        exit(EXIT_FAILURE);
    }
    unsigned int len, n;
    len = sizeof(clientAddr);

    n = recvfrom(udpSock, (char *)buffer, 500, 0, (struct sockaddr *)&clientAddr, &len);
    std::cout << "Total received: " << n << std::endl;
    std::cout << "Received message: " << buffer << "\n";

}