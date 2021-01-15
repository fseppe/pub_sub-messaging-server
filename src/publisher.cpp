#include "publisher.h"

Publisher::Publisher() {
	this->queue_lock = PTHREAD_MUTEX_INITIALIZER;
	this->subs_lock = PTHREAD_MUTEX_INITIALIZER;
}

bool Publisher::queue_isempty() {
	return this->pub_queue.empty();
}

msg_t Publisher::get_message() {
	msg_t m;
	pthread_mutex_lock(&this->queue_lock);
	m = pub_queue.front();
	pub_queue.pop();
	pthread_mutex_unlock(&this->queue_lock);
	return m;
}

void Publisher::put_message(const msg_t msg) {
	pthread_mutex_lock(&this->queue_lock);
	pub_queue.push(msg);
	pthread_mutex_unlock(&this->queue_lock);
}

set<int> Publisher::get_subs(const string tag) {
	set<int> subs;
	pthread_mutex_lock(&this->subs_lock);
	subs = this->subs[tag];
	pthread_mutex_unlock(&this->subs_lock);
	return subs;
}

void Publisher::put_sub(const string tag, const int client_sock) {
	pthread_mutex_lock(&this->subs_lock);
	this->subs[tag].insert(client_sock);
	pthread_mutex_unlock(&this->subs_lock);
}

void Publisher::remove_sub(const string tag, const int client_sock) {
	pthread_mutex_lock(&this->subs_lock);
	for (auto sock : subs[tag]) {
		if (sock == client_sock) {
			subs[tag].erase(sock);
			break;
		}
	}
	pthread_mutex_unlock(&this->subs_lock);
}

void Publisher::remove_allSubs(const set<string> tags, const int client_sock) {
	pthread_mutex_lock(&this->subs_lock);
	for (auto tag : tags) {
		if (subs[tag].find(client_sock) != subs[tag].end()) {
			subs[tag].erase(client_sock);
		}
	}
	pthread_mutex_unlock(&this->subs_lock);
}


