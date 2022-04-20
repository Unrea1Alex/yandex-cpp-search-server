#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <numeric>
#include <cassert>
#include <utility>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <random>
#include "search_server.h"
#include "paginator.h"
#include "string_processing.h"
#include "read_input_functions.h"
#include "test_framework.h"
//#include "remove_duplicates.h"
#include "request_queue.h"
#include "process_queries.h"
#include "log_duration.h"
#include <execution>

using namespace std;

template <typename Container>
auto Paginate(const Container& c, size_t page_size)
{
	return Paginator(c.begin(), c.end(), page_size);
}


void PrintDocument(const Document& document)
{
	std::cout << "{ "s
		 << "document_id = "s << document.id << ", "s
		 << "relevance = "s << document.relevance << ", "s
		 << "rating = "s << document.rating << " }"s << std::endl;
}

void PrintMatchDocumentResult(int document_id, const std::vector<std::string_view>& words, DocumentStatus status) {
	std::cout << "{ "s
		 << "document_id = "s << document_id << ", "s
		 << "status = "s << static_cast<int>(status) << ", "s
		 << "words ="s;
	for (const auto word : words) {
		std::cout << ' ' << word;
	}
	std::cout << "}"s << std::endl;
}

void AddDocument(SearchServer& search_server, int document_id, const std::string& document, DocumentStatus status,
				 const std::vector<int>& ratings)
{
	try
	{
		search_server.AddDocument(document_id, document, status, ratings);
	}
	catch (const std::invalid_argument& e)
	{
		std::cout << "Ошибка добавления документа "s << document_id << ": "s << e.what() << std::endl;
	}
}

void FindTopDocuments(const SearchServer& search_server, const std::string& raw_query)
{
	std::cout << "Результаты поиска по запросу: "s << raw_query << std::endl;
	try
	{
		for (const Document& document : search_server.FindTopDocuments(raw_query))
		{
			PrintDocument(document);
		}
	}
	catch (const std::invalid_argument& e)
	{
		std::cout << "Ошибка поиска: "s << e.what() << std::endl;
	}
}

void MatchDocuments(const SearchServer& search_server, const std::string& query)
{
	SearchServer server = static_cast<SearchServer>(search_server);

	try
	{
		std::cout << "Матчинг документов по запросу: "s << query << std::endl;

		for(const auto document_id : server)
		{
			const auto [words, status] = search_server.MatchDocument(query, document_id);
			PrintMatchDocumentResult(document_id, words, status);
		}
	}
	catch (const std::invalid_argument& e)
	{
		std::cout << "Ошибка матчинга документов на запрос "s << query << ": "s << e.what() << std::endl;
	}
}


/*string GenerateWord(mt19937& generator, int max_length) {
    const int length = uniform_int_distribution(1, max_length)(generator);
    string word;
    word.reserve(length);
    for (int i = 0; i < length; ++i) {
        word.push_back(uniform_int_distribution('a', 'z')(generator));
    }
    return word;
}

vector<string> GenerateDictionary(mt19937& generator, int word_count, int max_length) {
    vector<string> words;
    words.reserve(word_count);
    for (int i = 0; i < word_count; ++i) {
        words.push_back(GenerateWord(generator, max_length));
    }
    sort(words.begin(), words.end());
    words.erase(unique(words.begin(), words.end()), words.end());
    return words;
}

string GenerateQuery(mt19937& generator, const vector<string>& dictionary, int max_word_count) {
    const int word_count = uniform_int_distribution(1, max_word_count)(generator);
    string query;
    for (int i = 0; i < word_count; ++i) {
        if (!query.empty()) {
            query.push_back(' ');
        }
        query += dictionary[uniform_int_distribution<int>(0, dictionary.size() - 1)(generator)];
    }
    return query;
}

vector<string> GenerateQueries(mt19937& generator, const vector<string>& dictionary, int query_count, int max_word_count) {
    vector<string> queries;
    queries.reserve(query_count);
    for (int i = 0; i < query_count; ++i) {
        queries.push_back(GenerateQuery(generator, dictionary, max_word_count));
    }
    return queries;
}

template <typename ExecutionPolicy>
void Test(string_view mark, SearchServer search_server, ExecutionPolicy&& policy) {
    LOG_DURATION(mark);
    const int document_count = search_server.GetDocumentCount();
    for (int id = 0; id < document_count; ++id) {
        search_server.RemoveDocument(policy, id);
    }
    cout << search_server.GetDocumentCount() << endl;
}

#define TEST(mode) Test(#mode, search_server, execution::mode)*/

