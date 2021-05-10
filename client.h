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
#include "SocketWrapper.h"

extern int listening_port;
extern SocketWrapper *tcpwrapper;
extern SocketWrapper *udpwrapper;

void GetTime(struct timespec *my_exec_time);

void signal_callback_handler(int signum) {
    std::cout << "\nCaught signal. Terminating... " << std::endl;

    sleep(3);
    Header *header = (Header *)malloc(sizeof(Header));
    header->message_type = htons(0);
    header->message_length = htons(0);
    int *test = (int *)malloc(sizeof(int));
    *test = htons(2);
    tcpwrapper->Send(header, test, sizeof(int));

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
    int sleep_before_tran = 0;

    // Init buffer
    for(int i = 0; i < BUFFER_SIZE; i++){
        buffer[i] = 'a';
    }

    init(params,parallel_data_streams, udp_packet_size);
    signal(SIGINT, signal_callback_handler);

    // TCP Communication
    tcpwrapper = new SocketWrapper(TCP);
    tcpwrapper->Connect(server_ip, listening_port);
    
    Header *tcp_header = (Header *)malloc(sizeof(Header));
    tcp_header->message_type = htons(0);

    int f_info[2];
    f_info[0] = parallel_data_streams;
    f_info[1] = udp_packet_size;

    tcp_header->message_length = htons(sizeof(struct Header) + sizeof(f_info));

    sleep(1);
    std::cout << "Sending first tcp message...\n";
    tcpwrapper->Send(tcp_header, (void *)f_info, sizeof(f_info));

    // Receive back the UDP port.
    int sock = tcpwrapper->GetSocket();
    void *temp = tcpwrapper->Receive(sock);
    memcpy(buffer, temp, BUFFER_SIZE);

    tcp_header = (Header *)buffer;
    int *udp_port = (int *)(buffer + sizeof(Header));
    std::cout << "Udp port: " << *udp_port << std::endl;
    
    // UDP ~~~~~~~~~~~~~
    udpwrapper = new SocketWrapper(UDP);
    udpwrapper->SetServerAddr(server_ip, *udp_port);
    for(int i = 0; i < 500; i++){
        UDP_Header udp_header;
        
        sleep(1);
        udpwrapper->SendTo(&udp_header, buffer, BUFFER_SIZE);

    }

    udpwrapper->Close();
    tcpwrapper->Close();

    return -1;
}