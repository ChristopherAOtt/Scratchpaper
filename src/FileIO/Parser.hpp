#pragma once

#include "Tokenizer.hpp"

#include <unordered_map>
#include <unordered_set>
#include <stack>
#include <cassert>

namespace Parser{
	//-------------------------------------------
	// Structs
	//-------------------------------------------
	struct DummyKVPair{
		/*
		This just pulls the tokens along for the ride without
		actually converting them to a value
		*/
		Token key_token;
		std::vector<Token> value_tokens;
	};

	struct TokenizedFile{
		/*
		All tokens returned by parsing a file.
		*/

		std::string text;
		std::vector<Token> tokens;
	};

	struct Namespace{
		std::vector<DummyKVPair> pairs;
		std::unordered_set<std::string> namespaces;
	};

	struct ResourceDeclaration{
		Token type;
		Token id;
		Token location;
	};

	struct SettingsParseResult{
		std::unordered_map<std::string, Namespace> namespaces;
		std::vector<ResourceDeclaration> declarations;
	};

	//-------------------------------------------
	// Functions
	//-------------------------------------------
	void printDummyKVPair(DummyKVPair& pair, InputBuffer* buffer);

	std::pair<bool, TokenizedFile> tokenizeFile(std::string filepath);
	std::pair<bool, TokenizedFile> tokenizeFile(std::string filepath, Tokenizer::Settings settings);
	std::pair<bool, SettingsParseResult> jankParseSettingsFile(const TokenizedFile& data);
}


