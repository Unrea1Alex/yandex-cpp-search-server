#pragma once
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include "document.h"
#include "paginator.h"

template <typename T, typename U>
std::ostream& operator<<(std::ostream& stream,const std::pair<T,U>& item)
{
	stream << item.first << ": " << item.second;
	return stream;
}

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

std::ostream& operator<<(std::ostream& stream, const Document& document);

template<typename T>
std::ostream& operator<<(std::ostream& stream, const PaginatorRange<T>& pagination)
{

	for(auto it = pagination.begin(); it != pagination.end(); ++it)
	{
		stream << *it;
	}

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
