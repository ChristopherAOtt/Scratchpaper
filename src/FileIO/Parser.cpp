#include "Parser.hpp"


constexpr bool shouldProceed(Token token){
	return token.type != TOKEN_STREAM_END && token.type != TOKEN_INVALID_TOKEN;
}

void Parser::printDummyKVPair(DummyKVPair& pair, InputBuffer* buffer){
	printf("<DummyKVPair>\n");
	printf("\tKey:");Tokenization::printToken(pair.key_token, buffer); printf("\n");
	printf("\tValues:\n");
	for(Uint64 i = 0; i < pair.value_tokens.size(); ++i){
		Token token = pair.value_tokens[i];
		printf("\t\t[%04ld]: ", i); Tokenization::printToken(token, buffer); printf("\n");
	}
	printf("</DummyKVPair>");
}


Parser::TokenizedFile Parser::tokenizeFile(std::string filepath){
	/*
	A pass-through function for when we just use the default settings
	*/

	return tokenizeFile(filepath, Tokenizer::Settings::initDefault());
}

Parser::TokenizedFile Parser::tokenizeFile(std::string filepath, Tokenizer::Settings settings){
	/*
	Returns a list of tokens created from the file, along with the original
	contents in a string.
	*/

	Parser::TokenizedFile tokenized_file;
	
	InputBuffer buffer = Tokenization::loadFileContents(filepath);
	tokenized_file.text = buffer.contents;

	Tokenizer tokenizer(&buffer, settings);
	Token token{TOKEN_STREAM_BEGIN};
	while(shouldProceed(token)){
		token = tokenizer.nextToken();
		tokenized_file.tokens.push_back(token);
	}

	return tokenized_file;
}

bool isNumeric(Token token){
	return token.type == TOKEN_INTEGER || token.type == TOKEN_REAL;
}

