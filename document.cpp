#include "document.h"

Document::Document()
	:id(0), relevance(0), rating(0)
{}

Document::Document(int id_, double relevance_, int rating)
	: id(id_), relevance(relevance_), rating(rating)
{}

std::ostream& operator<<(std::ostream& stream, const Document& document)
{
	stream << "{ ";
	stream << "document_id = ";
	stream << std::to_string(document.id);
	stream << ", ";
	stream << "relevance = ";
	stream << std::to_string(document.relevance);
	stream << ", ";
	stream << "rating = ";
	stream << std::to_string(document.rating);
	stream << " }";
	return stream;
}
