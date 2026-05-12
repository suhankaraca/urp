#include "segment.h"

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>

uint16_t compute_checksum(uint8_t* buffer, size_t total_len);

int segment_init(segment_t* seg, uint16_t seq, uint8_t flags, const uint8_t* data, size_t data_len) {
    if ((flags & ~0x07) != 0) {
        fprintf(stderr ,"Invalid control field: %d\n", flags);
        return -1;
    }
    if (data_len > MSS) {
        fprintf(stderr ,"Data length exceeds MSS: %d\n", (int)data_len);
        return -1;
    } 
    seg->seq = seq;
    seg->flags = flags;
    for (int i = 0; i < data_len; i++) {
        seg->data[i] = data[i];
    }
    seg->data_len = data_len;
    return 0;
}

int segment_to_bytes(segment_t* seg, uint8_t** out_buffer) {
    size_t total_len = HEADER_LEN + seg->data_len;

    uint8_t* buffer = (uint8_t*)malloc(total_len);
    if (buffer == NULL) {
        return -1;
    }

    uint16_t seq_net = htons(seg->seq & 0xFFFF);
    memcpy(buffer, &seq_net, 2);

    uint16_t control_net = htons(seg->flags & 0x7);
    memcpy(buffer + 2, &control_net, 2);

    uint16_t zero = 0;
    memcpy(buffer + 4, &zero, 2);

    if (seg->data_len != 0) {
        memcpy(buffer + HEADER_LEN, seg->data, seg->data_len);
    }

    uint16_t checksum = compute_checksum(buffer, total_len);
    uint16_t checksum_net = htons(checksum & 0xFFFF);
    memcpy(buffer + 4, &checksum, 2);

    *out_buffer = buffer;
    return 0;
}

int segment_from_bytes(segment_t* seg, uint8_t* buffer, size_t total_len) {
    if (total_len < HEADER_LEN) {
        return -1;
    }

    uint16_t seq;
    memcpy(&seq, buffer, 2);
    seq = ntohs(seq); 

    uint16_t control;
    memcpy(&control, buffer + 2, 2);
    control = ntohs(control);

    uint16_t reserved = (control >> 3) & 0x1FFF; // upper 13 bits
    uint8_t flags = control & 0x7;   // lower 3 bits
    if (reserved != 0) {
        return -1;
    }

    uint16_t checksum = compute_checksum(buffer, total_len);
    if (checksum != 0) {
        // Invalid checksum
        return -1;
    }

    seg->seq = seq;
    seg->flags = flags;
    size_t data_len = total_len - HEADER_LEN;
    memcpy(seg->data, buffer + HEADER_LEN, data_len);
    seg->data_len = data_len;
    return 0;
}

uint16_t compute_checksum(uint8_t* buffer, size_t total_len) {
    int sum = 0;

    // Sum all 16-bit words
    for (int i = 0; i < total_len; i += 2) {
        int word = (buffer[i] & 0xFF) << 8;
        if (i + 1 < total_len) {
            word |= buffer[i + 1] & 0xFF;
        }
        sum += word;
    }

    // Add carries
    while ((sum >> 16) != 0) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    // One's complement
    return (uint16_t) ~sum;
}
