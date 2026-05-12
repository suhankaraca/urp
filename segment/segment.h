#pragma once

#include <stdint.h>
#include <stddef.h>

#define MSS 1000
#define HEADER_LEN 6
#define SEQUENCE_SPACE 1 << 16

#define FLAG_SYN 0x01
#define FLAG_ACK 0x02
#define FLAG_FIN 0x04

typedef struct {
    uint16_t seq;
    uint8_t flags;
    uint8_t data[MSS];
    size_t data_len;
} segment_t;

int segment_init(segment_t* seg, uint16_t seq, uint8_t flags, const uint8_t* data, size_t len);

// Serialization for the wire
int segment_to_bytes(segment_t* seg, uint8_t** out_buffer);

// Deserialization (with checksum validation)
int segment_from_bytes(segment_t* seg, uint8_t* buffer, size_t total_len);
