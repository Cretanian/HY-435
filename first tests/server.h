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

    unsigned int udp_packet_size = 1;
    unsigned int parallel_data_streams = 1;

    // Set basic params
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
    
    const char *client_ip = inet_ntoa(client_info.sin_addr);
    std::cout << "Client IP: " << client_ip << std::endl;

    if (new_returned_client_socket < 0){
        perror("Opening new TCP listening socket");
        exit(EXIT_FAILURE);
    }

    // TCP Communication
    Header *test_header = (Header *)malloc(sizeof(Header));
    int data_length = sizeof(Header);
    int data_recv = 0;
    if ((data_recv= recv(new_returned_client_socket, (void *)test_header, data_length, 0)) < 0)
        perror("recv() failed");

    udp_packet_size = ntohs(test_header->message_length);
    parallel_data_streams = ntohs(test_header->num_parallel_streams);
    


    std::cout << "UDP len:" << udp_packet_size << "\nParalle streams: " << parallel_data_streams << std::endl ;

    test_header->message_type = htons(4000);
    if (send(new_returned_client_socket, (void *)test_header, data_length, 0) != data_length)
        perror("send() failed");


    // UDP Communication
    int udpSock;
    if((udpSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
        perror("Opening UDP listening socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in clientAddr_tmp,clientAddr;
    memset(&clientAddr, 0, sizeof(sockaddr_in));
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_port = htons(4000);
    clientAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    void * echoString = (void*)malloc(udp_packet_size);

    if(bind(udpSock, (struct sockaddr *)&clientAddr, sizeof(clientAddr)) < 0){
        perror("Bind failed...");
        exit(EXIT_FAILURE);
    }
    unsigned int len, n;
    len = sizeof(clientAddr);

  
    UDP_Message *udp_message = (UDP_Message *)malloc(sizeof(UDP_Message));
    udp_message->buffer = (void*)malloc(udp_packet_size - 4);

    int recvMsgSize;
    while(1){
        clientAddr_tmp = clientAddr;
        if ((recvMsgSize= recvfrom(udpSock, udp_message, udp_packet_size, 0, (struct sockaddr *) &clientAddr, &len)) < 0)
            perror("recvfrom() failed");
        std::cout << recvMsgSize  << std::endl;
        if(ntohs(udp_message->SEQ_NO) == 1)
            break;
        clientAddr = clientAddr_tmp;
    }
    std::cout << ntohs(udp_message->SEQ_NO)  << std::endl;

  //std::cout << (char*)udp_message->buffer  << std::endl;

    std::cout << "Connected\n";
    close(sock);
    close(new_returned_client_socket);
    close(udpSock);
     return -1;
  
}