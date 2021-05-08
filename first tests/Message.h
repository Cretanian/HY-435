#ifndef HEADER_H
#define HEADER_H

#define BUFFER_SIZE  1024 


typedef struct Header{
    uint16_t message_type;
    uint16_t message_length;
} Header;


typedef struct UDP_Header{
    uint16_t seq_no;
} UDP_Header;



#endif