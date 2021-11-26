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
#include <list>
#include <chrono>
#include <thread>

#include "Message.h"
#include "Parameters.h"
#include "SocketWrapper.h"
#include "InfoData.h"

extern int listening_port;
extern SocketWrapper *tcpwrapper;
extern SocketWrapper *udpwrapper;
extern unsigned int udp_packet_size;

// Global variables for signal to be able to access it.
bool has_one_way_delay = false;
std::list<unsigned long long> time_list;

// Info Data for parallel streams
extern InfoData **threads_info_array;

extern bool is_over;
extern bool start_flag;
extern struct timespec start_time;
extern struct timespec end_time;

void GetTime(struct timespec *my_exec_time);
int CheckingNsec(long then, long now);
unsigned long long int toNanoSeconds(struct timespec time_exec);

int thread_printing_client(float interval, unsigned int parallel_data_streams){
    struct timespec my_exec_time;
    struct timespec interval_timer;

    GetTime(&my_exec_time);
    interval_timer = my_exec_time;

    // Prepare info data placeholders to subtract from the newer ones.
    InfoData **prev_info_array = (InfoData **)malloc(sizeof(InfoData *) * parallel_data_streams);
    for(int i = 0; i < parallel_data_streams; i++)
        prev_info_array[i] = new InfoData();

    while(start_flag == false){
        // Spin to eternal nothingness
    }

    // Interval Info Printing
    while(is_over == false){
        GetTime(&my_exec_time);
        
        interval_timer = my_exec_time;

        std::cout << "\n\n~~~~~~~~~~~~~~~~~~~~~~~~~~`\n";
        std::cout << "Interval: " << (toNanoSeconds(my_exec_time) - toNanoSeconds(start_time)) / 1000000 << " ms" << std::endl;

        for(int i = 0; i < parallel_data_streams; i++){
            if(threads_info_array[i] == NULL)
                continue;

            InfoData current_info = *threads_info_array[i] - *prev_info_array[i];

            std::cout << "~~~~~~~~~~~~~" << std::endl;
            std::cout << "Stream [" << i << "]:" << std::endl; 
            auto jitter_list = current_info.findJitterList();
            auto averageJitter = current_info.findAverageJitter(jitter_list);
            unsigned int total_interval_packets = current_info.num_of_packets;

            std::cout << "Transfer: " << ((float)current_info.data_sum / (1024*1024)) << "MB" << std::endl;
            std::cout << "Bandwidth: " << ((float)current_info.data_sum) * 8 / (1024*1024) / interval << "Mbits/sec" << std::endl;

            prev_info_array[i]->Copy(*threads_info_array[i]);
        }

        usleep(interval * 1000 * 1000);
    } 

    std::cout << "\n\n";
    std::cout << "Experiment Total time: " << toNanoSeconds(end_time) - toNanoSeconds(start_time) << " nanoseconds.\n";
    std::cout << "~~~~ Sender Results ~~~~\n";
    for(int i = 0; i < parallel_data_streams; i++){
        std::cout << "~~~~~~~~~~~~~" << std::endl;
        std::cout << "Stream [" << i << "]:" << std::endl; 

        auto jitter_list = threads_info_array[i]->findJitterList();
        auto averageJitter = threads_info_array[i]->findAverageJitter(jitter_list);
        unsigned int total_interval_packets = threads_info_array[i]->num_of_packets;

        std::cout << "Transfer: " << ((float)threads_info_array[i]->data_sum / (1024*1024)) << "MB" << std::endl;
        std::cout << "Bandwidth: " << ((float)threads_info_array[i]->data_sum) * 8 / (1024*1024) / interval << "Mbits/sec" << std::endl;

        prev_info_array[i]->Copy(*threads_info_array[i]);
    }

    return 1;
}

