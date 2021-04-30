#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <poll.h>

int main(){

    // Set basic params
    int listening_port = 4200;

    // Creating socket
    int sock;
    if((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1){
        perror("Opening TCP listening socket");
        exit(EXIT_FAILURE);
    }

    // Binding port with a specific interface (IPv4)
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(listening_port);
    sin.sin_addr.s_addr = htonl (INADDR_ANY);

    if(bind(sock, (struct sockaddr *)&sin, sizeof(sin)) == -1){
        perror("Erron in TCP bind");
        exit(EXIT_FAILURE);
    }

    //Setting port listening mode
    std::cout << "Listening to port: " << listening_port << std::endl;
    if (listen(sock, 10) != -1){
        perror("Error when marking the socket in listening mode");
        exit(EXIT_FAILURE);
    }

    //Accepting incoming connections
    struct sockaddr_in client_info;
    memset(&client_info, 0, sizeof(sockaddr_in));
    unsigned int clientLength = sizeof(sockaddr_in);

    int new_returned_client_socket = accept(sock, (struct sockaddr *)&client_info, &clientLength);
    
    if (new_returned_client_socket < 0){
        perror("Opening new TCP listening socket");
        exit(EXIT_FAILURE);
    }

    
    //poll() use to check data in socket non blocking
    //recv doent allocate memory


    close(sock);
}