Parser::SettingsParseResult Parser::jankParseSettingsFile(const TokenizedFile& data){
	/*
	WARNING: This function is pure jank. It is a temporary placeholder to get
		settings parsing running.
	TODO: Set up a proper grammar instead of this nonsense.
	*/

	SettingsParseResult invalid_result{.is_valid=false};

	InputBuffer buffer{data.text};

	bool is_id_parse_in_progress = false;
	std::string curr_namespace_name;
	Int32 read_pos = 0;
	Token curr_token;
	std::unordered_map<std::string, Namespace> namespace_map;
	std::stack<std::string> stack;
	while(stack.size() > 0 || read_pos == 0){
		// Consume leading newlines
		curr_token = data.tokens[read_pos++];
		while(curr_token.type == TOKEN_NEWLINE){
			curr_token = data.tokens[read_pos++];
		}

		if(curr_token.type == TOKEN_NAMESPACE){
			/*
			Push a new namespace
			*/

			curr_token = data.tokens[read_pos++];
			if(curr_token.type != TOKEN_IDENTIFIER){
				printf("ERROR: 'namespace' MUST be followed by an id!\n");
				return invalid_result;
			}
			std::string text = tokenText(curr_token, &buffer);
			auto iter = namespace_map.find(text);
			if(iter != namespace_map.end()){
				printf("ERROR: Namespace '%s' was already declared!\n", text.c_str());
				return invalid_result;
			}

			if(data.tokens[read_pos++].type != TOKEN_OPEN_CURLY_BRACKET){
				printf("ERROR: Namespace '%s' didn't have an open bracket {\n", text.c_str());
				return invalid_result;
			}

			// Namespace was entered properly. We can now push it to the stack
			curr_namespace_name = text;
			stack.push(text);
		}else if(curr_token.type == TOKEN_CLOSE_CURLY_BRACKET){
			/*
			Close the current namespace and transition to the lower one if
			applicable. if there is no lower one we're near the end of the
			parse.
			*/

			if(stack.size() == 0){
				printf("ERROR: Too many closed brackets!\n");
				return invalid_result;
			}

			// If there's a lower namespace, use it. Otherwise, we're at
			// the end of the parse and it's not OK to look at the stack.
			std::string old_namespace = stack.top();
			stack.pop();
			if(stack.size() > 0){
				curr_namespace_name = stack.top();
				namespace_map[curr_namespace_name].namespaces.insert(old_namespace);
			}

			// Consume semicolon.
			if(data.tokens[read_pos++].type != TOKEN_SEMICOLON){
				printf("ERROR: Namespace '%s' wasn't closed with a semicolon!\n",
					old_namespace.c_str());
				return invalid_result;
			}
		}else if(curr_token.type == TOKEN_IDENTIFIER){
			/*
			Parse an "Identifier : Value;" combination of tokens.
			*/

			// Make sure this id is on its own and not part of a malformed chain.
			if(is_id_parse_in_progress){
				printf("ERROR: Tried to put multiple ids on the same line!\n");
				return invalid_result;
			}
			Token key_token = curr_token;
			std::vector<Token> value_tokens;
			is_id_parse_in_progress = true;
			std::string id_text = tokenText(curr_token, &buffer);
			if(data.tokens[read_pos++].type != TOKEN_COLON){
				printf("ERROR: Id '%s' wasn't followed by a colon : \n", id_text.c_str());
				return invalid_result;
			}

			// Figure out what kind of value we got
			curr_token = data.tokens[read_pos++];
			if(curr_token.type == TOKEN_OPEN_CURLY_BRACKET){
				/*
				Parse a vector of unknown length
				*/

				int num_consecutive_commas = 0;
				std::vector<Token> vector_members;
				while(true){
					curr_token = data.tokens[read_pos++];
					if(isNumeric(curr_token)){
						vector_members.push_back(curr_token);
						num_consecutive_commas = 0;
					}else if(curr_token.type == TOKEN_COMMA){
						// The vector continues
						++num_consecutive_commas;
					}else if(curr_token.type == TOKEN_CLOSE_CURLY_BRACKET){
						// The vector was closed
						break;
					}else{
						// Something appeared in the vector that shouldn't be there.
						printf("ERROR: Bad token in vector: ");
						Tokenization::printToken(curr_token, &buffer); printf("\n");
						return invalid_result;
					}
				}
				if(vector_members.size() == 2 || vector_members.size() == 3){
					value_tokens = vector_members;
				}else{
					printf("ERROR: Got vector with %li members. Must be 2 or 3!\n", 
						vector_members.size());
					return invalid_result;
				}
			}else{
				/*
				Parse a standalone value
				*/

				constexpr Int32 NUM_ALLOWED_TYPES = 5;
				constexpr TokenType ALLOWED_TYPES[] = {
					TOKEN_BOOLEAN, TOKEN_INTEGER, TOKEN_REAL,
					TOKEN_STRING, TOKEN_IDENTIFIER
				};

				bool is_ok_type = false;
				for(Int32 i = 0; i < NUM_ALLOWED_TYPES; ++i){
					if(curr_token.type == ALLOWED_TYPES[i]){
						is_ok_type = true;
						break;
					}
				}

				if(is_ok_type){
					value_tokens.push_back(curr_token);
				}else{
					printf("ERROR: Got invalid token type: ");
					Tokenization::printToken(curr_token, &buffer); printf("\n");
					return invalid_result;
				}
			}

			// Make sure the expression is terminated.
			curr_token = data.tokens[read_pos++];
			if(curr_token.type == TOKEN_SEMICOLON || curr_token.type == TOKEN_NEWLINE){
				is_id_parse_in_progress = false;
			}	

			// Save the KV pair
			DummyKVPair new_kv_pair = {
				.key_token=key_token,
				.value_tokens=value_tokens
			};
			namespace_map[curr_namespace_name].pairs.push_back(new_kv_pair);
		}else{
			/*
			Catchall for bad inputs
			*/

			printf("ERROR: Token not expected for any recognized pattern!\n");
			printf("\t"); Tokenization::printToken(curr_token, &buffer); printf("\n");
			return invalid_result;
		}
	}

	// Consume trailing newlines until the end of the stream.
	curr_token = data.tokens[read_pos++];
	while(curr_token.type == TOKEN_NEWLINE){
		curr_token = data.tokens[read_pos++];
	}
	if(curr_token.type != TOKEN_STREAM_END){
		printf("ERROR: Token stream continued after final namespace was closed!\n");
		Tokenization::printToken(curr_token, &buffer); printf("\n");
		return invalid_result;
	}

	SettingsParseResult successful_result ={
		.is_valid=true,
		.namespaces=namespace_map
	};
	return successful_result;
}


