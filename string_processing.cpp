#include "string_processing.h"
#include <sstream>
#include <iterator>
#include <string_view>
#include "log_duration.h"


/*std::vector<std::string_view> SplitIntoWords(std::string_view text)
{
	std::istringstream ss(text.data());
	std::vector<std::string_view> words;

	std::copy(std::istream_iterator<std::string>(ss), std::istream_iterator<std::string>(), std::back_inserter(words));

	return words;
}*/

std::vector<std::string_view> SplitIntoWords(std::string_view text)
{
	std::vector<std::string_view> words;

	auto start = text.begin();
	auto end = text.end();
	auto next = std::find( start, end, ' ');
	
	while ( next != end ) 
	{
		if(*(next + 1) != ' ')
		{
			//words.push_back( std::string( start, next ) );
			//words.push_back(std::basic_string_view(start, std::distance(start, next)));
			words.push_back(std::string_view(start, next - start));
		}
		
		start = next + 1;
		next = std::find( start, end, ' ');
	}
	//words.push_back( std::string( start, next ) );

	if(*start != 0)
	{
		words.push_back(std::string_view(start, next - start));
	}
	//words.push_back(std::basic_string_view(start, std::distance(start, next)));

	return words;
}

std::unordered_set<std::string_view> SplitIntoUniqueWords(std::string_view text)
{
	std::unordered_set<std::string_view> words;

	auto start = text.begin();
	auto end = text.end();
	auto next = std::find( start, end, ' ');
	
	while ( next != end ) 
	{
		if(*(next + 1) != ' ')
		{
			//words.push_back( std::string( start, next ) );
			//words.push_back(std::basic_string_view(start, std::distance(start, next)));
			words.insert(std::string_view(start, next - start));
		}
		
		start = next + 1;
		next = std::find( start, end, ' ');
	}
	//words.push_back( std::string( start, next ) );

	if(*start != 0)
	{
		words.insert(std::string_view(start, next - start));
	}
	//words.push_back(std::basic_string_view(start, std::distance(start, next)));
	
	return words;
}