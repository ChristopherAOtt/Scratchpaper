#include "Tokenizer.hpp"


//-----------------------------------------------------------------------------
// Symbol
//-----------------------------------------------------------------------------
Symbol::Symbol(int data){
	this->data = data;
}

Symbol::Symbol(const Symbol& other){
	this->data = other.data;
}

bool Symbol::operator==(const Symbol& other){
	return this->data == other.data;
}

bool Symbol::operator==(int other){
	return this->data == other;
}

bool Symbol::operator!=(const Symbol& other){
	return this->data != other.data;
}

bool Symbol::operator!=(int other){
	return this->data != other;
}

bool Symbol::operator<(int other){
	return this->data < other;
}

bool Symbol::operator>(int other){
	return this->data > other;
}

bool Symbol::operator<=(int other){
	return this->data <= other;
}

bool Symbol::operator>=(int other){
	return this->data >= other;
}

bool Symbol::isDigit() const{
	return data >= 48 && data <= 57;
}

bool Symbol::isCapitalAlphabet() const{
	return data >= 65 && data <= 90;
}

bool Symbol::isLowercaseAlphabet() const{
	return data >= 97 && data <= 122;
}

bool Symbol::isAlphabet() const{
	return isCapitalAlphabet() || isLowercaseAlphabet();
}

bool Symbol::isWhitespace() const{
	return data == ' ' || data == '\n' || data == '\t';
}

bool Symbol::isAllowedIdentifierChar() const{
	return isCapitalAlphabet() || isLowercaseAlphabet() || 
		isDigit() || data == TOKEN_UNDERSCORE;
}

bool Symbol::isPrintableAscii() const{
	return data >= LOWEST_PRINTABLE_ASCII && data <= HIGHEST_PRINTABLE_ASCII;
}


//-----------------------------------------------------------------------------
// Token helper functions
//-----------------------------------------------------------------------------
std::string tokenText(Token token, const InputBuffer* buffer){
	/*
	Helper function to reduce boilerplate
	*/

	assert(token.type == TOKEN_IDENTIFIER || token.type == TOKEN_STRING);
	StringSlice slice = token.data.value_string;
	std::string text = buffer->byteSlice(slice.start_byte, slice.num_bytes);
	return text;
}


//-----------------------------------------------------------------------------
// InputBuffer
//-----------------------------------------------------------------------------
Symbol InputBuffer::symbolAt(Int64 byte_index) const{
	/*
	WARNING: Placeholder function. This does not correctly parse Unicode 
		symbols yet.
	TODO: Actually get symbols instead of assuming each code point is one char.
	*/

	return contents[byte_index];
}

std::string InputBuffer::byteSlice(Int64 byte_index, Int32 range) const{
	return contents.substr(byte_index, range);
}

bool InputBuffer::matchesString(Int64 byte_index, const char* word) const{
	/*
	Returns true if the contents starting at the given index match 
	the provided string.

	NOTE: For tokenization the length of string matches is very low. Usually
		measured in the tens of bytes. After profiling it turns out that the 
		branching caused by
			if(found_mismatch) return;
		was slowing this method down significantly. The &= pattern is 
		intentional to reduce branches.
	*/

	bool is_valid = byte_index < (Int64) contents.size() && word[0] != 0;
	Int32 word_index = 0;
	Uint64 buffer_index = byte_index;
	while(word[word_index] != 0 && buffer_index < contents.size()){
		is_valid &= contents[buffer_index] == word[word_index];
		++word_index;
		++buffer_index;
	}
	return is_valid;
}

Int64 InputBuffer::bytesRemaining(Int64 byte_index) const{
	/*
	Returns the number of bytes remaining after the byte_index
	*/

	return contents.size() - byte_index;
}


//-----------------------------------------------------------------------------
// Tokenizer
//-----------------------------------------------------------------------------
Tokenizer::Settings Tokenizer::Settings::initDefault(){
	Settings default_settings;
	default_settings.line_comment_start = "//";
	default_settings.block_comment_start = "/*";
	default_settings.block_comment_end = "*/";
	default_settings.emit_newline_tokens = true;
	default_settings.continue_identifiers_until_whitespace = false;
	default_settings.emit_whitespace_tokens = false;
	default_settings.attempt_reserved_words = true;

	return default_settings;
}

Tokenizer::Tokenizer(const InputBuffer* buffer_ptr, Settings settings){
	init(buffer_ptr, settings);
}

Tokenizer::Tokenizer(const InputBuffer* buffer_ptr){
	init(buffer_ptr, Settings::initDefault());
}

