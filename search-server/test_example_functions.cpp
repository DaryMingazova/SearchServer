#include "test_example_functions.h"
#include "log_duration.h"

#include <iostream>

using namespace std;

void PrintDocument(const Document& document) {
    cout << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s << endl;
}

void PrintMatchDocumentResult(int document_id, const vector<string_view>& words, DocumentStatus status) {
    cout << "{ "s
        << "document_id = "s << document_id << ", "s
        << "status = "s << static_cast<int>(status) << ", "s
        << "words ="s;
    for (const string_view word : words) {
        cout << ' ' << word;
    }
    cout << "}"s << endl;
}

void AddDocument(SearchServer& search_server, int document_id, string document, DocumentStatus status,
    const vector<int>& ratings) {
    LOG_DURATION("AddDocument"s);
    try {
        search_server.AddDocument(document_id, document, status, ratings);
    }
    catch (const invalid_argument& e) {
        cout << "Ошибка добавления документа: "s << document_id << ": "s << e.what() << endl;
    }
}

void FindTopDocuments(const SearchServer& search_server, string raw_query) {
    LOG_DURATION("FindTopDocuments"s);
    cout << "Результаты поиска по запросу: "s << raw_query << endl;
    try {
        for (const Document& document : search_server.FindTopDocuments(raw_query)) {
            PrintDocument(document);
        }
    }
    catch (const invalid_argument& e) {
        cout << "Ошибка поиска: "s << e.what() << endl;
    }
}

void MatchDocuments(const SearchServer& search_server, string query) {
    LOG_DURATION ("MatchDocuments"s);
    try {
        cout << "Матчинг документов по запросу: "s << query << endl;

        for (int document_id : search_server) {
            const auto [words, status] = search_server.MatchDocument(query, document_id);
            PrintMatchDocumentResult(document_id, words, status);
        }
    }
    catch (const invalid_argument& e) {
        cout << "Ошибка матчинга документов на запрос "s << query << ": "s << e.what() << endl;
    }
}

void RemoveDuplicates(SearchServer& search_server)
{ 
    //нужно найти набор слов для каждого документа
    // Затем сравнить set и map и удалить по найденному document_id
    map<set<string_view>, int> word_to_document_freqs;   
    set<int> documents_to_delete;

    for (auto document_id : search_server) {
        set<string_view> words;
        
        for (auto& [document, freqs] : search_server.GetWordFrequencies(document_id)) {
            words.insert(document);
            
        }
        if (word_to_document_freqs.find(words) == word_to_document_freqs.end()) {
            word_to_document_freqs[words] = document_id;
        }
        else if (document_id > word_to_document_freqs[words])
        {
            //cout<<document_id <<endl;
            //cout<<word_to_document_freqs[words]<<endl;
            word_to_document_freqs[words] = document_id;
            documents_to_delete.insert(document_id);
       }   
    }
    for (const int& document : documents_to_delete) {
        cout << "Found duplicate document id " << document << endl;
        search_server.RemoveDocument(document);
    }
}