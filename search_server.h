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
#include "document.h"

class SearchServer
{
public:

	inline static constexpr int MAX_RESULT_DOCUMENT_COUNT = 5;
	inline static constexpr double EPSILON = 1e-6;

	SearchServer(){};

	explicit SearchServer(const std::string& words);

	template<typename T>
    explicit SearchServer(T container);

	void SetStopWords(const std::string& text);

	void AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings);

    template<typename T>
    std::vector<Document> FindTopDocuments(const std::string& raw_query, T predicate) const;
	std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentStatus doc_status) const;
	std::vector<Document> FindTopDocuments(const std::string& raw_query) const;

	int GetDocumentCount() const;
	int GetDocumentId(int index) const;

	std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string& raw_query, int document_id) const;

private:

	struct QueryWord
	{
		std::string data;
		bool is_minus;
		bool is_stop;
	};

	struct Query
	{
		std::set<std::string> plus_words;
		std::set<std::string> minus_words;
	};

	std::set<std::string> stop_words_;
	std::map<std::string, std::map<int, double>> word_to_document_freqs_;
	std::map<int, DocumentData> documents_;
	std::vector<int> document_ids_;

	bool IsStopWord(const std::string& word) const;

	std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const;

	static int ComputeAverageRating(const std::vector<int>& ratings);

	QueryWord ParseQueryWord(std::string text) const;

	Query ParseQuery(const std::string& text) const;

	static bool IsValidWord(const std::string& word);

    void CheckIsValidDocument(int document_id) const;

	double ComputeWordInverseDocumentFreq(const std::string& word) const;

    template<typename T>
    std::vector<Document> FindAllDocuments(const Query& query, T predicate) const;
    std::vector<Document> FindAllDocuments(const Query& query, DocumentStatus document_status) const;
};

template<typename T>
SearchServer::SearchServer(T container)
{
    using namespace std::string_literals;

    for(const auto& w : container)
    {
        if(!IsValidWord(w))
        {
            throw invalid_argument("word {"s + w + "} contains illegal characters"s);
        }

        if(!w.empty())
        {
            stop_words_.insert(w);
        }
    }
}

template<typename T>
std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, T predicate) const
{
    const Query query = ParseQuery(raw_query);
    auto matched_documents = FindAllDocuments(query, predicate);

    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
    {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}

template<typename T>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, T predicate) const
{
    std::map<int, double> document_to_relevance;
    for (const std::string& word : query.plus_words)
    {
        if (word_to_document_freqs_.count(word) == 0)
        {
            continue;
        }

        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);

        for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word))
        {
            if (predicate(document_id, documents_.at(document_id).status, documents_.at(document_id).rating))
            {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    for (const std::string& word : query.minus_words)
    {
        if (word_to_document_freqs_.count(word) == 0)
        {
            continue;
        }
        for (const auto& [document_id, _] : word_to_document_freqs_.at(word))
        {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    for (const auto& [document_id, relevance] : document_to_relevance)
    {
        matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
    }

    sort(matched_documents.begin(), matched_documents.end(), [this, predicate](const Document& lhs, const Document& rhs)
    {
        if (abs(lhs.relevance - rhs.relevance) < EPSILON)
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
