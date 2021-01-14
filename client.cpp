#include "common.h"

#include <istream>
#include <pthread.h>

#include <chrono>
#include <thread>

static pthread_mutex_t lock;

cdata_t cdata;

void p_exit(int value);
void *input_thread(void *data);

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

	// iniciando dados do client
	cdata.csock = s;
	memcpy(&cdata.storage, &storage, sizeof(storage));

	pthread_mutex_init(&lock, NULL);

	pthread_t tid_input;
	pthread_create(&tid_input, NULL, input_thread, NULL);

	char buf[BUFSZ];
	size_t count;
	while(1) {
		memset(buf, 0, BUFSZ);
		count = recv(cdata.csock, buf, BUFSZ, 0);
		if(0 == count) {
			cout << "\nCan not receive message from server. Disconnecting...\n";
			break;
		}
		cout << "< " << buf;
	}

	close(s);
	pthread_cancel(tid_input);
	exit(EXIT_SUCCESS);
}

void p_exit(int value) {
	exit(value);
}

void *input_thread(void *data) {
	char buf[BUFSZ];
	string str;
	size_t count;

	// versao correta
	// while (1) {
	//     memset(buf, 0, BUFSZ);

	//     cout << "> "; fgets(buf, BUFSZ, stdin);

	//     count = send(cdata.csock, buf, strlen(buf)+1, 0);
	//     if(count != strlen(buf)+1) {
	//         cout << "\n\n[log] send error\n";
	//         p_exit(EXIT_FAILURE);
	//     }
	//     // usleep(100000); // para nao bugar visualizacao no terminal
	// }

	// modificacao para testes
	while (1) {

		memset(buf, 0, BUFSZ);
		memcpy(buf, "boa tarde #MaisUmDia", 20);
		count = send(cdata.csock, buf, strlen(buf)+1, 0);
		if(count != strlen(buf)+1) {
			cout << "\n\n[log] send error\n";
			p_exit(EXIT_FAILURE);
		}

		// usleep(100000);

		memset(buf, 0, BUFSZ);
		memcpy(buf, " bom almoço #DiarioAlimentar\n", 30);
		// memcpy(buf, "boa tarde #MaisUmDia\nbom almoço #DiarioAlimentar\n", 50);
		count = send(cdata.csock, buf, strlen(buf)+1, 0);
		if(count != strlen(buf)+1) {
			cout << "\n\n[log] send error\n";
			p_exit(EXIT_FAILURE);
		}

		// usleep(100000);

		break;

	}

	pthread_exit(EXIT_SUCCESS);
}
