#ifndef HEADER_H
#define HEADER_H

#define BUFFER_SIZE 5000

enum MessageType{
    SOMETHING1,
    SOMETHING2
};

typedef struct Header{
    unsigned int seq_number;
    unsigned int package_length;
    MessageType type;
} Header;

typedef struct Message{
    Header header;
    char buffer[BUFFER_SIZE];
} Message;

#endif