/*string GenerateWord(mt19937& generator, int max_length) {
    const int length = uniform_int_distribution(1, max_length)(generator);
    string word;
    word.reserve(length);
    for (int i = 0; i < length; ++i) {
        word.push_back(uniform_int_distribution('a', 'z')(generator));
    }
    return word;
}

vector<string> GenerateDictionary(mt19937& generator, int word_count, int max_length) {
    vector<string> words;
    words.reserve(word_count);
    for (int i = 0; i < word_count; ++i) {
        words.push_back(GenerateWord(generator, max_length));
    }
    sort(words.begin(), words.end());
    words.erase(unique(words.begin(), words.end()), words.end());
    return words;
}

string GenerateQuery(mt19937& generator, const vector<string>& dictionary, int word_count, double minus_prob = 0) {
    string query;
    for (int i = 0; i < word_count; ++i) {
        if (!query.empty()) {
            query.push_back(' ');
        }
        if (uniform_real_distribution<>(0, 1)(generator) < minus_prob) {
            query.push_back('-');
        }
        query += dictionary[uniform_int_distribution<int>(0, dictionary.size() - 1)(generator)];
    }
    return query;
}

vector<string> GenerateQueries(mt19937& generator, const vector<string>& dictionary, int query_count, int max_word_count) {
    vector<string> queries;
    queries.reserve(query_count);
    for (int i = 0; i < query_count; ++i) {
        queries.push_back(GenerateQuery(generator, dictionary, max_word_count));
    }
    return queries;
}

template <typename ExecutionPolicy>
void Test(string_view mark, SearchServer search_server, const string& query, ExecutionPolicy&& policy) {
    LOG_DURATION(mark);
    const int document_count = search_server.GetDocumentCount();
    int word_count = 0;
    for (int id = 0; id < document_count; ++id) {
        const auto [words, status] = search_server.MatchDocument(policy, query, id);
        word_count += words.size();
    }
    cout << word_count << endl;
}

#define TEST(policy) Test(#policy, search_server, query, execution::policy);*/

string GenerateWord(mt19937& generator, int max_length) {
    const int length = uniform_int_distribution(1, max_length)(generator);
    string word;
    word.reserve(length);
    for (int i = 0; i < length; ++i) {
        word.push_back(uniform_int_distribution('a', 'z')(generator));
    }
    return word;
}

vector<string> GenerateDictionary(mt19937& generator, int word_count, int max_length) {
    vector<string> words;
    words.reserve(word_count);
    for (int i = 0; i < word_count; ++i) {
        words.push_back(GenerateWord(generator, max_length));
    }
    words.erase(unique(words.begin(), words.end()), words.end());
    return words;
}

string GenerateQuery(mt19937& generator, const vector<string>& dictionary, int word_count, double minus_prob = 0) {
    string query;
    for (int i = 0; i < word_count; ++i) {
        if (!query.empty()) {
            query.push_back(' ');
        }
        if (uniform_real_distribution<>(0, 1)(generator) < minus_prob) {
            query.push_back('-');
        }
        query += dictionary[uniform_int_distribution<int>(0, dictionary.size() - 1)(generator)];
    }
    return query;
}

vector<string> GenerateQueries(mt19937& generator, const vector<string>& dictionary, int query_count, int max_word_count) {
    vector<string> queries;
    queries.reserve(query_count);
    for (int i = 0; i < query_count; ++i) {
        queries.push_back(GenerateQuery(generator, dictionary, max_word_count));
    }
    return queries;
}

template <typename ExecutionPolicy>
void Test(string_view mark, const SearchServer& search_server, const vector<string>& queries, ExecutionPolicy&& policy) {
    LOG_DURATION(mark);
    double total_relevance = 0;
    for (const string_view query : queries) {
        for (const auto& document : search_server.FindTopDocuments(policy, query)) {
            total_relevance += document.relevance;
        }
    }
    cout << total_relevance << endl;
}

#define TEST(policy) Test(#policy, search_server, queries, execution::policy)

