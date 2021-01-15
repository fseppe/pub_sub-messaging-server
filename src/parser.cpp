#include "parser.h"

bool check_ascii(const char *msg) {
	for (size_t i = 0; i < strlen(msg); i++) {
		if (msg[i] > 127) {
			return false;
		}
	}
	return true;
}

parser_data parse_msg(const string msg) {

	set<string> tags;
	size_t idx;
	string temp = msg;
	int category = V_COMMON;

	if (temp.back() == '\n') {
		temp = temp.substr(0, temp.size()-1);
	}

	if (temp.find(S_KILL) != string::npos) {
		category = V_KILL;
	}

	else if (temp.length() > 1) {

		if (temp[0] == S_SUBSCRIBE) {
			category = V_SUBSCRIBE;
			idx = temp.find(' ');
			if (idx != string::npos) {
				tags.insert(temp.substr(1, idx-1));
			}
			else {
				tags.insert(temp.substr(1));
			}
		}

		else if(temp[0] == S_UNSUBSCRIBE) {
			category = V_UNSUBSCRIBE;
			idx = temp.find(' ');
			if (idx != string::npos) {
				tags.insert(temp.substr(1, idx-1));
			}
			else {
				tags.insert(temp.substr(1));
			}
		}

		else {
			// repeats while finding a tag
			while (temp.length() > 1) {
				idx = temp.find(S_TAG);
				if(idx == string::npos) {
					break;
				}
				// set category if find a tag
				category = V_TAG;
				temp = temp.substr(idx);
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
	// return a parser_data
	return {category, tags};
}

split_data split_msg(const string msg, const char delimiter, const size_t max_size) {

	size_t itr, pos;
	split_data s_data;
	s_data.success = true; 

	itr = 0;
	while (true) {
		pos = msg.find(delimiter, itr);
		if (pos == string::npos) {
			if (itr != msg.size()) {
				s_data.buffer = msg.substr(itr, msg.size());
			}
			else {
				s_data.buffer.clear();
			}
			break;
		}
		else {
			// if msg is greater than max size, stop immediately
			if (pos-itr >= max_size) {
				s_data.success = false;
				s_data.buffer.clear();
				break;
			}
			// using +1 to include \n
			s_data.msgs.push_back(msg.substr(itr, pos-itr+1));
			itr = pos+1;
		}
	}

	return s_data;
}
