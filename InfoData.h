#ifndef INFO_DATA_H
#define INFO_DATA_H

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <list>

class InfoData{
public:
    unsigned int data_sum = 0;
    unsigned int gdata_sum = 0;
    unsigned int prev_seq_no = 0;
    unsigned int lost_packet_sum = 0;
    unsigned int num_of_packets = -1;
    std::list<unsigned long long> time_arrived;

    InfoData() = default;
    ~InfoData(){};

    void *Encode(){
        return NULL;
    }

    std::list<unsigned long long> findJitterList(){
        std::list<unsigned long long> jitter_list;
        
        for(auto it = time_arrived.begin(); it != time_arrived.end(); ++it){
            if(std::next(it) == time_arrived.end())
                break;

            jitter_list.push_back(*std::next(it) - *it); 
        }

        return jitter_list;
    }

    unsigned long long findAverageJitter(std::list<unsigned long long> jitter_list){
        unsigned long long mean = 0;
        for(auto it = jitter_list.begin(); it != jitter_list.end(); ++it){
            mean += *it;    
        }

        mean = mean/((unsigned long long)jitter_list.size() - 1); 

        return mean;
    }

    unsigned long long findStandardDeviationJitter(std::list<unsigned long long> jitter_list, unsigned long long mean){
        unsigned long long standardDeviation = 0;
        for(auto it = jitter_list.begin(); it != jitter_list.end(); ++it){       
            if(*it > mean)  
                standardDeviation += (unsigned long long)powl(*it - mean, 2);
            else
                standardDeviation += (unsigned long long)powl(mean - *it, 2);
        }
        return sqrt(standardDeviation / jitter_list.size());
    }
};

#endif