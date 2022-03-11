#include "string_processing.h"
#include <sstream>

std::vector<std::string> SplitIntoWords(const std::string& text)
{
    std::istringstream ss(text);
    std::vector<std::string> words;

    std::string word;

    while (ss >> word)
    {
        words.push_back(word);
    }

    return words;
}
