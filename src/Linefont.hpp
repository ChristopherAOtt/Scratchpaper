#pragma once

#include "Primitives.hpp"

#include <string>
#include <unordered_map>
#include <vector>

/*
To define a symbol, play "connect-the-dots" with the indices in this grid,
assuming that (0,0) is the bottom left corner and (0.5, 1.0) is the top right.
The coordinates in the list are indexed as shown. The list of actual values
is written with a descending Y value due to top-to-bottom syntax. 

12  13  14
09  10  11
06  07  08
03  04  05
00  01  02
*/

struct TextArea{
	FVec2 bottom_left;
	FVec2 range;
};


struct LineIndices{
	char i0, i1;
};


// NOTE: The longest symbol is the @ character
constexpr int MAX_INDEX_PAIR_LIST_SIZE = 9;
struct IndexPairList{
	char num_defined;
	LineIndices lines[MAX_INDEX_PAIR_LIST_SIZE];
};


constexpr int NUM_COORDINATE_POINTS = 3 * 5;
constexpr FVec2 COORDINATE_LIST[NUM_COORDINATE_POINTS] = {
	{0.00, 0.00}, {0.25, 0.00}, {0.50, 0.00},
	{0.00, 0.25}, {0.25, 0.25}, {0.50, 0.25},
	{0.00, 0.50}, {0.25, 0.50}, {0.50, 0.50},
	{0.00, 0.75}, {0.25, 0.75}, {0.50, 0.75},
	{0.00, 1.00}, {0.25, 1.00}, {0.50, 1.00},
};