int thread_udp_client(int id, char *server_ip, int port, unsigned long long bandwidth, unsigned long long experiment_duration_nsec){    
    // Instantiate my personal info data and buffer;
    threads_info_array[id] = new InfoData();
    InfoData *info_data = threads_info_array[id];

    uint8_t buffer[BUFFER_SIZE];

    // Prepare wrapper params
    SocketWrapper *wrapper = new SocketWrapper(UDP);
    wrapper->SetServerAddr(server_ip, port);
    wrapper->SetUDPPacketLength(udp_packet_size);
    bool first_message_flag = false;

    // Calculate sleep time in ms for throttling
    unsigned int packets_per_second = bandwidth / (udp_packet_size * 8);
    unsigned int num_of_chunks = 300;
    unsigned int chunk_size = packets_per_second / num_of_chunks;
    float sleep_interval  = (1 / (float)num_of_chunks) * 1000 * 1000 * 1000;

    unsigned int chunk_counter = -1;
    unsigned long long chunk_start = 0;

    struct timespec my_exec_time;
    struct timespec start_timer, finish_timer;

    while(start_flag == false){
        // Spin to eternal nothingness
    }

	GetTime(&my_exec_time);
	chunk_start = toNanoSeconds(my_exec_time);
    while(1){
        ++chunk_counter;
        if(chunk_counter > chunk_size){
		    GetTime(&my_exec_time);
		    unsigned long long chunk_end = toNanoSeconds(my_exec_time);
            chunk_counter = 0;
            std::this_thread::sleep_for(std::chrono::nanoseconds((int)(sleep_interval) - (chunk_end - chunk_start)));
            GetTime(&my_exec_time);
            chunk_start = toNanoSeconds(my_exec_time);
        }

        UDP_Header udp_header; // Header is filled in SendTo
        unsigned int sent_data = wrapper->SendTo(&udp_header, buffer, udp_packet_size);
        assert(sent_data == udp_packet_size);

        info_data->data_sum += udp_packet_size + 42;
        info_data->num_of_packets++;

        GetTime(&my_exec_time);
        if(first_message_flag == false){
            first_message_flag = true;
            start_timer = my_exec_time;
            finish_timer = my_exec_time;
        }else{
            finish_timer = my_exec_time;
        }

        // Terminate after experiment time.
        if(experiment_duration_nsec < (toNanoSeconds(finish_timer) - toNanoSeconds(start_timer))){
            break;
        }
    }

    return 1;
}

void handle_one_way_delay_calculation(){
    // Make sure it wasnt the servers turn to send, otherwise it will block on Send.
    if(tcpwrapper->Poll(tcpwrapper->GetSocket(), 1000)){
        tcpwrapper->Receive(tcpwrapper->GetSocket(), sizeof(int));
    }

    // Calculate average one way delay.
    unsigned long long sum = 0;
    for(auto it = time_list.begin(); it != time_list.end(); it++){
        sum += *it;
    }   
    sum = sum/time_list.size();

    Header *one_way_header = (Header *)malloc(sizeof(Header));
    one_way_header->message_type = sum;
    one_way_header->message_length = sizeof(int);
    
    void *payload = malloc(sizeof(int));
    *(int *)payload = htons(1); // Set unsigned long to 0;

    tcpwrapper->Send(one_way_header, payload, sizeof(int));

    std::cout << "\nOne way delay: " << sum << " nanoseconds" << "\n\n";
}

void signal_callback_handler(int signum) {
    if(has_one_way_delay){
        handle_one_way_delay_calculation();
    }
    else{
        std::cout << "\nCaught signal. Terminating... " << std::endl;

        sleep(1);
        Header *header = (Header *)malloc(sizeof(Header));
        header->message_type = htons(0);
        header->message_length = htons(0);
        tcpwrapper->Send(header, NULL, 0);
    }
    
    // Terminate program
    if(tcpwrapper != NULL)
        tcpwrapper->Close();
    if(udpwrapper != NULL)
        udpwrapper->Close();

    exit(signum);
}

