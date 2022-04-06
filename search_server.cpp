#include <cmath>
#include <iostream>
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

int SearchServer::AddUniqueWord(std::string word)
{
	auto it = std::find(word_ids_.begin(), word_ids_.end(), word);

	if(it == word_ids_.end())
	{
		word_ids_.push_back(word);

		return word_ids_.size() - 1;
	}

	return std::distance(word_ids_.begin(), it);
}

void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings)
{
	CheckIsValidDocument(document_id);

	const std::vector<std::string> words = SplitIntoWordsNoStop(document);

	const double inv_word_count = 1.0 / words.size();

	std::set<int> document_words_ids;

	for (const std::string& word : words)
	{
		word_to_document_freqs_[word][document_id] += inv_word_count;
		document_words_ids.emplace(AddUniqueWord(word));
	}

	documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status, document_words_ids });
	document_ids_.emplace(document_id);

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
			result.at(str) = it->second;
		}
	}

	return result;
}

void SearchServer::RemoveDocument(int document_id)
{
	document_ids_.erase(document_id);
	documents_.erase(document_id);

	for(auto& [str, fr] : word_to_document_freqs_)
	{
		fr.erase(document_id);
	}
}

void SearchServer::RemoveDocument(std::execution::sequenced_policy, int document_id)
{
	document_ids_.erase(document_id);
	documents_.erase(document_id);

	for(auto& [str, fr] : word_to_document_freqs_)
	{
		fr.erase(document_id);
	}
}

void SearchServer::RemoveDocument(std::execution::parallel_policy policy, int document_id)
{
	

	/*std::vector<std::string*> docs_to_remove;
	auto it = std::find_if(policy, std::begin(word_to_document_freqs_), std::end(word_to_document_freqs_), [document_id](auto word)
		{
			return word.second.count(document_id);
		});

	while(it != std::end(word_to_document_freqs_))
	{
		//docs_to_remove.push_back(const_cast<std::string*>(&(*it).first));

		(*it).second.erase(document_id);

		it++;

		it = std::find_if(policy, it, std::end(word_to_document_freqs_), [document_id](auto word)
		{
			return word.second.count(document_id);
		});
	}*/

	/*//std::for_each(policy, std::begin(docs_to_remove), std::end(docs_to_remove), [document_id, this](auto s)
	//{
		//word_to_document_freqs_[*s].erase(document_id);
	//});*/

	
	
	/*std::for_each(policy, std::begin(word_to_document_freqs_), std::end(word_to_document_freqs_), [document_id, this](auto& s)
	{
		s.second.erase(document_id);
	});*/



	auto words_ids = documents_.at(document_id).word_ids;

	std::for_each(policy, words_ids.begin(), words_ids.end(), [document_id, this](int id)
	{
		word_to_document_freqs_.at(word_ids_.at(id)).erase(document_id);
	});
	//{
		//word_to_document_freqs_.at(word_ids_.at(id)).erase(document_id);
	//}

	document_ids_.erase(document_id);
	documents_.erase(document_id);

	/*std::for_each(std::execution::par, std::move(document_to_words_freqs_.at(document_id).begin()), std::move(document_to_words_freqs_.at(document_id).end()),
	[&](const std::pair<std::string, double>& word_and_freq) 
	{
            word_to_document_freqs_.at(word_and_freq.first).erase(document_id);
    });
    

    documens_and_set_words_.erase(move(lower_bound(move(documens_and_set_words_.begin()), 
                                  move(documens_and_set_words_.end()),
                                  document_id,
                                  [](const auto& lhs, int document_id) { return (lhs.first < document_id); })));
    document_ids_.erase(move(lower_bound(move(begin()), move(end()), move(document_id))));*/
}

std::set<int> SearchServer::GetDuplicatedIds() const
{
	std::set<int> result;
	std::set<std::set<int>> unique_ids;

	auto current = std::begin(documents_);
	const auto last = std::end(documents_);

	while(current != last)
	{
		auto&& [key, value] = *current;

		if(unique_ids.count(value.word_ids) == 0)
		{
			unique_ids.emplace(value.word_ids);
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
