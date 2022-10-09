#include "search_server.h"
#include <cmath>
#include <execution>
 

SearchServer:: SearchServer(const string& stop_words_text): SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor
{
}

SearchServer:: SearchServer(string_view stop_words_text): SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor
{
}

void SearchServer::AddDocument(int document_id, string_view document, DocumentStatus status, const vector<int>& ratings) { // S8 9.3 
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw invalid_argument("document contains wrong id"s);
    }
    const auto [it, inserted] = documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status, string(document)}); 
    const auto words = SplitIntoWordsNoStop(it->second.data);
    const double inv_word_count = 1.0 / words.size();
    map<string_view, double> word_to_freqs;
    for (const string_view word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        word_to_freqs[word] += inv_word_count;
    }
    document_ids_.insert(document_id);
}

void SearchServer::RemoveDocument(int document_id) { //les12
    if (document_ids_.count(document_id) == 1) {
        
        for (auto [word, freq] : GetWordFrequencies(document_id)) {
            word_to_document_freqs_[word].erase(document_id);
            }
        
        document_ids_.erase(document_id);
        documents_.erase(document_id);
        document_words_freqs_.erase(document_id);
    }
    return;
}
 
void SearchServer::RemoveDocument(const std::execution::sequenced_policy&, int document_id) {
     RemoveDocument(document_id);
 }
 
void SearchServer::RemoveDocument(const std::execution::parallel_policy&, int document_id) {
    if (document_words_freqs_.count(document_id) == 0) {
        return;
    }

    document_ids_.erase(document_id);
    documents_.erase(document_id);
    document_words_freqs_.erase(document_id);

    const auto& word_freqs = document_words_freqs_.at(document_id);
    vector<string> words(word_freqs.size());
    transform(
        execution::par,
        word_freqs.begin(), word_freqs.end(),
        words.begin(),
        [](const auto& item) { return item.first; }
    );
    for_each(
        execution::par,
        words.begin(), words.end(),
        [this, document_id](string word) {
            word_to_document_freqs_.at(word).erase(document_id);
        });
}

vector<Document> SearchServer::FindTopDocuments(const string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(execution::seq, raw_query, status);
}

vector<Document> SearchServer::FindTopDocuments(const string_view raw_query) const {
    return FindTopDocuments(execution::seq, raw_query);
}


int SearchServer:: GetDocumentCount() const {
    return documents_.size();
}

const map<string_view, double>& SearchServer::GetWordFrequencies(int document_id) const { //les12
    if (document_ids_.count(document_id) == 1) {
        return document_words_freqs_.at(document_id);
    }
    static map<string_view, double> empty_map;
    return empty_map;
}

set<int>::const_iterator SearchServer::begin() const { // new
    return document_ids_.begin();
}

set<int>::const_iterator SearchServer::end() const { // new les 12
    return document_ids_.end();
}


 
tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(const execution::parallel_policy&, const string_view raw_query, int document_id) const {
    bool sorting = true;
    const Query query = ParseQuery(raw_query, sorting);
    const auto word_checker =
        [this, document_id](string_view word) {
            const auto it = word_to_document_freqs_.find(word);
            return it != word_to_document_freqs_.end()&& it->second.count(document_id);
        };

    if (any_of(execution::par,
        query.minus_words.begin(), query.minus_words.end(),
        word_checker)) {
        vector<string_view> empty;
        return { empty, documents_.at(document_id).status };
    }

    vector<string_view> matched_words(query.plus_words.size());
    auto words_end = copy_if(execution::par,
        query.plus_words.begin(), query.plus_words.end(),
        matched_words.begin(),
        word_checker
    );
    sort(matched_words.begin(), words_end);
    words_end = unique(matched_words.begin(), words_end);
    matched_words.erase(words_end, matched_words.end());

    return make_tuple(matched_words, documents_.at(document_id).status);
}
 
tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(const string_view raw_query, int document_id) const {
    return MatchDocument(execution::seq, raw_query, document_id);
}
 

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(const execution::sequenced_policy&, const string_view raw_query, int document_id) const {
    
    if (document_ids_.count(document_id) == 0) {
        return { {}, {} };
    }
    const Query query = ParseQuery(raw_query);

    const auto word_checker =
        [this, document_id](string_view word) {
            const auto it = word_to_document_freqs_.find(word);
            return it != word_to_document_freqs_.end() && it->second.count(document_id);
        };


    if (any_of(execution::seq,
        query.minus_words.begin(), query.minus_words.end(),
        word_checker)) {
        vector<string_view> empty;
        return { empty, documents_.at(document_id).status };
    }

    vector<string_view> matched_words(query.plus_words.size());
    auto words_end = copy_if(execution::seq,
        query.plus_words.begin(), query.plus_words.end(),
        matched_words.begin(),
        word_checker
    );
    sort(matched_words.begin(), words_end);
    words_end = unique(matched_words.begin(), words_end);
    matched_words.erase(words_end, matched_words.end());

    return make_tuple(matched_words, documents_.at(document_id).status);
}


bool SearchServer::IsStopWord(const string_view word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const string_view word) {
        // A valid word must not contain special characters
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

vector<string_view> SearchServer::SplitIntoWordsNoStop(const string_view text) const {
    vector<string_view> words;
    for (const string_view word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw invalid_argument("Word "s + string(word) + " is invalid"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);;
    
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(string_view word) const {
    if (word.empty()) {
        throw invalid_argument("Query word is empty"s);
    }
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word.remove_prefix(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw invalid_argument("Query word "s + string(word) + " is invalid");
    }
    return {word, is_minus, IsStopWord(word)};
}
 

SearchServer::Query SearchServer::ParseQuery(const string_view text, bool sorting) const {
    Query result;
    for (const string_view word : SplitIntoWords(text)) {
            const auto query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    result.minus_words.push_back(query_word.data);
                } else {
                    result.plus_words.push_back(query_word.data);
                }
            }
        }
    if (sorting) {
        sort(result.plus_words.begin(), result.plus_words.end());
        auto unique_p = unique(result.plus_words.begin(), result.plus_words.end());
        result.plus_words.erase(unique_p, result.plus_words.end());
        sort(result.minus_words.begin(), result.minus_words.end());
        auto unique_m = unique(result.minus_words.begin(), result.minus_words.end());
        result.minus_words.erase(unique_m, result.minus_words.end());
    } 
    return result;
}

// Existence required
double SearchServer::ComputeWordInverseDocumentFreq(const string_view word) const {
    return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}