void Tokenizer::init(const InputBuffer* buffer_ptr, Settings settings){
	m_settings = settings;
	m_buffer_ptr = buffer_ptr;
	m_byte_read_index = 0;

	// These are ranked in order of priority. The result is the first match.
	MatchGroup match_functions[] = {
		{
			MATCH_WHITESPACE,          
			&Tokenizer::attemptWhitespace,   
			settings.emit_whitespace_tokens
		},
		{
			MATCH_LINE_COMMENT,        
			&Tokenizer::attemptLineComment,  
			false
		},
		{
			MATCH_BLOCK_COMMENT,       
			&Tokenizer::attemptBlockComment, 
			false
		},
		{
			MATCH_NEWLINE,             
			&Tokenizer::attemptNewline,           
			settings.emit_newline_tokens
		},
		{
			MATCH_NUMERIC,             
			&Tokenizer::attemptNumeric,           
			true
		},
		{
			MATCH_RESERVED_WORD,       
			&Tokenizer::attemptReservedWord,      
			true
		},
		{
			MATCH_BOOLEAN,             
			&Tokenizer::attemptBoolean,           
			true
		},
		{
			MATCH_IDENTIFIER,          
			&Tokenizer::attemptIdentifier,        
			true
		},
		{
			MATCH_STRING,              
			&Tokenizer::attemptString,            
			true
		},
		{
			MATCH_SINGLE_SYMBOL_TOKEN, 
			&Tokenizer::attemptSingleSymbolToken, 
			true
		},
	};

	// For each group, decide what to add to this tokenizer's internal 
	// match function list.
	for(MatchGroup group : match_functions){
		if(group.type == MATCH_LINE_COMMENT){
			if(settings.line_comment_start.size() == 0){
				continue;
			}
		}else if(group.type == MATCH_BLOCK_COMMENT){
			if(settings.block_comment_start.size() == 0){
				continue;
			}

			if(settings.block_comment_end.size() == 0){
				continue;
			}
		}else if(group.type == MATCH_RESERVED_WORD){
			if(!settings.attempt_reserved_words){
				continue;
			}
		}

		m_match_list.push_back(group);
	}
}

Token Tokenizer::nextToken(){
	while(true){
		// Stop immediately if we're at the end of the stream
		if(m_buffer_ptr->bytesRemaining(m_byte_read_index) == 0){
			return {TOKEN_STREAM_END};
		}

		// There's text left to tokenize. Get the best match.
		MatchResult best_match = {MatchType::MATCH_INVALID};
		bool is_match_token_type;
		for(auto [type, match_attempt_func_ptr, is_token_type] : m_match_list){
			MatchResult match = (this->*match_attempt_func_ptr)();
			if(match.result == RESULT_SUCCESS){
				best_match = match;
				is_match_token_type = is_token_type;
				break;
			}
		}

		// TODO: Track line and column counts
		// Advance the read position, and return the token if applicable.
		m_byte_read_index += best_match.success.num_bytes_consumed;
		if(is_match_token_type){
			return best_match.success.token;
		}
	}
}

MatchResult Tokenizer::attemptWhitespace() const{
	/*
	Matches runs of whitespace, not including newlines.
	*/

	MatchResult match{MATCH_WHITESPACE};

	Int64 curr_byte = m_byte_read_index;
	while(m_buffer_ptr->bytesRemaining(curr_byte)){
		Symbol symbol = m_buffer_ptr->symbolAt(curr_byte);
		if(symbol != ' ' && symbol != '\t'){
			break;
		}else{
			++curr_byte;
		}
	}

	Int64 num_bytes_consumed = curr_byte - m_byte_read_index;
	if(num_bytes_consumed > 0){
		match.result = RESULT_SUCCESS;
		match.success.num_bytes_consumed = num_bytes_consumed;
		match.success.token.type = TOKEN_WHITESPACE;
	}else{
		match.result = RESULT_FAILURE;
	}

	return match;
}

MatchResult Tokenizer::attemptLineComment() const{
	/*
	Attempts to parse a line comment. Runs from start of comment
	until the next newline
	*/

	MatchResult match{MATCH_LINE_COMMENT};

	Int64 curr_byte = m_byte_read_index;
	if(m_buffer_ptr->matchesString(curr_byte, 
		&m_settings.line_comment_start[0])){

		while(m_buffer_ptr->bytesRemaining(curr_byte)){
			if(m_buffer_ptr->symbolAt(++curr_byte) == '\n'){
				match.success.num_newlines_consumed = 1;
				break;
			}
		}

		match.result = RESULT_SUCCESS;
		match.success.num_bytes_consumed = curr_byte - m_byte_read_index;
	}else{
		match.result = RESULT_FAILURE;
	}

	return match;
}

