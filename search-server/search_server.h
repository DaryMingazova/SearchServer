#pragma once


#include <string>
#include <map>
#include <execution>
#include <vector>
#include <algorithm> 
#include <numeric>
#include <set>
#include <string>
#include <functional>

#include "document.h"
#include "string_processing.h"
#include "concurrent_map.h"
const int kMaxDocumentCount = 5;
const double kEps = 1e-6;

class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);
    // Invoke delegating constructor
    explicit SearchServer(const std::string& stop_words_text);
    explicit SearchServer(std::string_view stop_words_text);
                 
    
    void AddDocument(int document_id, std::string_view document, DocumentStatus status, const vector<int>& ratings); 
   
    void RemoveDocument(int document_id);
    void RemoveDocument(const std::execution::sequenced_policy&, int document_id);
    void RemoveDocument(const std::execution::parallel_policy&, int document_id);
  
     template <typename DocumentPredicate>
     std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentPredicate document_predicate) const; 
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const; 
    std::vector<Document> FindTopDocuments(const std::string_view raw_query) const; 
    
    template <typename DocumentPredicate, typename Policy>
    std::vector<Document> FindTopDocuments(Policy& policy, const std::string_view raw_query, DocumentPredicate document_predicate) const; 

    template <typename Policy>
    std::vector<Document> FindTopDocuments(Policy& policy, const std::string_view raw_query, DocumentStatus status) const; 
    template <typename Policy>
    std::vector<Document> FindTopDocuments(Policy& policy, const std::string_view raw_query) const; 
          
    int GetDocumentCount() const; 
 
    const map<string_view, double>& GetWordFrequencies(int document_id) const; // new
    set<int>::const_iterator begin() const;//new lesson 12
    set<int>::const_iterator end() const;//new
    
    tuple<vector<string_view>, DocumentStatus> MatchDocument(const string_view raw_query, int document_id) const;
    tuple<vector<string_view>, DocumentStatus> MatchDocument(const execution::parallel_policy&, const string_view raw_query, int document_id) const;
    tuple<vector<string_view>, DocumentStatus> MatchDocument(const execution::sequenced_policy&, const string_view raw_query, int document_id) const ;
    
private:
    
    struct DocumentData {
        int rating;
        DocumentStatus status;
        string data;//s8
     //   map<std::string_view, double> word_to_freqs;//s8
    };
    
    const set<string, std::less<>> stop_words_;
    map<string_view, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;
    set<int> document_ids_;
    map<int, map<string_view, double>> document_words_freqs_;//new 
  
    bool IsStopWord(const string_view word) const;
    
    static bool IsValidWord(const string_view word);

    vector<string_view> SplitIntoWordsNoStop(const string_view text) const;

    static int ComputeAverageRating(const vector<int>& ratings); 

    struct QueryWord {
        string_view data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(const string_view text) const; 

    struct Query {
        vector<string_view> plus_words;
        vector<string_view> minus_words;
    };
     
    Query ParseQuery(const string_view text, bool sorting = false) const; 
   
    // Existence required
    double ComputeWordInverseDocumentFreq(const string_view word) const;

    template <typename DocumentPredicate, typename Policy>
    std::vector<Document>  FindAllDocuments(Policy& policy, const Query& query, DocumentPredicate document_predicate) const; 
    
    };
    

template <typename StringContainer>
SearchServer:: SearchServer(const StringContainer& stop_words): stop_words_(MakeUniqueNonEmptyStrings(stop_words)) // Extract non-empty stop words
    {
    if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
        throw invalid_argument("Some of stop words are invalid"s);
    }
}


template <typename DocumentPredicate>
vector<Document>SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentPredicate document_predicate) const {
    return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
}

template <typename DocumentPredicate, typename Policy>
vector<Document> SearchServer::FindTopDocuments(Policy& policy, const std::string_view raw_query, DocumentPredicate document_predicate) const {
    const auto query = ParseQuery(raw_query);
    auto matched_documents = FindAllDocuments(policy, query, document_predicate);
    std::sort(policy,
        matched_documents.begin(), matched_documents.end(),
        [](const Document& lhs, const Document& rhs) {
        return (std::abs(lhs.relevance - rhs.relevance) < kEps && lhs.rating > rhs.rating)
            || (lhs.relevance > rhs.relevance);
    });
    if (matched_documents.size() > kMaxDocumentCount) {
        matched_documents.resize(kMaxDocumentCount);
    }
    return matched_documents;
}

template <typename Policy>
vector<Document> SearchServer::FindTopDocuments(Policy& policy, const std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(policy, raw_query, [status](int, DocumentStatus document_status, int) {
        return document_status == status;
    });
}

template <typename Policy>
vector<Document> SearchServer::FindTopDocuments(Policy& policy, const std::string_view raw_query) const {
    return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}
      
template <typename DocumentPredicate, typename Policy>
std::vector<Document> SearchServer::FindAllDocuments(Policy& policy, const SearchServer::Query& query, DocumentPredicate document_predicate) const {
    ConcurrentMap<int, double> document_to_relevance(50);
    std::for_each(policy,
        query.plus_words.begin(), query.plus_words.end(),
        [this, &document_to_relevance, &document_predicate, &policy] (const std::string_view word) {
            if (word_to_document_freqs_.count(word) > 0) {
                const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
                std::for_each(policy,
                    word_to_document_freqs_.at(word).begin(), word_to_document_freqs_.at(word).end(),
                    [this, &document_to_relevance, &document_predicate, &inverse_document_freq] (const auto& pair) {
                        const auto& document_data = documents_.at(pair.first);
                        if (document_predicate(pair.first, document_data.status, document_data.rating)) {
                            document_to_relevance[pair.first].ref_to_value += pair.second * inverse_document_freq;
                        }
                });
            }
        });
    std::for_each(policy,
        query.minus_words.begin(), query.minus_words.end(),
        [this, &document_to_relevance, &policy] (const std::string_view word) {
            if (word_to_document_freqs_.count(word) > 0) {
                std::for_each(policy,
                    word_to_document_freqs_.at(word).begin(), word_to_document_freqs_.at(word).end(),
                    [&document_to_relevance] (const auto& pair) {
                        document_to_relevance.Erase(pair.first);
                });
            }
        });
     
    std::map<int, double> doc_to_relevance = document_to_relevance.BuildOrdinaryMap();
    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : doc_to_relevance) { 
        matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating}); 
    } 
    return matched_documents;
}