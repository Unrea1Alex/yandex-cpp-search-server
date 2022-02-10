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

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;

template <typename T, typename U>
ostream& operator<<(ostream& stream,const pair<T,U>& item)
{
	stream << item.first << ": " << item.second;
	return stream;
}

template <typename T>
string Print(const T& container)
{
	bool is_first = true;
	ostringstream ss;

	for(const auto& item : container)
	{
		if(!is_first)
			ss << ", ";

		is_first = false;

		ss << item;
	}

	return ss.str();
}

template <typename T>
ostream& operator<<(ostream& stream, const vector<T>& container)
{
	stream << "[";
	stream << Print(container);
	stream << "]";
	return stream;
}

template <typename T>
ostream& operator<<(ostream& stream, const set<T>& container)
{
	stream << "{";
	stream << Print(container);
	stream << "}";
	return stream;
}

template <typename T, typename U>
ostream& operator<<(ostream& stream, const map<T, U>& container)
{
	stream << "{";
	stream << Print(container);
	stream << "}";
	return stream;
}

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
					 const string& func, unsigned line, const string& hint) {
	if (t != u) {
		cout << boolalpha;
		cout << file << "("s << line << "): "s << func << ": "s;
		cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
		cout << t << " != "s << u << "."s;
		if (!hint.empty()) {
			cout << " Hint: "s << hint;
		}
		cout << endl;
		abort();
	}
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
				const string& hint) {
	if (!value) {
		cout << file << "("s << line << "): "s << func << ": "s;
		cout << "ASSERT("s << expr_str << ") failed."s;
		if (!hint.empty()) {
			cout << " Hint: "s << hint;
		}
		cout << endl;
		abort();
	}
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename T, typename F>
void RunTestImpl(T funcname, F func)
{
	func();
	cerr << funcname << " OK" << endl;
}

#define RUN_TEST(func) RunTestImpl(#func, func)  // напишите недостающий код

string ReadLine()
{
	string s;
	getline(cin, s);
	return s;
}

int ReadLineWithNumber()
{
	int result;
	cin >> result;
	ReadLine();
	return result;
}

struct Document
{
	int id;
	double relevance;
	int rating;
};

enum class DocumentStatus
{
	ACTUAL,
	IRRELEVANT,
	BANNED,
	REMOVED,
};

class SearchServer
{
public:
	void SetStopWords(const string& text)
	{
		for (const string& word : SplitIntoWords(text))
		{
			stop_words_.insert(word);
		}
	}

	void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings)
	{
		const vector<string> words = SplitIntoWordsNoStop(document);
		const double inv_word_count = 1.0 / words.size();
		for (const string& word : words)
		{
			word_to_document_freqs_[word][document_id] += inv_word_count;
		}
		documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
	}

	template<typename predicate>
	vector<Document> FindTopDocuments(const string& raw_query, predicate pred) const
	{
		const Query query = ParseQuery(raw_query);
		auto matched_documents = FindAllDocuments(query, pred);

		if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
		{
			matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
		}
		return matched_documents;
	}

	vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus doc_status) const
	{
		return FindTopDocuments(raw_query, [doc_status](int document_id, DocumentStatus status, int rating) { return status == doc_status; });
	}

	vector<Document> FindTopDocuments(const string& raw_query) const
	{
		return FindTopDocuments(raw_query, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::ACTUAL; });
	}

	int GetDocumentCount() const
	{
		return documents_.size();
	}

	tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const
	{
		const Query query = ParseQuery(raw_query);
		vector<string> matched_words;
		for (const string& word : query.plus_words)
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
		for (const string& word : query.minus_words)
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

private:
	struct DocumentData
	{
		int rating;
		DocumentStatus status;
	};

	struct QueryWord
	{
		string data;
		bool is_minus;
		bool is_stop;
	};

	struct Query
	{
		set<string> plus_words;
		set<string> minus_words;
	};

	set<string> stop_words_;
	map<string, map<int, double>> word_to_document_freqs_;
	map<int, DocumentData> documents_;

	static vector<string> SplitIntoWords(const string& text)
	{
		vector<string> words;
		string word;
		for (const char c : text)
		{
			if (c == ' ')
			{
				if (!word.empty())
				{
					words.push_back(word);
					word.clear();
				}
			}
			else
			{
				word += c;
			}
		}
		if (!word.empty())
		{
			words.push_back(word);
		}

		return words;
	}

	bool IsStopWord(const string& word) const
	{
		return stop_words_.count(word) > 0;
	}

	vector<string> SplitIntoWordsNoStop(const string& text) const
	{
		vector<string> words;
		for (const string& word : SplitIntoWords(text))
		{
			if (!IsStopWord(word))
			{
				words.push_back(word);
			}
		}
		return words;
	}

	static int ComputeAverageRating(const vector<int>& ratings)
	{
		if (ratings.empty())
		{
			return 0;
		}

		int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
		return rating_sum / static_cast<int>(ratings.size());
	}

	QueryWord ParseQueryWord(string text) const
	{
		bool is_minus = false;

		if (text[0] == '-')
		{
			is_minus = true;
			text = text.substr(1);
		}
		return {text, is_minus, IsStopWord(text)};
	}

	Query ParseQuery(const string& text) const
	{
		Query query;
		for (const string& word : SplitIntoWords(text))
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

	double ComputeWordInverseDocumentFreq(const string& word) const
	{
		return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
	}

	template<typename predicate>
	vector<Document> FindAllDocuments(const Query& query, predicate pred) const
	{
		map<int, double> document_to_relevance;
		for (const string& word : query.plus_words)
		{
			if (word_to_document_freqs_.count(word) == 0)
			{
				continue;
			}

			const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);

			for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word))
			{
				if (pred(document_id, documents_.at(document_id).status, documents_.at(document_id).rating))
				{
					document_to_relevance[document_id] += term_freq * inverse_document_freq;
				}
			}
		}

		for (const string& word : query.minus_words)
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

		vector<Document> matched_documents;
		for (const auto& [document_id, relevance] : document_to_relevance)
		{
			matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
		}

		sort(matched_documents.begin(), matched_documents.end(), [this, pred](const Document& lhs, const Document& rhs)
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

	vector<Document> FindAllDocuments(const Query& query, DocumentStatus document_status) const
	{
		return FindAllDocuments(query, [document_status](int document_id, DocumentStatus status, int rating) { return status == document_status; });
	}

};

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
		ASSERT(abs(found_docs[0].relevance - 0.12602676010180824) < EPSILON);
	}

	{
		const auto found_docs = server.FindTopDocuments("wide"s);
		ASSERT_EQUAL(found_docs.size(), 1);
		ASSERT(abs(found_docs[0].relevance - 0.13862943611198905) < EPSILON);
	}

	{
		const auto found_docs = server.FindTopDocuments("example"s);
		ASSERT_EQUAL(found_docs.size(), 1);
		ASSERT(abs(found_docs[0].relevance - 0.10663802777845313) < EPSILON);
	}

	{
		const auto found_docs = server.FindTopDocuments("apparently"s);
		ASSERT_EQUAL(found_docs.size(), 1);
		ASSERT(abs(found_docs[0].relevance - 0.2520535202036165) < EPSILON);
	}
}

void TestSearchServer() {
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

int main() {
	TestSearchServer();
	cout << "Search server testing finished"s << endl;
}