MatchResult Tokenizer::attemptBlockComment() const{
	/*
	Attempts to parse a block comment. This means any text that
	appears between 
		m_settings.block_comment_start and m_settings.block_comment_end
	will be considered one long continuous run.
	*/

	MatchResult match{MATCH_BLOCK_COMMENT};
	
	Int64 curr_byte = m_byte_read_index;
	if(m_buffer_ptr->matchesString(curr_byte, 
		&m_settings.block_comment_start[0])){

		while(m_buffer_ptr->bytesRemaining(curr_byte)){
			Symbol curr_symbol = m_buffer_ptr->symbolAt(++curr_byte);
			if(curr_symbol == '\n'){
				match.success.num_newlines_consumed += 1;
			}

			if(m_buffer_ptr->matchesString(curr_byte, 
				&m_settings.block_comment_end[0])){
				break;
			}
		}

		if(m_buffer_ptr->bytesRemaining(curr_byte)){
			// Found a block comment that was closed before the buffer ended
			match.result = RESULT_SUCCESS;
			match.success.num_bytes_consumed = 
				curr_byte - m_byte_read_index + 2;
		}else{
			// Block comment wasn't closed
			match.result = RESULT_MALFORMED_INPUT;
		}
	}else{
		match.result = RESULT_FAILURE;
	}
	
	return match;
}

MatchResult Tokenizer::attemptNewline() const{
	/*
	Newlines cause enough special stuff to happen to warrant their own "type".
	*/

	MatchResult match{MATCH_NEWLINE, RESULT_FAILURE};
	if(m_buffer_ptr->bytesRemaining(m_byte_read_index)){
		Symbol symbol = m_buffer_ptr->symbolAt(m_byte_read_index);
		if(symbol == '\n'){
			// Handle \n newline
			match.result = RESULT_SUCCESS;
			match.success.num_bytes_consumed = 1;
			match.success.token.type = TOKEN_NEWLINE;
		}else if(symbol == '\r'){
			// Handle \r\n newline.
			Symbol next_symbol = m_buffer_ptr->symbolAt(m_byte_read_index + 1);
			if(next_symbol == '\n'){
				match.result = RESULT_SUCCESS;
				match.success.num_bytes_consumed = 2;
				match.success.token.type = TOKEN_NEWLINE;
			}
		}

	}
	return match;
}

MatchResult Tokenizer::attemptNumeric() const{
	/*
	Attempts to handle both integer and decimal numeric types.
	
	TODO: Support e notation at the end of floats
	*/

	MatchResult match{MATCH_NUMERIC};

	bool is_real = false;
	bool is_signed = false;
	Int64 curr_byte = m_byte_read_index;

	// Optional sign
	Symbol curr_symbol = m_buffer_ptr->symbolAt(curr_byte);
	if(curr_symbol == '-' || curr_symbol == '+'){
		is_signed = true;
		curr_symbol = m_buffer_ptr->symbolAt(++curr_byte);
	}
	
	// Pre-decimal digits
	while(curr_symbol.isDigit()){
		curr_symbol = m_buffer_ptr->symbolAt(++curr_byte);
	}

	// Handle decimal component
	if(curr_symbol == '.'){
		if(m_buffer_ptr->symbolAt(curr_byte + 1).isDigit()){
			is_real = true;
			++curr_byte;

			while(m_buffer_ptr->symbolAt(curr_byte).isDigit()){
				++curr_byte;
			}
		}
	}

	// Handle exponent
	if(m_buffer_ptr->symbolAt(curr_byte) == 'e'){
		Symbol peek = m_buffer_ptr->symbolAt(curr_byte + 1);
		bool is_peek_sign = (peek == '-' || peek == '+');
		if(peek.isDigit() || is_peek_sign){
			is_real = true;
			curr_byte += is_peek_sign;
			++curr_byte;

			while(m_buffer_ptr->symbolAt(curr_byte).isDigit()){
				++curr_byte;
			}
		}else{
			match.result = RESULT_MALFORMED_INPUT;
			return match;
		}
	}

	if(curr_byte > m_byte_read_index + is_signed){
		Int32 num_bytes_consumed = curr_byte - m_byte_read_index;
		Token& token = match.success.token;
		match.result = RESULT_SUCCESS;
		match.success.num_bytes_consumed = num_bytes_consumed;
		token.type = is_real ? TOKEN_REAL : TOKEN_INTEGER;
		std::string slice = m_buffer_ptr->byteSlice(m_byte_read_index, 
			num_bytes_consumed);
		if(is_real){
			token.data.value_real = atof(&slice[0]);
		}else{
			token.data.value_int = atoi(&slice[0]);
		}
	}else{
		match.result = RESULT_FAILURE;
	}

	return match;
}

