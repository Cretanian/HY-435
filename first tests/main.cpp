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

#include "client.h"
#include "server.h"
#include "Parameters.h"

int listening_port = 4211;

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