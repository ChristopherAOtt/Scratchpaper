#pragma once

#include <string>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include <cassert>
#include <vector>

typedef int8_t  Int08;
typedef int16_t Int16;
typedef int32_t Int32;
typedef int64_t Int64;

typedef uint8_t  Uint08;
typedef uint16_t Uint16;
typedef uint32_t Uint32;
typedef uint64_t Uint64;
typedef double Real;

enum TokenType{
	/*
	Enum to get ascii characters and a limited group of symbols
	into the same format.

	NOTE: The commented out operators are there as a message. Don't add these,
	because they conflict with values in the lower ascii range, and I want all
	multi-character tokens to be above 256
	*/

	// ASCII-Character tokens
	TOKEN_SPACE = 32,                //' '
	TOKEN_EXCLAMATION = 33,          // !
	TOKEN_DOUBLE_QUOTE = 34,         // "
	TOKEN_POUND_SIGN = 35,           // #
	TOKEN_DOLLAR_SIGN = 36,          // $
	TOKEN_PERCENTAGE = 37,           // %
	TOKEN_AMPERSAND = 38,            // &
	TOKEN_SINGLE_QUOTE = 39,         // '
	TOKEN_OPEN_PARENTHESES = 40,     // (
	TOKEN_CLOSE_PARENTHESES = 41,    // )
	TOKEN_ASTERIX = 42,              // *
	TOKEN_PLUS_SIGN = 43,            // +
	TOKEN_COMMA = 44,                // ,
	TOKEN_MINUS_SIGN = 45,           // -
	TOKEN_PERIOD = 46,               // .
	TOKEN_FORWARD_SLASH = 47,        // /
	TOKEN_0 = 48,                    // 0
	TOKEN_1 = 49,                    // 1
	TOKEN_2 = 50,                    // 2
	TOKEN_3 = 51,                    // 3
	TOKEN_4 = 52,                    // 4
	TOKEN_5 = 53,                    // 5
	TOKEN_6 = 54,                    // 6
	TOKEN_7 = 55,                    // 7
	TOKEN_8 = 56,                    // 8
	TOKEN_9 = 57,                    // 9
	TOKEN_COLON = 58,                // :
	TOKEN_SEMICOLON = 59,            // ;
	TOKEN_LESS_THAN_SIGN = 60,       // <
	TOKEN_EQUALS_SIGN = 61,          // =
	TOKEN_GREATER_THAN_SIGN = 62,    // >
	TOKEN_QUESTION_MARK = 63,        // ?
	TOKEN_AT_SIGN = 64,              // @
	TOKEN_UPPERCASE_A = 65,          // A
	TOKEN_UPPERCASE_B = 66,          // B
	TOKEN_UPPERCASE_C = 67,          // C
	TOKEN_UPPERCASE_D = 68,          // D
	TOKEN_UPPERCASE_E = 69,          // E
	TOKEN_UPPERCASE_F = 70,          // F
	TOKEN_UPPERCASE_G = 71,          // G
	TOKEN_UPPERCASE_H = 72,          // H
	TOKEN_UPPERCASE_I = 73,          // I
	TOKEN_UPPERCASE_J = 74,          // J
	TOKEN_UPPERCASE_K = 75,          // K
	TOKEN_UPPERCASE_L = 76,          // L
	TOKEN_UPPERCASE_M = 77,          // M
	TOKEN_UPPERCASE_N = 78,          // N
	TOKEN_UPPERCASE_O = 79,          // O
	TOKEN_UPPERCASE_P = 80,          // P
	TOKEN_UPPERCASE_Q = 81,          // Q
	TOKEN_UPPERCASE_R = 82,          // R
	TOKEN_UPPERCASE_S = 83,          // S
	TOKEN_UPPERCASE_T = 84,          // T
	TOKEN_UPPERCASE_U = 85,          // U
	TOKEN_UPPERCASE_V = 86,          // V
	TOKEN_UPPERCASE_W = 87,          // W
	TOKEN_UPPERCASE_X = 88,          // X
	TOKEN_UPPERCASE_Y = 89,          // Y
	TOKEN_UPPERCASE_Z = 90,          // Z
	TOKEN_OPEN_SQUARE_BRACKET = 91,  // [
	TOKEN_BACKSLASH = 92,            /* \ */
	TOKEN_CLOSE_SQUARE_BRACKET = 93, // ]
	TOKEN_CIRCUMFLEX_ACCENT = 94,    // ^
	TOKEN_UNDERSCORE = 95,           // _
	TOKEN_BACK_APOSTROPHE = 96,      // `
	TOKEN_LOWERCASE_A = 97,          // a
	TOKEN_LOWERCASE_B = 98,          // b
	TOKEN_LOWERCASE_C = 99,          // c
	TOKEN_LOWERCASE_D = 100,         // d
	TOKEN_LOWERCASE_E = 101,         // e
	TOKEN_LOWERCASE_F = 102,         // f
	TOKEN_LOWERCASE_G = 103,         // g
	TOKEN_LOWERCASE_H = 104,         // h
	TOKEN_LOWERCASE_I = 105,         // i
	TOKEN_LOWERCASE_J = 106,         // j
	TOKEN_LOWERCASE_K = 107,         // k
	TOKEN_LOWERCASE_L = 108,         // l
	TOKEN_LOWERCASE_M = 109,         // m
	TOKEN_LOWERCASE_N = 110,         // n
	TOKEN_LOWERCASE_O = 111,         // o
	TOKEN_LOWERCASE_P = 112,         // p
	TOKEN_LOWERCASE_Q = 113,         // q
	TOKEN_LOWERCASE_R = 114,         // r
	TOKEN_LOWERCASE_S = 115,         // s
	TOKEN_LOWERCASE_T = 116,         // t
	TOKEN_LOWERCASE_U = 117,         // u
	TOKEN_LOWERCASE_V = 118,         // v
	TOKEN_LOWERCASE_W = 119,         // w
	TOKEN_LOWERCASE_X = 120,         // x
	TOKEN_LOWERCASE_Y = 121,         // y
	TOKEN_LOWERCASE_Z = 122,         // z
	TOKEN_OPEN_CURLY_BRACKET = 123,  // {
	TOKEN_VERTICAL_BAR = 124,        // |
	TOKEN_CLOSE_CURLY_BRACKET = 125, // }
	TOKEN_TILDE = 126,               // ~

