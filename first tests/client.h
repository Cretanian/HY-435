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

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(sockaddr_in));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(4000);
    const char *server_ip = "147.52.19.9";

    unsigned int udp_packet_size = 256;

    uint8_t udp_buffer[udp_packet_size];
    UDP_Header *udp_header = (UDP_Header *)malloc(sizeof(UDP_Header));
    char tmp[udp_packet_size - sizeof(struct UDP_Header)];

     int udpSock;
    if((udpSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
        perror("Opening UDP listening socket");
        exit(EXIT_FAILURE);
    }

    
    if(inet_aton (server_ip, &serverAddr.sin_addr) == 0){
        perror("Error converting IP to Network Byte Order");
        exit(EXIT_FAILURE);
    }

    udp_header->seq_no = htonl(0);
    memcpy( udp_buffer , &udp_header ,sizeof(struct UDP_Header));   
    tmp[0] = 'D'; 
    memcpy( udp_buffer + sizeof(struct UDP_Header), &tmp ,sizeof(tmp));
    if (sendto(udpSock, udp_buffer, udp_packet_size, 0, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) 
                        != udp_packet_size)
                    perror("send() sent a different number of bytes than expected");

   // Terminate program
   exit(signum);
}


void init(Parameters *params,unsigned int parallel_data_streams,unsigned int udp_packet_size){
    if(params->HasKey("-n")){
        parallel_data_streams = stoi(params->GetValue("-n"));
        assert(parallel_data_streams > 0);
    }

    if(params->HasKey("-l")){
        udp_packet_size = stoi(params->GetValue("-l"));
        assert(udp_packet_size > 0);
    }

    return;
}


int Client(Parameters *params){
    // Set basic params
    
    uint8_t buffer[BUFFER_SIZE];
    unsigned int udp_packet_size = 256;
    unsigned int experiment_duration = 3;
    unsigned int parallel_data_streams = 1;
    const char *server_ip = "147.52.19.9";
    uint8_t *data;
    int data_recv = 0;
    int data_sent = 0;

    init(params,parallel_data_streams, udp_packet_size);
    signal(SIGINT, signal_callback_handler);

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
    Header *first_tcp_msg = (Header *)malloc(sizeof(Header));
    first_tcp_msg->message_type = htons(0);
    first_tcp_msg->message_length = htons(2);
    memcpy( buffer , &first_tcp_msg ,sizeof(struct Header)) ; 

    int f_info[2];
    f_info[0] = parallel_data_streams;
    f_info[1] = udp_packet_size;

    memcpy( buffer + sizeof(struct Header), &f_info ,sizeof(f_info));
   
   data_sent = send(sock, buffer, sizeof(struct Header) + sizeof(f_info), 0);
    if (data_sent != sizeof(struct Header) + sizeof(f_info)){
        perror("send() sent a different number of bytes than expected");

        // while(1){
        //    memcpy( buffer, buffer + data_sent ,sizeof(buffer));
        //    data_sent = send(sock, buffer, sizeof(data) - data_sent, 0);
        //      if (data_sent == sizeof(struct Header) + sizeof(data))
        //        break;
        // }
    }
   
   
    std::cout << "Total data sent: " << data_sent << std::endl;
    
    memset(&buffer, 0, sizeof(buffer));
    if ((data_recv = recv(sock, buffer, 1024, 0)) < 0)
        perror("recv() failed");

    Header *data_rec;
    Header recieved_head;
    data_rec = (struct Header *)buffer;
    data = buffer + sizeof(struct Header);


    recieved_head.message_length = ntohs(data_rec->message_length); //??

    int *UDP_port;
    UDP_port = (int *)data;

    std::cout << "SIZE " << recieved_head.message_length << "\n udp_port: " << UDP_port[0] << std::endl ;


    

    // UDP Communication
    int udpSock;
    if((udpSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
        perror("Opening UDP listening socket");
        exit(EXIT_FAILURE);
    }

    
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(sockaddr_in));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(4000);
 

    if(inet_aton (server_ip, &serverAddr.sin_addr) == 0){
        perror("Error converting IP to Network Byte Order");
        exit(EXIT_FAILURE);
    }

    if(params->HasKey("-w"))
        sleep(stoi(params->GetValue("-w")));

    if(params->HasKey("-t"))
        experiment_duration = stoi(params->GetValue("-w"));

    struct timespec my_exec_time;

    int start_timer;
    if(experiment_duration != 0){
        if( clock_gettime( CLOCK_MONOTONIC, &my_exec_time) == -1 ) {
        perror( "getclock" );
        exit( EXIT_FAILURE );
        }
        start_timer = my_exec_time.tv_sec;
    }

    uint8_t udp_buffer[udp_packet_size];
    UDP_Header *udp_header = (UDP_Header *)malloc(sizeof(UDP_Header));
    int seq_no = 30;
    char tmp[udp_packet_size - sizeof(struct UDP_Header)];

    while(1){
        udp_header->seq_no = htonl(seq_no);
        memcpy( udp_buffer , &udp_header ,sizeof(struct UDP_Header));     
        memcpy( udp_buffer + sizeof(struct UDP_Header), &tmp ,sizeof(tmp));

        sleep(1);
        if (sendto(udpSock, udp_buffer, udp_packet_size, 0, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) 
            != udp_packet_size)
        perror("send() sent a different number of bytes than expected");

        if(experiment_duration != 0){
            if( clock_gettime(CLOCK_MONOTONIC_RAW, &my_exec_time) == -1 ) {
                perror( "clock gettime" );
                exit( EXIT_FAILURE );
            }

            int check_break = my_exec_time.tv_sec - start_timer;
            if(check_break >= experiment_duration){
                sleep(2);
                //to do
                udp_header->seq_no = htonl(0);
                memcpy( udp_buffer , &udp_header ,sizeof(struct UDP_Header));   
                tmp[0] = 'D'; 
                memcpy( udp_buffer + sizeof(struct UDP_Header), &tmp ,sizeof(tmp));
                
                if (sendto(udpSock, udp_buffer, udp_packet_size, 0, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) 
                        != udp_packet_size)
                    perror("send() sent a different number of bytes than expected");
                break;
            }
        }

        seq_no++;
    }

    udp_header->seq_no = htonl(0);
    memcpy( udp_buffer , &udp_header ,sizeof(struct UDP_Header));   
    tmp[0] = 'D'; 
    memcpy( udp_buffer + sizeof(struct UDP_Header), &tmp ,sizeof(tmp));
    if (sendto(udpSock, udp_buffer, udp_packet_size, 0, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) 
                        != udp_packet_size)
                    perror("send() sent a different number of bytes than expected");

    std::cout << "Connected\n";

    
    close(sock);
    close(udpSock);
    return -1;
}