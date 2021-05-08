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

    uint8_t buffer[BUFFER_SIZE];
    unsigned int udp_packet_size = 256;
    unsigned int parallel_data_streams = 1;
    unsigned int udp_port = 4000;
    uint8_t *data;
    int data_recv = 0;
    int data_sent = 0;

   

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
    Header *first_tcp_msg ;
    Header recieved_head;
    
    if ((data_recv = recv(new_returned_client_socket, buffer, 1024, 0)) < 0)
        perror("recv() failed");

    std::cout << "DATA rec:" << data_recv << "\n";

    first_tcp_msg = (struct Header *)buffer;
    data = buffer + sizeof(struct Header);
    
    int *a;
    a = (int *)data;

    

    recieved_head.message_length = ntohs(first_tcp_msg->message_length);  //??
       
    std::cout << "UDP len:" << recieved_head.message_length << "\nParalle streams: " << a[0]<< "\nudp_pac_size " << a[1] << std::endl ;


    uint8_t udp_buffer[a[1]];
    uint8_t* udp_data;

    memset(&buffer, 0, sizeof(buffer));
    first_tcp_msg = (Header *)malloc(sizeof(Header));
    first_tcp_msg->message_type = htons(0);
    first_tcp_msg->message_length = htons(1);
    memcpy( buffer , &first_tcp_msg ,sizeof(struct Header)) ;
    memcpy( buffer + sizeof(struct Header), &udp_port ,sizeof(udp_port));
   
    data_sent = send(new_returned_client_socket, buffer, sizeof(struct Header) + sizeof(udp_port), 0);
    if (data_sent != sizeof(struct Header) + sizeof(udp_port))
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

    

    if(bind(udpSock, (struct sockaddr *)&clientAddr, sizeof(clientAddr)) < 0){
        perror("Bind failed...");
        exit(EXIT_FAILURE);
    }
    unsigned int len, n;
    len = sizeof(clientAddr);

  
    UDP_Header *udp_header;
    UDP_Header test;
    

    int recvMsgSize;
    while(1){
        clientAddr_tmp = clientAddr;
        if ((recvMsgSize= recvfrom(udpSock, udp_buffer, sizeof(udp_buffer), 0, (struct sockaddr *) &clientAddr, &len)) < 0)
            perror("recvfrom() failed");

        udp_header = (struct UDP_Header *)udp_buffer;
        udp_data = udp_buffer + sizeof(struct UDP_Header);
        
        char *are;
        are = (char *)udp_data;

        std::cout << "Data res " <<recvMsgSize  << " seq_no " << ntohs(udp_header->seq_no) <<std::endl;  //??
        
        std::cout << "msg " << *are   <<std::endl; 

        if(*are == 'D')
            break;
       clientAddr = clientAddr_tmp;
    }


    std::cout << "Connected\n";
    close(sock);
    close(new_returned_client_socket);
    close(udpSock);
     return -1;
  
}