void init(Parameters *params,unsigned int *parallel_data_streams,unsigned int *udp_packet_size,unsigned long long *experiment_duration_nsec, unsigned int *bandwidth, char **server_ip, float *interval){
    if(params->HasKey("-n")){
        *parallel_data_streams = stoi(params->GetValue("-n"));
        assert(*parallel_data_streams > 0);
    }

    if(params->HasKey("-l")){
        *udp_packet_size = stoi(params->GetValue("-l"));
        assert(*udp_packet_size > 0);
    }
    
    if(params->HasKey("-t")){
        *experiment_duration_nsec = (unsigned long long)stoi(params->GetValue("-t")) * 1000 * 1000 * 1000;
        std::cout << "Duration of experiment: " << *experiment_duration_nsec << std::endl;
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
        assert(*bandwidth > 0);

        if(*bandwidth < 150)
            *udp_packet_size = 1460;
        else if(*bandwidth < 330)
            *udp_packet_size = 1460 * 3;
        else if(*bandwidth < 530)
            *udp_packet_size = 1460 * 3.3;
        else if(*bandwidth < 700)
            *udp_packet_size = 1460 * 3.6;
        else if(*bandwidth < 800)
            *udp_packet_size = 1460 * 4;
        else
            *udp_packet_size = 1460 * 4.3; 

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
        *server_ip = (char *)malloc(sizeof(char) * strlen((char *)params->GetValue("-a").c_str()));
        strcpy(*server_ip, (char *)params->GetValue("-a").c_str());
        std::cout << "Custom server ip: " << *server_ip << std::endl;
    }

    if(params->HasKey("-i")){
        std::string interval_string = params->GetValue("-i");
         *interval = stof(interval_string);
        std::cout << "Interval: " << *interval << std::endl;
    }

    if(params->HasKey("-p"))
        listening_port = stoi(params->GetValue("-p"));

    return;
}


