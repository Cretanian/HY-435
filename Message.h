#ifndef HEADER_H
#define HEADER_H

#define BUFFER_SIZE  2000 


typedef struct Header{
    uint16_t message_type;
    uint16_t message_length;
} Header;


typedef struct UDP_Header{
    uint32_t seq_no;
} UDP_Header;



#endif