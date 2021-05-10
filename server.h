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
#include "SocketWrapper.h"

extern int listening_port;
extern SocketWrapper *tcpwrapper;
extern SocketWrapper *udpwrapper;

void GetTime(struct timespec *my_exec_time);

int CheckingNsec(long then, long now);

int Server(Parameters *params){

    uint8_t buffer[BUFFER_SIZE];
    unsigned int udp_packet_size = 256;
    unsigned int parallel_data_streams = 1;
    unsigned int udp_port = 4001;
    uint8_t *data;
    int data_recv = 0;
    int data_sent = 0;

    struct timespec my_exec_time;
    struct timespec start_timer, finish_timer;

    // Set sin.sin_addr
    const char *server_ip = NULL;
    
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

    void *temp = (uint8_t *)tcpwrapper->Receive(client_socket);
    memcpy(buffer, temp, BUFFER_SIZE);

    first_header = (struct Header *)buffer;
    data = buffer + sizeof(struct Header);
    
    int *init_data;
    init_data = (int *)data;
       
    std::cout << "messsage len:" << ntohs(first_header->message_length) << "\nParalle streams: " << init_data[0]<< "\nudp_pac_size " << init_data[1] << std::endl ;
    
    // Send message back to client specifing the UDP port.
    init_data[0] = udp_port;
    
    tcpwrapper->Send(client_socket, first_header, init_data, sizeof(int));

    // UDP Communication
    std::cout << "\n\nUDP:\n";
    udpwrapper = new SocketWrapper(UDP);
    udpwrapper->Bind(udp_port);

    UDP_Header *udp_header;    
    bool first_message_flag = false;

    int data_send_sum = 0;
    int Gdata_send_sum = 0;
    int first_seq_no = 0;
    int lost_packet_sum = 0;

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

            // When the first message arrives, start the timer!
            if(first_message_flag == false){
                first_message_flag = true;
                GetTime(&my_exec_time);
                start_timer = my_exec_time;
                first_seq_no = udp_header->seq_no;
            }else{
                data_send_sum += udp_packet_size + 34;
                Gdata_send_sum += udp_packet_size - 4;
                if(first_seq_no + 1 != udp_header->seq_no)
                    ++lost_packet_sum;
                ++first_seq_no; 
            }

            memcpy(buffer, temp, BUFFER_SIZE); 
        }
    }

    std::cout << "Test run for: " << finish_timer.tv_sec - start_timer.tv_sec << " sec " << start_timer.tv_nsec - finish_timer.tv_nsec << " nsec\n";
    std::cout << "Data send: " << data_send_sum << std::endl;
    std::cout << "GData send: " << Gdata_send_sum << std::endl;
    std::cout << "Lost Packets: " << lost_packet_sum << std::endl;
    
    tcpwrapper->Close();
    udpwrapper->Close();
    close(client_socket);
    
    return -1;
}