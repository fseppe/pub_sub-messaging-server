#include "common.h"

#include <pthread.h>

void p_exit(int value) {
    exit(value);
}

void *input_thread(void *data) {
    cdata_t *cdata = (cdata_t *) data;
    char buf[BUFSZ];
    size_t count;
    while(1) {
        memset(buf, 0, BUFSZ);
        printf("> ");
        fgets(buf, BUFSZ, stdin);
        count = send(cdata->csock, buf, strlen(buf)+1, 0);
        if(count != strlen(buf)+1) {
            printf("[log] send error\n");
            p_exit(EXIT_FAILURE);
        }
    }
    free(buf);
    pthread_exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    if(argc < 3) 
        return -1;

    struct sockaddr_storage storage;
    if(0 != addrparse(argv[1], argv[2], &storage)) 
        return -1;

    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if(s == -1) {
        printf("[log] socket error\n");
        p_exit(EXIT_FAILURE);
    }

    struct sockaddr *addr = (struct sockaddr*) (&storage);
 
    if(0 != connect(s, addr, sizeof(storage))) {
        printf("[log] connect error\n");
        p_exit(EXIT_FAILURE);
    }

    char addrstr[BUFSZ];
    if(0 != addrtostr(addr, addrstr, BUFSZ)){
        printf("[log] addrtostr error\n");
        p_exit(EXIT_FAILURE);
    }

    printf("connected to %s \n", addrstr);

    char buf[BUFSZ];

    cdata_t *cdata = new cdata_t();

    if(!cdata) {
        printf("[log] malloc error\n");
        p_exit(EXIT_FAILURE);
    }

    cdata->csock = s;
    memcpy(&cdata->storage, &storage, sizeof(storage));

    pthread_t tid;
    pthread_create(&tid, NULL, input_thread, (void *) cdata);

    size_t count;
    while(1) {
        memset(buf, 0, BUFSZ);
        count = recv(s, buf, BUFSZ, 0);
        if(0 == count)
            break;
        printf("%s", buf);
    }

    close(s);
    pthread_cancel(tid);
    exit(EXIT_SUCCESS);
}