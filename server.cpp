#include "common.h"

#include <pthread.h>
#include <semaphore.h>

using namespace std;

map<string, vector<entry_t *>> subs;
// tuple<tag, msg>
queue<tuple<string, string>> pub_queue;

static pthread_mutex_t lock;
static sem_t mutex; 

void p_exit(int value) {
    sem_destroy(&mutex); 
    pthread_mutex_destroy(&lock);
    exit(value);
}

void *publish_thread(void *data) {
    size_t count;
    // tuple<tag, msg>
    tuple<string, string> pub_data;
    vector<entry_t> v;
    string tag, msg;

    printf("publish thread started\n");

    while(1) {
        if (pub_queue.size() > 0) {

            pthread_mutex_lock(&lock);
            pub_data = pub_queue.front();
            pub_queue.pop();
            pthread_mutex_unlock(&lock);

            tag = get<0>(pub_data);
            msg = get<1>(pub_data); 

            for(auto itr : subs[tag]) {
                count = send(itr->cdata->csock, msg.c_str(), strlen(msg.c_str()), 0);
                // cout << "[log] sending " << msg << " to " << itr.cdata->csock << "\n";
                printf("[log] sending %s to %d\n", msg.c_str(), itr->cdata->csock);
                // verifica se mensagem foi enviada corretamente
                if (count != strlen(msg.c_str())) {
                    printf("[log] publish thread failed to send message.\n");
                    p_exit(EXIT_FAILURE);
                }
            }
        }
        else {
            // como a fila está vazia, irá aguardar até q possua novas msgs
            sem_wait(&mutex); 
        }
    }
    pthread_exit(EXIT_SUCCESS);
}

void *client_thread(void *data) {
    // return client socket
    cdata_t *cdata = (cdata_t *) data;
    struct sockaddr *caddr = (struct sockaddr*) (&cdata->storage);

    char caddrstr[BUFSZ];
    if (0 != addrtostr(caddr, caddrstr, BUFSZ)) {
        printf("[log] failed at addrtostr.\n");
        close(cdata->csock);
        p_exit(EXIT_FAILURE);
    }

    printf("[log] connection from %s\n", caddrstr);

    char buf[BUFSZ];
    int category;
    size_t count;
    set<string> tags;
    string tag;

    while(1) {
        memset(buf, 0, BUFSZ);
    
        count = recv(cdata->csock, buf, BUFSZ, 0);
        if (count == 0){
            printf("[log] disconnected from %s\n", caddrstr);
            close(cdata->csock);
            p_exit(EXIT_SUCCESS);
        }

        tuple<int, set<string>> t_msg = parse_msg(buf);
        category = get<0>(t_msg);
        tags = get<1>(t_msg);

        bool already_sub;
        int i;

        switch(category) {
            case S_COMMON:
                break;

            case S_SUBSCRIBE:
                already_sub = false;
                tag = *tags.begin();
                for(auto itr : cdata->tags) {
                    if(itr == tag) {
                        already_sub = true;
                        break;
                    }
                }
                if(already_sub) {
                    sprintf(buf, "already subscribed +%s\n", tag.c_str());
                }
                else {
                    entry_t *e = init_entry(tag, cdata);
                    pthread_mutex_lock(&lock);
                    subs[tag].push_back(e);
                    pthread_mutex_unlock(&lock);
                    cdata->tags.push_back(tag);
                    sprintf(buf, "subscribed +%s\n", tag.c_str());
                }                
                break;

            case S_UNSUBSCRIBE:
                already_sub = false;
                tag = *tags.begin();
                for(auto itr : cdata->tags) {
                    if(itr == tag) {
                        already_sub = true;
                        break;
                    }
                }
                if(already_sub) {
                    pthread_mutex_lock(&lock);
                    for(i = 0; (size_t) i < subs[tag].size(); i++) {
                        if(subs[tag][i]->cdata->csock == cdata->csock) {
                            subs[tag].erase(subs[tag].begin()+i);
                            break;
                        }
                    }
                    pthread_mutex_unlock(&lock);
                    cdata->tags.erase(cdata->tags.begin()+i);
                    sprintf(buf, "unsubscribed -%s\n", tag.c_str());
                }
                else {
                    sprintf(buf, "not subscribed -%s\n", tag.c_str());
                }
                break;

            case S_HASTAG:
                pthread_mutex_lock(&lock);
                for(auto tag : tags) 
                    pub_queue.push(make_tuple(tag, buf));
                pthread_mutex_unlock(&lock); 
                // sinaliza para a thread de publicação que a fila não está vazia
                sem_post(&mutex);
                break;

            case S_KILL:
                printf("[log] received from %s to close server\n", caddrstr);
                close(cdata->csock);
                p_exit(EXIT_SUCCESS);
                break;

            default:
                printf("[log] could not handle msg %s\n", buf);
                break;
        }

        if(category == S_SUBSCRIBE || category == S_UNSUBSCRIBE){
            count = send(cdata->csock, buf, strlen(buf), 0);
            printf("[log] sending %s to %s\n", buf, caddrstr);
            if (count != strlen(buf)) {
                close(cdata->csock);
                p_exit(EXIT_FAILURE);
            }
        }
    }

    // removendo todas as inscricoes do client
    for(auto tag : cdata->tags) {
        pthread_mutex_lock(&lock);
        for(size_t i = 0; i < subs[tag].size(); i++) {
            if(subs[tag][i]->cdata->csock == cdata->csock)
                subs[tag].erase(subs[tag].begin()+i);
        }
        pthread_mutex_unlock(&lock);
    }

    close(cdata->csock);
    pthread_exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    if(argc < 2) 
        return -1;

    pthread_mutex_init(&lock, NULL);
    sem_init(&mutex, 0, 1);

    struct sockaddr_storage storage;
    if(0 != server_sockaddr_init("v4", argv[1], &storage))
         return -1;

    int s; 
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if(s == -1) {
        printf("[log] socket error\n");
        p_exit(EXIT_FAILURE);
    }

    int enable = 1;
    if(0 != setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {
        printf("[log] setsockopt error\n");
        p_exit(EXIT_FAILURE);
    }

    struct sockaddr *addr = (struct sockaddr*) (&storage);

    if(0 != bind(s, addr, sizeof(storage))) {
        printf("[log] port error\n");
        p_exit(EXIT_FAILURE);
    }

    if(0 != listen(s, 10)) {
        printf("[log] listen error\n");
        p_exit(EXIT_FAILURE);
    }

    char addrstr[BUFSZ];
    if(0 != addrtostr(addr, addrstr, BUFSZ)) {
        printf("[log] address conversion error\n");
        p_exit(EXIT_FAILURE);
    }

    pthread_t tid_publish;
    pthread_create(&tid_publish, NULL, publish_thread, NULL);


    printf("bound to %s, waiting connection\n", addrstr);

    // accept
    while(1) {
        struct sockaddr_storage cstorage;
        struct sockaddr *caddr = (struct sockaddr *) (&cstorage);
        socklen_t caddrlen = sizeof(cstorage);
        int csock = accept(s, caddr, &caddrlen);
        if(csock == -1) {
            printf("[log] accept error\n");
            p_exit(EXIT_FAILURE);
        }
       
        cdata_t *cdata = new cdata_t();

        cdata->csock = csock;
        memcpy(&cdata->storage, &storage, sizeof(storage));

        pthread_t tid;
        pthread_create(&tid, NULL, client_thread, (void *) cdata);
    }

    p_exit(EXIT_SUCCESS);
}

