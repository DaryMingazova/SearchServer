#include "remove_duplicates.h"

#include <iostream>


void RemoveDuplicates(SearchServer& search_server)
{ 
    //нужно найти набор слов для каждого документа
    // Затем сравнить set и map и удалить по найденному document_id
    map<set<string>, int> word_to_document_freqs;   
    set<int> documents_to_delete;

    for (auto document_id : search_server) {
        set<string> words;
        
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