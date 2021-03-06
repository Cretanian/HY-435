#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include <list>
#include <cmath>
#include <iomanip>
#include <fstream>
#include <thread>

#include "json.hpp"
#include "Parameters.h"
#include "Message.h"
#include "SocketWrapper.h"
#include "InfoData.h"

using json = nlohmann::json;


json sum;
std::vector<json> sum_vec;
std::vector<json> intervals_vec;


bool dont_create_file = true;

extern int listening_port;
extern SocketWrapper *tcpwrapper;
extern SocketWrapper *udpwrapper;
extern unsigned int udp_packet_size;
extern bool has_one_way_delay;

extern InfoData **threads_info_array; //Array of InfoData pointers

// Global, shared variable to indicate the end of the experiment.
extern bool is_over;
extern bool start_flag;
extern struct timespec start_time;
extern struct timespec end_time;

void GetTime(struct timespec *my_exec_time);
int CheckingNsec(long then, long now);
unsigned long long int toNanoSeconds(struct timespec time_exec);

// Precondition: Receive it in decimal format of Mega Bits
void print_bandwidth_bar(float bandwidth, float target_bandwidth){
    float reach_percent = (float)bandwidth/(float)target_bandwidth;
    int bar_max = 50;
    int bar_counter = bar_max * reach_percent; // Only care for the first decimal accuracy;
    
    std::cout << "[";
    for(int i = 0; i < bar_counter; i++)
        std::cout << "#";
    for(int i = bar_counter; i < bar_max; i++)
        std::cout << ".";
    std::cout << "]  " << reach_percent * 100 << "%" << std::endl;
}

