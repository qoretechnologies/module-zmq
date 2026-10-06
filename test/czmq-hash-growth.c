/* Copyright (C) 2026 Qore Technologies, s.r.o.; SPDX-License-Identifier: MIT */
#include <czmq.h>

/* Single-threaded fixture state: old keys reach this callback during rehash. */
static const char *inserting_key;
static size_t rehashed_keys;

static size_t count_hash(const void *key) {
    const char *text = (const char *)key;
    if (inserting_key && strcmp(text, inserting_key) != 0) {
        ++rehashed_keys;
    }
    size_t hash = 5381;
    for (const unsigned char *p = (const unsigned char *)text; *p; ++p) {
        hash = hash * 33 + *p;
    }
    return hash;
}

int main(void) {
    int status = 0;
    int values[10000];
    zhashx_t *table = zhashx_new();
    if (!table) {
        return 1;
    }
    zhashx_set_key_hasher(table, count_hash);
    for (int i = 0; i < 10000; ++i) {
        char key[40];
        snprintf(key, sizeof(key), "record-%d", i);
        values[i] = i;
        inserting_key = key;
        int rc = zhashx_insert(table, key, &values[i]);
        inserting_key = NULL;
        if (rc != 0) {
            status = 2;
            goto done;
        }
    }
    if (rehashed_keys == 0 || zhashx_size(table) != 10000) {
        fprintf(stderr, "CZMQ did not resize its hash table during 10000 insertions\n");
        status = 3;
        goto done;
    }
    /* Duplicate keys must remain rejected without replacing their values. */
    if (zhashx_insert(table, "record-1", &values[2]) != -1) {
        status = 4;
        goto done;
    }
    for (int i = 0; i < 10000; ++i) {
        char key[40];
        snprintf(key, sizeof(key), "record-%d", i);
        if (zhashx_lookup(table, key) != &values[i]) {
            status = 5;
            goto done;
        }
        zhashx_delete(table, key);
        if (zhashx_lookup(table, key) != NULL) {
            status = 6;
            goto done;
        }
    }
    if (zhashx_size(table) != 0) {
        status = 7;
        goto done;
    }
    printf("10000 hash insert/lookup/delete cases passed; %zu existing keys rehashed\n", rehashed_keys);
done:
    inserting_key = NULL;
    zhashx_destroy(&table);
    return status;
}
