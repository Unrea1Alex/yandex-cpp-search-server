#include <cmath>
#include <iostream>
#include <array>
#include <string_view>

#include "log_duration.h"
#include "search_server.h"
#include "string_processing.h"

SearchServer::SearchServer(const std::string& words)
{
	SetStopWords(words);
}

SearchServer::SearchServer(const std::string_view words)
{
	SetStopWords(words);
}

void SearchServer::SetStopWords(std::string_view words)
{
	using namespace std::string_literals;

	auto str_vector = SplitIntoWords(words);

	for(const std::string& word : str_vector)
	{
		if(!IsValidWord(word))
		{
			throw std::invalid_argument("word {"s + word.data() + "} contains illegal characters"s);
		}
	}

	stop_words_.insert(std::begin(str_vector), std::end(str_vector));
}

std::string_view SearchServer::AddUniqueWord(const std::string& word)
{
	return *((unique_words.insert(word)).first);
}

void SearchServer::AddDocument(int document_id, std::string_view document, DocumentStatus status, const std::vector<int>& ratings)
{
	CheckIsValidDocument(document_id);

	const std::vector<std::string_view> words = std::move(SplitIntoWordsNoStop(document));

	const double inv_word_count = 1.0 / words.size();

	std::set<std::string_view> document_words_ids;

	for (const std::string_view word : words)
	{
		std::string_view current_word = AddUniqueWord(word.data());

		word_to_document_freqs_[current_word][document_id] += inv_word_count;
		document_words_ids.emplace(current_word);
	}

	documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status, document_words_ids });
	document_ids_.emplace(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentStatus doc_status) const
{
	return FindTopDocuments(raw_query, [doc_status](int document_id, DocumentStatus status, int rating) { return status == doc_status; });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query) const
{
	return FindTopDocuments(raw_query, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::ACTUAL; });
}