int thread_printing_server(float interval, unsigned int parallel_data_streams, unsigned int target_bandwidth){
    struct timespec my_exec_time;
    struct timespec before_sleep;
    struct timespec after_sleep;

    json threads_iteration_data;
   
    GetTime(&my_exec_time);
    before_sleep = my_exec_time;

    // Prepare info data placeholders to subtract from the newer ones.
    InfoData **prev_info_array = (InfoData **)malloc(sizeof(InfoData *) * parallel_data_streams);
    for(int i = 0; i < parallel_data_streams; i++)
        prev_info_array[i] = new InfoData();

    // For lost/total
    unsigned int *last_interval_seq_no = (unsigned int *)malloc(sizeof(unsigned int) * parallel_data_streams);
    for(int i = 0; i < parallel_data_streams; i++)
        last_interval_seq_no[i] = 0;

    while(start_flag == false){
        // Spin to eternal nothingness
    }

    // Interval Info Printing
    while(is_over == false){
        std::cout << "\n\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
        std::cout << "Interval: " << (toNanoSeconds(my_exec_time) - toNanoSeconds(start_time)) / 1000000 << " ms" << std::endl;

        GetTime(&my_exec_time);
        before_sleep = my_exec_time;
        usleep(interval * 1000 * 1000);
        GetTime(&my_exec_time);
        after_sleep = my_exec_time;

        for(int i = 0; i < parallel_data_streams; i++){
            if(threads_info_array[i] == NULL)
                continue;

            InfoData current_info = *threads_info_array[i] - *prev_info_array[i];
            unsigned long long true_interval_time = toNanoSeconds(after_sleep) - toNanoSeconds(before_sleep);        
            float interval_bandwidth =  ((current_info.data_sum * 8 / (1024*1024)) / ((float)(true_interval_time/1000) / 1000)) * 1000;    

            std::cout << "~~~~~~~~~~~~~" << std::endl;
            std::cout << "Stream [" << i << "]:" << std::endl; 
            auto jitter_list = current_info.findJitterList();
            auto averageJitter = current_info.findAverageJitter(jitter_list);
            unsigned int total_interval_packets = current_info.num_of_packets;

            std::cout << "Transfer: " << ((float)current_info.data_sum / (1024*1024)) << "MB" << std::endl;
            std::cout << "Good Transfer: " << ((float)current_info.gdata_sum / (1024*1024)) << "MB" << std::endl;
            std::cout << "Bandwidth: " << interval_bandwidth << " Mbits/sec\n";
            print_bandwidth_bar(interval_bandwidth, (float)target_bandwidth/(1000 * 1000));
            std::cout << "Jitter: " << averageJitter << " nanoseconds" << std::endl;
            std::cout << "Lost/Total: " << current_info.lost_packet_sum << " / " << total_interval_packets 
                                        << " (" << ((float)current_info.lost_packet_sum/(float)total_interval_packets)*100 << "%)" << std::endl;

            if(!dont_create_file){
                threads_iteration_data["Port"] = threads_info_array[i]->udp_port;
                threads_iteration_data["Start_Time"] = toNanoSeconds(my_exec_time);
                threads_iteration_data["Finish_Time"] = toNanoSeconds(my_exec_time) + (unsigned long long)(interval * 1000) * 1000 * 1000;
                threads_iteration_data["Total_Data send"] = current_info.data_sum;
                threads_iteration_data["Total_Goodput_send"] = current_info.gdata_sum;
                threads_iteration_data["Total_Lost_Packets"] = current_info.lost_packet_sum;
                threads_iteration_data["Total_packets_received"] = current_info.num_of_packets;
                threads_iteration_data["Throuput"] = current_info.data_sum / interval;
                threads_iteration_data["Average_Jitter"] = current_info.jitter_average;
                threads_iteration_data["Standard_Devination_of_Jitter"] = current_info.jitter_deviation;          

                intervals_vec[i].push_back(threads_iteration_data);
            }

            prev_info_array[i]->Copy(*threads_info_array[i]);
        }
        
    } 
        
    unsigned long long total_experiment_time = toNanoSeconds(end_time) - toNanoSeconds(start_time);
    float total_experiment_float = (float)(total_experiment_time / (1000 * 1000)) / 1000;

    std::cout << "\n\n";
    std::cout << "Experiment Total time: " << total_experiment_time << " nanoseconds.\n";
    std::cout << "~~~~ Results ~~~~\n";

    for(int i = 0; i < parallel_data_streams; i++){
        std::cout << "~~~~~~~~~~~~~" << std::endl;
        std::cout << "Stream [" << i << "]:" << std::endl; 

        auto jitter_list = threads_info_array[i]->findJitterList();
        auto averageJitter = threads_info_array[i]->findAverageJitter(jitter_list);

        std::cout << "Transfer: " << ((float)threads_info_array[i]->data_sum / (1024*1024)) << "MB" << std::endl;
        std::cout << "Good Transfer: " << ((float)threads_info_array[i]->gdata_sum / (1024*1024)) << "MB" << std::endl;
        std::cout << "Bandwidth: " << ((float)threads_info_array[i]->data_sum) * 8 / (1024*1024) / total_experiment_float << "Mbits/sec" << std::endl;
        print_bandwidth_bar(((float)threads_info_array[i]->data_sum) * 8 / (1024*1024) / total_experiment_float, (float)target_bandwidth/(1000 * 1000));
        std::cout << "Jitter Average: " << threads_info_array[i]->jitter_average << " nanoseconds" << std::endl;
        std::cout << "Jitter Deviation: " << threads_info_array[i]->jitter_deviation << " nanosecond" << std::endl;
        std::cout << "Lost/Total: " << threads_info_array[i]->lost_packet_sum << " / " << threads_info_array[i]->num_of_packets 
                                    << " (" << ((float)threads_info_array[i]->lost_packet_sum/(float)threads_info_array[i]->num_of_packets)*100 << "%)" << std::endl;

       
        if(!dont_create_file){
            sum["Port"] = threads_info_array[i]->udp_port;
            sum["Execution_Time"] = total_experiment_time;
            sum["Total_Data send"] = threads_info_array[i]->data_sum;
            sum["Total_Goodput_send"] = threads_info_array[i]->gdata_sum;
            sum["Total_Lost_Packets"] = threads_info_array[i]->lost_packet_sum;
            sum["Total_packets_received"] = threads_info_array[i]->num_of_packets;
            sum["Throuput"] = threads_info_array[i]->data_sum / total_experiment_float;
            sum["Average_Jitter"] = threads_info_array[i]->jitter_average;
            sum["Standard_Devination_of_Jitter"] = threads_info_array[i]->jitter_deviation;          

            sum_vec.push_back(sum);
        }

    }

    return 1;
}

