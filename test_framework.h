#pragma once

#include <string>
#include <vector>
#include <set>
#include <map>
#include <iostream>

using namespace std::string_literals;

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

#define RUN_TEST(func) RunTestImpl(#func, func)

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str, const std::string& file,
					 const std::string& func, unsigned line, const std::string& hint)
{
	if (t != u)
	{
		std::cout << std::boolalpha;
		std::cout << file << "("s << line << "): "s << func << ": "s;
		std::cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
		std::cout << t << " != "s << u << "."s;
		if (!hint.empty()) {
			std::cout << " Hint: "s << hint;
		}
		std::cout << std::endl;
		abort();
	}
}

void AssertImpl(bool value, const std::string& expr_str, const std::string& file, const std::string& func, unsigned line,
				const std::string& hint);

template <typename T, typename F>
void RunTestImpl(T funcname, F func)
{
	func();
	std::cerr << funcname << " OK" << std::endl;
}

template <typename T, typename U>
std::ostream& operator<<(std::ostream& stream,const std::pair<T,U>& item)
{
	stream << item.first << ": " << item.second;
	return stream;
}

template <typename T>
std::ostream& operator<<(std::ostream& stream, const std::vector<T>& container)
{
	stream << "[";
	stream << Print(container);
	stream << "]";
	return stream;
}

template <typename T>
std::ostream& operator<<(std::ostream& stream, const std::set<T>& container)
{
	stream << "{";
	stream << Print(container);
	stream << "}";
	return stream;
}

template <typename T, typename U>
std::ostream& operator<<(std::ostream& stream, const std::map<T, U>& container)
{
	stream << "{";
	stream << Print(container);
	stream << "}";
	return stream;
}





