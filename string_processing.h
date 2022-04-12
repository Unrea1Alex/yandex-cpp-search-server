#pragma once

#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <unordered_set>
#include <map>
#include "document.h"
#include "paginator.h"

std::vector<std::string_view> SplitIntoWords(std::string_view text);
std::unordered_set<std::string_view> SplitIntoUniqueWords(std::string_view text);

template <typename T>
std::string Print(const T& container)
{
	bool is_first = true;
	std::ostringstream ss;

	for(const auto& item : container)
	{
		if(!is_first)
			ss << ", ";

		is_first = false;

		ss << item;
	}

	return ss.str();
}