int Client(Parameters *params){
    // Set basic params
    
    uint8_t buffer[BUFFER_SIZE];
    unsigned int bandwidth = -1;
    unsigned long long experiment_duration_nsec = (unsigned long long)10 * 1000000000;
    unsigned int parallel_data_streams = 1;
    char *server_ip = NULL;
    float interval = 1;
    unsigned int link_speed = 50 * (1024 * 1024); // 1 GB default link speed.
    uint8_t *data;
    int data_recv = 0;
    int data_sent = 0;
    float sleep_before_tran = 0;
    bool one_way_delay_flag = false;

    if(params->HasKey("-d"))
        one_way_delay_flag = true;

    if(params->HasKey("-w")){
        sleep_before_tran = stof(params->GetValue("-w"));
        assert(sleep_before_tran > 0);
    }

    if(params->HasKey("-ls")){
        link_speed = stoi(params->GetValue("-ls"));
        std::cout << "Link Speed: " << link_speed << "MB" << std::endl;
    }

    if(params->HasKey("-d"))
        has_one_way_delay = true;    
    else
        has_one_way_delay = false;

    struct timespec my_exec_time;
    struct timespec start_timer, finish_timer;

    // Init buffer
    for(int i = 0; i < BUFFER_SIZE; i++){
        buffer[i] = 'a';
    }

    init(params, &parallel_data_streams, &udp_packet_size, &experiment_duration_nsec, &bandwidth, &server_ip, &interval);
    signal(SIGINT, signal_callback_handler);

    if(bandwidth == -1)
        bandwidth = link_speed;

    // TCP Communication
    tcpwrapper = new SocketWrapper(TCP);
    tcpwrapper->Connect(server_ip, listening_port);
    
    Header *tcp_header = (Header *)malloc(sizeof(Header));
    tcp_header->message_type = htons(0);

    int f_info[5];
    std::cout << "Sizeof long long " << sizeof(unsigned long long) << std::endl;
    f_info[0] = htonl(parallel_data_streams);
    f_info[1] = htonl(udp_packet_size);
    f_info[2] = htonl(experiment_duration_nsec);
    f_info[3] = htonl(has_one_way_delay);
    f_info[4] = htonl(bandwidth);

    tcp_header->message_length = htons(sizeof(struct Header) + sizeof(f_info));

    std::cout << "Sending first tcp message...\n";
    tcpwrapper->Send(tcp_header, (void *)f_info, sizeof(f_info));

    // Receive back the UDP port.
    int sock = tcpwrapper->GetSocket();
    void *temp = tcpwrapper->Receive(sock, sizeof(int));
    memcpy(buffer, temp, BUFFER_SIZE);

    tcp_header = (Header *)buffer;
    int *udp_port = (int *)(buffer + sizeof(Header));
    std::cout << "Udp port: " << *udp_port << std::endl;
    
    // TCP One side delay
    if(has_one_way_delay){
        Header *one_way_header = (Header *)malloc(sizeof(Header));
        one_way_header->message_type = htons(0);
        one_way_header->message_length = htons(sizeof(int));
        
        void *payload = (int *)malloc(sizeof(int));
        *(int *)payload = htons(0); // Set terminating flag to 0;

        GetTime(&my_exec_time);
        struct timespec finish_timer = my_exec_time;
        struct timespec start_timer = my_exec_time;

        while(experiment_duration_nsec > (toNanoSeconds(finish_timer) - toNanoSeconds(start_timer))){
            GetTime(&my_exec_time);
            finish_timer = my_exec_time; 
            unsigned long long roundtrip_start = toNanoSeconds(my_exec_time);

            tcpwrapper->Send(one_way_header, (void *)payload, sizeof(int));
            tcpwrapper->Receive(sock, sizeof(int));
            
            GetTime(&my_exec_time);
            unsigned long long roundtrip_end = toNanoSeconds(my_exec_time);

            time_list.push_back((roundtrip_end - roundtrip_start)/2);

        }

        handle_one_way_delay_calculation();
        exit(1);
    }

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // zoumi

    // Initiate InfoData for parallel streams case
    threads_info_array = (InfoData **)malloc(sizeof(InfoData *) * parallel_data_streams);
    for(int i = 0; i < parallel_data_streams; i++)
        threads_info_array[i] = NULL;

    // Create Printing Thread.
    std::thread printing_t(thread_printing_client, interval, parallel_data_streams);

    // Create UDP threads
    std::list<std::thread *> udp_threads;
    for(int i = 0; i < parallel_data_streams; i++){
        std::thread *udp_client_t = new std::thread(thread_udp_client, i, server_ip, *udp_port + i, bandwidth, experiment_duration_nsec);
        udp_threads.push_back(udp_client_t);
    }

    // In case of -w parameter
    usleep(sleep_before_tran * 1000 * 1000);

    // Inform the start of the experiment.
    Header header;
    tcpwrapper->Send(&header, &buffer, sizeof(int));
    GetTime(&start_time);
    start_flag = true;

    for(auto it = udp_threads.begin(); it != udp_threads.end(); it++){
        std::thread *udp_thread = (std::thread *)*it;
        udp_thread->join();
    }

    GetTime(&end_time);

    // END OF EXPERIMENT
    is_over = true;
    tcpwrapper->Send(&header, &buffer, sizeof(int));

    printing_t.join();

    unsigned long long total_experiment_time = toNanoSeconds(end_time) - toNanoSeconds(start_time);
    float total_experiment_float = (float)(total_experiment_time / (1000 * 1000)) / 1000;

    // Receive back the final results
    std::cout << "\n\n~~~~ Receiver Results ~~~~\n";;
    for(int i = 0; i < parallel_data_streams; i++){
        std::cout << "~~~~~~~~~~~~~" << std::endl;
        std::cout << "Stream [" << i << "]:" << std::endl; 

        temp = tcpwrapper->Receive(tcpwrapper->GetSocket(), sizeof(InfoData));
        memcpy(buffer, temp, sizeof(Header) + sizeof(InfoData));

        InfoData *info_data = (InfoData *)(buffer + sizeof(Header));
        info_data->NTOH();
        std::cout << "Throughput: " << ((float)info_data->data_sum / (1024*1024)) << "MB" << std::endl;
        std::cout << "Good Throughput: " << ((float)info_data->gdata_sum / (1024*1024)) << "MB" << std::endl;
        std::cout << "Bandwidth: " << ((float)info_data->data_sum) * 8 / (1024*1024) / total_experiment_float << "Mbits/sec" << std::endl;
        std::cout << "Jitter Average: " << info_data->jitter_average << " nanoseconds" << std::endl;
        std::cout << "Jitter Deviation: " << info_data->jitter_deviation << " nanoseconds" << std::endl;
        std::cout << "Lost/Total: " << info_data->lost_packet_sum << " / " << info_data->num_of_packets 
                                    << " (" << ((float)info_data->lost_packet_sum/(float)info_data->num_of_packets)*100 << "%)" << std::endl;

        memset(buffer, 0, BUFFER_SIZE);
    }

    sleep(1);
    tcpwrapper->Close();
    std::cout << "\n\n";

    return -1;
}
