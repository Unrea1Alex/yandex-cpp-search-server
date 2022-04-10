#pragma once

#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include "document.h"
#include "paginator.h"

<<<<<<< HEAD
<<<<<<< HEAD
std::vector<std::string> SplitIntoWords(std::string_view text);
=======
std::vector<std::string> SplitIntoWords(const std::string& text);
>>>>>>> parent of a8b72ec (12.9.3  slow)
=======
std::vector<std::string> SplitIntoWords(const std::string& text);
>>>>>>> parent of a8b72ec (12.9.3  slow)

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

