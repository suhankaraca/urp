#include "server.h"
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

session_t* sessions[MAX_SENDERS];
pthread_mutex_t sessions_mutex = PTHREAD_MUTEX_INITIALIZER;

int init_session_table() {
    for (int i = 0; i < MAX_SENDERS; i++) {
        sessions[i] = NULL;
    }
    return 0;
}

session_t* get_session(struct sockaddr_in* client_addr) {
    pthread_mutex_lock(&sessions_mutex);

    // sequential search to find session
    for (int i = 0; i < MAX_SENDERS; i++) {
        if (sessions[i] && 
            sessions[i]->client_addr.sin_addr.s_addr == client_addr->sin_addr.s_addr &&
            sessions[i]->client_addr.sin_port == client_addr->sin_port) {
            pthread_mutex_unlock(&sessions_mutex);
            return sessions[i];
        }
    }

    // if session does not exist, create new session
    for (int i = 0; i < MAX_SENDERS; i++) {
        if (sessions[i] == NULL) {
            sessions[i] = (session_t*)malloc(sizeof(session_t));
            sessions[i]->client_addr = *client_addr;
            sessions[i]->segment_queue = queue_create();
            sessions[i]->is_running = false;
            sessions[i]->next_expected_seq = 0;
            sessions[i]->table_index = i;
            pthread_mutex_unlock(&sessions_mutex);
            return sessions[i];
        }
    }

    // if session does not exist and all slots are full, return null
    pthread_mutex_unlock(&sessions_mutex);
    return NULL;
}

void send_ack(session_t* s, uint16_t seq) {
    segment_t ack_seg;
    segment_init(&ack_seg, seq, FLAG_ACK, NULL, 0);
    uint8_t* buffer;
    segment_to_bytes(&ack_seg, &buffer);
    sendto(s->sockfd, buffer, HEADER_LEN, 0, (struct sockaddr*)&s->client_addr, sizeof(s->client_addr));
    free(buffer);
}

/* 
 * Thread runs until FIN. When FIN is received, session is cleaned up from session_table.
*/
void* session_worker_thread(void *arg) {
    session_t *s = (session_t *)arg;
    char ip4[INET_ADDRSTRLEN];
    printf("Session thread started for %s:%d\n", inet_ntop(AF_INET, &(s->client_addr.sin_addr), ip4, INET_ADDRSTRLEN), ntohs(s->client_addr.sin_port));
    
    while (s->is_running) {
        segment_t *seg = dequeue_segment(s->segment_queue);
        if (!seg) continue;

        if (seg->flags & FLAG_SYN) {
            s->next_expected_seq = seg->seq + 1;
            send_ack(s, s->next_expected_seq);
            printf("SYN received, next expected: %d\n", s->next_expected_seq);
        } else if (seg->flags & FLAG_FIN) {
            send_ack(s, seg->seq + 1);
            printf("FIN received, session closing\n");
            s->is_running = false;
        } else {
            if (seg->seq == s->next_expected_seq) {
                printf("Received expected segment %d, data: %.*s\n", seg->seq, (int)seg->data_len, seg->data);
                s->next_expected_seq += seg->data_len;
                send_ack(s, s->next_expected_seq);
            } else {
                printf("Received out-of-order segment %d (expected %d)\n", seg->seq, s->next_expected_seq);
                send_ack(s, s->next_expected_seq);
            }
        }
        free(seg);
    }
    return NULL;
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    uint8_t buffer[2000]; 

    init_session_table();

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        return -1;
    }

    printf("Server listening on port 8080...\n");

    while (1) { 
        ssize_t bytes_received = recvfrom(sockfd, buffer, sizeof(buffer), 0, 
                                         (struct sockaddr *)&client_addr, &addr_len);

        if (bytes_received < HEADER_LEN) continue;

        segment_t incoming_seg;
        if (segment_from_bytes(&incoming_seg, buffer, bytes_received) != 0) {
            fprintf(stderr, "Invalid segment received\n");
            continue;
        }

        session_t *s = get_session(&client_addr);
        if (!s) {
            fprintf(stderr, "Max senders reached or session creation failed\n");
            continue;
        }

        s->sockfd = sockfd;
        if (!s->is_running) {
            s->is_running = true;
            pthread_create(&s->thread_handle, NULL, session_worker_thread, s);
            pthread_detach(s->thread_handle);
        }
        enqueue_segment(s->segment_queue, &incoming_seg);
    }

    close(sockfd);
    return 0;
}