MatchResult Tokenizer::attemptReservedWord() const{
	/*
	Returns info about the best match for the next token starting
	at the current lexer position.

	YAGNI: Replace with a Trie to improve time complexity.
	*/

	MatchResult match{MATCH_RESERVED_WORD};

	constexpr int INVALID_MATCH_INDEX = -1;

	// Just doing a dumb elementwise comparison
	Int32 longest_match_size = 0;
	Int32 longest_match_index = INVALID_MATCH_INDEX;
	for(Int32 i = 0; i < NUM_RESERVED_WORD_MAPPINGS; ++i){
		auto& [word, num_chars, type] = RESERVED_WORD_MAPPINGS[i];
		if(longest_match_size >= num_chars){
			continue;
		}

		if(m_buffer_ptr->matchesString(m_byte_read_index, word)){
			longest_match_size = num_chars;
			longest_match_index = i;
		}
	}

	// Determine success state and return
	if(longest_match_index != INVALID_MATCH_INDEX){
		match.result = RESULT_SUCCESS;
		auto longest_match = RESERVED_WORD_MAPPINGS[longest_match_index];

		match.success.num_bytes_consumed = longest_match_size;
		match.success.token = {
			.type=longest_match.type,
		};
	}else{
		match.result = RESULT_FAILURE;
	}

	return match;
}

MatchResult Tokenizer::attemptBoolean() const{
	/*
	NOTE: I am not including 0 or 1, since int is easily
	convertible to bool when the situation requires it.
	*/

	MatchResult match{MATCH_BOOLEAN};

	const std::string values[] = {
		"False", "false", "True", "true"
	};

	for(int i = 0; i < 4; ++i){
		if(m_buffer_ptr->matchesString(m_byte_read_index, &values[i][0])){
			match.result = RESULT_SUCCESS;
			match.success.num_bytes_consumed = values[i].size();
			match.success.token.type = TOKEN_BOOLEAN;
			match.success.token.data.value_bool = i / 2;

			return match;
		}
	}

	match.result = RESULT_FAILURE;
	return match;
}

MatchResult Tokenizer::attemptIdentifier() const{
	MatchResult match{MATCH_IDENTIFIER};

	Int64 curr_byte = m_byte_read_index;
	Symbol curr_symbol = m_buffer_ptr->symbolAt(curr_byte);
	if(curr_symbol.isAlphabet()){
		if(m_settings.continue_identifiers_until_whitespace){
			while(m_buffer_ptr->bytesRemaining(curr_byte)){
				curr_symbol = m_buffer_ptr->symbolAt(++curr_byte);
				if(curr_symbol.isWhitespace()){
					break;
				}
			}
		}else{
			while(m_buffer_ptr->bytesRemaining(curr_byte)){
				curr_symbol = m_buffer_ptr->symbolAt(++curr_byte);
				if(!curr_symbol.isAllowedIdentifierChar()){
					break;
				}
			}
		}
		
		// I can't think of it at the moment, but there's definitely some
		// stuff that can't go at the end of an identifier.
		// TODO: Put that check here.
		if(true){
			Int64 num_bytes_consumed = curr_byte - m_byte_read_index;
			if(num_bytes_consumed > 0){
				match.result = RESULT_SUCCESS;
				match.success.num_bytes_consumed = num_bytes_consumed;
				match.success.token.type = TOKEN_IDENTIFIER;
				match.success.token.data.value_string={
					.start_byte=m_byte_read_index,
					.num_bytes=num_bytes_consumed,
				};

				return match;
			}
		}
	}else{
		match.result = RESULT_FAILURE;
	}

	return match;
}

MatchResult Tokenizer::attemptString() const{
	MatchResult match{MATCH_STRING};

	Int64 curr_byte = m_byte_read_index;
	Symbol start_symbol = m_buffer_ptr->symbolAt(curr_byte);
	if(start_symbol == '\'' || start_symbol == '"'){
		while(m_buffer_ptr->bytesRemaining(curr_byte)){
			if(m_buffer_ptr->symbolAt(++curr_byte) == start_symbol){
				break;
			}
		}

		if(m_buffer_ptr->bytesRemaining(curr_byte)){
			// Found a string that was closed before the buffer ended
			Int64 num_bytes_consumed = curr_byte - m_byte_read_index;
			match.result = RESULT_SUCCESS;
			match.success.num_bytes_consumed = 
				curr_byte - m_byte_read_index + 1;
			match.success.token.type = TOKEN_STRING;
			match.success.token.data.value_string={
				.start_byte=m_byte_read_index+1,
				.num_bytes=num_bytes_consumed-1,
			};
		}else{
			// String wasn't closed by matching type
			match.result = RESULT_MALFORMED_INPUT;
		}
	}else{
		match.result = RESULT_FAILURE;	
	}
	
	return match;
}

