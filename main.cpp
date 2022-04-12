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
#include "remove_duplicates.h"
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

void TestFindDocument()
{
	int doc_id = 42;
	string content = "Reading practice to help you understand texts with everyday"s;
	vector<int> ratings = {1, 2, 3};

	SearchServer server;
	server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

	{
		const auto found_docs = server.FindTopDocuments("Reading"s);
		ASSERT_EQUAL(found_docs.size(), 1);
		ASSERT_EQUAL(found_docs[0].id, 42);
	}
}

void TestExcludeStopWordsFromAddedDocumentContent() {
	const int doc_id = 42;
	const string content = "cat in the city"s;
	const vector<int> ratings = {1, 2, 3};

	{
		SearchServer server;
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		const auto found_docs = server.FindTopDocuments("in"s);
		ASSERT_EQUAL(found_docs.size(), 1);
		const Document& doc0 = found_docs[0];
		ASSERT_EQUAL(doc0.id, doc_id);
	}

	{
		SearchServer server;
		server.SetStopWords("in the"s);
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Documents vector must be empty");
	}
}

void TestExcludeDocumentWithMinusWords()
{

	int doc_id = 42;
	string content = "Reading practice to help you understand texts with everyday"s;
	vector<int> ratings = {1, 2, 3};

	SearchServer server;
	server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

	{
		const auto found_docs = server.FindTopDocuments("Reading"s);
		ASSERT_EQUAL(found_docs.size(), 1);
		ASSERT_EQUAL(found_docs[0].id, 42);
	}

	{
		const auto found_docs = server.FindTopDocuments("Reading -help"s);
		ASSERT_HINT(found_docs.empty(), "Documents vector must be empty");
	}

	{
		const auto found_docs = server.FindTopDocuments("-Reading -help"s);
		ASSERT_HINT(found_docs.empty(), "Documents vector must be empty");
	}

	{
		const auto found_docs = server.FindTopDocuments(""s);
		ASSERT_HINT(found_docs.empty(), "Documents vector must be empty");
	}
}

void TestMatching()
{
	int doc_id = 42;
	string content = "Reading practice to help you understand texts with everyday"s;
	vector<int> ratings = {1, 2, 3};

	SearchServer server;
	server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

	{
		const auto matching_words = server.MatchDocument("to help you understand reports, messages, short"s, 42);
		const auto words = get<0>( matching_words);
		ASSERT_EQUAL(words.size(), 4);
		//assert(words[0] == "help"s);
		//assert(words[1] == "to"s);
	}

	{
		const auto matching_words = server.MatchDocument("messages, short"s, 42);
		const auto words = get<0>( matching_words);
		ASSERT_EQUAL(words.size(), 0);
	}

	{
		const auto matching_words = server.MatchDocument("to help -you understand reports, messages, short"s, 42);
		const auto words = get<0>( matching_words);
		ASSERT_EQUAL(words.size(), 0);
	}
}

void TestRelevance()
{
	int doc_id = 42;
	string content = "Reading practice Reading to help you Reading understand texts with everyday"s;
	vector<int> ratings = {1, 2, 3};
	int doc_id3 = 15;
	string content3 = "practice to help you understand texts with a wide"s;
	vector<int> ratings3 = {2, -5, 30};
	int doc_id4 = 16;
	string content4 = "As with so many such answers, this one could use an example"s;
	vector<int> ratings4 = {2, 10, 30};
	int doc_id5 = 17;
	string content5 = "expected result. To Reading this struct, apparently the developer must "s;
	vector<int> ratings5 = {7, 10, 30};

	SearchServer server;
	server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
	server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings3);
	server.AddDocument(doc_id4, content4, DocumentStatus::ACTUAL, ratings4);
	server.AddDocument(doc_id5, content5, DocumentStatus::ACTUAL, ratings5);

	{
		const auto found_docs = server.FindTopDocuments("Reading"s);
		ASSERT_EQUAL(found_docs.size(), 2);
		ASSERT_HINT(found_docs[0].relevance > 0.0, "Relevance first document must be greater than 0");
		ASSERT_HINT(found_docs[1].relevance > 0.0, "Relevance second document must be greater than 0");
		ASSERT_HINT(found_docs[0].relevance >= found_docs[1].relevance, "Relevance first document must be greater than or equal to second document");
	}
}

