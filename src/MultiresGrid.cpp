#include "MultiresGrid.hpp"


MultiresGrid::MultiresGrid(){

}

MultiresGrid::~MultiresGrid(){
	
}

bool MultiresGrid::addCell(CellAddress addr, GridCell cell){
	/*
	Attempts to add the cell at the specified address. Returns true if the address
	was not already accounted for. False otherwise.
	*/

	auto iter = m_cells.find(addr);
	if(iter != m_cells.end()){
		return false;
	}else{
		m_cells[addr] = cell;
		return true;
	}
}

bool MultiresGrid::eraseData(CellAddress addr){
	/*

	*/
	auto iter = m_cells.find(addr);
	if(iter != m_cells.end()){
		m_cells.erase(iter);
		return true;
	}else{
		return false;
	}
}

CellQuery MultiresGrid::cellData(CellAddress addr) const{
	/*

	*/

	CellQuery query_result = {
		{}, false
	};

	auto iter = m_cells.find(addr);
	if(iter != m_cells.end()){
		query_result.cell = iter->second;
		query_result.is_valid = true;
	}

	return query_result;
}

std::vector<CellAddress> MultiresGrid::allAddresses() const{
	/*
	Return a list of all addresses stored by the grid.
	*/
	
	std::vector<CellAddress> output;
	for(auto iter = m_cells.begin(); iter != m_cells.end(); ++iter){
		output.push_back(iter->first);
	}
	return output;
}