	// Data-carrying tokens
	TOKEN_BOOLEAN = 256,
	TOKEN_INTEGER,
	TOKEN_REAL,
	TOKEN_STRING,
	TOKEN_IDENTIFIER,

	// Misc
	TOKEN_STATIC,
	TOKEN_NEWLINE,
	TOKEN_WHITESPACE,
	TOKEN_NULLPOINTER,
	TOKEN_DECLARATION,
	TOKEN_ASSIGNMENT,
	TOKEN_SCOPE_RESOLUTION,
	TOKEN_NAMESPACE,
	TOKEN_TEMPLATE,

	// Control flow
	TOKEN_BEGIN_IF,
	TOKEN_ELSE_IF,
	TOKEN_ELSE,
	TOKEN_WHILE,
	TOKEN_FOR,
	TOKEN_RETURN,

	// Operators
	//TOKEN_OPERATOR_PLUS,
	//TOKEN_OPERATOR_MINUS,
	//TOKEN_OPERATOR_MULTIPLY,
	//TOKEN_OPERATOR_DIVIDE,
	TOKEN_OPERATOR_PLUS_EQUALS,
	TOKEN_OPERATOR_MINUS_EQUALS,
	TOKEN_OPERATOR_MULTIPLY_EQUALS,
	TOKEN_OPERATOR_DIVIDE_EQUALS,
	TOKEN_OPERATOR_EQUALITY_CHECK,
	TOKEN_OPERATOR_INEQUALITY_CHECK,
	//TOKEN_OPERATOR_GREATER_THAN,
	//TOKEN_OPERATOR_LESS_THAN,
	TOKEN_OPERATOR_GREATER_THAN_OR_EQUALS,
	TOKEN_OPERATOR_LESS_THAN_OR_EQUALS,
	TOKEN_OPERATOR_UNARY_INCREMENT,
	TOKEN_OPERATOR_UNARY_DECREMENT,

