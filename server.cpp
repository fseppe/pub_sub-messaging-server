#include "common.h"
#include "publisher.h"
#include "parser.h"

#include <pthread.h>

using namespace std;

static pthread_mutex_t aux_lock;
static pthread_cond_t  not_empty; 

pthread_t tid_publish;
Publisher publisher;

void p_exit(int value);
void *publish_thread(void *data);
void *client_thread(void *data);

int main(int argc, char *argv[]) {
	if (argc < 2) {
		cout << "error argv\n";
		return -1;
	}

	pthread_mutex_init(&aux_lock, NULL);
	pthread_cond_init(&not_empty, NULL);

	struct sockaddr_storage storage;
	if(0 != server_sockaddr_init("v4", argv[1], &storage))
		 return -1;

	int s; 
	s = socket(storage.ss_family, SOCK_STREAM, 0);
	if (s == -1) {
		cout << "[log] socket error\n";
		p_exit(EXIT_FAILURE);
	}

	int enable = 1;
	if (0 != setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {
		cout << "[log] setsockopt error\n";
		p_exit(EXIT_FAILURE);
	}

	struct sockaddr *addr = (struct sockaddr*) (&storage);

	if (0 != bind(s, addr, sizeof(storage))) {
		cout << "[log] port error\n";
		p_exit(EXIT_FAILURE);
	}

	if (0 != listen(s, 10)) {
		cout << "[log] listen error\n";
		p_exit(EXIT_FAILURE);
	}

	char addrstr[BUFSZ];
	if (0 != addrtostr(addr, addrstr, BUFSZ)) {
		cout << "[log] address conversion error\n";
		p_exit(EXIT_FAILURE);
	}

	pthread_create(&tid_publish, NULL, publish_thread, NULL);

	cout << "bound to " << addrstr << ", waiting connection\n";

	// accept
	while (true) {
		pthread_t tid;
		struct sockaddr_storage cstorage;
		struct sockaddr *caddr = (struct sockaddr *) (&cstorage);
		socklen_t caddrlen = sizeof(cstorage);
		cdata_t *cdata = new cdata_t();

		int csock = accept(s, caddr, &caddrlen);
		if (csock == -1) {
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
	pthread_cancel(tid_publish);
	pthread_cond_destroy(&not_empty);
	pthread_mutex_destroy(&aux_lock);
	exit(value);
}

void *publish_thread(void *data) {
	size_t count;
	msg_t pub_data;
	string msg;
	set<string> tags;

	cout << "publish thread started\n";

	while (true) {
		
		if (!publisher.queue_isempty()) {

			// guarda clients que ja receberam a msg atual
			set<int> clients_to_recv;

			pub_data = publisher.get_message();
			msg = pub_data.msg;

			for (auto tag : pub_data.tags) {
				for (auto sock : publisher.get_subs(tag)) {
					// verifica se o client atual ja recebeu essa mensagem (evita msgs duplicadas quando houver duas tags distintas que o client seja inscrito)
					if (clients_to_recv.find(sock) == clients_to_recv.end()) {
						clients_to_recv.insert(sock);
						count = send(sock, msg.c_str(), strlen(msg.c_str()), 0);
						cout << "[log] sending " << msg.substr(0, msg.length()-1) << " to sock " << sock << "\n";
						
						if (count != strlen(msg.c_str())) {
							cout << "[log] publish thread failed to send message.\n";
							p_exit(EXIT_FAILURE);
						}
					}
				}
			}
		}
		else {
			// como a fila está vazia, irá aguardar até q possua novas msgs
			pthread_mutex_lock(&aux_lock);
			pthread_cond_wait(&not_empty, &aux_lock);
			pthread_mutex_unlock(&aux_lock);
		}
	}
	pthread_exit(EXIT_SUCCESS);
}

void *client_thread(void *data) {
	cdata_t *cdata = (cdata_t *) data;
	struct sockaddr *caddr = (struct sockaddr*) (&cdata->storage);
	bool shutdown_server = false;
	bool disconnect_client = false;

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
		string tag, msg, aux, temp_buf;
		split_data s_data;
		parser_data p_data;

		temp_buf.clear();

		while (true) {

			if (disconnect_client == true) {
				cout << "disconnecting client " << cdata->csock << " \n";
				break;
			} 

			memset(buf, 0, BUFSZ);
		
			count = recv(cdata->csock, buf, BUFSZ, 0);
			if (count == 0) {
				cout << "[log] disconnected from " << caddrstr << "\n";
				break;
			}

			if (check_ascii(buf) == false) {
				break;
			} 

			msg = temp_buf + string(buf); 
			s_data = split_msg(msg, DELIMITER, BUFSZ);

			temp_buf = s_data.buffer;	// atualizando buffer
			disconnect_client = !s_data.success;	// desconecta o cliente em caso de erro no parse

			for (auto msg : s_data.msgs) {
				cout << "[log] receive msg from " << caddrstr << ":\t" << msg;
				p_data = parse_msg(msg);

				category = p_data.category;
				tags = p_data.tags_parsed;

				switch (category) {

					// quando for uma mensagem comum, nao ha nada para ser feito
					case V_COMMON: {
						break;
					}

					case V_SUBSCRIBE: {
						tag = *tags.begin();
						if(cdata->tags.find(tag) == cdata->tags.end()) {
							cdata->tags.insert(tag);
							publisher.put_sub(tag, cdata->csock);
							sprintf(buf, "subscribed +%s\n", tag.c_str());
						}
						else {
							sprintf(buf, "already subscribed +%s\n", tag.c_str());
						}
						break;
					}

					case V_UNSUBSCRIBE: {
						tag = *tags.begin();
						if(cdata->tags.find(tag) == cdata->tags.end()) {
							sprintf(buf, "not subscribed -%s\n", tag.c_str());
						}
						else {
							publisher.remove_sub(tag, cdata->csock);
							cdata->tags.erase(cdata->tags.find(tag));
							sprintf(buf, "unsubscribed -%s\n", tag.c_str());
						}
						break;
					}

					case V_TAG: {
						publisher.put_message({msg, tags});
						// sinaliza para a thread de publicação que a fila não está vazia
						pthread_cond_signal(&not_empty);
						break;
					}

					case V_KILL: {
						cout << "[log] received from " << caddrstr << " to close server.\n";
						shutdown_server = true;
						break;
					}

					// 
					default: {
						cout << "[log] could not handle msg " << msg << "\n";
						break;
					}
				}

				if (category == V_SUBSCRIBE || category == V_UNSUBSCRIBE){
					count = send(cdata->csock, buf, strlen(buf), 0);
					cout << "[log] sending msg to " << caddrstr << ":\t" << buf;
					if (count != strlen(buf)) {
						cout << "[log] failed to sent msg to. Disconnecting " << caddrstr << "\n";
						close(cdata->csock);
						p_exit(EXIT_FAILURE);
					}
				}
			}
		}

		// removendo todas as inscricoes do client
		publisher.remove_allSubs(cdata->tags, cdata->csock);
	}

	close(cdata->csock);
	delete cdata;
	if (shutdown_server) {
		p_exit(EXIT_SUCCESS);
	}
	pthread_exit(EXIT_SUCCESS);
}
