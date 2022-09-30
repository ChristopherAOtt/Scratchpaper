#include "EntityManager.hpp"

EntityManager::EntityManager(){
	Int64 ARBITRARY_INITIAL_STATE = 2718281828;
	m_id_generator.state = ARBITRARY_INITIAL_STATE;
}

EntityManager::~EntityManager(){

}
