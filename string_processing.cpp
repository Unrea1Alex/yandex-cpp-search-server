#include "string_processing.h"
#include <sstream>
#include <iterator>
#include "log_duration.h"


/*std::vector<std::string> SplitIntoWords(std::string_view text)
=======
std::vector<std::string> SplitIntoWords(const std::string& text)
>>>>>>> parent of a8b72ec (12.9.3  slow)
=======
std::vector<std::string> SplitIntoWords(const std::string& text)
>>>>>>> parent of a8b72ec (12.9.3  slow)
{
	std::istringstream ss(text);
	std::vector<std::string> words;

	std::copy(std::istream_iterator<std::string>(ss), std::istream_iterator<std::string>(), std::back_inserter(words));

	return words;
}*/

std::vector<std::string> SplitIntoWords(std::string_view text)
{
	std::vector<std::string> words;

	auto start = text.begin();
	auto end = text.end();
	auto next = std::find( start, end, ' ');
	
	while ( next != end ) 
	{
		if(*(next + 1) != ' ')
		{
			words.push_back( std::string( start, next ) );
		}
		
		start = next + 1;
		next = std::find( start, end, ' ');
	}
	words.push_back( std::string( start, next ) );

	return words;
}