void TestRating()
{
	int doc_id = 42;
	string content = "Reading practice Reading to help you Reading understand texts with everyday"s;
	vector<int> ratings = {1, 2, 3};
	int doc_id3 = 15;
	string content3 = "practice to help you understand texts with a wide"s;
	vector<int> ratings3 = {2, -20, 30};
	int doc_id4 = 16;
	string content4 = "As with so many such answers, this one could use an example"s;
	vector<int> ratings4 = {0, 0, 0};
	int doc_id5 = 17;
	string content5 = "expected result. To Reading this struct, apparently the developer must "s;
	vector<int> ratings5 = {-7, -10, -30};

	SearchServer server;
	server.SetStopWords("in the"s);
	server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
	server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings3);
	server.AddDocument(doc_id4, content4, DocumentStatus::ACTUAL, ratings4);
	server.AddDocument(doc_id5, content5, DocumentStatus::ACTUAL, ratings5);

	{
		const auto found_docs = server.FindTopDocuments("everyday"s);
		ASSERT_EQUAL(found_docs.size(), 1);
		ASSERT_EQUAL(found_docs[0].rating, 2);
	}

	{
		const auto found_docs = server.FindTopDocuments("wide"s);
		ASSERT_EQUAL(found_docs.size(), 1);
		ASSERT_EQUAL(found_docs[0].rating, 4);
	}

	{
		const auto found_docs = server.FindTopDocuments("example"s);
		ASSERT_EQUAL(found_docs.size(), 1);
		ASSERT_EQUAL(found_docs[0].rating, 0);
	}

	{
		const auto found_docs = server.FindTopDocuments("must"s);
		ASSERT_EQUAL(found_docs.size(), 1);
		ASSERT_EQUAL(found_docs[0].rating, -15);
	}
}

void TestPredicate()
{

	int doc_id4 = 17;
	string content4 = "vocabulary where you may need to consider the writer's"s;
	vector<int> ratings4 = {2, 10, 3};

	SearchServer server;

	server.AddDocument(doc_id4, content4, DocumentStatus::BANNED, ratings4);

	DocumentStatus doc_status = DocumentStatus::BANNED;

	{
		const auto found_docs = server.FindTopDocuments("vocabulary"s, [doc_status](int document_id, DocumentStatus status, int rating) { return status == doc_status; });
		ASSERT_EQUAL(found_docs.size(), 1);
		ASSERT_EQUAL(found_docs[0].id, 17);
	}

	doc_status = static_cast<DocumentStatus>(7);

	{
		const auto found_docs = server.FindTopDocuments("texts"s, [doc_status](int document_id, DocumentStatus status, int rating) { return status == doc_status; });
		ASSERT_EQUAL(found_docs.size(), 0);
	}
}

void TestStatus()
{
	int doc_id = 42;
	string content = "Reading practice Reading to help you Reading understand texts with everyday"s;
	vector<int> ratings = {1, 2, 3};
	int doc_id3 = 15;
	string content3 = "Reading practice to help you understand texts with a wide"s;
	vector<int> ratings3 = {2, -20, 30};
	int doc_id4 = 16;
	string content4 = "Reading As with so many such answers, this one could use an example"s;
	vector<int> ratings4 = {0, 0, 0};
	int doc_id5 = 17;
	string content5 = "Reading expected result. To Reading this struct, apparently the developer must "s;
	vector<int> ratings5 = {-7, -10, -30};

	SearchServer server;
	server.SetStopWords("in the"s);
	server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
	server.AddDocument(doc_id3, content3, DocumentStatus::BANNED, ratings3);
	server.AddDocument(doc_id4, content4, DocumentStatus::IRRELEVANT, ratings4);
	server.AddDocument(doc_id5, content5, DocumentStatus::REMOVED, ratings5);

	DocumentStatus doc_status = DocumentStatus::ACTUAL;

	{
		const auto found_docs = server.FindTopDocuments("Reading"s, doc_status);
		ASSERT_EQUAL(found_docs.size(), 1);
		ASSERT_EQUAL(found_docs[0].id, 42);
	}

	doc_status = DocumentStatus::BANNED;

	{
		const auto found_docs = server.FindTopDocuments("Reading"s, doc_status);
		ASSERT_EQUAL(found_docs.size(), 1);
		ASSERT_EQUAL(found_docs[0].id, 15);
	}

	doc_status = DocumentStatus::IRRELEVANT;

	{
		const auto found_docs = server.FindTopDocuments("Reading"s, doc_status);
		ASSERT_EQUAL(found_docs.size(), 1);
		ASSERT_EQUAL(found_docs[0].id, 16);
	}

	doc_status = DocumentStatus::REMOVED;

	{
		const auto found_docs = server.FindTopDocuments("Reading"s, doc_status);
		ASSERT_EQUAL(found_docs.size(), 1);
		ASSERT_EQUAL(found_docs[0].id, 17);
	}

}

