#include <execution>
#include "process_queries.h"

std::vector<std::vector<Document>> ProcessQueries(const SearchServer& search_server, const std::vector<std::string>& queries)
{
	std::vector<std::vector<Document>> result(queries.size());

	transform(std::execution::par, queries.begin(), queries.end(), result.begin(), [&search_server](const auto& query)
	{
		return search_server.FindTopDocuments(query);
	});

	return result;
}

std::list<Document> ProcessQueriesJoined(const SearchServer& search_server, const std::vector<std::string>& queries)
{
	std::vector<std::vector<Document>> docs = ProcessQueries(search_server, queries);

	std::list<Document> result;

	for (const auto& doc : docs)
	{
		result.insert(result.end(), doc.begin(), doc.end());
	}

	return result;
}
