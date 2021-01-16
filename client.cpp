#include "common.h"

#include <iostream>
#include <pthread.h>

#define INPUT_SIZE 1024

cdata_t cdata;

void p_exit(int value);
void *input_thread(void *data);

int main(int argc, char *argv[]) {
	if (argc < 3) 
		return -1;

	struct sockaddr_storage storage;
	if (0 != addrparse(argv[1], argv[2], &storage)) 
		return -1;

	int s;
	s = socket(storage.ss_family, SOCK_STREAM, 0);
	if (s == -1) {
		printf("[log] socket error\n");
		p_exit(EXIT_FAILURE);
	}

	struct sockaddr *addr = (struct sockaddr*) (&storage);
 
	if (0 != connect(s, addr, sizeof(storage))) {
		printf("[log] connect error\n");
		p_exit(EXIT_FAILURE);
	}

	char addrstr[BUFSZ];
	if (0 != addrtostr(addr, addrstr, BUFSZ)){
		printf("[log] addrtostr error\n");
		p_exit(EXIT_FAILURE);
	}

	printf("connected to %s \n", addrstr);

	// iniciando dados do client
	cdata.csock = s;
	memcpy(&cdata.storage, &storage, sizeof(storage));

	// iniciando thread para receber da entrada padrao
	pthread_t tid_input;
	pthread_create(&tid_input, NULL, input_thread, NULL);

	char buf[BUFSZ];
	size_t count;
	while (1) {
		memset(buf, 0, BUFSZ);
		count = recv(cdata.csock, buf, BUFSZ, 0);
		if (0 == count) {
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
	char buf[INPUT_SIZE];
	string str;
	size_t count;

	while (true) {
	    memset(buf, 0, INPUT_SIZE);

	    cout << "> "; fgets(buf, INPUT_SIZE, stdin);

		// para seguir ao protocolo, valida a entrada antes de enviar ao servidor
		// como nao ira enviar mais de um \n pelo stdin, verificar apenas o tamanho eh valido
		if (check_ascii(buf) == false || strlen(buf) >= BUFSZ) {
			cout << "[log] invalid message\n"; 
		} 
		else {
			count = send(cdata.csock, buf, strlen(buf)+1, 0);
			if(count != strlen(buf)+1) {
				cout << "[log] send error\n";
				p_exit(EXIT_FAILURE);
			}
			// usleep(100000); // para nao bugar visualizacao no terminal
		}
	}

	pthread_exit(EXIT_SUCCESS);
}
