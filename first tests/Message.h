#ifndef HEADER_H
#define HEADER_H

#define BUFFER_SIZE 5000

#define SEQ_NO udp_header.seq_no

enum MessageType{
    SOMETHING1,
    SOMETHING2
};

typedef struct Header{
    unsigned int message_type;
    unsigned int message_length;
    unsigned int num_parallel_streams;
} Header;

typedef struct Message{
    Header header;
    char buffer[BUFFER_SIZE];
} Message;


typedef struct UDP_Header{
    unsigned int seq_no;
} UDP_Header;

typedef struct UDP_Message{
    UDP_Header udp_header;
    void* buffer;
} UDP_Message;



#endif