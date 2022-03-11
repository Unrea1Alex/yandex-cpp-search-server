#pragma once

#include <sstream>

enum class DocumentStatus
{
	ACTUAL,
	IRRELEVANT,
	BANNED,
	REMOVED,
};

struct Document
{
	int id;
	double relevance;
	int rating;

	Document()
		:id(0), relevance(0), rating(0)
	{}

	Document(int id_, double relevance_, int rating)
		: id(id_), relevance(relevance_), rating(rating)
	{}
};

struct DocumentData
{
	int rating;
	DocumentStatus status;
};

std::ostream& operator<<(std::ostream& stream, const Document& document);

