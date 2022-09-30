#pragma once

#include "Primitives.hpp"

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cassert>

class Settings{
	/*
	Group of nested namespaces each containing KV pairs of
	variants. 
	*/

	public:
		class Namespace{
			/*
			Groups settings by system. Namespaces can be nested.
			*/

			public:
				PODVariant& operator[](std::string key);
			
			public:
				// TODO: Make all this private
				bool m_is_locked{false};
				std::unordered_map<std::string, PODVariant> dict;
				std::unordered_set<std::string> contained_namespaces;
		};

	public:
		std::vector<std::string> availableNamespaces() const;
		Namespace namespaceData(std::string name) const;
		Namespace& namespaceRef(std::string namespace_name);
		void update(std::string name, Namespace other);
		void fillUndefinedValuesFromReference(Settings& other);

	protected:
		std::unordered_map<std::string, Namespace> m_namespaces; 
};
