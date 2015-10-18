#include "shapes.h"
#include "planet.h"
#include "star.h"

Shape* Shape::buildFromXMLNode(XMLNode& node)
{
	if (rapidxml::count_attributes(&node) != 1)
		raiseXMLException(node, "Expected 1 attribute");

	XMLAttribute* attr = node.first_attribute();
	if (strcmp(attr->name(), "type"))
		raiseXMLException(node, "Missing \"type\" attribute");

	const std::string type(attr->value());

	if (type == "Planet")
		return Planet::buildFromXMLNode(node);
	else if (type == "Star")
		return Star::buildFromXMLNode(node);

	raiseXMLException(node, std::string("Invalid type: ") + type);
	return nullptr; // Keep compiler happy
}
