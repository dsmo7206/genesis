#include "xml.h"
#include "glstuff.h"
#include "utils.h"

void raiseXMLException(const XMLNode& node, const std::string& message)
{
	std::stringstream oss;
	oss << "XML exception at node \"" << node.name() << "\": " << message;
	//throw std::exception(oss.str().c_str());
	printf("%s\nExiting.\n", oss.str().c_str());
	getchar();
	exit(1);
}

bool nodeNameIs(const XMLNode& node, const char* name)
{
	return strcmp(node.name(), name) == 0;
}

bool nodeHasChild(const XMLNode& node, const char* name)
{
	return node.first_node(name) != 0;
}

std::string getXMLTypeAttribute(const XMLNode& node)
{
	if (rapidxml::count_attributes(const_cast<XMLNode*>(&node)) != 1)
		raiseXMLException(node, "Expected 1 attribute");

	XMLAttribute* attr = node.first_attribute();
	if (strcmp(attr->name(), "type"))
		raiseXMLException(node, "Missing \"type\" attribute");

	return std::string(attr->value());
}

std::string buildStringFromXMLNode(XMLNode& node)
{
	return std::string(node.value());
}

bool buildBoolFromXMLNode(XMLNode& node)
{
	const std::string valueAsString = buildStringFromXMLNode(node);
	if (valueAsString == "true")
		return true;
	else if (valueAsString == "false")
		return false;
	else
	{
		raiseXMLException(node, std::string("Invalid bool: ") + valueAsString);
		return false;
	}
}

float buildFloatFromXMLNode(XMLNode& node)
{
	const std::string valueAsString = buildStringFromXMLNode(node);
	return std::stof(valueAsString);
}

double buildDoubleFromXMLNode(XMLNode& node)
{
	const std::string valueAsString = buildStringFromXMLNode(node);
	return std::stod(valueAsString);
}

int buildIntFromXMLNode(XMLNode& node)
{
	const std::string valueAsString = buildStringFromXMLNode(node);
	return std::stoi(valueAsString);
}