	// Bit operators
	TOKEN_BITSHIFT_LEFT,
	TOKEN_BITSHIFT_RIGHT,
	//TOKEN_BITWISE_AND,
	//TOKEN_BITWISE_XOR,

	// Logic
	TOKEN_LOGIC_AND,
	TOKEN_LOGIC_OR,
	//TOKEN_LOGIC_NOT,

	// TODO: More tokens

	// Status info
	TOKEN_STREAM_BEGIN,
	TOKEN_STREAM_END,
	TOKEN_INVALID_TOKEN,
};

constexpr Int32 TOKEN_PRINTABLE_STRINGS_OFFSET = 256;
constexpr const char* TOKEN_TYPE_STRINGS[] = {
	// NOTE: These start offset by 256
	"TOKEN_BOOLEAN",
	"TOKEN_INTEGER",
	"TOKEN_REAL",
	"TOKEN_STRING",
	"TOKEN_IDENTIFIER",

	"TOKEN_STATIC",
	"TOKEN_NEWLINE",
	"TOKEN_WHITESPACE",
	"TOKEN_NULLPOINTER",
	"TOKEN_DECLARATION",
	"TOKEN_ASSIGNMENT",
	"TOKEN_SCOPE_RESOLUTION",
	"TOKEN_NAMESPACE",
	"TOKEN_TEMPLATE",

	"TOKEN_BEGIN_IF",
	"TOKEN_ELSE_IF",
	"TOKEN_ELSE",
	"TOKEN_WHILE",
	"TOKEN_FOR",
	"TOKEN_RETURN",

	"TOKEN_OPERATOR_PLUS_EQUALS",
	"TOKEN_OPERATOR_MINUS_EQUALS",
	"TOKEN_OPERATOR_MULTIPLY_EQUALS",
	"TOKEN_OPERATOR_DIVIDE_EQUALS",
	"TOKEN_OPERATOR_EQUALITY_CHECK",
	"TOKEN_OPERATOR_INEQUALITY_CHECK",
	"TOKEN_OPERATOR_GREATER_THAN_OR_EQUALS",
	"TOKEN_OPERATOR_LESS_THAN_OR_EQUALS",
	"TOKEN_OPERATOR_UNARY_INCREMENT",
	"TOKEN_OPERATOR_UNARY_DECREMENT",

	"TOKEN_BITSHIFT_LEFT",
	"TOKEN_BITSHIFT_RIGHT",
	
	"TOKEN_LOGIC_AND",
	"TOKEN_LOGIC_OR",
	
	"TOKEN_STREAM_BEGIN",
	"TOKEN_STREAM_END",
	"TOKEN_INVALID_TOKEN",
};


struct ReservedWordMapping{
	const char* word;
	Int32 string_len;
	TokenType type;
};

