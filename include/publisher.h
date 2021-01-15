#pragma once

#include <string.h>
#include <vector>
#include <string>
#include <queue>
#include <set>
#include <unordered_map>
#include <pthread.h>

using namespace std;

// store msg and the tags that is referred 
typedef struct msg_data {
	string msg;
	set<string> tags;
} msg_t;

class Publisher {

	private:
		pthread_mutex_t queue_lock, subs_lock;
		// queue of msgs to be published
		queue<msg_t> pub_queue;
		// map from tags to subscribers
		unordered_map<string, set<int>> subs;

	public:
		Publisher();

		bool queue_isempty();
		// get next message from queue to publish
		msg_t get_message();
		// push a message in the queue
		void put_message(const msg_t msg);
		// returns the set of subs from a tag
		set<int> get_subs(const string tag);
		// put a sub in a tag
		void put_sub(const string tag, const int client_sock);
		// removes a sub from a specific tag
		void remove_sub(const string tag, const int client_sock);
		// removes all the tags that a client is subscribed
		void remove_allSubs(const set<string> tags, const int client_sock);

};