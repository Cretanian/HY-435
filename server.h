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

#include "json.hpp"
#include "Parameters.h"
#include "Message.h"
#include "SocketWrapper.h"
#include "InfoData.h"

using json = nlohmann::json;


extern int listening_port;
extern SocketWrapper *tcpwrapper;
extern SocketWrapper *udpwrapper;

extern bool has_one_way_delay;

void GetTime(struct timespec *my_exec_time);

int CheckingNsec(long then, long now);

int Server(Parameters *params){

    uint8_t buffer[BUFFER_SIZE];
    unsigned int udp_packet_size = 1460;
    unsigned int parallel_data_streams = 1;
    unsigned int udp_port = 4001;
    unsigned int experiment_duration_sec = -1;
    unsigned int interval = 1000;
    uint8_t *data;
    int data_recv = 0;
    int data_sent = 0;

    struct timespec my_exec_time;
    struct timespec start_timer, finish_timer;
    struct timespec jitter_start, jitter_finish;

    // Set sin.sin_addr
    const char *server_ip = NULL;
    
    if(params->HasKey("-f")){
        json j;

        // add a number that is stored as double (note the implicit conversion of j to an object)
        j["pi"] = 3.141;

        // add a Boolean that is stored as bool
        j["happy"] = true;

        // add a string that is stored as std::string
        j["name"] = "Niels";

        // add another null object by passing nullptr
        j["nothing"] = nullptr;

        // add an object inside the object
        j["answer"]["everything"] = 42;

        // add an array that is stored as std::vector (using an initializer list)
        j["list"] = { 1, 0, 2 };

        // add another object (using an initializer list of pairs)
        j["object"] = { {"currency", "USD"}, {"value", 42.99} };
        j["object2"] = { {"currency", "USD"}, {"value", 42.99} };
 

        std::ofstream output_file(params->GetValue("-f"));

        //to be removed
        output_file << std::setw(4) << j << std::endl;
        output_file.close();
    }   


    if(params->HasKey("-a")){
        server_ip = params->GetValue("-a").c_str();
    }   
    if(params->HasKey("-p"))
        listening_port = stoi(params->GetValue("-p"));

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
       
    std::cout << "messsage len:" << ntohs(first_header->message_length) << "\nParalle streams: " << init_data[0]<< "\nudp_pac_size " << init_data[1] << std::endl ;
    udp_packet_size = init_data[1];
    experiment_duration_sec = init_data[2];
    interval = init_data[3];
    has_one_way_delay = init_data[4];
    std::cout << "Experiment time: " << experiment_duration_sec << std::endl;
    std::cout << "Printing Interval: " << interval << std::endl;
    std::cout << "Is one way: " << has_one_way_delay << std::endl;

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
                    exit(EXIT_SUCCESS);
                }

                tcpwrapper->Send(client_socket, first_header, (void *)payload, sizeof(int));
            }
        }
    }



    // UDP Communication
    std::cout << "\n\nUDP:\n";
    udpwrapper = new SocketWrapper(UDP);
    udpwrapper->Bind(udp_port);

    UDP_Header *udp_header;    
    bool first_message_flag = false;

    InfoData *info_data = new InfoData();
    InfoData *info_data_interval = new InfoData();
    struct timespec interval_timer;
    unsigned int last_interval_seq_no = 0;

    while(true){
        // In case of signal exit.
        if(tcpwrapper->Poll(client_socket) == true){
            GetTime(&my_exec_time);
            finish_timer = my_exec_time;
            std::cout << "Caught a signal.. terminate\n";
            break;
        }

        // Only if data exist, receive them.
        if(udpwrapper->Poll(udpwrapper->GetSocket()) == true){
            temp = udpwrapper->ReceiveFrom();
            udp_header = (UDP_Header *)temp;

            GetTime(&my_exec_time);

            // When the first message arrives, start the timer!
            if(first_message_flag == false){
                first_message_flag = true;
                start_timer = my_exec_time;
                info_data->prev_seq_no = udp_header->seq_no;
                interval_timer = start_timer;
            }
            else{
                // Increase counter
                // if(info_data->num_of_packets < udp_header->seq_no + 1)
                info_data->num_of_packets = udp_header->seq_no + 1;
                

                // Push time arrived for later calculation of jitter
                info_data->time_arrived.push_back(toNanoSeconds(my_exec_time));
                info_data_interval->time_arrived.push_back(toNanoSeconds(my_exec_time));
                
                // Capture data for data arrivede
                info_data->data_sum += udp_packet_size + 42; // This will be used for the final printing of the whole data.
                info_data->gdata_sum += udp_packet_size - sizeof(UDP_Header);
                info_data_interval->data_sum += udp_packet_size + 42; // This will be used for interval printing;

                // Check if package arrived off order.
                if(info_data->prev_seq_no < udp_header->seq_no){
                    if(info_data->prev_seq_no + 1 != udp_header->seq_no){
                        info_data->lost_packet_sum += udp_header->seq_no - (info_data->prev_seq_no + 1);
                        info_data_interval->lost_packet_sum += udp_header->seq_no - (info_data->prev_seq_no + 1);
                    }
                    
                    info_data->prev_seq_no = udp_header->seq_no; 
                }
            }

            // Interval Info Printing
            if(toNanoSeconds(interval_timer) <= toNanoSeconds(my_exec_time) - (unsigned long long)interval * 1000000000){
                interval_timer = my_exec_time;

                auto jitter_list = info_data_interval->findJitterList();
                auto averageJitter = info_data_interval->findAverageJitter(jitter_list);
                unsigned int total_interval_packets = info_data->num_of_packets - last_interval_seq_no;

                std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~`\n";
                std::cout << "Transfer: " << ((float)info_data_interval->data_sum / (1024*1024)) << "MB" << std::endl;
                std::cout << "Bandwidth: " << ((float)info_data_interval->data_sum) * 8 / (1024*1024) / interval << "Mbits/sec" << std::endl;
                std::cout << "Jitter: " << averageJitter << " nanoseconds" << std::endl;
                std::cout << "Lost/Total: " << info_data_interval->lost_packet_sum << " / " << total_interval_packets 
                                            << " (" << ((float)info_data_interval->lost_packet_sum/(float)total_interval_packets)*100 << "%)" << std::endl;

                // Write json information here.
                // mpla mpla

                last_interval_seq_no = info_data->num_of_packets;

                delete info_data_interval; // Free memory and reset.
                info_data_interval = new InfoData(); // Create new InfoData
            }
        }

    }

    std::cout << std::endl << "~~ Results ~~" << std::endl;
    std::cout << "Test run for: " << finish_timer.tv_sec - start_timer.tv_sec << " sec " << start_timer.tv_nsec - finish_timer.tv_nsec << " nsec\n";
    std::cout << "Data send: " << info_data->data_sum << std::endl;
    std::cout << "GData send: " << info_data->gdata_sum << std::endl;
    std::cout << "Lost Packets: " << info_data->lost_packet_sum << std::endl;
    std::cout << "Total packets received: " << info_data->num_of_packets << std::endl;

    std::cout << "Throuput: " << info_data->data_sum / (experiment_duration_sec/1000000000) << std::endl;

    std::list<unsigned long long> jitter_list = info_data->findJitterList();
    unsigned long long mean = info_data->findAverageJitter(jitter_list);
    unsigned long long devination_jitter = info_data->findStandardDeviationJitter(jitter_list, mean);
    // std::cout << "Average Jitter: " << mean << std::endl;
    // std::cout << "Standard Devination of Jitter: " << devination_jitter << std::endl; 

    Header *header = (Header *)malloc(sizeof(Header));
    header->message_type = 0;
    header->message_length = sizeof(InfoData);

    tcpwrapper->Send(client_socket, header, info_data, sizeof(InfoData));


    tcpwrapper->Close();
    udpwrapper->Close();
    close(client_socket);
    
    return -1;
}