#include "utils.h"
#include "debug.h"
#include <errno.h>
#include <string.h>

#define MAP_KEY(base, len) (map_key_t) {.key_base = base, .key_len = len}
#define MAP_VAL(base, len) (map_val_t) {.val_base = base, .val_len = len}
#define MAP_NODE(key_arg, val_arg, tombstone_arg) (map_node_t) {.key = key_arg, .val = val_arg, .tombstone = tombstone_arg}

bool can_put_key(map_node_t node, map_key_t key);
bool node_has_key(map_node_t node, map_key_t key);
bool node_is_empty(map_node_t node);
bool get_keys_equal(map_key_t key1, map_key_t key2);
bool get_vals_equal(map_val_t val1, map_val_t val2);

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
    if(!self || get_keys_equal(key, MAP_KEY(NULL, 0)) || get_vals_equal(val, MAP_VAL(NULL, 0)) || self->invalid){
        errno = EINVAL;
        return false;
    }
    if(self->capacity == self->size && !force){
        errno = ENOMEM;
        return false;
    }
    pthread_mutex_lock(&self->write_lock);
    while(self->num_readers > 0);
    self->num_readers = -1;
    pthread_mutex_lock(&self->fields_lock);
    if(self->invalid){
        self->num_readers = -1;
        pthread_mutex_unlock(&self->fields_lock);
        pthread_mutex_unlock(&self->write_lock);
        errno = EINVAL;
        return false;
    }
    int start_index = get_index(self, key);
    if(self->capacity == self->size && force){
        self->nodes[start_index].key = key;
        self->nodes[start_index].val = val;
        self->nodes[start_index].tombstone = false;
        self->size--;
    }
    else if(!can_put_key(self->nodes[start_index], key))
    {
        map_key_t bad_key = self->nodes[start_index].key;
        debug("node with key (%p, %lu) and tombstone = %d was not empty\n",bad_key.key_base, bad_key.key_len, (int)self->nodes[start_index].tombstone );
        map_node_t *first_tombstone = NULL;
        if(!first_tombstone && self->nodes[start_index].tombstone){
            first_tombstone = &self->nodes[start_index];
        }
        int index = (start_index+1)%self->capacity;
        while(index != start_index && !can_put_key(self->nodes[index], key))
        {
            if(!first_tombstone && self->nodes[index].tombstone){
                first_tombstone = &self->nodes[index];
            }
            index = (index+1)%self->capacity;
        }
        if(index != start_index){
            if(node_is_empty(self->nodes[index])){
                self->nodes[index].key = key;
                self->nodes[index].tombstone = false;
            }
            self->nodes[index].val = val;
        }
        else{
            first_tombstone->key = key;
            first_tombstone->val = val;
            first_tombstone->tombstone = false;
        }
    }
    else{
        self->nodes[start_index].val = val;
    }
    self->size++;
    self->num_readers = 0;
    pthread_mutex_unlock(&self->fields_lock);
    pthread_mutex_unlock(&self->write_lock);
    return true;
}

map_val_t get(hashmap_t *self, map_key_t key) {
    if(!self || get_keys_equal(key, MAP_KEY(NULL, 0)) || self->invalid){
        errno = EINVAL;
        return MAP_VAL(NULL, 0);
    }
    while(self->num_readers < 0);
    pthread_mutex_lock(&self->fields_lock);
    if(self->invalid){
        pthread_mutex_unlock(&self->fields_lock);
        errno = EINVAL;
        return MAP_VAL(NULL, 0);
    }
    self->num_readers++;
    pthread_mutex_unlock(&self->fields_lock);
    map_val_t ret_val = MAP_VAL(NULL, 0);
    int start_index = get_index(self, key);
    if(node_is_empty(self->nodes[start_index])){
        ret_val = MAP_VAL(NULL, 0);
    }
    else if(node_has_key(self->nodes[start_index], key)){
        ret_val = self->nodes[start_index].val;
    }
    else{
        int index = (start_index+1)%self->capacity;
        while(index != start_index && !node_is_empty(self->nodes[index]) && !node_has_key(self->nodes[index], key)){
            index = (index+1)%self->capacity;
        }
        if(node_has_key(self->nodes[index], key)){
            ret_val = self->nodes[index].val;
        }
    }
    pthread_mutex_lock(&self->fields_lock);
    if(self->invalid){
        self->num_readers = 0;
        pthread_mutex_unlock(&self->fields_lock);
        errno = EINVAL;
        return MAP_VAL(NULL, 0);
    }
    self->num_readers--;
    pthread_mutex_unlock(&self->fields_lock);
    return ret_val;
}

