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

int find_char_idx(const char *str, const char s) {
    // return -1 index value if char was not found. otherwise will return the index
    for (size_t  i = 0; i < strlen(str); i++)
        if(str[i] == s)
            return i;
    return -1;
}

int match(char const *text, char const *pattern) {
    int c, d, e, text_length, pattern_length, position = -1;
    text_length    = strlen(text);
    pattern_length = strlen(pattern);

    if (pattern_length > text_length) 
        return -1;

    for (c = 0; c <= text_length - pattern_length; c++) {
        position = e = c;

        for (d = 0; d < pattern_length; d++) {
            if (pattern[d] == text[e]) 
                e++;
            else 
                break;
        }

        if (d == pattern_length) 
            return position;
    }

    return -1;
}

tuple<int, set<string>> parse_msg(char const *msg) {

    set<string> tags;
    
    char *temp = (char *) malloc(sizeof(char) * BUFSZ);
    char *tag = (char *)  malloc(sizeof(char) * BUFSZ);

    /*  como msgs sao terminadas em \n, procura-se pela primeira ocorrencia 
        e substitui por \0  */
    for(size_t i = 0; i < strlen(msg); i++) {
        if(msg[i] == '\n') {
            temp[i] = '\0';
            break;
        }
        else
            temp[i] = msg[i];
    }

    int category = S_COMMON;
    
    if(match(temp, MSG_KILL) != -1)
        category = S_KILL;

    else if(strlen(temp) > 1) {
        if(temp[0] == '+') {
            category = S_SUBSCRIBE;
            int idx = find_char_idx(temp, ' ');
            if (idx != -1) {
                strncpy(tag, temp+1, (size_t) idx);
                tags.insert(tag);
            }
            else {
                strcpy(tag, temp+1);
                tags.insert(tag);
            }
        }
        else if(temp[0] == '-') {
            category = S_UNSUBSCRIBE;
            int idx = find_char_idx(temp, ' ');
            if (idx != -1) {
                strncpy(tag, temp+1, (size_t) idx);
                tags.insert(tag);
            }
            else {
                strcpy(tag, temp+1);
                tags.insert(tag);
            }
        }
        else {
            int first_idx, last_idx, itr_idx = 0;
            while((size_t) itr_idx < strlen(temp)) {
                first_idx = find_char_idx(temp+itr_idx, '#');
                if(first_idx == -1)
                    break;
                    
                first_idx += itr_idx + 1;
                category = S_HASTAG;

                last_idx = find_char_idx(temp+first_idx, ' ');
                if (last_idx != -1) {
                    last_idx += first_idx;
                    strncpy(tag, temp+first_idx, (size_t) last_idx-first_idx);
                    tags.insert(tag);
                    itr_idx = last_idx+1;
                }
                else {
                    strcpy(tag, temp+first_idx);
                    tags.insert(tag);
                    break;
                }
            }
        }
    }
    free(temp);
    free(tag);
    return make_tuple(category, tags);
}

entry_t *init_entry(string tag, cdata_t *cdata) {
    entry_t *entry = new entry_t();
    entry->tag = tag;
    entry->cdata = cdata;
    return entry;
}
