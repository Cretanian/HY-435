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
#include <iomanip>

#define TESTPRINT ;std::cout << "test line\n";

#include "client.h"
#include "server.h"
#include "Parameters.h"
#include "SocketWrapper.h"
#include "json.hpp"



void GetTime(struct timespec *my_exec_time){
    if( clock_gettime( CLOCK_MONOTONIC, my_exec_time) == -1 ) {
        perror( "getclock" );
        exit( EXIT_FAILURE );
    }
}
int CheckingNsec(long then, long now){
    if(now >= then)
        return 1;
    else
        return 0;
}
unsigned long long toNanoSeconds(struct timespec time_exec){
    return (unsigned long long)(time_exec.tv_sec % 1000) * 1000000000 + time_exec.tv_nsec;
    // return time_exec.tv_sec;
}

int listening_port = 4344;
SocketWrapper *tcpwrapper = NULL;
SocketWrapper *udpwrapper = NULL;

unsigned int udp_packet_size = 1460 * 3;
InfoData **threads_info_array = NULL;

struct timespec start_time;
struct timespec end_time;
bool start_flag = false;
bool is_over = false;

int main(int argc, char *argv[]){

    auto params = new Parameters();
    params->Parse(argc, argv);
    
    std::vector<std::string> keys = params->Get_Keys();
    for(std::string key : keys){
        if(key == "-c")
            Client(params);
        else if(key == "-s")
            Server(params);
    }

    return -1;   
}