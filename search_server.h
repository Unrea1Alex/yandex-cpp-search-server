#pragma once

#include <string>
#include <vector>
#include <set>
#include <map>
#include <utility>
#include <stdexcept>
#include <algorithm>
#include <numeric>
#include <sstream>
#include <unordered_set>
#include <execution>
#include <chrono>
#include <unordered_set>
#include <list>
#include <deque>
#include <string_view>
#include "document.h"
#include "log_duration.h"
#include "concurrent_map.h"

class SearchServer
{
public:

	inline static constexpr int MAX_RESULT_DOCUMENT_COUNT = 5;
	inline static constexpr double EPSILON = 1e-6;

	SearchServer(){};

	explicit SearchServer(const std::string& words);

	template<typename T>
	explicit SearchServer(T container);

	void SetStopWords(const std::string_view text);

	void AddDocument(int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings);

	template<typename T>
	std::vector<Document> FindTopDocuments(const std::string_view raw_query, T predicate) const;

	template<typename T, typename Policy>
	std::vector<Document> FindTopDocuments(Policy polycy, const std::string_view raw_query, T predicate) const;

	template<typename Policy>
	std::vector<Document> FindTopDocuments(Policy polycy, const std::string_view raw_query, DocumentStatus doc_status) const;	
	std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentStatus doc_status) const;

	template<typename Policy>
	std::vector<Document> FindTopDocuments(Policy polycy, const std::string_view raw_query) const;
	std::vector<Document> FindTopDocuments(const std::string_view raw_query) const;

	int GetDocumentCount() const;

	std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::string_view raw_query, int document_id) const;
	std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::execution::sequenced_policy policy, const std::string_view raw_query, int document_id) const;
	std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::execution::parallel_policy policy, const std::string_view raw_query, int document_id) const;

	std::set<int>::iterator begin() const;
	std::set<int>::iterator end() const;

	const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

	void RemoveDocument(int document_id);
	void RemoveDocument(std::execution::parallel_policy policy, int document_id);
	void RemoveDocument(std::execution::sequenced_policy policy, int document_id);

	std::set<int> GetDuplicatedIds() const;

private:

	struct QueryWord
	{
		std::string_view data;
		bool is_minus;
		bool is_stop;
	};

	struct Query
	{
		std::deque<std::string_view> plus_words;
		std::deque<std::string_view> minus_words;
	};

	std::set<std::string, std::less<>> stop_words_;
	std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
	std::map<int, DocumentData> documents_;
	std::set<int> document_ids_;

	std::set<std::string> unique_words;

	std::string_view AddUniqueWord(const std::string& word);

	bool IsStopWord(const std::string_view word) const;

	std::deque<std::string_view> SplitIntoWordsNoStop(const std::string_view text) const;

	static int ComputeAverageRating(const std::vector<int>& ratings);

	QueryWord ParseQueryWord(const std::string_view text) const;

	Query ParseQuery(const std::string_view text) const;

	static bool IsValidWord(const std::string_view word);

	void CheckIsValidDocument(int document_id) const;

	double ComputeWordInverseDocumentFreq(const std::string& word) const;

	template<typename T>
	std::vector<Document> FindAllDocuments(const Query& query, T predicate) const;
	template<typename T, typename Policy>
	std::vector<Document> FindAllDocuments(Policy policy, const Query& query, T predicate) const;
	std::vector<Document> FindAllDocuments(const Query& query, DocumentStatus document_status) const;
};

template<typename T>
SearchServer::SearchServer(T container)
{
	using namespace std::string_literals;

	for(const auto& item : container)
	{
		if(!IsValidWord(item))
		{
			throw std::invalid_argument("word {"s + item + "} contains illegal characters"s);
		}

		if(!item.empty())
		{
			stop_words_.insert(item);
		}
	}
}

template<typename T>
std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, T predicate) const
{
	return FindTopDocuments(std::execution::seq, raw_query, predicate);
}

template<typename T, typename Policy>
std::vector<Document> SearchServer::FindTopDocuments(Policy policy, const std::string_view raw_query, T predicate) const
{
	Query query = ParseQuery(raw_query);

	std::sort(query.plus_words.begin(), query.plus_words.end()); 
	auto last_plus = std::unique(query.plus_words.begin(), query.plus_words.end());
	query.plus_words.erase(last_plus, query.plus_words.end());

	std::sort(query.minus_words.begin(), query.minus_words.end()); 
	auto last_minus = std::unique(query.minus_words.begin(), query.minus_words.end());
	query.minus_words.erase(last_minus, query.minus_words.end());

	std::vector<Document> matched_documents;

	matched_documents = FindAllDocuments(policy, query, predicate);

	if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
	{
		matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
	}
	return matched_documents;
}

template<typename Policy>
std::vector<Document> SearchServer::FindTopDocuments(Policy policy, const std::string_view raw_query, DocumentStatus doc_status) const
{
	return FindTopDocuments(policy, raw_query, [doc_status](int document_id, DocumentStatus status, int rating) { return status == doc_status; });
}

template<typename Policy>
std::vector<Document> SearchServer::FindTopDocuments(Policy policy, const std::string_view raw_query) const
{
	return FindTopDocuments(policy, raw_query, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::ACTUAL; });
}

template<typename T>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, T predicate) const
{
	return FindAllDocuments(std::execution::seq, query, predicate);
}

template<typename T, typename Policy>
std::vector<Document> SearchServer::FindAllDocuments(Policy policy, const Query& query, T predicate) const
{
	ConcurrentMap<int, double> document_to_relevance(100);

	std::for_each(policy, query.plus_words.begin(), query.plus_words.end(), [&](auto word)
	{
		if(word_to_document_freqs_.count(word))
		{
			const double inverse_document_freq = ComputeWordInverseDocumentFreq(std::string(word));

			for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(std::string(word)))
			{
				if (predicate(document_id, documents_.at(document_id).status, documents_.at(document_id).rating))
				{
					document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
				}
			}
		}
	});

	std::map<int, double> doc_to_rel = document_to_relevance.BuildOrdinaryMap();

	for (const auto word : query.minus_words)
	{
		if (word_to_document_freqs_.count(std::string(word)) == 0)
		{
			continue;
		}
		for (const auto& [document_id, _] : word_to_document_freqs_.at(std::string(word)))
		{
			doc_to_rel.erase(document_id);
		}
	}
	
	std::vector<Document> matched_documents;
	matched_documents.reserve(doc_to_rel.size());
	
	for (const auto& [document_id, relevance] : doc_to_rel)
	{
		matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
	}

	sort(matched_documents.begin(), matched_documents.end(), [this, predicate](const Document &lhs, const Document &rhs)
	{
		if (std::abs(lhs.relevance - rhs.relevance) < EPSILON)
		{
			return lhs.rating > rhs.rating;
		}
		else
		{
			return lhs.relevance > rhs.relevance;
		} 
	});

	return matched_documents;
}