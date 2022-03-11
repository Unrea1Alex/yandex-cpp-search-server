#include "document.h"

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
