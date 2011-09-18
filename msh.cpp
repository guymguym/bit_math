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
typedef bit_math::Int<> Int;

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

bool 
process(const std::deque<std::string>& words)
{
	if (words.empty()) return true;
	if (words.size() == 1) {
		if (!symbol_exist(words[0])) return false;
		cout << words[0] << " = " << symbols[words[0]] << endl;
		return true;
	}
	if (words.size() == 2) {
		cout << "ERROR: expected more than 2 words in statement" << endl;
		return false;
	}
	
	size_t i = 0;
	std::string sym;
	if (words[1] == "=") {
		sym = words[0];
		i += 2;
	}
	Int result;
	if (i+1 == words.size()) {
		if (!load_symbol_or_int(words[i], result)) return false;
	} else if (i+3 == words.size() && words[i+1] == "+") {
		Int first, second;
		if (!load_symbol_or_int(words[i], first)) return false;
		if (!load_symbol_or_int(words[i+2], second)) return false;
		result = first + second;
	} else {
		// for now only do plus:
		cout << "ERROR: expected plus statement" << endl;
		return false;
	}

	cout << result << endl;
	if (!sym.empty()) {
		symbols[sym] = result;
	}
	return true;
}


int 
main(int ac, char** av)
{
	bool pipe_mode = false;
	std::deque<std::string> args;
	for (int i=1; i<ac; ++i) {
		std::string arg(av[i]);
		if (arg == "--pipe") {
			pipe_mode = true;
			continue;
		}
		args.push_back(arg);
	}

	if (!args.empty()) {
		process(args);
		return 0;
	}
	while (true) {
		std::deque<std::string> words;
		{	// prompt and read words
			if (cin.eof()) { 
				cout << endl; 
				break; 
			}
			cout << "<msh> ";
			if (pipe_mode)
				cout << endl;
			else
				cout.flush();
			std::string line;
			std::getline(cin, line);
			std::istringstream iss(line);
			std::string word;
			while (iss >> word) {
				words.push_back(word);
			}
		}
		process(words);
	}
	return 0;
}