void TestRelevanceCorrect()
{
	int doc_id = 42;
	string content = "Reading practice Reading to help you Reading understand texts with everyday"s;
	vector<int> ratings = {1, 2, 3};
	int doc_id3 = 15;
	string content3 = "Reading practice to help you understand texts with a wide"s;
	vector<int> ratings3 = {2, -20, 30};
	int doc_id4 = 16;
	string content4 = "Reading As with so many such answers, this one could use an example"s;
	vector<int> ratings4 = {0, 0, 0};
	int doc_id5 = 17;
	string content5 = "Reading expected result. To Reading this struct, apparently the developer must apparently"s;
	vector<int> ratings5 = {-7, -10, -30};

	SearchServer server;
	server.SetStopWords("in the"s);
	server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
	server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings3);
	server.AddDocument(doc_id4, content4, DocumentStatus::ACTUAL, ratings4);
	server.AddDocument(doc_id5, content5, DocumentStatus::ACTUAL, ratings5);

	{
		const auto found_docs = server.FindTopDocuments("everyday"s);
		ASSERT_EQUAL(found_docs.size(), 1);
		ASSERT(std::abs(found_docs[0].relevance - 0.12602676010180824) < SearchServer::EPSILON);
	}

	{
		const auto found_docs = server.FindTopDocuments("wide"s);
		ASSERT_EQUAL(found_docs.size(), 1);
		ASSERT(std::abs(found_docs[0].relevance - 0.13862943611198905) < SearchServer::EPSILON);
	}

	{
		const auto found_docs = server.FindTopDocuments("example"s);
		ASSERT_EQUAL(found_docs.size(), 1);
		ASSERT(std::abs(found_docs[0].relevance - 0.10663802777845313) < SearchServer::EPSILON);
	}

	{
		const auto found_docs = server.FindTopDocuments("apparently"s);
		ASSERT_EQUAL(found_docs.size(), 1);
		ASSERT(std::abs(found_docs[0].relevance - 0.2520535202036165) < SearchServer::EPSILON);
	}
}

void TestDocumentsMatching() 
{
		using namespace std::string_literals;

    const int doc_id = 42;
    const std::string content = "cat in the city";
    const std::vector<int> ratings = {1, 2, 3};

    SearchServer server;
    server.AddDocument(doc_id, "cat in the city"s, DocumentStatus::ACTUAL, ratings);

    std::vector<std::string_view> result_vec;
    DocumentStatus result_status;
    tie(result_vec, result_status) = server.MatchDocument("cat", 42);

		for(auto res : result_vec)
		{
			std::cout << res << std::endl;
		}

    //std::cout << "Returned: " << result_vec << std::endl;
    // More code here
}

void TestSearchServer()
{
	RUN_TEST(TestFindDocument);
	RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
	RUN_TEST(TestExcludeDocumentWithMinusWords);
	RUN_TEST(TestMatching);
	RUN_TEST(TestRelevance);
	RUN_TEST(TestRating);
	RUN_TEST(TestPredicate);
	RUN_TEST(TestStatus);
	RUN_TEST(TestRelevanceCorrect);
	RUN_TEST(TestDocumentsMatching);
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

void AddDocument(SearchServer& search_server, int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings)
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

#define TEST(policy) Test(#policy, search_server, query, execution::policy);

int main()
{
	TestSearchServer();
	{
		SearchServer search_server("и в на and with"s);

		search_server.AddDocument(10, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
		search_server.AddDocument(20, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2, 3});
		search_server.AddDocument(30, "большой кот модный ошейник "s, DocumentStatus::ACTUAL, {1, 2, 8});
		search_server.AddDocument(40, "большой пёс скворец евгений"s, DocumentStatus::ACTUAL, {1, 3, 2});
		search_server.AddDocument(50, "большой пёс скворец василий"s, DocumentStatus::ACTUAL, {1, 1, 1});

		AddDocument(search_server, 1, std::string("funny pet and nasty rat"s), DocumentStatus::ACTUAL, {7, 2, 7});

		AddDocument(search_server, 2, std::string("funny pet with curly hair"s), DocumentStatus::ACTUAL, {1, 2});

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

		MatchDocuments(search_server, "пушистый pet"s);



		const auto search_results = search_server.FindTopDocuments("пушистый пушистый pet"s);
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
	}

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
        std::string("nasty rat -not"s),
        "not very funny nasty pet"s,
        "curly hair"s
    };
    id = 0;
    for ( const auto& documents : ProcessQueries(search_server, queries))
    {
        cout << documents.size() << " documents for query ["s << queries[id++] << "]"s << endl;
    }

	}

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

mt19937 generator;

	const auto dictionary = GenerateDictionary(generator, 1000, 10);
	const auto documents = GenerateQueries(generator, dictionary, 10'000, 70);

	const string query = GenerateQuery(generator, dictionary, 500, 0.1);

	SearchServer search_server(dictionary[0]);
	for (size_t i = 0; i < documents.size(); ++i) {
			search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, {1, 2, 3});
	}

	TEST(seq);
	TEST(par);

	/*{
	SearchServer search_server("and with"s);

    int id = 0;
    for (
        const string& text : {
            std::string("funny pet and nasty rat"s),
            std::string("funny pet with curly hair"s),
            std::string("funny pet and not very nasty rat"s),
            std::string("pet with rat and rat and rat"s),
            std::string("nasty rat with curly hair"s),
        }
    ) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
    }

    const string query = "curly and funny -not"s;

    {
        const auto [words, status] = search_server.MatchDocument(std::string("curly and funny -not"s), 1);
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
    }
	}*/
}
