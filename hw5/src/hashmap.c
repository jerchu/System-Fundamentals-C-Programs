#include "utils.h"
#include <errno.h>

#define MAP_KEY(base, len) (map_key_t) {.key_base = base, .key_len = len}
#define MAP_VAL(base, len) (map_val_t) {.val_base = base, .val_len = len}
#define MAP_NODE(key_arg, val_arg, tombstone_arg) (map_node_t) {.key = key_arg, .val = val_arg, .tombstone = tombstone_arg}

hashmap_t *create_map(uint32_t capacity, hash_func_f hash_function, destructor_f destroy_function) {
    if(!capacity || !hash_function || !destroy_function){
        errno = EINVAL;
        return NULL;
    }
    hashmap_t *new_hashmap = calloc(1, sizeof(hashmap_t));
    if(!new_hashmap)
        return NULL;
    new_hashmap->capacity = capacity;
    new_hashmap->nodes = calloc(capacity, sizeof(map_node_t));
    if(!new_hashmap)
        return NULL;
    new_hashmap->hash_function = hash_function;
    new_hashmap->destroy_function = destroy_function;
    if(pthread_mutex_init(&new_hashmap->write_lock, NULL) < 0)
        return NULL;
    if(pthread_mutex_init(&new_hashmap->fields_lock, NULL) < 0)
        return NULL;
    return new_hashmap;
}

bool put(hashmap_t *self, map_key_t key, map_val_t val, bool force) {
    if(!self || !key || !val || self->invalid){
        errno = EINVAL;
        return false;
    }
    if(self->capacity == self->size && !force){
        errno = ENOMEM;
        return false;
    }
    pthread_mutex_lock(&self->write_lock);
    while(self->num_readers > 0);
    pthread_mutex_lock(&self->fields_lock);
    int start_index = get_index(self, key);
    if(self->capacity == self->size && force){
        self->nodes[index]->val = val;
        self->size--;
    }
    else if(!can_put_key(self->nodes[start_index], key))
    {
        int index = (start_index+1)%self->capacity;
        while(index != start_index && !can_put_key(self->nodes[index], key))
        {
            index = (index+1)%self->capacity;
        }
        if(index != start_index){
            self->nodes[index]->val = val;
        }
        else{
            while(!self->nodes[index]->tombstone){
                index = (index+1)%self->capacity;
            }
            self->nodes[index]->val = val;
        }
    }
    else{
        self->nodes[index]->val = val;
    }
    self->size++;
    pthread_mutex_unlock(&self->fields_lock);
    pthread_mutex_unlock(&self->write_lock);
    return true;
}

map_val_t get(hashmap_t *self, map_key_t key) {
    return MAP_VAL(NULL, 0);
}

map_node_t delete(hashmap_t *self, map_key_t key) {
    return MAP_NODE(MAP_KEY(NULL, 0), MAP_VAL(NULL, 0), false);
}

bool clear_map(hashmap_t *self) {
	return false;
}

bool invalidate_map(hashmap_t *self) {
    return false;
}

bool can_put_key(map_node_t *node, map_key_t key){
    return node_is_empty(node) || node_has_key(node, key);
}

bool node_has_key(map_node_t *node, map_key_t key){
    return !node->tombstone && get_keys_equal(node->key, key);
}

bool node_is_empty(map_node_t *node){
    return node->key == NULL && !node->tombstone;
}

bool get_keys_equal(map_key_t key1, map_key_t key2){
    return key1->key_base == key2->key_base && key1->key_len == key2->key_len;
}
