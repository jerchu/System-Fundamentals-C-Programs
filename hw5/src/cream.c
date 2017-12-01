#include "cream.h"
#include "csapp.h"
#include "debug.h"
#include "hashmap.h"
#include "queue.h"
#include "utils.h"
#include <string.h>
#include <stdlib.h>

void map_free_function(map_key_t key, map_val_t val);
void *worker_thread(void *vargsp);

queue_t *requests;
hashmap_t *cache;

int main(int argc, char *argv[]) {

    signal(SIGPIPE, SIG_IGN);

    int NUM_WORKERS;
    //int PORT_NUMBER;
    int MAX_ENTRIES;
    int listenfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;


    for(int i = 1; i < argc; i++){
        if(argc < 2 || strcmp(argv[i], "-h") == 0 ){
            printf("Usage: cream [-h] NUM_WORKERS PORT_NUMBER MAX_ENTRIES\n"
                "-h                 Displays this help menu and returns EXIT_SUCCESS.\n"
                "NUM_WORKERS        The number of worker threads used to service requests.\n"
                "PORT_NUMBER        Port number to listen on for incoming connections.\n"
                "MAX_ENTRIES        The maximum number of entries that can be stored in `cream`'s underlying data store.\n"
            );
            exit(0);
        }
    }
    if(argc < 4){
        printf("not enough arguments\n");
        exit(1);
    }
    if(strspn(argv[1], "1234567890") == strlen(argv[1])){
        NUM_WORKERS = atoi(argv[1]);
    }
    else{
        printf("%s is not a valid worker count\n", argv[1]);
        exit(1);
    }
    if(strspn(argv[2], "1234567890") == strlen(argv[2])){
        //PORT_NUMBER = atoi(argv[3]);
    }
    else{
        printf("%s is not a valid port number\n", argv[2]);
        exit(1);
    }
    if(strspn(argv[3], "1234567890") == strlen(argv[3])){
        MAX_ENTRIES = atoi(argv[3]);
    }
    else{
        printf("%s is not a valid entry count\n", argv[3]);
        exit(1);
    }
    requests = create_queue();
    for(int i = 0; i < NUM_WORKERS; i++){
        pthread_t tid;
        pthread_create(&tid, NULL, worker_thread, NULL);
    }
    cache = create_map(MAX_ENTRIES, jenkins_one_at_a_time_hash, map_free_function);
    listenfd = Open_listenfd(argv[2]);
    while(1){
        clientlen = sizeof(struct sockaddr_storage);
        int *connfd = malloc(sizeof(int));
        *connfd = Accept(listenfd, (SA *) &clientaddr, &clientlen);
        enqueue(requests, connfd);
    }
    exit(0);
}

void map_free_function(map_key_t key, map_val_t val) {
    free(key.key_base);
    free(val.val_base);
}

void *worker_thread(void *vargsp){
    while(1){
        int connfd = *((int *)dequeue(requests));
        debug("connfd: %d", connfd);
        request_header_t req_header;
        response_header_t res_header = {UNSUPPORTED, 0};
        map_val_t value;
        Rio_readn(connfd, &req_header, sizeof(request_header_t));
        if(req_header.key_size > MAX_KEY_SIZE || req_header.key_size < MIN_KEY_SIZE ||
            req_header.value_size > MAX_VALUE_SIZE || req_header.value_size < MIN_VALUE_SIZE)
        {
            //tell client it failed
        }
        void *key = calloc(1, req_header.key_size);
        Rio_readn(connfd, key, req_header.key_size);
        void *val = calloc(1, req_header.value_size);
        Rio_readn(connfd, val, req_header.value_size);
        if(req_header.request_code == PUT){
            if(put(cache, MAP_KEY(key, req_header.key_size), MAP_VAL(val, req_header.value_size), true))
                res_header.response_code = OK;
            else
                res_header.response_code = BAD_REQUEST;
        }
        else if(req_header.request_code == GET){
            value = get(cache, MAP_KEY(key, req_header.key_size));
            if(value.val_base && value.val_len){
                res_header.response_code = OK;
                res_header.value_size = value.val_len;
            }
            else
                res_header.response_code = NOT_FOUND;
        }
        else if(req_header.request_code == EVICT){
            delete(cache, (MAP_KEY(key, req_header.key_size)));
            res_header.response_code = OK;
        }
        else if(req_header.request_code == CLEAR){
            clear_map(cache);
            res_header.response_code = OK;
        }
        else{
            res_header.response_code = UNSUPPORTED;
        }
        Rio_writen(connfd, &res_header, sizeof(response_header_t));
        if(res_header.value_size > 0)
            Rio_writen(connfd, value.val_base, value.val_len);
        Close(connfd);
        debug("%s\n", "Task Complete");
    }
    return NULL;
}