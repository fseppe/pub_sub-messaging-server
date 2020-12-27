#include "common.h"

#include <pthread.h>
#include <semaphore.h>

using namespace std;

static pthread_mutex_t lock;
static sem_t mutex; 

map<string, set<int>> subs;
queue<msg_t> pub_queue;

void p_exit(int value);
void *publish_thread(void *data);
void *client_thread(void *data);

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
        cout << "[log] socket error\n";
        p_exit(EXIT_FAILURE);
    }

    int enable = 1;
    if(0 != setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {
        cout << "[log] setsockopt error\n";
        p_exit(EXIT_FAILURE);
    }

    struct sockaddr *addr = (struct sockaddr*) (&storage);

    if(0 != bind(s, addr, sizeof(storage))) {
        cout << "[log] port error\n";
        p_exit(EXIT_FAILURE);
    }

    if(0 != listen(s, 10)) {
        cout << "[log] listen error\n";
        p_exit(EXIT_FAILURE);
    }

    char addrstr[BUFSZ];
    if(0 != addrtostr(addr, addrstr, BUFSZ)) {
        cout << "[log] address conversion error\n";
        p_exit(EXIT_FAILURE);
    }

    pthread_t tid_publish;
    pthread_create(&tid_publish, NULL, publish_thread, NULL);

    cout << "bound to " << addrstr << ", waiting connection\n";

    // accept
    while(1) {
        pthread_t tid;
        struct sockaddr_storage cstorage;
        struct sockaddr *caddr = (struct sockaddr *) (&cstorage);
        socklen_t caddrlen = sizeof(cstorage);
        cdata_t *cdata = new cdata_t();

        int csock = accept(s, caddr, &caddrlen);
        if(csock == -1) {
            cout << "[log] accept error\n";
            p_exit(EXIT_FAILURE);
        }
       
        cdata->csock = csock;
        memcpy(&cdata->storage, &storage, sizeof(storage));

        pthread_create(&tid, NULL, client_thread, (void *) cdata);
    }

    delete addr;
    p_exit(EXIT_SUCCESS);
}

void p_exit(int value) {
    sem_destroy(&mutex); 
    pthread_mutex_destroy(&lock);
    exit(value);
}

void *publish_thread(void *data) {
    size_t count;
    msg_t pub_data;
    string msg;
    set<string> tags;
    int csock;

    cout << "publish thread started\n";

    while(1) {
        if(pub_queue.size() > 0) {

            pthread_mutex_lock(&lock);
            pub_data = pub_queue.front();
            pub_queue.pop();
            pthread_mutex_unlock(&lock);

            msg = pub_data.msg;
            tags = pub_data.tags; 

            // guarda clients que ja receberam a msg atual
            set<int> clients_to_recv;

            for(auto tag : tags) {
                for(auto sock : subs[tag]) {
                    // verifica se o client atual ja recebeu essa mensagem (evita msgs duplicadas quando houver duas tags distintas que o client seja inscrito)
                    if(clients_to_recv.find(sock) == clients_to_recv.end()) {
                        clients_to_recv.insert(sock);
                        count = send(sock, msg.c_str(), strlen(msg.c_str()), 0);
                        cout << "[log] sending " << msg.substr(0, msg.length()-1) << " to sock " << sock << "\n";
                        
                        if(count != strlen(msg.c_str())) {
                            cout << "[log] publish thread failed to send message.\n";
                            p_exit(EXIT_FAILURE);
                        }
                    }
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
    cdata_t *cdata = (cdata_t *) data;
    struct sockaddr *caddr = (struct sockaddr*) (&cdata->storage);
    bool shutdown_server = false;

    char caddrstr[BUFSZ];
    if (0 != addrtostr(caddr, caddrstr, BUFSZ)) {
        cout << "[log] failed at addrtostr.\n";
    }
    else {
        cout << "[log] connection from " << caddrstr << "\n";

        char buf[BUFSZ];
        int category;
        size_t count;
        set<string> tags;
        string tag;
        int i;
        pair<int, set<string>> t_msg;

        while(1) {
            memset(buf, 0, BUFSZ);
        
            count = recv(cdata->csock, buf, BUFSZ, 0);
            if (count == 0){
                cout << "[log] disconnected from " << caddrstr << "\n";
                break;
            }

            t_msg = parse_msg(buf);
            category = t_msg.first;
            tags = t_msg.second;

            switch(category) {
                case S_COMMON: {
                    break;
                }

                case S_SUBSCRIBE: {
                    tag = *tags.begin();
                    if(cdata->tags.find(tag) == cdata->tags.end()) {
                        cdata->tags.insert(tag);
                        pthread_mutex_lock(&lock);
                        subs[tag].insert(cdata->csock);
                        pthread_mutex_unlock(&lock);
                        sprintf(buf, "subscribed +%s\n", tag.c_str());
                    }
                    else {
                        sprintf(buf, "already subscribed +%s\n", tag.c_str());
                    }
                    break;
                }

                case S_UNSUBSCRIBE: {
                    tag = *tags.begin();
                    if(cdata->tags.find(tag) == cdata->tags.end()) {
                        sprintf(buf, "not subscribed -%s\n", tag.c_str());
                    }
                    else {
                        pthread_mutex_lock(&lock);
                        for(auto sock : subs[tag]) {
                            if(sock == cdata->csock) {
                                subs[tag].erase(sock);
                                break;
                            }
                        }
                        pthread_mutex_unlock(&lock);
                        cdata->tags.erase(cdata->tags.find(tag));
                        sprintf(buf, "unsubscribed -%s\n", tag.c_str());
                    }
                    break;
                }

                case S_HASTAG: {
                    msg_t v = {buf, tags};
                    pthread_mutex_lock(&lock);
                    pub_queue.push(v);
                    pthread_mutex_unlock(&lock); 
                    // sinaliza para a thread de publicação que a fila não está vazia
                    sem_post(&mutex);
                    break;
                }

                case S_KILL: {
                    cout << "[log] received from " << caddrstr << " to close server.\n";
                    shutdown_server = true;
                    break;
                }

                default: {
                    cout << "[log] could not handle msg " << buf << "\n";
                    break;
                }
            }

            if(category == S_SUBSCRIBE || category == S_UNSUBSCRIBE){
                count = send(cdata->csock, buf, strlen(buf), 0);
                cout << "[log] sending " << buf << " to " << caddrstr << "\n";
                if (count != strlen(buf)) {
                    close(cdata->csock);
                    p_exit(EXIT_FAILURE);
                }
            }
        }

        // removendo todas as inscricoes do client
        pthread_mutex_lock(&lock);
        for(auto tag : cdata->tags) {
            for(auto sock : subs[tag]) {
                if(sock == cdata->csock) {
                    subs[tag].erase(sock);
                }
            }
        }
        pthread_mutex_unlock(&lock);
    }

    close(cdata->csock);
    delete cdata;
    if(shutdown_server) {
        p_exit(EXIT_SUCCESS);
    }
    pthread_exit(EXIT_SUCCESS);
}