constexpr Int32 NUM_RESERVED_WORD_MAPPINGS = 30;
constexpr ReservedWordMapping RESERVED_WORD_MAPPINGS[] = {
	// Misc
	{"static",    6, TOKEN_STATIC},
	{"null",      4, TOKEN_NULLPOINTER},
	{"Null",      4, TOKEN_NULLPOINTER},
	{"NULL",      4, TOKEN_NULLPOINTER},
	{"nullptr",   7, TOKEN_NULLPOINTER},
	{"let",       3, TOKEN_DECLARATION},
	{":=",        2, TOKEN_ASSIGNMENT},
	{"::",        2, TOKEN_SCOPE_RESOLUTION},
	{"namespace", 9, TOKEN_NAMESPACE},
	{"template",  8, TOKEN_TEMPLATE},

	// Control flow
	{"if",        2, TOKEN_BEGIN_IF},
	{"elif",      4, TOKEN_ELSE_IF},
	{"else",      4, TOKEN_ELSE},
	{"while",     5, TOKEN_WHILE},
	{"for",       3, TOKEN_FOR},
	{"return",    6, TOKEN_RETURN},

	// Operators
	{"+=",        2, TOKEN_OPERATOR_PLUS_EQUALS},
	{"-=",        2, TOKEN_OPERATOR_MINUS_EQUALS},
	{"*=",        2, TOKEN_OPERATOR_MULTIPLY_EQUALS},
	{"/=",        2, TOKEN_OPERATOR_DIVIDE_EQUALS},
	{"==",        2, TOKEN_OPERATOR_EQUALITY_CHECK},
	{"!=",        2, TOKEN_OPERATOR_INEQUALITY_CHECK},
	{">=",        2, TOKEN_OPERATOR_GREATER_THAN_OR_EQUALS},
	{"<=",        2, TOKEN_OPERATOR_LESS_THAN_OR_EQUALS},
	{"++",        2, TOKEN_OPERATOR_UNARY_INCREMENT},
	{"--",        2, TOKEN_OPERATOR_UNARY_DECREMENT},

	// Bit operators
	{"<<",        2, TOKEN_BITSHIFT_LEFT},
	{">>",        2, TOKEN_BITSHIFT_RIGHT},

	// Logic
	{"&&",        2, TOKEN_LOGIC_AND},
	{"||",        2, TOKEN_LOGIC_OR},
};


constexpr Int32 LOWEST_PRINTABLE_ASCII = 32;    // ' ';
constexpr Int32 HIGHEST_PRINTABLE_ASCII = 126;  // '~';
constexpr Int32 INVALID_SYMBOL_VALUE = -1;
struct Symbol{
	Symbol(int data);
	Symbol(const Symbol& other);

	bool operator==(const Symbol& other);
	bool operator==(int other);
	bool operator!=(const Symbol& other);
	bool operator!=(int other);
	bool operator<(int other);
	bool operator>(int other);
	bool operator<=(int other);
	bool operator>=(int other);
	bool isDigit() const;
	bool isCapitalAlphabet() const;
	bool isLowercaseAlphabet() const;
	bool isAlphabet() const;
	bool isWhitespace() const;
	bool isAllowedIdentifierChar() const;
	bool isPrintableAscii() const;

	// Unicode is max 4 bytes, this is for -1. 
	Int64 data;  
};


struct StringSlice{
	/*
	Like a non Unicode-hostile version of StringView
	*/

	Int64 start_byte;
	Int64 num_bytes;
};


constexpr Int64 UNDEFINED_POSITION = -1;
struct Token{
	TokenType type{TOKEN_INVALID_TOKEN};
	union{
		bool        value_bool;
		Int64       value_int;
		Real        value_real;
		StringSlice value_string;
	} data;

	struct{
		Int64 start_line, end_line{UNDEFINED_POSITION};
		Int64 start_col,  end_col{UNDEFINED_POSITION};
	};
};
struct InputBuffer;
std::string tokenText(Token token, const InputBuffer* buffer);


struct InputBuffer{
	/*
	For now this is just a dumb wrapper to get function signatures consistent.
	TODO: Encapsulate and add bounds checking.
	YAGNI: Page in data from file in chunks instead of copying everything
		into RAM.
	*/

	std::string contents;

	// Helper functions
	Symbol symbolAt(Int64 byte_index) const;
	std::string byteSlice(Int64 byte_index, Int32 range) const;
	bool matchesString(Int64 byte_index, const char* word) const;
	Int64 bytesRemaining(Int64 byte_index) const;
};

//-----------------------------------------------------------------------------
// Helper functions/definitions that will be moved into the tokenizer private
// namespace later. For now I'm trying to avoid all the formatting
// cruft that comes with that.
// TODO: Move to Tokenzier/Private once everything is functional
//-----------------------------------------------------------------------------
enum MatchType{
	MATCH_INVALID = 0,

	MATCH_NEWLINE,
	MATCH_WHITESPACE,
	MATCH_LINE_COMMENT,
	MATCH_BLOCK_COMMENT,
	
	MATCH_RESERVED_WORD,
	MATCH_NUMERIC,
	MATCH_BOOLEAN,
	MATCH_IDENTIFIER,
	MATCH_STRING,
	MATCH_SINGLE_SYMBOL_TOKEN,
};

