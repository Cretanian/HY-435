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

extern int listening_port;

void signal_callback_handler(int signum) {
   std::cout << "Caught signal. Terminating... " << std::endl;

   // Terminate program
   exit(signum);
}


int Client(Parameters *params){
    // Set basic params
    unsigned int udp_packet_size = 100;
    unsigned int experiment_duration = 10;
    unsigned int parallel_data_streams = 1;
    const char *server_ip = "147.52.19.9";

    if(params->HasKey("-n")){
        parallel_data_streams = stoi(params->GetValue("-n"));
        assert(parallel_data_streams > 0);
    }

    if(params->HasKey("-l")){
        udp_packet_size = stoi(params->GetValue("-l"));
        assert(udp_packet_size > 0);
    }


    // Create socket
    int sock;
    if((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1){
        perror("Opening TCP listening socket");
    }

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sockaddr_in));
    sin.sin_family = AF_INET;

    
    // Set listening port 
    if(params->HasKey("-p")){
        listening_port = stoi(params->GetValue("-p"));
        sin.sin_port = htons(listening_port);
    }else
        //assert(false); //TO CHECK

    // Set sin.sin_addr
    if(params->HasKey("-a")){
        if(inet_aton(params->GetValue("-a").c_str(), &sin.sin_addr) == 0){
            perror("Error converting IP to Network Byte Order");
            exit(EXIT_FAILURE);
        }
    }
    else
        //assert(false); //TO CHECK
    
   

    //to be removed
    {
        sin.sin_port = htons(listening_port);
        if(inet_aton (server_ip, &sin.sin_addr) == 0){
            perror("Error converting IP to Network Byte Order");
            exit(EXIT_FAILURE);
        }
    }

    if(connect(sock, (struct sockaddr *)&sin, sizeof(struct sockaddr_in)) < 0){
        perror("Connection Error in socket ");
        exit(EXIT_FAILURE);
    }



    // TCP Communication
    Header *test_header = (Header *)malloc(sizeof(Header));
    test_header->message_length = htons(udp_packet_size);
    test_header->message_type = htons(0);
    test_header->num_parallel_streams = htons(parallel_data_streams);

    int data_len = sizeof(Header);
    int data_sent = send(sock, test_header, data_len, 0);
    if (data_sent != data_len)
        perror("send() sent a different number of bytes than expected");

    std::cout << "Total data sent: " << data_sent << std::endl;
    int data_recv = 0;
    if ((data_recv = recv(sock, (void *)test_header, data_len, 0)) < 0)
        perror("recv() failed");

    int UDP_port = ntohs(test_header->message_type);
    std::cout << "UDP port " << UDP_port << std::endl;
  

    // UDP Communication
    int udpSock;
    if((udpSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
        perror("Opening UDP listening socket");
        exit(EXIT_FAILURE);
    }

    
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(sockaddr_in));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(UDP_port);
 

    if(inet_aton (server_ip, &serverAddr.sin_addr) == 0){
        perror("Error converting IP to Network Byte Order");
        exit(EXIT_FAILURE);
    }

    if(params->HasKey("-w"))
        sleep(stoi(params->GetValue("-w")));

    if(params->HasKey("-t"))
        experiment_duration = stoi(params->GetValue("-w"));


    UDP_Message *udp_message = (UDP_Message *)malloc(sizeof(UDP_Message));
    udp_message->buffer = (void*)malloc(udp_packet_size - 4);
    udp_message->SEQ_NO = htons(0);

 
    char const *text = "hello world\n";
    udp_message->buffer = (void *)text;
    signal(SIGINT, signal_callback_handler);

    struct timespec my_exec_time;

    int start_timer;
    if(experiment_duration != 0){
        if( clock_gettime( CLOCK_MONOTONIC, &my_exec_time) == -1 ) {
        perror( "getclock" );
        exit( EXIT_FAILURE );
        }
        start_timer = my_exec_time.tv_sec;
    }



    while(1){
        if (sendto(udpSock, udp_message, udp_packet_size, 0, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) 
            != udp_packet_size)
        perror("send() sent a different number of bytes than expected");

        if(experiment_duration != 0){
            if( clock_gettime(CLOCK_MONOTONIC_RAW, &my_exec_time) == -1 ) {
                perror( "clock gettime" );
                exit( EXIT_FAILURE );
            }

            int check_break = my_exec_time.tv_sec - start_timer;
            if(check_break >= experiment_duration){
                sleep(1);
                udp_message->SEQ_NO = htons(1);
                if (sendto(udpSock, udp_message, udp_packet_size, 0, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) 
                        != udp_packet_size)
                    perror("send() sent a different number of bytes than expected");
                break;
            }
        }
    }

    udp_message->SEQ_NO = htons(1);
    if (sendto(udpSock, udp_message, udp_packet_size, 0, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) 
            != udp_packet_size)
        perror("send() sent a different number of bytes than expected");

    std::cout << "Connected\n";

    
    close(sock);
    close(udpSock);
    return -1;
}