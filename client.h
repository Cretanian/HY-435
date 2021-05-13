#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <cassert>
#include <arpa/inet.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <algorithm>

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


void init(Parameters *params,unsigned int *parallel_data_streams,unsigned int *udp_packet_size,unsigned long long *experiment_duration_nsec, unsigned int *bandwidth, char *server_ip, unsigned int *interval){
    if(params->HasKey("-n")){
        *parallel_data_streams = stoi(params->GetValue("-n"));
        assert(*parallel_data_streams > 0);
    }

    if(params->HasKey("-l")){
        *udp_packet_size = stoi(params->GetValue("-l"));
        assert(*udp_packet_size > 0);
    }
    
    if(params->HasKey("-t")){
        *experiment_duration_nsec = stoi(params->GetValue("-t"));
        assert(*experiment_duration_nsec > 0);
    }

    if(params->HasKey("-b")){
        std::string helper = params->GetValue("-b");
        helper.erase(remove(helper.begin(), helper.end(), ' '), helper.end());

        std::cout << helper<< "\n";
        int flag = 0;

        char *num = (char *)malloc(sizeof(char) * 10);
        int num_it = 0;

        for (int i = 0; i < helper.length(); i++){
            char c = helper.at(i);
            

        if(c == 'k' || c == 'K')
            flag = 1;        
        else if(c == 'm' || c == 'M')
            flag = 2;
        else if(c == 'g' || c == 'G')
            flag = 3;
        else if(c == 'b' || c == 'B')
            break;    
        else if(c == '0')
            num[num_it] = c;
        else if(c == '1')
            num[num_it] = c;
        else if(c == '2')
            num[num_it] = c;
        else if(c == '3')
            num[num_it] = c;
        else if(c == '4')
            num[num_it] = c;
        else if(c == '5')
            num[num_it] = c;
        else if(c == '6')
            num[num_it] = c;
        else if(c == '7')
            num[num_it] = c;
        else if(c == '8')
            num[num_it] = c;
        else if(c == '9')
            num[num_it] = c;
        else
            flag = 4;
        ++num_it;
            
        }

        *bandwidth = atoi(num);

        if(flag == 1)
            *bandwidth = *bandwidth * 1024;
        else if(flag == 2)
            *bandwidth = *bandwidth * 1024 * 1024; 
        else if(flag == 3)
            *bandwidth = *bandwidth * 1024 * 1024 * 1024;
        else{
            std::cout << "Bad input.\n";
            assert(false);   
        }
        
       
       assert(*bandwidth > 0);
    }

    if(params->HasKey("-a")){
        server_ip = (char *)params->GetValue("-a").c_str();
        std::cout << "Custom server ip: " << *server_ip << std::endl;
    }

    if(params->HasKey("-i")){
        std::string interval_string = params->GetValue("-i");
         *interval = stoi(interval_string);
        std::cout << "Interval: " << *interval << std::endl;
    }

    return;
}


int Client(Parameters *params){
    // Set basic params
    
    uint8_t buffer[BUFFER_SIZE];
    unsigned int udp_packet_size = 1460;
    unsigned int bandwidth = -1;
    unsigned long long experiment_duration_nsec = (unsigned long long)4 * 1000000000;
    unsigned int parallel_data_streams = 1;
    char *server_ip = NULL;
    unsigned int interval = 1;
    unsigned int link_speed = 1 * (1024); // 1 GB default link speed.
    uint8_t *data;
    int data_recv = 0;
    int data_sent = 0;
    int sleep_before_tran = 0;
    bool one_way_delay_flag = false;

    if(params->HasKey("-d"))
        one_way_delay_flag = true;

    if(params->HasKey("-w")){
        sleep_before_tran = stoi(params->GetValue("-w"));
        assert(sleep_before_tran > 0);
    }

    if(params->HasKey("-ls")){
        link_speed = stoi(params->GetValue("-ls"));
        std::cout << "Link Speed: " << link_speed << "MB" << std::endl;
    }

    struct timespec my_exec_time;
    struct timespec start_timer, finish_timer;

    // Init buffer
    for(int i = 0; i < BUFFER_SIZE; i++){
        buffer[i] = 'a';
    }

    init(params, &parallel_data_streams, &udp_packet_size, &experiment_duration_nsec, &bandwidth, server_ip, &interval);
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

    int f_info[4];
    f_info[0] = parallel_data_streams;
    f_info[1] = udp_packet_size;
    f_info[2] = experiment_duration_nsec;
    f_info[3] = interval;

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


    sleep(sleep_before_tran);
    struct timespec interval_timer;

    unsigned int data_sum = 0;
    unsigned int num_of_packets = 0;

    while(1){
        UDP_Header udp_header;
    
        udpwrapper->SendTo(&udp_header, buffer, udp_packet_size);
        data_sum += udp_packet_size + 34;
        num_of_packets++;

        GetTime(&my_exec_time);
        // std::cout << "Seconds: " << toNanoSeconds(my_exec_time) << std::endl;
        if(first_message_flag == false){
            first_message_flag = true;
            start_timer = my_exec_time;
            finish_timer = my_exec_time;

            interval_timer = start_timer;
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

        if(toNanoSeconds(interval_timer) <= toNanoSeconds(my_exec_time) - (unsigned long long)interval * 1000000000){
            interval_timer = my_exec_time;

            std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~`\n";
            std::cout << "Transfer: " << ((float)data_sum / (1024*1024)) << std::endl;
            std::cout << "Bandwidth: " << ((bandwidth > 0) ? ((float)bandwidth)/(1024*1024) : link_speed) << std::endl;
            std::cout << "Num of packets: " << num_of_packets << std::endl;

            num_of_packets = 0;
            data_sum = 0;
        }
    }

    udpwrapper->Close();
    tcpwrapper->Close();

    return -1;
}