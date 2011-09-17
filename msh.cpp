#include <iostream>
#include <sstream>
#include <map>
#include <vector>
#include <deque>
using std::cout;
using std::cin;
using std::cerr;
using std::endl;

#include "bit_math.hpp"
using namespace bit_math;

typedef std::map<std::string, Int> Symbols;
Symbols symbols;

bool 
symbol_exist(std::string name)
{
	if (symbols.count(name) == 0) {
		cout << "ERROR: no such symbol " << name << endl;
		return false;
	}
	return true;
}

bool 
load_symbol_or_int(std::string arg, Int& num)
{
	if (symbols.count(arg)) {
		num = symbols[arg];
	} else {
		if (!num.parse(arg)) {
			cout << "ERROR: parse failed " << arg << endl;
			return false;
		}
	}
	return true;
}


int main()
{
	while (true) {
		std::deque<std::string> words;
		{	// prompt and read words
			cout << "> ";
			std::string line;
			std::getline(cin, line);
			std::istringstream iss(line);
			std::string word;
			while (iss >> word) {
				words.push_back(word);
			}
		}
		if (words.empty()) {
			continue;
		} else if (words.size() == 1) {
			if (!symbol_exist(words[0])) continue;
			cout << words[0] << " = " << symbols[words[0]] << endl;
		} else if (words.size() > 2) {
			std::string sym;
			if (words[1] == "=") {
				sym = words[0];
				words.pop_front();
				words.pop_front();
			}

			// for now only do plus:
			if (words.size() != 3 || words[1] != "+") {
				cout << "ERROR: expected plus statement" << endl;
				continue;
			}
			Int first, second;
			if (!load_symbol_or_int(words[0], first)) continue;
			if (!load_symbol_or_int(words[2], second)) continue;
			Int result = first + second;

			if (sym.empty()) {
				cout << "<result> = " << result << endl;
			} else {
				symbols[sym] = result;
				cout << sym << " = " << result << endl;
			}
		}
	}
	return 0;
}

