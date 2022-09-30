#include "Settings.hpp"


//-------------------------------------------------------------------------------------------------
// Namespace
//-------------------------------------------------------------------------------------------------
PODVariant& Settings::Namespace::operator[](std::string key){
	/*
	Use normal dict functionality normally, and fail if locked.
	*/

	if(m_is_locked){
		auto iter = dict.find(key);
		if(iter == dict.end()){
			printf("Couldn't find '%s' in locked namespace\n", key.c_str());
			assert(false);
		}
	}

	return dict[key];
}


//-------------------------------------------------------------------------------------------------
// Settings
//-------------------------------------------------------------------------------------------------
std::vector<std::string> Settings::availableNamespaces() const{
	/*
	Returns a list of all saved namespace names
	*/

	std::vector<std::string> names;
	names.reserve(m_namespaces.size());
	for(auto [name, _] : m_namespaces){
		names.push_back(name);
	}

	return names;
}

Settings::Namespace Settings::namespaceData(std::string name) const{
	/*
	Returns a copy of the namespace data. Returns an empty
	namespace if the name wasn't found.
	*/

	auto iter = m_namespaces.find(name);
	if(iter != m_namespaces.end()){
		return iter->second;
	}else{
		return {};
	}
}

Settings::Namespace& Settings::namespaceRef(std::string namespace_name){
	return m_namespaces[namespace_name];
}

void Settings::update(std::string name, Namespace other){
	/*
	If the namespace already exists, copy over values. Conflicts
	are resolved by overwriting existing values with new ones.
	*/

	auto iter = m_namespaces.find(name);
	if(iter != m_namespaces.end()){
		// TODO: Implement
		Namespace& this_namespace = iter->second;
		
		// Merge set of contained namespaces
		this_namespace.contained_namespaces.insert(
			other.contained_namespaces.begin(),
			other.contained_namespaces.end());

		// Overwrite values with values from "other"
		for(auto [key, value] : other.dict){
			this_namespace.dict[key] = value;
		}
	}else{
		m_namespaces[name] = other;
	}
	m_namespaces[name].m_is_locked = true;
}

void Settings::fillUndefinedValuesFromReference(Settings& other){
	/*
	Any values undefined in the current object but defined in "other"
	will be copied over.
	*/

	for(auto& [other_name_string, other_namespace] : other.m_namespaces){
		auto iter = m_namespaces.find(other_name_string);
		if(iter != m_namespaces.end()){
			Namespace& this_namespace = iter->second;

			// Merge set of contained namespaces
			this_namespace.contained_namespaces.insert(
				other_namespace.contained_namespaces.begin(),
				other_namespace.contained_namespaces.end());

			// Copy values that don't exist locally.
			for(auto [other_key, other_value] : other_namespace.dict){
				auto key_iter = this_namespace.dict.find(other_key);
				if(key_iter == this_namespace.dict.end()){
					this_namespace.dict[other_key] = other_value;
				}
			}
		}else{
			// Can just copy everything
			m_namespaces[other_name_string] = other_namespace;
		}
	}
}