int thread_udp_server(int id, int port){
    SocketWrapper *wrapper = new SocketWrapper(UDP);
    wrapper->SetUDPPacketLength(udp_packet_size);
    wrapper->Bind(port);

    UDP_Header *udp_header;    
    bool first_message_flag = false;

    void *temp;
    struct timespec my_exec_time;

    threads_info_array[id] = new InfoData();
    InfoData *info_data = threads_info_array[id];
    info_data->udp_port = port;
    assert(info_data != NULL);

    while(start_flag = false){
        // Spin to eternal nothingness
    }

    int sock = wrapper->GetSocket();
    while(is_over == false){
        // Only if data exist, receive them.
        if(wrapper->Poll(sock) == true){
            temp = wrapper->ReceiveFrom();
            udp_header = (UDP_Header *)temp;

            GetTime(&my_exec_time);
            
            // When the first message arrives, start the timer!
            if(first_message_flag == false){
                first_message_flag = true;
                info_data->prev_seq_no = udp_header->seq_no;
            }
            else{
                // Increase counter
                info_data->num_of_packets = udp_header->seq_no + 1;
                
                // Push time arrived for later calculation of jitter
                info_data->time_arrived.push_back(toNanoSeconds(my_exec_time));
                
                // Capture data for data arrivede
                info_data->data_sum += udp_packet_size + 42; // This will be used for the final printing of the whole data.
                info_data->gdata_sum += udp_packet_size - sizeof(UDP_Header);

                // Check if package arrived off order.
                if(info_data->prev_seq_no < udp_header->seq_no){
                    if(info_data->prev_seq_no + 1 != udp_header->seq_no){
                        info_data->lost_packet_sum += udp_header->seq_no - (info_data->prev_seq_no + 1);
                    }
                    
                    info_data->prev_seq_no = udp_header->seq_no; 
                }   
            }
        }
    }


    wrapper->Close();

    return 1;
}


