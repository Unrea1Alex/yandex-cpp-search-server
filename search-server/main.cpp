#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>
#include <numeric>

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
    double relevance;
    int rating;
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

    void AddDocument(int document_id, const string& document, const vector<int>& ratings) 
    {
        const vector<string> words = SplitIntoWordsNoStop(document);

        for (const auto& word : words)
        {
            indexes_[word].insert({ document_id, GetWordTF(words, word)});
        }

        documents_rating_.insert({document_id, ComputeAverageRating(ratings) });

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

    map<string, map<int, double>> indexes_;

    set<string> stop_words_;
    map<int, int> documents_rating_;

    int document_count{ 0 };

    int ComputeAverageRating(const vector<int>& ratings) const
    {
        if (ratings.empty())
        {
            return 0;
        }

        int sum = accumulate(ratings.begin(), ratings.end(), 0);

        return sum / ratings.size();
    }

    double GetWordTF(const vector<string>& words, const string& word) const
    {
        int words_count = count_if(words.begin(), words.end(), [&word](const string& w)
            {
                return w == word;
            });
            
        return words_count * 1.0 / words.size() * 1.0;
    }

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

    QueryWord ParseQueryWord(string text) const 
    {
        bool is_minus = false;

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

        for (const auto& word : query.plus_words)
        {
            if (indexes_.count(word))
            {
                idf[word] = log(document_count * 1.0 / indexes_.at(word).size() * 1.0);
            }
        }

        return idf;
    }

    vector<Document> FindAllDocuments(const Query& query) const 
    {
        vector<Document> matched_documents;

        for(int i = 0; i < document_count; i++)
        {
            double relevance = MatchDocument(i, query);

            if (relevance > 0)
            {
                matched_documents.push_back({ i, relevance, documents_rating_.at(i)});
            }
        }

        return matched_documents;
    }

    double MatchDocument(int document_id, const Query& query) const
    {
        if (query.plus_words.empty()) 
        {
            return 0;
        }

        if (count_if(indexes_.begin(), indexes_.end(), [document_id, &query](const pair<string, map<int, double>>& index)
            {
                return index.second.count(document_id) > 0 && query.minus_words.count(index.first);
            }))
        {
            return 0;
        }

        map<string, double> idf = GetQueryIDF(query);

        double relevance = accumulate(indexes_.begin(), indexes_.end(), 0.0, [&idf, document_id](double d, const pair<string, map<int, double>> index)
            {
                if (idf.count(index.first) && index.second.count(document_id))
                {
                    return d + index.second.at(document_id) * idf[index.first];
                }
                return d;
            });

        return relevance;
    }
};

SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) 
    {
        string document = ReadLine();

        int rating_count{ 0 };
        cin >> rating_count;

        vector<int> ratings;

        for (int i = 0; i < rating_count; i++)
        {
            int r;
            cin >> r;
            ratings.push_back(r);
        }

        search_server.AddDocument(document_id, document, ratings);
    }

    return search_server;
}

int main() {
    const SearchServer search_server = CreateSearchServer();

    const string query = ReadLine();
    for (const auto& [document_id, relevance, rating] : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", "
            << "relevance = "s << relevance << " }"s << endl;
    }
}