/*
Initializer maps characters to lists of line segments. 
	{char, {num_segments, {{segment1}, {segment2}, {segment3}, etc}}}},

	where
	{char, { <-- character to be drawn
		num_segments,  <-- Number of line segments
		{{segment1}, {segment2}, {segment3}, etc}}} <-- Segment list
	},

	The segment list is initialized as a pair of indices
	{{segment1}, {segment2}, {segment3}, etc}}} ==
	{{index1a, index2a}, {index1b, index2b}, {index1c, index2c}, etc}}}


NOTE: I understand that this could be done by using the char values as
indices into an array, but I'd like to later add support for non
ascii compatible shapes and don't want to overcomplicate this and
order-dependent sparse array. These lookups will only by performed when
the text mesh is being generated, so the performance hit of a map won't
be too bad.
*/
const std::unordered_map<char, IndexPairList> CHARACTER_INDEX_MAP = {
	// Lowercase letters
	{'a', {6, {{2,11}, {11,10}, {10,6}, {6,3}, {3,1}, {1,5}}}},
	{'b', {5, {{9,0}, {3,1}, {1,5}, {5,7}, {7,6}}}},
	{'c', {5, {{11,10}, {10,6}, {6,3}, {3,1}, {1,2}}}},
	{'d', {5, {{11,2}, {5,1}, {1,3}, {3,7}, {7,8}}}},
	{'e', {8, {{3,4}, {4,8}, {8,10}, {10,6}, {6,3}, {3,0}, {0,1}, {1,5}}}},
	{'f', {3, {{1,10}, {10,11}, {6,8}}}},
	{'g', {7, {{0,1}, {1,5}, {5,8}, {8,10}, {10,6}, {6,4}, {4,8}}}},
	{'h', {4, {{9,0}, {3,7}, {7,5}, {5,2}}}},
	{'i', {2, {{1,7}, {9,11}}}},
	{'j', {3, {{3,1}, {1,7}, {9,11}}}},
	{'k', {3, {{9,0}, {11,3}, {3,2}}}},
	{'l', {2, {{10,1}, {1,2}}}},
	{'m', {4, {{0,9}, {9,11}, {11,2}, {10,1}}}},
	{'n', {4, {{9,0}, {6,10}, {10,8}, {8,2}}}},
	{'o', {6, {{10,8}, {8,5}, {5,1}, {1,3}, {3,6}, {6,10}}}},
	{'p', {5, {{9,0}, {6,10}, {10,8}, {8,4}, {4,6}}}},
	{'q', {5, {{11,2}, {8,10}, {10,6}, {6,4}, {4,8}}}},
	{'r', {3, {{9,0}, {6,10}, {10,8}}}},
	{'s', {5, {{11, 10}, {10,6}, {6,5}, {5,1}, {1,0}}}},
	{'t', {2, {{1,10}, {6,8}}}},
	{'u', {4, {{9,3}, {3,1}, {1,5}, {11,2}}}},
	{'v', {2, {{9,1}, {1,11}}}},
	{'w', {5, {{9,0}, {0,4}, {4,2}, {2,11}, {4,10}}}},
	{'x', {2, {{9,2}, {0,11}}}},
	{'y', {2, {{9,7}, {11,0}}}},
	{'z', {3, {{9,11}, {11,0}, {0,2}}}},

	// Uppercase letters
	{'A', {5, {{0,9}, {9,13}, {13,11}, {11,2}, {6,8}}}},
	{'B', {8, {{0,12}, {12,13}, {13,11}, {11,7}, {7,5}, {5,1}, {1,0}, {7,6}}}},
	{'C', {5, {{2,1}, {1,3}, {3,9}, {9,13}, {13,14}}}},
	{'D', {6, {{0,12}, {12,13}, {13,11}, {11,5}, {5,1}, {1,0}}}},
	{'E', {4, {{2,0}, {0,12}, {12,14}, {6,8}}}},
	{'F', {3, {{0,12}, {12,14}, {6,8}}}},
	{'G', {7, {{7,8}, {8,5}, {5,1}, {1,3}, {3,9}, {9,13}, {13,14}}}},
	{'H', {3, {{0,12}, {6,8}, {2,14}}}},
	{'I', {3, {{0,2}, {12,14}, {1,13}}}},
	{'J', {4, {{12,14}, {13,4}, {4,0}, {0,3}}}},
	{'K', {3, {{0,12}, {14,6}, {6,2}}}},
	{'L', {2, {{0,12}, {0,2}}}},
	{'M', {4, {{0,12}, {12,7}, {7,14}, {14,2}}}},
	{'N', {3, {{0,12}, {12,2}, {2,14}}}},
	{'O', {6, {{1,3}, {3,9}, {9,13}, {13,11}, {11,5}, {5,1}}}},
	{'P', {5, {{0,12}, {12,13}, {13,11}, {11,7}, {7,6}}}},
	{'Q', {7, {{3,9}, {9,13}, {13,11}, {11,5}, {5,1}, {1,3}, {4,2}}}},
	{'R', {6, {{0,12}, {12,13}, {13,11}, {11,7}, {7,6}, {7,2}}}},
	{'S', {7, {{14,13}, {13,9}, {9,6}, {6,8}, {8,5}, {5,1}, {1,0}}}},
	{'T', {2, {{12,14}, {13,1}}}},
	{'U', {3, {{12,0}, {0,2}, {2,14}}}},
	{'V', {2, {{12,1}, {1,14}}}},
	{'W', {5, {{12,0}, {0,4}, {4,2}, {2,14}, {4,13}}}},
	{'X', {2, {{12,2}, {0,14}}}},
	{'Y', {4, {{12,6}, {6,8}, {8,14}, {7,1}}}},
	{'Z', {3, {{12,14}, {14,0}, {0,2}}}},

	// Numbers
	{'0', {5, {{0,12}, {12,14}, {14,2}, {2,0}, {12,2}}}},
	{'1', {3, {{0,2}, {1,13}, {13,9}}}},
	{'2', {5, {{12,14}, {14,8}, {8,6}, {6,0}, {0,2}}}},
	{'3', {4, {{12,14}, {14,2}, {2,0}, {8,6}}}},
	{'4', {3, {{12,6}, {6,8}, {14,2}}}},
	{'5', {5, {{14,12}, {12,6}, {6,8}, {8,2}, {2,0}}}},
	{'6', {5, {{14,12}, {12,0}, {0,2}, {2,8}, {8,6}}}},
	{'7', {3, {{12,14}, {14,0}, {6,8}}}},
	{'8', {5, {{12,14}, {6,8}, {0,2}, {0,12}, {2,14}}}},
	{'9', {4, {{8,6}, {6,12}, {12,14}, {14,2}}}},

	// Misc characters
	{' ',  {0, {}}},
	{'~',  {3, {{6,10}, {10,4}, {4,8}}}},
	{'`',  {2, {{12,10}}}},
	{'!',  {2, {{13,7}, {4,1}}}},
	{'#',  {4, {{3,13}, {1,11}, {12,8}, {6,2}}}},
	{'$',  {6, {{11,9}, {9,6}, {6,8}, {8,5}, {5,3}, {13,1}}}},
	{'^',  {2, {{6,13}, {13,8}}}},
	{'&',  {6, {{11,13}, {13,9}, {9,2}, {7,3}, {3,1}, {1,5}}}},
	{'*',  {4, {{6,8}, {10,4}, {9,5}, {3,11}}}},
	{'(',  {3, {{13,9}, {9,3}, {3,1}}}},
	{')',  {3, {{13,11}, {11,5}, {5,1}}}},
	{'-',  {1, {{6,8}}}},
	{'_',  {1, {{0,2}}}},
	{'+',  {2, {{10,4}, {6,8}}}},
	{'=',  {2, {{9,11}, {3,5}}}},
	{'{',  {4, {{14,13}, {13,1}, {1,2}, {7,6}}}},
	{'{',  {3, {{13,12}, {12,0}, {0,1}}}},
	{'}',  {4, {{12,13}, {13,1}, {1,0}, {7,8}}}},
	{'}',  {3, {{13,14}, {14,2}, {2,1}}}},
	{'|',  {1, {{13,1}}}},
	{':',  {2, {{13,10}, {4,1}}}},
	{';',  {2, {{13,10}, {4,0}}}},
	{'"',  {2, {{13,10}, {14,11}}}},
	{'<',  {2, {{11,6}, {6,5}}}},
	{',',  {1, {{4,0}}}},
	{'>',  {2, {{9,8}, {8,3}}}},
	{'.',  {4, {{0,1}, {1,4}, {4,3}, {3,0}}}},
	{'?',  {5, {{12,13}, {13,11}, {11,7}, {7,4}, {0,2}}}},
	{'/',  {1, {{0,14}}}},
	{'@',  {9, {
		{8,4}, {4,6}, {6,10}, {10,8}, {8,11}, {11,13}, {13,9}, {9,3}, {3,1}}}},
	{'%',  {9, {
		{12,13}, {13,10}, {10,9}, {9,12}, {1,4}, {4,5}, {5,2}, {2,1}, {0,14}}}},
	{'\\', {1, {{12,2}}}},
	{'\'', {1, {{13,10}}}},
	{'\n', {5, {{12,14}, {14,5}, {5,3}, {3,7}, {3,1}}}},
	{'\t', {3, {{6, 8}, {10,8}, {8,4}}}},
};

constexpr IndexPairList ERROR_INDICES =  {3, {{3,0}, {0,2}, {2,5}}};
constexpr IndexPairList BORDER_INDICES = {4, {{0,12}, {12,14}, {14,2}, {2,0}}};

struct TextMesh{
	/*
	Pairs vector of text coordinates with vector of indices

	NOTE: For now it's just returning a list of a->b line segments. 
	*/
	std::vector<FVec2> text_coords;
	std::vector<int> indices;
};

TextMesh generateMeshForString(std::string string, TextArea area);

