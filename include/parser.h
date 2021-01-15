#pragma once

#include <string.h>  // only for strlen
#include <vector>
#include <string>
#include <set>

// signal definers
#define S_SUBSCRIBE '+'
#define S_UNSUBSCRIBE '-'
#define S_TAG '#'
#define S_KILL "##kill"

// signal values definer
#define V_COMMON 0
#define V_SUBSCRIBE 1
#define V_UNSUBSCRIBE 2
#define V_TAG 3
#define V_KILL 4

using namespace std;

typedef struct parser_data_t {
	int category;
	set<string> tags_parsed;
} parser_data;

typedef struct split_data_t {
	bool success;
	string buffer;
	vector<string> msgs;
} split_data;

// return true if msg is in ascii
bool check_ascii(const char *msg);

// parse a msg and return its category and its tags
parser_data parse_msg(const string msg);

/* split a msg by a delimiter, returning:
	- success, false when some msg is greater than max_size
	- buffer, string between last delimiter and msg terminator
	- msgs, a list with the msgs                                */
split_data split_msg(const string msg, const char delimiter, const size_t max_size);


