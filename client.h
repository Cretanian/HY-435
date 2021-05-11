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

int CheckingNsec(long then, long now);

unsigned long long int toNanoSeconds(struct timespec time_exec);

void signal_callback_handler(int signum) {
    std::cout << "\nCaught signal. Terminating... " << std::endl;

    sleep(3);
    Header *header = (Header *)malloc(sizeof(Header));
    header->message_type = htons(0);
    header->message_length = htons(0);
    tcpwrapper->Send(header, NULL, 0);

   // Terminate program
   exit(signum);
}


void init(Parameters *params,unsigned int parallel_data_streams,unsigned int udp_packet_size,unsigned long experiment_duration_nsec,unsigned int bandwidth, char *server_ip){
    if(params->HasKey("-n")){
        parallel_data_streams = stoi(params->GetValue("-n"));
        assert(parallel_data_streams > 0);
    }

    if(params->HasKey("-l")){
        udp_packet_size = stoi(params->GetValue("-l"));
        assert(udp_packet_size > 0);
    }
    
    if(params->HasKey("-t")){
        experiment_duration_nsec = stoi(params->GetValue("-t"));
        assert(experiment_duration_nsec > 0);
    }

    if(params->HasKey("-b")){
        bandwidth = stoi(params->GetValue("-b"));
        assert(bandwidth > 0);
    }

    if(params->HasKey("-a")){
        server_ip = (char *)params->GetValue("-a").c_str();
        std::cout << "Custom server ip: " << *server_ip << std::endl;
    }

    return;
}


int Client(Parameters *params){
    // Set basic params
    
    uint8_t buffer[BUFFER_SIZE];
    unsigned int udp_packet_size = 1460;
    unsigned int bandwidth = 0;
    unsigned long long experiment_duration_nsec = (unsigned long long)4 * 1000000000;
    unsigned int parallel_data_streams = 1;
    char *server_ip = NULL;
    uint8_t *data;
    int data_recv = 0;
    int data_sent = 0;
    int sleep_before_tran = 0;
    bool one_way_delay_flag = false;

    if(params->HasKey("-d"))
        one_way_delay_flag = true;

    struct timespec my_exec_time;
    struct timespec start_timer, finish_timer;

    // Init buffer
    for(int i = 0; i < BUFFER_SIZE; i++){
        buffer[i] = 'a';
    }

    init(params, parallel_data_streams, udp_packet_size, experiment_duration_nsec, bandwidth, server_ip);
    signal(SIGINT, signal_callback_handler);

    // just for testing
    if(server_ip == NULL){
        server_ip = (char *)malloc(sizeof(char) * 40);
        strcpy(server_ip, "147.52.19.9");
    }

    // TCP Communication
    tcpwrapper = new SocketWrapper(TCP);
    tcpwrapper->Connect(server_ip, listening_port);
    
    Header *tcp_header = (Header *)malloc(sizeof(Header));
    tcp_header->message_type = htons(0);

    int f_info[3];
    f_info[0] = parallel_data_streams;
    f_info[1] = udp_packet_size;
    f_info[2] = experiment_duration_nsec;

    tcp_header->message_length = htons(sizeof(struct Header) + sizeof(f_info));

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
    bool first_message_flag = false;


    while(1){
        UDP_Header udp_header;
        
        //sleep(1);
        udpwrapper->SendTo(&udp_header, buffer, BUFFER_SIZE - sizeof(UDP_Header));
        GetTime(&my_exec_time);
        // std::cout << "Seconds: " << toNanoSeconds(my_exec_time) << std::endl;
        if(first_message_flag == false){
            first_message_flag = true;
            start_timer = my_exec_time;
            finish_timer = my_exec_time;
        }else{
            finish_timer = my_exec_time;
        }

        // Terminate after experiment time.
        if(experiment_duration_nsec < (toNanoSeconds(finish_timer) - toNanoSeconds(start_timer))){
            Header *header = (Header *)malloc(sizeof(Header));
            header->message_type = htons(0);
            header->message_length = htons(0);
            tcpwrapper->Send(header, NULL, 0);
        }
    }

    udpwrapper->Close();
    tcpwrapper->Close();

    return -1;
}