int main()
{
	/*TestSearchServer();

	SearchServer search_server("и в на and with"s);

	search_server.AddDocument(10, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
	search_server.AddDocument(20, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2, 3});
	search_server.AddDocument(30, "большой кот модный ошейник "s, DocumentStatus::ACTUAL, {1, 2, 8});
	search_server.AddDocument(40, "большой пёс скворец евгений"s, DocumentStatus::ACTUAL, {1, 3, 2});
	search_server.AddDocument(50, "большой пёс скворец василий"s, DocumentStatus::ACTUAL, {1, 1, 1});

	AddDocument(search_server, 1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, {7, 2, 7});

	AddDocument(search_server, 2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});

	// дубликат документа 2, будет удалён
	AddDocument(search_server, 3, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});

	// отличие только в стоп-словах, считаем дубликатом
	AddDocument(search_server, 4, "funny pet and curly hair"s, DocumentStatus::ACTUAL, {1, 2});

	// множество слов такое же, считаем дубликатом документа 1
	AddDocument(search_server, 5, "funny funny pet and nasty nasty rat"s, DocumentStatus::ACTUAL, {1, 2});

	// добавились новые слова, дубликатом не является
	AddDocument(search_server, 6, "funny pet and not very nasty rat"s, DocumentStatus::ACTUAL, {1, 2});

	// множество слов такое же, как в id 6, несмотря на другой порядок, считаем дубликатом
	AddDocument(search_server, 7, "very nasty rat and not very funny pet"s, DocumentStatus::ACTUAL, {1, 2});

	// есть не все слова, не является дубликатом
	AddDocument(search_server, 8, "pet with rat and rat and rat"s, DocumentStatus::ACTUAL, {1, 2});

	// слова из разных документов, не является дубликатом
	AddDocument(search_server, 9, "nasty rat with curly hair"s, DocumentStatus::ACTUAL, {1, 2});



	const auto search_results = search_server.FindTopDocuments("пушистый пёс"s);
	int page_size = 2;
	const auto pages = Paginate(search_results, page_size);

	for (auto page = pages.begin(); page != pages.end(); ++page)
	{
		std::cout << *page << std::endl;
		std::cout << "Разрыв страницы"s << std::endl;
	}

	cout << "Before duplicates removed: "s << search_server.GetDocumentCount() << endl;
	RemoveDuplicates(search_server);
	cout << "After duplicates removed: "s << search_server.GetDocumentCount() << endl;

	{
    SearchServer search_server("and with"s);

    int id = 0;
    for (
        const string& text : {
            "funny pet and nasty rat"s,
            "funny pet with curly hair"s,
            "funny pet and not very nasty rat"s,
            "pet with rat and rat and rat"s,
            "nasty rat with curly hair"s,
        }
    ) 
	{
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
    }

    const vector<string> queries = {
        "nasty rat -not"s,
        "not very funny nasty pet"s,
        "curly hair"s
    };
    id = 0;
    for ( const auto& documents : ProcessQueries(search_server, queries))
    {
        cout << documents.size() << " documents for query ["s << queries[id++] << "]"s << endl;
    }

	}*/

	/*{
		mt19937 generator;

		const auto dictionary = GenerateDictionary(generator, 10000, 25);
		const auto documents = GenerateQueries(generator, dictionary, 10000, 100);

		{
			SearchServer search_server(dictionary[0]);
			for (size_t i = 0; i < documents.size(); ++i) {
				search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, {1, 2, 3});
			}

			TEST(seq);

			std::cout << "test 1" << "\n";
		}
		{
			SearchServer search_server(dictionary[0]);
			for (size_t i = 0; i < documents.size(); ++i) {
				search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, {1, 2, 3});
			}

			TEST(par);

			std::cout << "test 2" << "\n";
		}
	}*/

    /*mt19937 generator;

	const auto dictionary = GenerateDictionary(generator, 1000, 10);
	const auto documents = GenerateQueries(generator, dictionary, 10'000, 70);

	const string query = GenerateQuery(generator, dictionary, 500, 0.1);

	SearchServer search_server(dictionary[0]);
	for (size_t i = 0; i < documents.size(); ++i) {
			search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, {1, 2, 3});
	}

	TEST(seq);
	TEST(par);*/
    
    /*SearchServer search_server("and with"s);

    int id = 0;
    for (
        const string& text : {
            "funny pet and nasty rat"s,
            "funny pet with curly hair"s,
            "funny pet and not very nasty rat"s,
            "pet with rat and rat and rat"s,
            "nasty rat with curly hair"s,
        }
    ) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
    }

    const string query = "curly and funny -not"s;

    {
        const auto [words, status] = search_server.MatchDocument(query, 1);
        cout << words.size() << " words for document 1"s << endl;
        // 1 words for document 1
    }

    {
        const auto [words, status] = search_server.MatchDocument(execution::seq, query, 2);
        cout << words.size() << " words for document 2"s << endl;
        // 2 words for document 2
    }

    {
        const auto [words, status] = search_server.MatchDocument(execution::par, query, 3);
        cout << words.size() << " words for document 3"s << endl;
        // 0 words for document 3
    }*/

    
    
    
    /*SearchServer search_server("and with"s);

    int id = 0;
    for (
        const string& text : {
            "white cat and yellow hat"s,
            "curly cat curly tail"s,
            "nasty dog with big eyes"s,
            "nasty pigeon john"s,
        }
    ) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
    }


    cout << "ACTUAL by default:"s << endl;
    // последовательная версия
    for (const Document& document : search_server.FindTopDocuments("curly nasty cat"s)) {
        PrintDocument(document);
    }
    cout << "BANNED:"s << endl;
    // последовательная версия
    for (const Document& document : search_server.FindTopDocuments(execution::seq, "curly nasty cat"s, DocumentStatus::BANNED)) {
        PrintDocument(document);
    }

    cout << "Even ids:"s << endl;
    // параллельная версия
    for (const Document& document : search_server.FindTopDocuments(execution::par, "curly nasty cat"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
        PrintDocument(document);
    }*/


    mt19937 generator;

    const auto dictionary = GenerateDictionary(generator, 1000, 10);
    const auto documents = GenerateQueries(generator, dictionary, 10'000, 70);

    SearchServer search_server(dictionary[0]);
    for (size_t i = 0; i < documents.size(); ++i) {
        search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, {1, 2, 3});
    }

    const auto queries = GenerateQueries(generator, dictionary, 100, 70);

    TEST(seq);
    TEST(par);
}