int SearchServer::GetDocumentCount() const
{
	return documents_.size();
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::string_view raw_query, int document_id) const
{
	return MatchDocument(std::execution::seq, raw_query, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::sequenced_policy policy, const std::string_view raw_query, int document_id) const
{
	const Query query = ParseQuery(raw_query);
	std::vector<std::string_view> matched_words;

	std::vector<std::string_view> document_words(documents_.at(document_id).words.size());

	std::transform(documents_.at(document_id).words.begin(), documents_.at(document_id).words.end(), document_words.begin(), [](std::string_view word)
	{
		return word;
	});

	std::copy_if(document_words.begin(), document_words.end(), std::back_inserter(matched_words), [&](const auto word)
	{
		return query.plus_words.count(word.data());
	});

	for (const std::string_view word : query.minus_words)
	{
		if (documents_.at(document_id).words.count(word))
		{
			return {{}, documents_.at(document_id).status};
		}
	}

	return {matched_words, documents_.at(document_id).status};
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::parallel_policy policy, const std::string_view raw_query, int document_id) const
{
	//LOG_DURATION_NS("MatchDocument");

	const Query query = ParseQuery(raw_query.data());
	std::vector<std::string_view> matched_words;

	for (const std::string_view word : query.minus_words)
	{
		if (documents_.at(document_id).words.count(word))
		{
			return {matched_words, documents_.at(document_id).status};
		}
	}

	std::vector<std::string_view> document_words(documents_.at(document_id).words.size());

	std::transform(documents_.at(document_id).words.begin(), documents_.at(document_id).words.end(), document_words.begin(), [](std::string_view word)
	{
		return word;
	});

	std::copy_if(document_words.begin(), document_words.end(), std::back_inserter(matched_words), [&](std::string_view word)
	{
		return query.plus_words.count(word.data());
	});

	return {matched_words, documents_.at(document_id).status};
}

std::set<int>::iterator SearchServer::begin() const
{
	return document_ids_.begin();
}

std::set<int>::iterator SearchServer::end() const
{
	return document_ids_.end();
}

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const
{
	static std::map<std::string_view, double> result;

	for(auto& [str, freq_map] : word_to_document_freqs_)
	{
		auto it = freq_map.find(document_id);

		if(it != freq_map.end())
		{
			result.at(str) = it->second;
		}
	}

	return result;
}

void SearchServer::RemoveDocument(int document_id)
{    
  RemoveDocument(std::execution::seq, document_id);
}

void SearchServer::RemoveDocument(std::execution::sequenced_policy policy, int document_id)
{
	std::vector<std::string_view> words_ids(documents_[document_id].words.begin(), documents_[document_id].words.end());

	std::for_each(policy, words_ids.begin(), words_ids.end(), [document_id, this](std::string_view word)
	{
		word_to_document_freqs_.at(word).erase(document_id);
	});

	document_ids_.erase(document_id);
	documents_.erase(document_id);
}

void SearchServer::RemoveDocument(std::execution::parallel_policy policy, int document_id)
{
	std::vector<std::string_view> words_ids(documents_[document_id].words.begin(), documents_[document_id].words.end());

	std::for_each(policy, words_ids.begin(), words_ids.end(), [document_id, this](std::string_view word)
	{
		word_to_document_freqs_[word].erase(document_id);
	});

	document_ids_.erase(document_id);
	documents_.erase(document_id);
}

std::set<int> SearchServer::GetDuplicatedIds() const
{
	std::set<int> result;
	std::set<std::set<std::string_view>> unique_ids;

	auto current = std::begin(documents_);
	const auto last = std::end(documents_);

	while(current != last)
	{
		auto&& [key, value] = *current;

		if(unique_ids.count(value.words) == 0)
		{
			unique_ids.emplace(value.words);
		}
		else
		{
			result.emplace(key);
		}
		current++;
	}

	return result;
}

bool SearchServer::IsStopWord(const std::string& word) const
{
	return stop_words_.count(word);
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view text) const
{
	using namespace std::string_literals;

	std::vector<std::string_view> words;
	for (const std::string& word : SplitIntoWords(text.data()))
	{
		if(!IsValidWord(word))
		{
			throw std::invalid_argument("word {"s + word.data() + "} contains illegal characters"s);
		}

		if (!IsStopWord(word.data()))
		{
			words.push_back(word);
		}
	}
	return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings)
{
	if (ratings.empty())
	{
		return 0;
	}

	int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
	return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(const std::string& text) const
{
	using namespace std::string_literals;

	if(!IsValidWord(text))
	{
		throw std::invalid_argument("word contains illegal characters"s);
	}

	if (text[0] == '-')
	{
		return {std::string(text).substr(1), true, IsStopWord(text)};
	}

	return {text.data(), false, IsStopWord(text)};
}

SearchServer::Query SearchServer::ParseQuery(const std::string_view text) const // string_view +200ms
{
	using namespace std::string_literals;

	//LOG_DURATION_NS("ParseQuery"s + text.data());

	Query query;

	for (const auto word : SplitIntoWords(text))
	{
		const QueryWord query_word = ParseQueryWord(word);

		if (!query_word.is_stop)
		{
			query_word.is_minus ? query.minus_words.insert(std::move(query_word.data)) : query.plus_words.insert(std::move(query_word.data));
		}
	}

	return query;
}

bool SearchServer::IsValidWord(const std::string& word)
{
	bool is_valid_char = std::none_of(word.begin(), word.end(), [](char c)
	{
		return c >= '\0' && c < ' ';
	});

	bool is_not_single_minus = !(word.size() == 1 && word[0] == '-');
	bool is_no_double_minus = !(word.size() >= 2 && word.substr(0, 2) == "--");

	return is_valid_char && is_not_single_minus && is_no_double_minus;
}

void SearchServer::CheckIsValidDocument(int document_id) const
{
	if(document_id < 0)
	{
		throw std::invalid_argument("document id is less than zero");
	}

	if(documents_.count(document_id))
	{
		throw std::invalid_argument("duplicate document id { id = " + std::to_string(document_id) + " }");
	}
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const
{
	return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

std::vector<Document> SearchServer::FindAllDocuments(const Query& query, DocumentStatus document_status) const
{
	return FindAllDocuments(query, [document_status](int, DocumentStatus status, int) { return status == document_status; });
}
