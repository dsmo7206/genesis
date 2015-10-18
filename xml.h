#pragma once

#include <string>
#include <sstream>
#include <hash_map>
#include <rapidxml.hpp>
#include <rapidxml_iterators.hpp>
#include <rapidxml_utils.hpp>

typedef rapidxml::xml_document<> XMLDocument;
typedef rapidxml::xml_node<> XMLNode;
typedef rapidxml::xml_attribute<> XMLAttribute;
typedef rapidxml::node_iterator<char> XMLNodeIterator;

void raiseXMLException(const XMLNode& node, const std::string& message);
bool nodeNameIs(const XMLNode& node, const char* name);
bool nodeHasChild(const XMLNode& node, const char* name);
std::string getXMLTypeAttribute(const XMLNode& node);

std::string              buildStringFromXMLNode(XMLNode& node);
bool                     buildBoolFromXMLNode(XMLNode& node);
float                    buildFloatFromXMLNode(XMLNode& node);
double                   buildDoubleFromXMLNode(XMLNode& node);
int                      buildIntFromXMLNode(XMLNode& node);

class XMLChildFinder
{
	const XMLNode& m_node;
	int m_childrenFound;

	public:

	XMLChildFinder(const XMLNode& node) : m_node(node), m_childrenFound(0) {}
	~XMLChildFinder()
	{
		XMLNodeIterator it(const_cast<XMLNode*>(&m_node));
		for ( ; it != XMLNodeIterator(); ++it) --m_childrenFound;
		if (m_childrenFound < 0) raiseXMLException(m_node, "Unexpected children");
	}

	template <typename T> T required(const char* name, T (*func)(XMLNode&))
	{
		XMLNode* child = m_node.first_node(name);
		if (!child) raiseXMLException(m_node, std::string("Missing child \"") + name + "\"");
		m_childrenFound += 1;
		return func(*child);
	}

	template <typename T> T optional(const char* name, T (*func)(XMLNode&))
	{
		XMLNode* child = m_node.first_node(name);
		if (child) { m_childrenFound += 1; return func(*child); }
		else { return T(); }
	}

	template <typename T> std::vector<T> requiredVector(const char* name, T (*func)(XMLNode&))
	{
		XMLNode* child = m_node.first_node(name);
		if (!child) raiseXMLException(m_node, std::string("Missing child \"") + name + "\"");
		m_childrenFound += 1;

		std::vector<T> result;

		XMLNodeIterator it(child);
		for ( ; it != XMLNodeIterator(); ++it)
		{
			if (nodeNameIs(*it, "Item"))
				result.push_back(func(*it));
			else
				raiseXMLException(*child, std::string("Unexpected child: ") + it->name());
		}

		return result;
	}
};
