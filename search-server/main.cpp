#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

struct Document {
    int id;
    int relevance;
};

bool HasDocumentGreaterRelevance(Document lhs, Document rhs)
{
    return rhs.relevance > lhs.relevance;
}

class SearchServer
{
private:
    struct DocumentContent {
        int id = 0;
        vector<string> words;
    };

    static vector<string> SplitIntoWords(const string& text) {
        vector<string> words;
        string word;
        for (const char c : text) {
            if (c == ' ') {
                if (!word.empty()) {
                    words.push_back(word);
                    word.clear();
                }
            }
            else {
                word += c;
            }
        }
        if (!word.empty()) {
            words.push_back(word);
        }

        return words;
    }

    static int MatchDocument(const DocumentContent& content, const set<string>& query_words) {
        if (query_words.empty()) {
            return 0;
        }
        set<string> matched_words;
        for (const string& word : content.words) {
            if (matched_words.count(word) != 0) {
                continue;
            }
            if (query_words.count(word) != 0) {
                matched_words.insert(word);
            }
        }
        return static_cast<int>(matched_words.size());
    }

    vector<string> SplitIntoWordsNoStop(const string& text)
    {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (stopWords.count(word) == 0) {
                words.push_back(word);
            }
        }
        return words;
    }

    vector<Document> FindAllDocuments(const set<string>& query_words) 
    {
        vector<Document> matched_documents;
        for (const auto& document : documents) {
            const int relevance = MatchDocument(document, query_words);
            if (relevance > 0) {
                matched_documents.push_back({ document.id, relevance });
            }
        }
        return matched_documents;
    }

    set<string> ParseQuery(const string& text) 
    {
        set<string> query_words;
        for (const string& word : SplitIntoWordsNoStop(text)) {
            query_words.insert(word);
        }
        return query_words;
    }

    vector<DocumentContent> documents;
    set<string> stopWords;

public:
    void AddDocument(int documentId, const string& document)
    {
        const vector<string> words = SplitIntoWordsNoStop(document);

        documents.push_back({ documentId, words });
    }

    void SetStopWords(const string& text)
    {
        for (const string& word : SplitIntoWords(text)) {
            stopWords.insert(word);
        }
    }

    vector<Document> FindTopDocuments(const string& raw_query) 
    {
        const set<string> query_words = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query_words);

        sort(matched_documents.begin(), matched_documents.end(), HasDocumentGreaterRelevance);
        reverse(matched_documents.begin(), matched_documents.end());

        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }

        return matched_documents;
    }
};

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}

SearchServer CreateSearchServer()
{
    SearchServer server;

    const string stop_words_joined = ReadLine();
    server.SetStopWords(stop_words_joined);

    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        server.AddDocument(document_id, ReadLine());
    }

    return server;
}

int main() 
{
    SearchServer server = CreateSearchServer();

    const string query = ReadLine();
    for (auto [document_id, relevance] : server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", relevance = "s << relevance << " }"s
            << endl;
    }
}