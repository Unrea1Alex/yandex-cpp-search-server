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
#include "search_server.h"
#include "paginator.h"
#include "string_processing.h"
#include "read_input_functions.h"
#include "test_framework.h"


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
		ASSERT(abs(found_docs[0].relevance - 0.12602676010180824) < SearchServer::EPSILON);
	}

	{
		const auto found_docs = server.FindTopDocuments("wide"s);
		ASSERT_EQUAL(found_docs.size(), 1);
		ASSERT(abs(found_docs[0].relevance - 0.13862943611198905) < SearchServer::EPSILON);
	}

	{
		const auto found_docs = server.FindTopDocuments("example"s);
		ASSERT_EQUAL(found_docs.size(), 1);
		ASSERT(abs(found_docs[0].relevance - 0.10663802777845313) < SearchServer::EPSILON);
	}

	{
		const auto found_docs = server.FindTopDocuments("apparently"s);
		ASSERT_EQUAL(found_docs.size(), 1);
		ASSERT(abs(found_docs[0].relevance - 0.2520535202036165) < SearchServer::EPSILON);
	}
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
}

void PrintDocument(const Document& document)
{
	std::cout << "{ "s
		 << "document_id = "s << document.id << ", "s
		 << "relevance = "s << document.relevance << ", "s
		 << "rating = "s << document.rating << " }"s << std::endl;
}

void PrintMatchDocumentResult(int document_id, const std::vector<std::string>& words, DocumentStatus status) {
	std::cout << "{ "s
		 << "document_id = "s << document_id << ", "s
		 << "status = "s << static_cast<int>(status) << ", "s
		 << "words ="s;
	for (const std::string& word : words) {
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
    try
    {
		std::cout << "Матчинг документов по запросу: "s << query << std::endl;
		const int document_count = search_server.GetDocumentCount();
        for (int index = 0; index < document_count; ++index)
        {
			//const int document_id = search_server.GetDocumentId(index);
			//const auto [words, status] = search_server.MatchDocument(query, document_id);
			//PrintMatchDocumentResult(document_id, words, status);
		}
    }
    catch (const std::invalid_argument& e)
    {
		std::cout << "Ошибка матчинга документов на запрос "s << query << ": "s << e.what() << std::endl;
	}
}

int main()
{
	//TestSearchServer();

	SearchServer search_server("и в на"s);

	search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
	search_server.AddDocument(2, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2, 3});
	search_server.AddDocument(3, "большой кот модный ошейник "s, DocumentStatus::ACTUAL, {1, 2, 8});
	search_server.AddDocument(4, "большой пёс скворец евгений"s, DocumentStatus::ACTUAL, {1, 3, 2});
	search_server.AddDocument(5, "большой пёс скворец василий"s, DocumentStatus::ACTUAL, {1, 1, 1});

	const auto search_results = search_server.FindTopDocuments("пушистый пёс"s);
	int page_size = 2;
	const auto pages = Paginate(search_results, page_size);

    for (auto page = pages.begin(); page != pages.end(); ++page)
    {
		std::cout << *page << std::endl;
		std::cout << "Разрыв страницы"s << std::endl;
	}
}