map_node_t delete(hashmap_t *self, map_key_t key) {
    if(!self || get_keys_equal(key, MAP_KEY(NULL, 0)) || self->invalid){
        errno = EINVAL;
        return MAP_NODE(MAP_KEY(NULL, 0), MAP_VAL(NULL, 0), false);
    }
    pthread_mutex_lock(&self->write_lock);
    while(self->num_readers > 0);
    self->num_readers = -1;
    pthread_mutex_lock(&self->fields_lock);
    if(self->invalid){
        self->num_readers = 0;
        pthread_mutex_unlock(&self->fields_lock);
        pthread_mutex_unlock(&self->write_lock);
        errno = EINVAL;
        return MAP_NODE(MAP_KEY(NULL, 0), MAP_VAL(NULL, 0), false);
    }
    map_node_t ret_node = MAP_NODE(MAP_KEY(NULL, 0), MAP_VAL(NULL, 0), false);
    int start_index = get_index(self, key);
    if(node_is_empty(self->nodes[start_index])){
        ret_node = MAP_NODE(MAP_KEY(NULL, 0), MAP_VAL(NULL, 0), false);
    }
    else if(node_has_key(self->nodes[start_index], key)){
        ret_node = self->nodes[start_index];
        self->nodes[start_index].tombstone = true;
    }
    else{
        int index = (start_index+1)%self->capacity;
        while(index != start_index && !node_is_empty(self->nodes[index]) && !node_has_key(self->nodes[index], key)){
            index = (index+1)%self->capacity;
        }
        if(node_has_key(self->nodes[index], key)){
            ret_node = self->nodes[index];
            self->nodes[start_index].tombstone = true;
        }
    }
    self->size--;
    self->num_readers = 0;
    pthread_mutex_unlock(&self->fields_lock);
    pthread_mutex_unlock(&self->write_lock);
    return ret_node;
}

bool clear_map(hashmap_t *self) {
    if(!self || self->invalid){
        errno = EINVAL;
        return false;
    }
    pthread_mutex_lock(&self->write_lock);
    while(self->num_readers > 0);
    self->num_readers = -1;
    pthread_mutex_lock(&self->fields_lock);
    if(self->invalid){
        pthread_mutex_unlock(&self->fields_lock);
        pthread_mutex_unlock(&self->write_lock);
        errno = EINVAL;
        return false;
    }
    for(int i = 0; i < self->capacity; i++){
        self->destroy_function(self->nodes[i].key, self->nodes[i].val);
        self->nodes[i].key = MAP_KEY(NULL, 0);
        self->nodes[i].val = MAP_VAL(NULL, 0);
        self->nodes[i].tombstone = false;
    }
    self->size = 0;
    self->num_readers = 0;
    pthread_mutex_unlock(&self->fields_lock);
    pthread_mutex_unlock(&self->write_lock);
	return true;
}

bool invalidate_map(hashmap_t *self) {
    if(!self || self->invalid){
        errno = EINVAL;
        return false;
    }
    pthread_mutex_lock(&self->write_lock);
    while(self->num_readers > 0);
    self->num_readers = -1;
    pthread_mutex_lock(&self->fields_lock);
    if(self->invalid){
        pthread_mutex_unlock(&self->fields_lock);
        pthread_mutex_unlock(&self->write_lock);
        errno = EINVAL;
        return false;
    }
    for(int i = 0; i < self->capacity; i++){
        self->destroy_function(self->nodes[i].key, self->nodes[i].val);
        self->nodes[i].key = MAP_KEY(NULL, 0);
        self->nodes[i].val = MAP_VAL(NULL, 0);
        self->nodes[i].tombstone = false;
    }
    free(self->nodes);
    self->size = 0;
    self->capacity = 0;
    self->num_readers = 0;
    self->invalid = true;
    pthread_mutex_unlock(&self->fields_lock);
    pthread_mutex_unlock(&self->write_lock);
    return true;
}

bool can_put_key(map_node_t node, map_key_t key){
    return node_is_empty(node) || node_has_key(node, key);
}

bool node_has_key(map_node_t node, map_key_t key){
    return !node.tombstone && get_keys_equal(node.key, key);
}

bool node_is_empty(map_node_t node){
    return get_keys_equal(node.key, MAP_KEY(NULL, 0)) && !node.tombstone;
}

bool get_keys_equal(map_key_t key1, map_key_t key2){
    return key1.key_len == key2.key_len && (key1.key_base == key2.key_base ||
    memcmp(key1.key_base, key2.key_base, key1.key_len));
}

bool get_vals_equal(map_val_t val1, map_val_t val2){
    return val1.val_len == val2.val_len && (val1.val_base == val2.val_base ||
     memcmp(val1.val_base, val2.val_base, val1.val_len));
}
