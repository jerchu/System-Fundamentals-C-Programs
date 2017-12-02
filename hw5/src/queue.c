#include "queue.h"
#include <errno.h>

int Pthread_mutex_lock(pthread_mutex_t *mutex);
int Pthread_mutex_unlock(pthread_mutex_t *mutex);
int Sem_wait(sem_t *sem);
int Sem_post(sem_t *sem);

queue_t *create_queue(void) {
    queue_t *new_queue = calloc(1,sizeof(queue_t));
    if(!new_queue)
        return NULL;
    new_queue->front = calloc(1,sizeof(queue_node_t));
    new_queue->rear = new_queue->front;
    sem_init(&new_queue->items, 0, 0);
    if(pthread_mutex_init(&new_queue->lock, NULL) < 0)
        return NULL;
    new_queue->invalid = false;
    return new_queue;
}

bool invalidate_queue(queue_t *self, item_destructor_f destroy_function) {
    if(!self || !destroy_function){
        errno = EINVAL;
        return false;
    }
    Pthread_mutex_lock(&self->lock);
    if(self->invalid){
        errno = EINVAL;
        Pthread_mutex_unlock(&self->lock);
        return false;
    }
    queue_node_t *curr_node = self->rear;
    while(curr_node != NULL){
        queue_node_t *next_node = curr_node->next;
        destroy_function(curr_node->item);
        free(curr_node);
        curr_node = next_node;
    }
    self->invalid = true;
    Pthread_mutex_unlock(&self->lock);
    return true;
}

bool enqueue(queue_t *self, void *item) {
    if(!self || !item){
        errno = EINVAL;
        return false;
    }
    Pthread_mutex_lock(&self->lock);
    if(self->invalid){
        errno = EINVAL;
        Pthread_mutex_unlock(&self->lock);
        return false;
    }
    self->front->item = item;
    self->front->next = calloc(1, sizeof(queue_node_t));
    self->front = self->front->next;
    Sem_post(&self->items);
    Pthread_mutex_unlock(&self->lock);
    return true;
}

void *dequeue(queue_t *self) {
    if(!self){
        errno = EINVAL;
        return false;
    }
    Sem_wait(&self->items);
    Pthread_mutex_lock(&self->lock);
    if(self->invalid){
        errno = EINVAL;
        Pthread_mutex_unlock(&self->lock);
        return false;
    }
    void *item = self->rear->item;
    queue_node_t *next_node = self->rear->next;
    free(self->rear);
    self->rear = next_node;
    Pthread_mutex_unlock(&self->lock);
    return item;
}

/*int Pthread_mutex_lock(pthread_mutex_t *mutex){
    while(Pthread_mutex_lock(mutex) < 0){
        if(errno != EINTR){
            return -1;
        }
    }
    return 0;
}
int Pthread_mutex_unlock(pthread_mutex_t *mutex){
    while(pthread_mutex_unlock(mutex) < 0){
        if(errno != EINTR){
            return -1;
        }
    }
    return 0;
}

int Sem_wait(sem_t *sem) {
    while(sem_wait(sem) < 0){
        if(errno != EINTR){
            return -1;
        }
    }
    return 0;
}

int Sem_post(sem_t *sem) {
    while(sem_post(sem) < 0){
        if(errno != EINTR){
            return -1;
        }
    }
    return 0;
}*/
