#pragma once

#include <string>
#include <vector>
#include <queue>
#include <set>
#include <unordered_map>
#include <iostream>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFSZ 500
#define DELIMITER '\n'

using namespace std;

typedef struct client_data {
	int csock;
	set<string> tags;
	struct sockaddr_storage storage;
} cdata_t;

int addrparse(const char *addrstr, const char *portstr, struct sockaddr_storage *storage);

int addrtostr(const struct sockaddr *addr, char *str, size_t strsize);

int server_sockaddr_init(const char *proto, const char *portstr, struct sockaddr_storage *storage);