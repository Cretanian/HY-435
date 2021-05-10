#include <iostream>
#include <vector>
#include <unordered_map>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>
#include <signal.h>
#include <time.h>

#include "client.h"
#include "server.h"
#include "Parameters.h"
#include "SocketWrapper.h"


#define TESTPRINT ;std::cout << "test line\n";

void GetTime(struct timespec *my_exec_time){
    if( clock_gettime( CLOCK_MONOTONIC, my_exec_time) == -1 ) {
        perror( "getclock" );
        exit( EXIT_FAILURE );
    }
}

int listening_port = 4331;
SocketWrapper *tcpwrapper;
SocketWrapper *udpwrapper;

int main(int argc, char *argv[]){

    auto params = new Parameters();
    params->Parse(argc, argv);
    
    std::vector<std::string> keys = params->Get_Keys();
    for(std::string key : keys){
        if(key == "-c")
            Client(params);
        else if(key == "-s")
            Server(params);
        else
            assert(false);
    }

    // startServer();
}