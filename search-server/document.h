#pragma once
#include <string>
#include <iostream>
#include <set>
using namespace std;

struct Document {
    Document() = default;

    Document(int id, double relevance, int rating);
   

    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};



ostream& operator<<(ostream& out, const Document& document);

    
enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};
