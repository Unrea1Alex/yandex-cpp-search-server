#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

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

vector<string> SplitIntoWords(const string& text) {
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

struct Document {
    int id;
    int relevance;
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

    void AddDocument(int document_id, const string& document) 
    {
        const vector<string> words = SplitIntoWordsNoStop(document);

        for (const auto& word : words)
        {
            indexes_[word].insert(document_id);
        }

        ++document_count;
    }

    vector<Document> FindTopDocuments(const string& raw_query) const 
    {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                return lhs.relevance > rhs.relevance;
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

private:

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    map<string, set<int>> indexes_;

    set<string> stop_words_;

    int document_count{ 0 };

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return { text, is_minus, IsStopWord(text) };
    }

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                }
                else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    map<string, double> GetQueryIDF(const Query& query) const
    {
        map<string, double> idf;

        map<string, int> idf_count;

        for (const auto& index : indexes_)
        {
            if (query.plus_words.count(index.first))
            {
                for (auto i : index.second)
                {
                    ++idf_count[index.first];
                }
            }
        }

        for (const auto& [word, count] : idf_count)
        {
            idf[word] = log(document_count / count);
        }

        return idf;
    }

    vector<Document> FindAllDocuments(const Query& query) const 
    {
        vector<Document> matched_documents;

        map<string, double> idf = GetQueryIDF(query);

        map<int, int> document_total_words_count;

        for (int i = 0; i < document_count; i++)
        {
            document_total_words_count[i] = count_if(indexes_.begin(), indexes_.end(), [i](const pair<string, set<int>>& index)
                {
                    return index.second.count(i) > 0;
                });
        }

        map<int, map<string, double>> document_tf;

        for (auto [id, words_count] : document_total_words_count)
        {
            int doc_id = id;

            map<string, double> word_tf;

            for (const auto& word : query.plus_words)
            {
                int count = count_if(indexes_.begin(), indexes_.end(), [doc_id, word](const pair<string, set<int>>& index)
                    {
                        return index.second.count(doc_id) > 0 && index.first == word;
                    });

                word_tf[word] = count * 1.0 / words_count * 1.0;

            }

            document_tf[id] = word_tf;
        }


        
        
        for (auto t : document_tf)
        {
           // matched_documents.push_back({ t.first, t.second *  });
        }

        
        map<int, int> relevance;






        for (const auto& index : indexes_)
        {
            const set<int> docsument_ids = MatchDocument(index, query, document_count);
            
            if (docsument_ids.empty())
            {
                continue;
            }

            for (auto document_id : docsument_ids)
            {
                ++relevance[document_id];
            }

        }

        for (auto [id, rel] : relevance)
        {
            matched_documents.push_back({ id, rel });
        }

        return matched_documents;
    }

    static set<int> MatchDocument(const pair<string, set<int>> content, const Query& query, int documents_count) 
    {
        if (query.plus_words.empty()) 
        {
            return set<int>{};
        }

        for (int i = 0; i < documents_count - 1; i++)
        {

        }

        
        if (query.plus_words.count(content.first) && !query.minus_words.count(content.first))
        {
            return content.second;
        }

        return set<int>{};
    }
};

SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }

    return search_server;
}

int main() {
    const SearchServer search_server = CreateSearchServer();

    const string query = ReadLine();
    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", "
            << "relevance = "s << relevance << " }"s << endl;
    }
}