MatchResult Tokenizer::attemptSingleSymbolToken() const{
	MatchResult match{MATCH_SINGLE_SYMBOL_TOKEN};
	Symbol curr_symbol = m_buffer_ptr->symbolAt(m_byte_read_index);
	if(curr_symbol.isPrintableAscii()){
		match.result = RESULT_SUCCESS;
		match.success.num_bytes_consumed = 1;
		match.success.token.type = (TokenType) curr_symbol.data;
	}else{
		match.result = RESULT_FAILURE;	
	}

	return match;
}


//-----------------------------------------------------------------------------
// Helper functions
//-----------------------------------------------------------------------------
constexpr bool Tokenization::hasDataPayload(TokenType type){
	switch(type){
		case TOKEN_BOOLEAN:
		case TOKEN_INTEGER:
		case TOKEN_REAL:
		case TOKEN_STRING:
		case TOKEN_IDENTIFIER:
			return true;
		default:
			return false;
	};
}

InputBuffer Tokenization::loadFileContents(std::string filepath){
	/*
	Returns the contents of the target file in a single string.

	ATTRIBUTION: Code mostly based on sample from
		https://insanecoding.blogspot.com/2011/11/how-to-read-in-file-in-c.html

	NOTE: From my understanding, tellg() only works on *nix systems 
		and binary Windows files. 

	TODO: Better file size determination method.
	*/

	std::string file_contents;

	std::ifstream instream(filepath, std::ios::in | std::ios::binary);
	if(instream){
		instream.seekg(0, std::ios::end);
		Uint64 file_size = instream.tellg();
		file_contents.resize(file_size);

		instream.seekg(0, std::ios::beg);
		instream.read(&file_contents[0], file_size);
	}
		
	return {file_contents};
}


//-----------------------------------------------------------------------------
// Printing functions
//-----------------------------------------------------------------------------
void Tokenization::printMatchResult(const MatchResult& match, 
	const InputBuffer* source_buffer){
	/*
	Prints parse results
	*/

	printf("<MatchResult|%s|%s|", 
		MATCH_TYPE_STRINGS[match.attempt_type], 
		MATCH_RESULT_TYPE_STRINGS[match.result]);
	if(match.result == RESULT_SUCCESS){
		printf("%li bytes|", match.success.num_bytes_consumed);
		Tokenization::printToken(match.success.token, source_buffer);
	}else{
		printf("First Bad Byte: %li", match.failure.first_bad_byte_index);
	}
	printf(">");
}

void Tokenization::printToken(const Token& token, 
	const InputBuffer* source_buffer){
	/*

	*/
	
	printf("<TOKEN|");

	// Print type info
	if(token.type < TOKEN_PRINTABLE_STRINGS_OFFSET){
		printf("TOKEN_%c", token.type);
	}else{
		Int32 string_index = token.type - TOKEN_PRINTABLE_STRINGS_OFFSET;
		printf("%s", TOKEN_TYPE_STRINGS[string_index]);
	}

	// Print contents if applicable.
	if(hasDataPayload(token.type)){
		printf("|");
		switch(token.type){
			case TOKEN_BOOLEAN:
				printf("%i", token.data.value_bool);
				break;
			case TOKEN_INTEGER:
				printf("%li", token.data.value_int);
				break;
			case TOKEN_REAL:
				printf("%f", token.data.value_real);
				break;
			case TOKEN_STRING:
				printf("STR");
				printStringslice(token.data.value_string, source_buffer);
				break;
			case TOKEN_IDENTIFIER:
				printf("ID");
				printStringslice(token.data.value_string, source_buffer);
				break;
			default:
				printf("NO_DATA_FOR_TYPE");
				break;
		};
	}
	printf(">");
}

void Tokenization::printStringslice(const StringSlice& slice, 
	const InputBuffer* source_buffer){
	/*

	*/

	std::string substr = source_buffer->contents.substr(
		slice.start_byte, slice.num_bytes);
	printf("('%s')", &substr[0]);
}
