#pragma once

#include "Primitives.hpp"

#include <unordered_map>
#include <vector>

//-------------------------------------------------------------------------------------------------
// Helper structs
//-------------------------------------------------------------------------------------------------
struct GridCell{
	/*
	Traversal Format
		|0 1 2 3 4 5 6 7| |8 9 A B C D E F|
		|U|U|U|U|C|C|C|C| |L|L|L|L|X|Y|Z|is_empty|
		Bits 0 - 3: Connections between (C)entral faces and the (U)pper face
		Bits 4 - 7: Connections between (C)entral faces and each other
		Bits 8 - B: Connections between (C)entral faces and the (L)ower face
		Bits C - E: bool for whether the X, Y, and Z axes are passable
		Bit  F    : bool indicating whether the cell is completely empty (no restrictions) 

		Bits 0 - B form the edges of a tetrahedron with points touching each of the cube faces.
		Bits C - E form a cross in the center of the cube
	
	Mesh Format
		16-bit unsigned int id of the mesh for that cell's LOD mesh.
	
	*/
	Bytes4 data;
};

struct CellQuery{
	/*
	Hacky way of packaging a cell with a flag to say if it's valid or not. Used so that
	the renderer can spam queries for different LOD levels as the player moves until 
	it finds one it likes.
	*/

	GridCell cell;
	bool is_valid;
};


//-------------------------------------------------------------------------------------------------
// Multiresolution Grid
//-------------------------------------------------------------------------------------------------
class MultiresGrid{
	/*
	Grid that stores cell data in a multiresolution grid of cells. 

	TODO: Replace GridCell struct with a template argument. For the moment, both intended use
		cases of this grid are fine with a 4-byte bitwise structure, so this is OK.
	WARNING: These are all individually addressed using a hash function + map. SLOW and untested.
	YAGNI: Replace with more efficient data structure.
		Lookup system using chunks of cells, each representing an X*X*X cube of cells. Addresses
		within each chunk can just use 3D->1D array indexing into a chunk selected by 
		the lod_number. Need to spend time optimizing.
	*/
	public:
		MultiresGrid();
		~MultiresGrid();

		bool addCell(CellAddress addr, GridCell cell);
		bool eraseData(CellAddress addr);
		CellQuery cellData(CellAddress addr) const;
		std::vector<CellAddress> allAddresses() const;
		
	private:
		std::unordered_map<CellAddress, GridCell, PODHasher> m_cells;
};
