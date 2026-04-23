#pragma once

#include <stdint.h>
#include <stddef.h>

#define MSS 1000
#define HEADER_LEN 6
#define SEQUENCE_SPACE 1 << 16

typedef struct {
    uint16_t seq;
    uint8_t flags;
    uint8_t data[MSS];
    size_t data_len;
} Segment;

int segment_init(Segment* seg, uint16_t seq, uint16_t control, const uint8_t* data, size_t len);

// Serialization for the wire
int segment_to_bytes(Segment* seg, uint8_t** out_buffer);

// Deserialization (with checksum validation)
int segment_from_bytes(Segment* seg, uint8_t* buffer, size_t total_len);
