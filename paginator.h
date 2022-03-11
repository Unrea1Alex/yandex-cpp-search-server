#pragma once

#include <iostream>
#include <vector>
#include <algorithm>

template<typename It>
class PaginatorRange
{

public:

	PaginatorRange(){};

	PaginatorRange(const It& begin, const It& end)
		: _begin(begin), _end(end)	{};

	It begin() const
	{
		return _begin;
	}

	It end()const
	{
		return _end;
	}

private:

	It _begin;
	It _end;
};

template<typename It>
class Paginator
{
public:

	Paginator(const It& begin, const It& end, size_t page_size)
	{
		auto size = end - begin;

		for(size_t i = 0; i < size; i += page_size)
		{
			int end = i + page_size < size ? i + page_size : size;
			pages.emplace_back(PaginatorRange(begin + i, begin + end));
		}
	}

	auto begin() const
	{
		return pages.begin();
	}

	auto end() const
	{
		return pages.end();
	}

private:
	Paginator(){};

	std::vector<PaginatorRange<It>> pages;
};

template<typename T>
std::ostream& operator<<(std::ostream& stream, const PaginatorRange<T>& pagination)
{

    for(auto it = pagination.begin(); it != pagination.end(); ++it)
    {
        stream << *it;
    }

    return stream;
}
