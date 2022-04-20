#include <algorithm>
#include <cstdlib>
#include <future>
#include <map>
#include <numeric>
#include <random>
#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include "log_duration.h"
#include "test_framework.h"

using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap 
{
public:
	static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);

	struct Access 
	{
		const std::lock_guard<std::mutex> lock;
		Value& ref_to_value;
	};

	explicit ConcurrentMap(size_t bucket_count) : m_maps_list(bucket_count), maps_list(bucket_count) {}

	std::map<Key, Value>& GetMapById(uint64_t id)
	{
		return maps_list[id];
	}

	auto& GetValue(uint64_t id, const Key& key)
	{
		auto& map = GetMapById(id); 
		return map[key];
	}

	Access operator[](const Key& key)
	{
		uint64_t id = key % maps_list.size();

		return {std::lock_guard<std::mutex>(m_maps_list[id]), GetValue(id, key)};
	}

	std::map<Key, Value> BuildOrdinaryMap()
	{
		std::map<Key, Value> result;

		for(auto i = 0; i < maps_list.size(); i++)
		{
			const std::lock_guard<std::mutex> lock(m_maps_list[i]);
			result.insert(maps_list[i].begin(), maps_list[i].end());
		}

		return result;
	}

private:

	std::vector<std::mutex> m_maps_list;
	std::vector<std::map<Key, Value>> maps_list;
};