constexpr const char* MATCH_TYPE_STRINGS[] = {
	"MATCH_INVALID",

	"MATCH_NEWLINE",
	"MATCH_WHITESPACE",
	"MATCH_LINE_COMMENT",
	"MATCH_BLOCK_COMMENT",
	"MATCH_RESERVED_WORD",
	"MATCH_NUMERIC",
	"MATCH_BOOLEAN",
	"MATCH_IDENTIFIER",
	"MATCH_STRING",
	"MATCH_SINGLE_SYMBOL_TOKEN",
};

enum MatchResultType{
	RESULT_INVALID = 0,

	RESULT_SUCCESS,  // The attempted parse worked
	RESULT_FAILURE,  // The parse failed for the attempted type
	RESULT_MALFORMED_INPUT,  // It's the input stream's fault
};

constexpr const char* MATCH_RESULT_TYPE_STRINGS[] = {
	"RESULT_INVALID",

	"RESULT_SUCCESS",
	"RESULT_FAILURE",
	"RESULT_MALFORMED_INPUT"
};

struct MatchResult{
	MatchType attempt_type{MATCH_INVALID};
	MatchResultType result{RESULT_INVALID};
	
	struct{  // Only defined for success
		Int64 num_bytes_consumed{0};  // Num bytes to advance by
		Int32 num_newlines_consumed{0};

		// Only defined for certain result types
		Token token;
	}success;

	struct{  // Only defined for failure and malformed input
		Int64 first_bad_byte_index{0};
	}failure;
};


class Tokenizer{
	/*
	TODO: Add support for \r\n newline type. Right now that's only handled
		in the attemptNewline() function.
	
	TODO: This thing runs like garbage. Improve tokenization speed.
	*/

	public:
		struct Settings{
			/*
			Customize the tokenizer's behavior so it works for a 
			wider range of input types.
			*/

			std::string line_comment_start;
			std::string block_comment_start;
			std::string block_comment_end;
			bool emit_newline_tokens;

			// Once an identifier has begun, only terminate it on whitespace
			bool continue_identifiers_until_whitespace;

			// Some file types are easier to parse if whitespace is included.
			bool emit_whitespace_tokens;

			// Can be expensive for non script text files, which don't have
			// these anyways.
			bool attempt_reserved_words;

			static Settings initDefault();
		};

	public:
		Tokenizer(const InputBuffer* buffer_ptr, Settings settings);
		Tokenizer(const InputBuffer* buffer_ptr);
		Token nextToken();

	public:	
		// What no Regex does to a mf
		MatchResult attemptWhitespace() const;
		MatchResult attemptLineComment() const;
		MatchResult attemptBlockComment() const;
		MatchResult attemptNewline() const;
		MatchResult attemptNumeric() const;
		MatchResult attemptReservedWord() const;
		MatchResult attemptBoolean() const;
		MatchResult attemptIdentifier() const;
		MatchResult attemptString() const;
		MatchResult attemptSingleSymbolToken() const;

	private:
		std::vector<MatchType> m_types_to_skip;
		Settings m_settings;
		const InputBuffer* m_buffer_ptr;
		Int64 m_byte_read_index;

		// Tracking info for line/column info
		Int64 curr_line_num;
		Int64 curr_column_num;

	private:
		typedef MatchResult (Tokenizer::*AttemptMatchFunction)() const;
		struct MatchGroup{
			MatchType type;
			AttemptMatchFunction function_pointer;
			bool is_token_type;
		};
		std::vector<MatchGroup> m_match_list;

		void init(const InputBuffer* buffer_ptr, Settings settings);
};

namespace Tokenization{
	// Helper functions
	constexpr bool hasDataPayload(TokenType type);
	InputBuffer loadFileContents(std::string filepath);

	// Printing functions
	void printMatchResult(const MatchResult& result, 
		const InputBuffer* source_buffer);
	void printToken(const Token& token, const InputBuffer* source_buffer);
	void printStringslice(const StringSlice& slice, 
		const InputBuffer* source_buffer);
};


