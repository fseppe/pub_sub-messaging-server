#pragma once

#include <string>
#include <vector>
#include <queue>
#include <tuple>
#include <set>
#include <map>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFSZ 500

#define S_COMMON 0
#define S_SUBSCRIBE 1
#define S_UNSUBSCRIBE 2
#define S_HASTAG 3
#define S_KILL 4

#define MSG_KILL "##kill"

using namespace std;

typedef struct client_data {
    int csock;
    vector<string> tags;
    struct sockaddr_storage storage;
} cdata_t;

typedef struct entry {
    string tag;
    cdata_t *cdata;
} entry_t;

void logexit(char const *msg);

int addrparse(const char *addrstr, const char *portstr, struct sockaddr_storage *storage);

int addrtostr(const struct sockaddr *addr, char *str, size_t strsize);

int server_sockaddr_init(const char *proto, const char *portstr, struct sockaddr_storage *storage);

int find_char_idx(const char *str, const char s);

tuple<int, set<string>> parse_msg(char const *msg);

int match(char const *text, char const *pattern);

entry_t *init_entry(string tag, cdata_t *cdata);