#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <list>
#include <cmath>
#include <iomanip>

#ifndef INFO_DATA_H
#define INFO_DATA_H

class InfoData{
public:
    unsigned int data_sum = 0;
    unsigned int gdata_sum = 0;
    unsigned int prev_seq_no = 0;
    unsigned int lost_packet_sum = 0;
    unsigned int num_of_packets = -1;
    unsigned int jitter_average = -1;
    unsigned int jitter_deviation = -1;
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
        jitter_average = mean;

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
        jitter_deviation = sqrt(standardDeviation / jitter_list.size());

        return sqrt(standardDeviation / jitter_list.size());
    }

    void Copy(InfoData to_copy){
        data_sum = to_copy.data_sum;
        gdata_sum = to_copy.gdata_sum;
        prev_seq_no = to_copy.prev_seq_no;
        lost_packet_sum = to_copy.lost_packet_sum;
        num_of_packets = to_copy.num_of_packets;
        time_arrived = to_copy.time_arrived;
        jitter_average = to_copy.jitter_average;
        jitter_deviation = to_copy.jitter_deviation;
    }

    void HTON(){
        htonl(data_sum);
        htonl(gdata_sum);
        htonl(prev_seq_no);
        htonl(lost_packet_sum);
        htonl(num_of_packets);
        htonl(jitter_average);
        htonl(jitter_deviation);
    }

    void NTOH(){
        ntohl(data_sum);
        ntohl(gdata_sum);
        ntohl(prev_seq_no);
        ntohl(lost_packet_sum);
        ntohl(num_of_packets);
        ntohl(jitter_average);
        ntohl(jitter_deviation);
    }

    InfoData operator-(const InfoData& i){
        InfoData data;
        
        data.data_sum = this->data_sum - i.data_sum;
        data.gdata_sum = this->gdata_sum - i.gdata_sum;
        data.prev_seq_no = this->prev_seq_no - i.prev_seq_no;
        data.lost_packet_sum = this->lost_packet_sum - i.lost_packet_sum;
        data.num_of_packets = this->num_of_packets - i.num_of_packets;
        
        auto it = this->time_arrived.begin();
        for(int j = 0; j < i.time_arrived.size(); j++)
            it++;

        for( ; it != this->time_arrived.end(); it++){
            data.time_arrived.push_back(*it);
        }

        return data;
    }
};

#endif