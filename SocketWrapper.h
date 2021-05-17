#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <cassert>
#include <arpa/inet.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <poll.h>

#include "Message.h"
#include "Parameters.h"

#ifndef SOCKET_WRAPPER_H
#define SOCKET_WRAPPER_H

extern unsigned int udp_packet_size;

enum SocketMode{
    TCP = 0,
    UDP = 1
};

class SocketWrapper{
private:
    int sock = -1;
    SocketMode mode;
    struct sockaddr_in *sin;
    uint8_t buffer[BUFFER_SIZE];
    struct pollfd pfd;

    unsigned int packet_size = udp_packet_size;
    unsigned int parallel_data_streams = 1;
    unsigned int port = 4000;
    struct sockaddr_in *server_addr;
    uint32_t seq_no = 0;

public:
    SocketWrapper() = delete;
    SocketWrapper(SocketMode mode){
        if(mode == TCP){
            if( (sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1){
                perror("Opening TCP listening socket");
            }
        }
        else if(mode == UDP){
            if( (sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
                perror("Opening UDP listening socket");
                exit(EXIT_FAILURE);
            }
        }
        else
            assert(0);
    }
    SocketWrapper(int sock, struct sockaddr_in *sin){
        this->sock = sock;
        this->sin = sin;
    }

    void Connect(const char* server_ip, int port){
        assert(sock != -1);
        assert(mode == TCP);
        sin = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
        memset(sin, 0, sizeof(sockaddr_in));
        sin->sin_family = AF_INET;
        sin->sin_port = htons(port);
        if(inet_aton (server_ip, &sin->sin_addr) == 0){
            perror("Error converting IP to Network Byte Order");
            exit(EXIT_FAILURE);
        }

        if(connect(sock, (struct sockaddr *)sin, sizeof(struct sockaddr_in)) < 0){
            perror("Connection Error in socket ");
            exit(EXIT_FAILURE);
        }
    }

    // Bind is same for udp and tcp.
    void Bind(unsigned int port){
        assert(sock != -1);
        this->port = port;

        sin = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
        memset(sin, 0, sizeof(sockaddr_in));
        sin->sin_family = AF_INET;
        sin->sin_port = htons(port);
        sin->sin_addr.s_addr = htonl(INADDR_ANY);
        
        if(bind(sock, (struct sockaddr *)sin, sizeof(*sin)) < 0){
            std::cout << "Port: " << port << std::endl;
            perror("Erron binding socket.");
            exit(EXIT_FAILURE);
        }
    
        std::cout << "Bind Complete\n";
    }
    void Bind(const char* server_ip, int port){
        assert(sock != -1);
        sin = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
        memset(sin, 0, sizeof(sockaddr_in));
        sin->sin_family = AF_INET;
        sin->sin_port = htons(port);
        if(inet_aton (server_ip, &sin->sin_addr) == 0){
            perror("Error converting IP to Network Byte Order");
            exit(EXIT_FAILURE);
        }
        if(bind(sock, (struct sockaddr *)sin, sizeof(struct sockaddr_in)) == -1){
            perror("Erron in bind 1");
            exit(EXIT_FAILURE);
        }
    }

    void Listen(int backlog){
        if (listen(sock, backlog) == -1){
            perror("Error when marking the socket in listening mode");
            exit(EXIT_FAILURE);
        }
        std::cout << "Listening\n";
    }

    std::pair<int, struct sockaddr_in *> Accept(){
        //Accepting incoming connections
        struct sockaddr_in *client_info = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
        memset(client_info, 0, sizeof(sockaddr_in));
        unsigned int clientLength = sizeof(sockaddr_in);
        int client_socket 
                = accept(sock, (struct sockaddr *)client_info, &clientLength);

        if (client_socket < 0){
            perror("Opening new TCP listening socket");
            exit(EXIT_FAILURE);
        }

        int on = 1;
        int rc = ioctl(client_socket, FIONBIO, (char *)&on);
        if (rc < 0)
        {
            perror("ioctl() failed");
            close(sock);
            exit(-1);
        }

        std::cout << "Accepted Client\n";
        return std::make_pair(client_socket, client_info);
    }

    // Precondition: Buffer must be of size BUFFER_SIZE
    void Send(Header *header, void *payload, uint32_t payload_size){
        memcpy(buffer, header, sizeof(Header));
        memcpy(buffer + sizeof(Header), payload, payload_size);

        unsigned int data_sent = send(sock, buffer, sizeof(Header) + payload_size, 0);
    }
    void Send(int defined_sock, Header *header, void *payload, uint32_t payload_size){
        memcpy(buffer, header, sizeof(Header));
        memcpy(buffer + sizeof(Header), payload, payload_size);

        unsigned int data_sent = send(defined_sock, buffer, sizeof(Header) + payload_size, 0);
    }

    void *Receive(int client_socket, uint32_t payload_size){
        int data_recv = recv(client_socket, buffer, sizeof(Header) + payload_size, 0);
        if(data_recv < 0)
            perror("Receiving data failed");
    
        return buffer;
    }

    void SendTo(UDP_Header *header, void *payload, uint32_t payload_size){
        header->seq_no = ++seq_no;
        memcpy(buffer, header, sizeof(struct UDP_Header));
        memcpy(buffer + sizeof(struct UDP_Header), payload, sizeof(payload_size) - sizeof(UDP_Header));

        unsigned int sent_data;
        sent_data = sendto(sock, buffer, packet_size, 0, (struct sockaddr *)server_addr, sizeof(struct sockaddr_in));
        if(sent_data != packet_size)
            perror("send() sent a different number of bytes than expected");
    }

    void *ReceiveFrom(){
        unsigned int len = sizeof(struct sockaddr_in);
        int data_recv = recvfrom(sock, buffer, packet_size, 0, (struct sockaddr *)sin, &len);
        if(data_recv < 0){
            perror("Receive from failed.");
        }

        return buffer;
    }

    void SetServerAddr(const char *server_ip, int port){
        server_addr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
        memset(server_addr, 0, sizeof(struct sockaddr_in));
        server_addr->sin_family = AF_INET;
        server_addr->sin_port = htons(port);
        if(inet_aton (server_ip, &server_addr->sin_addr) == 0){
            perror("Error converting IP to Network Byte Order");
            exit(EXIT_FAILURE);
        }
    }

    bool Poll(int client_socket){
        pfd.fd = client_socket;
        pfd.events = POLLIN;    
        int rc = poll(&pfd, 1, 0);
        
        if(pfd.revents & POLLIN){
            return true;
        }
        else
            return false;
    }
    bool Poll(int client_socket, int poll_time){
        pfd.fd = client_socket;
        pfd.events = POLLIN;    
        int rc = poll(&pfd, 1, poll_time);

        // if(rc == 0)
        //     std::cout << "Poll timeout.\n";
        // else if(rc < 0)
        //     std::cout << "Poll failed.\n";
        
        // std::cout << "Poll revents: " << pfd.revents << "\n";
        // std::cout << "End of poll.\n";
        
        if(pfd.revents & POLLIN){
            return true;
        }
        else
            return false;
    }


    void Close(){
        if(sock > 0)
            close(sock);
    }

    // Getters setters:
    int GetSocket(){
        return sock;
    }

    unsigned int GetUDPPort(){
        return port;
    }

    unsigned int GetUDPPacketLength(){
        return packet_size;
    }
   
    void SetUDPPacketLength(unsigned int packet_size){
        this->packet_size = packet_size;
    }
};

#endif