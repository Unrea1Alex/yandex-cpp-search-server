#pragma once

#include <sstream>
#include <set>
#include <vector>

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

	Document();
	
	Document(int id_, double relevance_, int rating);
};

struct DocumentData
{
	int rating;
	DocumentStatus status;
	std::vector<std::string_view> words;
};

std::ostream& operator<<(std::ostream& stream, const Document& document);

