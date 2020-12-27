#include "common.h"

int addrparse(const char *addrstr, const char *portstr, struct sockaddr_storage *storage) {
    if(addrstr == NULL || portstr == NULL)
        return -1;
    uint16_t port = (uint16_t) atoi(portstr);
    if(port == 0) 
        return -1;
    port = htons(port); // host to network short
 
    struct in_addr inaddr4; // 32-bit IP address
    if(inet_pton(AF_INET, addrstr, &inaddr4)) {
        struct sockaddr_in *addr4 = (struct sockaddr_in *) storage;
        addr4->sin_family = AF_INET;
        addr4->sin_port = port;
        addr4->sin_addr = inaddr4;
        return 0;
    }
 
    struct in6_addr inaddr6; // 128-bit IP address
    if(inet_pton(AF_INET6, addrstr, &inaddr6)) {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *) storage;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = port;
        // addr6->sin6_addr = inaddr6;
        memcpy(&(addr6->sin6_addr), &(inaddr6), sizeof(inaddr6));
        return 0;
    }
    return -1;
}

int addrtostr(const struct sockaddr *addr, char *str, size_t strsize) {

    int version;
    char addrstr[INET6_ADDRSTRLEN+1] = "";
    uint16_t port;

    if(addr->sa_family == AF_INET) {
        version = 4;
        struct sockaddr_in *addr4 = (struct sockaddr_in *) addr;
        if(!inet_ntop(AF_INET, &(addr4->sin_addr), addrstr, INET_ADDRSTRLEN+1))
            return -1;
        port = ntohs(addr4->sin_port);
    } 
    else if(addr->sa_family == AF_INET6) {
        version = 6;
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *) addr;
        if(!inet_ntop(AF_INET6, &(addr6->sin6_addr), addrstr, INET6_ADDRSTRLEN))
            return -1;
        port = ntohs(addr6->sin6_port);
    } 
    else 
        return -1;

    if(str)
        snprintf(str, strsize, "IPv%d %s %hu", version, addrstr, port);

    return 0;
}

int server_sockaddr_init(const char *proto, const char *portstr, struct sockaddr_storage *storage) {
    uint16_t port = (uint16_t) atoi(portstr);
    if(port == 0) 
        return -1;
    port = htons(port); // host to network short

    memset(storage, 0, sizeof(*storage));

    if(0 == strcmp(proto, "v4")) {
        struct sockaddr_in *addr4 = (struct sockaddr_in *) storage;
        addr4->sin_family = AF_INET;
        addr4->sin_addr.s_addr = INADDR_ANY;
        addr4->sin_port = port;
        return 0;
    }
    else if(0 == strcmp(proto, "v6")) {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *) storage;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_addr = in6addr_any;
        addr6->sin6_port = port;
        return 0;
    }
    else 
        return -1;
}

pair<int, set<string>> parse_msg(string const msg) {
    set<string> tags;

    string temp = msg;
    temp.replace(temp.find('\n'), 1, "\0");

    int category = S_COMMON;

    if(temp.find(MSG_KILL) != string::npos) {
        category = S_KILL;
    }
    else if(temp.length() > 1) {
        if(temp[0] == '+') {
            category = S_SUBSCRIBE;
            auto idx = temp.find(' ');
            if(idx != string::npos) {
                tags.insert(temp.substr(1, idx-1));
            }
            else {
                tags.insert(temp.substr(1));
            }
        }
        else if(temp[0] == '-') {
            category = S_UNSUBSCRIBE;
            auto idx = temp.find(' ');
            if(idx != string::npos) {
                tags.insert(temp.substr(1, idx-1));
            }
            else {
                tags.insert(temp.substr(1));
            }
        }
        else {
            size_t idx;
            while(temp.length() > 1) {
                idx = temp.find('#');
                if(idx == string::npos) {
                    break;
                }

                temp = temp.substr(idx);

                category = S_HASTAG;
                idx = temp.find(' ');
                if(idx != string::npos) {
                    tags.insert(temp.substr(1, idx-1));
                    temp = temp.substr(idx);
                }
                else {
                    tags.insert(temp.substr(1));
                    break;
                }
            }
        }
    }

    return make_pair(category, tags);
}