int Server(Parameters *params){

    uint8_t buffer[BUFFER_SIZE];
    unsigned int parallel_data_streams = 1;
    unsigned int udp_port = 4001;
    unsigned long long experiment_duration_nsec = 0;
    unsigned int target_bandwidth = -1;
    float interval = 1;
    uint8_t *data;
    int data_recv = 0;
    int data_sent = 0;

    struct timespec my_exec_time;


    // Set sin.sin_addr
    char *server_ip = NULL;

    std::ofstream output_file;

    if(params->HasKey("-f")){
        output_file.open(params->GetValue("-f"));
        dont_create_file = false;
    }

    if(params->HasKey("-a")){
        server_ip = (char *)malloc(sizeof(char) * strlen(params->GetValue("-a").c_str()));
        strcpy(server_ip, params->GetValue("-a").c_str());
    }

    if(params->HasKey("-p"))
        listening_port = stoi(params->GetValue("-p"));

    if(params->HasKey("-i"))
        interval = stof(params->GetValue("-i"));

    // Set basic params
    uint32_t bind_ip = INADDR_ANY;

    // Constructor automatically activates socket.
    tcpwrapper = new SocketWrapper(TCP);

    // Bind either for ANY or specific IP
    if(server_ip == NULL)
        tcpwrapper->Bind(listening_port);
    else
        tcpwrapper->Bind(server_ip, listening_port);

    // Start listening and define backlog
    tcpwrapper->Listen(10);
    
    // Accept.
    // Accept returns a pair of the socket and the sin of the client.
    std::pair<int, struct sockaddr_in *> accepted_client = tcpwrapper->Accept();
    int client_socket = accepted_client.first;
    struct sockaddr_in client_info = *accepted_client.second;

    const char *client_ip = inet_ntoa(client_info.sin_addr);
    std::cout << "Client IP: " << client_ip << std::endl;

    // TCP Communication
    Header *first_header;

    // Until something is to be received, loop.    
    while(true){
        if(tcpwrapper->Poll(client_socket) == true){
            break;
        }
    }

    void *temp = (uint8_t *)tcpwrapper->Receive(client_socket, sizeof(int) * 5);
    memcpy(buffer, temp, BUFFER_SIZE);

    first_header = (struct Header *)buffer;
    data = buffer + sizeof(struct Header);
    
    int *init_data;
    init_data = (int *)data;
       
    // Copy data from buffer to variables.
    parallel_data_streams       = ntohl(init_data[0]);
    udp_packet_size             = ntohl(init_data[1]);
    experiment_duration_nsec    = ntohl(init_data[2]);
    has_one_way_delay           = ntohl(init_data[3]);
    target_bandwidth            = ntohl(init_data[4]);

    // Print them out for good measure
    std::cout << "Parallel Streams: " << parallel_data_streams << std::endl;;
    std::cout << "Udp Packet Size: " << udp_packet_size << std::endl;
    std::cout << "Experiment time: " << experiment_duration_nsec << std::endl;
    std::cout << "Is one way: " << has_one_way_delay << std::endl;
    std::cout << "Target Bandwidth: " << target_bandwidth << std::endl;

    // Send message back to client specifing the UDP port.
    init_data[0] = udp_port;
    tcpwrapper->Send(client_socket, first_header, init_data, sizeof(int));

    // TCP One way connection
    if(has_one_way_delay){
        void *payload;
        int *oneway_data;
        while(true){
            if(tcpwrapper->Poll(client_socket)){
                payload = tcpwrapper->Receive(client_socket, sizeof(int));
                memcpy(buffer, payload, sizeof(Header) + sizeof(int));
                data = buffer + sizeof(Header);
                oneway_data = (int *)data;
                int value = ntohs(oneway_data[0]);
                
                if(value == 1){
                    // That means the terminating flag is 1. So terminate.
                    std::cout << "End of one-way delay test." << std::endl;
                    sleep(1);
                    tcpwrapper->Close();
                    close(client_socket);

                    if(params->HasKey("-f"))
                        output_file.close();

                    exit(EXIT_SUCCESS);
                }

                tcpwrapper->Send(client_socket, first_header, (void *)payload, sizeof(int));
            }
        }
    }

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // zoumi

    // Allocate InfoData memory
    threads_info_array = (InfoData **)malloc(sizeof(InfoData *) * parallel_data_streams);
    for(int i = 0; i < parallel_data_streams; i++){
        threads_info_array[i] = NULL;
        std::vector<json> tmp;
        intervals_vec.push_back(tmp);
    }

    // Create printing thread
    std::thread printing_t(thread_printing_server, interval, parallel_data_streams, target_bandwidth);

    // Create udp threads.
    std::list<std::thread *> udp_threads;
    for(int i = 0; i < parallel_data_streams; i++){
        std::thread *udp_server_t = new std::thread(thread_udp_server, i, udp_port + i);
        udp_threads.push_back(udp_server_t);
    }

    // Receive starting signal and start clock.
    // Do TCP thread work.
    while(true){
        if(tcpwrapper->Poll(client_socket) == true){ //Signal to terminate the experiment
            tcpwrapper->Receive(client_socket, sizeof(int));
            GetTime(&start_time);
            start_flag = true;
            break;
        }
    }

    while(true){
        if(tcpwrapper->Poll(client_socket) == true){ //Signal to terminate the experiment
            tcpwrapper->Receive(client_socket, sizeof(int));
            std::cout << "\n\nTerminating signal caught.\n";
            GetTime(&end_time);
            is_over = true;
            break;
        }
    }
    std::cout << "Join the threads\n";

    for(auto it = udp_threads.begin(); it != udp_threads.end(); it++){
        (*it)->join();
    }
    printing_t.join();

    // Send total data back
    Header header;
    for(int i = 0; i < parallel_data_streams; i++){
        header.message_type = 0;
        header.message_length = sizeof(InfoData);

        threads_info_array[i]->HTON();
        tcpwrapper->Send(client_socket, &header, threads_info_array[i], sizeof(InfoData));
        threads_info_array[i]->NTOH();
    }


    if(params->HasKey("-f")){
        //if doesnt work, for get all val and then create new
        json j_vec(intervals_vec);
        json final_sum(sum_vec);
        json sums, intro,intervals;
        
        intro["interval"] = interval;      
        sums["sum_intervals"] ={final_sum};

        intervals["intervals"] = {intro,j_vec, sums};
        output_file << std::setw(4) << intervals << std::endl;
    }

    shutdown(tcpwrapper->GetSocket(), SHUT_RDWR);
    close(client_socket);
    close(tcpwrapper->GetSocket());

    std::cout << "\n\nFIN\n";

    return -1;
}