#ifndef TCP_SEGMENT_H
#define TCP_SEGMENT_H

#include <stdint.h>

#define MSS 1024  // Maximum Segment Size (1 KB)

typedef struct {
    uint16_t src_port;
    uint16_t dest_port;
    uint32_t seq_num;
    uint32_t ack_num;
    uint8_t header_length;  // 4 bits, and 4 bits unused
    uint8_t flags;        // 6 bits, and 2 bits unused
    uint16_t receive_window;
    uint16_t checksum;
    uint16_t urgent_ptr;
    char data[MSS];
} tcp_segment_t;

#endif // TCP_SEGMENT_H

