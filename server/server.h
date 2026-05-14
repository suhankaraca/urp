#pragma once


#include "../segment/segment.h"
#include "queue.h"
#include <stdint.h>
#include <stdbool.h>
#include <netinet/in.h>  // This defines sockaddr_in
#include <arpa/inet.h>   // For inet_ntop, etc.
#include <sys/socket.h>  // For general socket definitions
#include <pthread.h>

#define MAX_SENDERS 10
#define RECEIVE_WINDOW_SIZE 65535
#define TIMEOUT_MS 500

typedef struct {
    struct sockaddr_in client_addr;
    queue_t* segment_queue;
    bool is_running;
    uint16_t next_expected_seq;
    int sockfd; // To send ACKs
    
    pthread_t thread_handle;
    size_t table_index; // for self-cleanup
} session_t;

int init_session_table();
session_t* get_session(struct sockaddr_in* client_addr);
void* session_worker_thread(void *arg);
