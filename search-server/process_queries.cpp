#include "process_queries.h"

#include <algorithm>
#include <execution>

using namespace std;

vector<vector<Document>> ProcessQueries(const SearchServer& search_server, const vector<string>& queries)
{
     //LOG_DURATION("ProcessQueries"s);
    vector<vector<Document>> result(queries.size());
    transform(execution::par, queries.begin(), queries.end(), result.begin(),
        [&search_server](const std::string& query) { 
            return search_server.FindTopDocuments(query); 
        });
    return result;
}

std::vector<Document> ProcessQueriesJoined(
    const SearchServer& search_server, const std::vector<std::string>& queries) {
    std::vector<Document> result;
    
    for (auto& documents : ProcessQueries(search_server, queries)) {
        for (auto& document : documents) {
            result.push_back(std::move(document));
        }
    }

    return result;
}
