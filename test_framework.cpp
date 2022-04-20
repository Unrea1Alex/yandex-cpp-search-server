#include "test_framework.h"

void AssertImpl(bool value, const std::string& expr_str, const std::string& file, const std::string& func, unsigned line,
				const std::string& hint)
{
	if (!value) 
	{
		std::cout << file << "("s << line << "): "s << func << ": "s;
		std::cout << "ASSERT("s << expr_str << ") failed."s;
		if (!hint.empty()) 
		{
			std::cout << " Hint: "s << hint;
		}
		std::cout << std::endl;
		abort();
	}
}
