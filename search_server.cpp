#include <cmath>
#include <iostream>
#include <array>
#include <string_view>

#include "search_server.h"
#include "string_processing.h"

SearchServer::SearchServer(const std::string& words)
{
	SetStopWords(words);
}

void SearchServer::SetStopWords(const std::string& words)
{
	std::vector<std::string> str_vector = SplitIntoWords(words);

	for(const auto& word : str_vector)
	{
		if(!IsValidWord(word))
		{
			throw std::invalid_argument("word {" + word + "} contains illegal characters");
		}
	}

	stop_words_.insert(std::begin(str_vector), std::end(str_vector));
}

std::string_view SearchServer::AddUniqueWord(std::string_view word)
{
	//auto it = std::find(std::execution::par, unique_words.begin(), unique_words.end(), word);

	/*if(it == unique_words.end())
	{
		unique_words.push_back(word.data());

		return unique_words.size() - 1;
	}

	return std::distance(unique_words.begin(), it);*/

	return *((unique_words.insert(word.data())).first);
}

void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings)
{
	CheckIsValidDocument(document_id);

	const std::vector<std::string> words = SplitIntoWordsNoStop(document);

	const double inv_word_count = 1.0 / words.size();

	std::set<std::string_view> document_words_ids;

	//std::chrono::nanoseconds dur_sum;

	for (const std::string& word : words)
	{
		//std::chrono::steady_clock::time_point start_time_ = std::chrono::steady_clock::now();

		std::string_view w = AddUniqueWord(word);

		word_to_document_freqs_[w][document_id] += inv_word_count;
		document_words_ids.emplace(w);

		//const auto end_time = std::chrono::steady_clock::now();
    	//const auto dur = end_time - start_time_;

		//dur_sum += dur;
	}

	//std::cerr << words.size() << ": "s << std::chrono::duration_cast<std::chrono::milliseconds>(dur_sum).count() << " ms"s << std::endl;

	documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status, document_words_ids });
	document_ids_.emplace(document_id);
}

int SearchServer::GetWordId(std::string_view word) const
{
	return std::distance(unique_words.begin(), std::find(std::execution::par, unique_words.begin(), unique_words.end(), word));
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus doc_status) const
{
	return FindTopDocuments(raw_query, [doc_status](int document_id, DocumentStatus status, int rating) { return status == doc_status; });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const
{
	return FindTopDocuments(raw_query, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::ACTUAL; });
}

int SearchServer::GetDocumentCount() const
{
	return documents_.size();
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query, int document_id) const
{
	const Query query = ParseQuery(raw_query);
	std::vector<std::string> matched_words;
	for (const std::string& word : query.plus_words)
	{
		if (word_to_document_freqs_.count(word) == 0)
		{
			continue;
		}
		if (word_to_document_freqs_.at(word).count(document_id))
		{
			matched_words.push_back(word);
		}
	}
	for (const std::string& word : query.minus_words)
	{
		if (word_to_document_freqs_.count(word) == 0)
		{
			continue;
		}
		if (word_to_document_freqs_.at(word).count(document_id))
		{
			matched_words.clear();
			break;
		}
	}
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

const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const
{
	static std::map<std::string, double> result;

	for(auto& [str, freq_map] : word_to_document_freqs_)
	{
		auto it = freq_map.find(document_id);

		if(it != freq_map.end())
		{
			result.at(str.data()) = it->second;
		}
	}

	return result;
}

void SearchServer::RemoveDocument(int document_id)
{
	auto words_ids = documents_.at(document_id).words;

	for(auto id : words_ids) 
	{
		word_to_document_freqs_.at(id).erase(document_id);
	}

	document_ids_.erase(document_id);
	documents_.erase(document_id);
}

void SearchServer::RemoveDocument(std::execution::sequenced_policy policy, int document_id)
{
	auto words_ids = documents_.at(document_id).words;

	std::for_each(policy, words_ids.begin(), words_ids.end(), [document_id, this](std::string_view word)
	{
		word_to_document_freqs_.at(word).erase(document_id);
	});

	document_ids_.erase(document_id);
	documents_.erase(document_id);
}

void SearchServer::RemoveDocument(std::execution::parallel_policy policy, int document_id)
{
	auto words_ids = documents_[document_id].words;

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
	return stop_words_.count(word) > 0;
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const
{
	std::vector<std::string> words;
	for (const std::string& word : SplitIntoWords(text))
	{
		if (!IsStopWord(word))
		{
			if(!IsValidWord(word))
			{
				throw std::invalid_argument("word {" + word + "} contains illegal characters");
			}
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

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string text) const
{
	if(!IsValidWord(text))
	{
		throw std::invalid_argument("word {" + text + "} contains illegal characters");
	}

	bool is_minus = false;

	if (text[0] == '-')
	{
		is_minus = true;
		text = text.substr(1);
	}
	return {text, is_minus, IsStopWord(text)};
}

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const
{
	Query query;
	for (const std::string& word : SplitIntoWords(text))
	{
		const QueryWord query_word = ParseQueryWord(word);
		if (!query_word.is_stop)
		{
			if (query_word.is_minus)
			{
				query.minus_words.insert(query_word.data);
			}
			else
			{
				query.plus_words.insert(query_word.data);
			}
		}
	}
	return query;
}

bool SearchServer::IsValidWord(const std::string& word)
{
	bool is_valid_char = none_of(word.begin(), word.end(), [](char c)
	{
		return c >= '\0' && c < ' ';
	});

	bool is_not_single_minus = !(word.size() == 1 && word[0] == '-');
	bool id_no_double_minus = !(word.size() >= 2 && word.substr(0, 2) == "--");

	return is_valid_char && is_not_single_minus && id_no_double_minus;
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
