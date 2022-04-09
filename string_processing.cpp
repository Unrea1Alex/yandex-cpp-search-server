#include "string_processing.h"
#include <sstream>
#include <iterator>
#include "log_duration.h"

std::vector<std::string> SplitIntoWords(const std::string& text)
{
	std::istringstream ss(text);
	std::vector<std::string> words;

	std::move(std::istream_iterator<std::string>(ss), std::istream_iterator<std::string>(), std::back_inserter(words));

	return words;
}
