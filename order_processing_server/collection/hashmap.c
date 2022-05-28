#include <stdlib.h>
#include <string.h>
#include "node.h"
#include "hashmap.h"

#define GET_INDEX_BY_KEY(key) (hashmap->hash_func(key) % hashmap->length);


static node_t* get_node_or_null(node_t** phead, const char* str)
{
    node_t** pp = phead;
    while (*pp != NULL) {
        if (strcmp((*pp)->key, str) == 0) {
            break;
        }
        pp = &(*pp)->next;
    }

    return *pp;
}

hashmap_t* init_hashmap_malloc(size_t length, size_t (*p_hash_func)(const char* key))
{
    hashmap_t* hashmap = malloc(sizeof(hashmap_t));

    hashmap->hash_func = p_hash_func;
    hashmap->length = length;
    hashmap->plist = malloc(sizeof(node_t*) * length);

    memset(hashmap->plist, 0, sizeof(node_t*) * length);

    return hashmap;
}

int add_key(hashmap_t* hashmap, const char* key, const int value)
{
    node_t* new_node;
    node_t** phead;

    unsigned int hashed = GET_INDEX_BY_KEY(key);

    if (strcmp(key, "") == 0) {
        return FALSE;
    }

    phead = &(hashmap->plist[hashed]);
    while (*phead != NULL) {
        if (strcmp((*phead)->key, key) == 0) {
            return FALSE;
        }
        phead = &(*phead)->next;
    }

    new_node = malloc(sizeof(node_t));
    new_node->key = malloc(strlen(key) + 1);
    strcpy(new_node->key, key);
    new_node->value = value;
    new_node->next = NULL;

    *phead = new_node;

    return TRUE;
}

int get_value(const hashmap_t* hashmap, const char* key)
{
    unsigned int hashed = hashmap->hash_func(key) % hashmap->length;
    node_t* head = hashmap->plist[hashed];
    node_t* node;

    if (head == NULL) {
        return -1;
    }

    node = get_node_or_null(&head, key);
    if (node == NULL) {
        return -1;
    }

    return node->value;
}

int update_value(hashmap_t* hashmap, const char* key, const int value)
{
    unsigned int hashed = hashmap->hash_func(key) % hashmap->length;
    node_t* head = hashmap->plist[hashed];
    node_t* node;

    if (head == NULL) {
        return FALSE;
    }

    node = get_node_or_null(&head, key);
    if (node == NULL) {
        return FALSE;
    }

    node->value = value;

    return TRUE;
}

int remove_key(hashmap_t* hashmap, const char* key)
{
    unsigned int hashed = hashmap->hash_func(key) % hashmap->length;
    node_t** pp = &hashmap->plist[hashed];
    node_t* trash;

    while (*pp != NULL) {        
        if (strcmp((*pp)->key, key) == 0) {
            break;
        }
        pp = &((*pp)->next);
    }

    if (*pp == NULL) {
        return FALSE;
    }

    trash = *pp;
    *pp = (*pp)->next;
    free(trash->key);
    free(trash);

    return TRUE;
}

void destroy(hashmap_t* hashmap)
{
    node_t** plist = hashmap->plist;
    size_t length = hashmap->length;
    size_t i;

    for (i = 0; i < length; ++i) {
        node_t* node = plist[i];
        while (node != NULL) {
            node_t* next = node->next;

            free(node->key);
            free(node);

            node = next;
        }
    }

    free(hashmap->plist);
    free(hashmap);
}
