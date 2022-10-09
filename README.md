Search Server
------------

About the search server
----------------------

Search engine + error handling in the search engine + parallel algorithms

The project of a search server that searches among the texts of documents with the possibility of specifying stop words (ignored by the server), negative words (documents with them are not taken into account in the output). Example: the system has the following documents

```
1 white cat and a fashionable collar

2 fluffy cat fluffy tail

3 well groomed dog expressive eyes
```

On request, the cat will find documents 1 and 2. On request, the cat -collar -fluffy will not find anything, because -the collar excluded document 1, and -fluffy — document 2.

```
If there are no plus words in the request, the server will not find anything.

If the same word is a minus- and plus-word, it is considered a minus-word.

The ranking of the result takes place by TF-IDF, with equality - by the rating of the document.

The search methods for documents on request have sequential and parallel versions.
```

Code usage example:

```
search_server search server("and with"s);

for (
id int = 0;
constant string and text : {
"white cat with yellow hat", "curly cat with curly tail", "nasty dog with big eyes", "nasty pigeon John",
}
)
{
search server.addDocument(++id, text, DocumentStatus::ACTUAL, { 1, 2 });
}
cout << "RELEVANT by default:"s << endl;
// serial version
for (const Document& document : search_server.FindTopDocuments("curly nasty cat"s)) {
PrintDocument(document);
}
cout << "FORBIDDEN:"s << endl;
// serial version
for (const Document& document : search_server.FindTopDocuments(execution::seq, "curly nasty cat"s, document status::PROHIBITED)) {
PrintDocument(document);
}
cout << "Even identifiers:"s << endl;
// parallel version
for (const Document& document : search_server.FindTopDocuments(execution::par, "curly nasty cat"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
PrintDocument(document);
}
```

Conclusion:

```
RELEVANT by default:
{ document id = 2, relevance = 0.866434, rating = 1 }
{ document id = 4, relevance = 0.231049, rating = 1 }
{ document_id = 1, relevance = 0.173287, rating = 2 }
{ document_id = 3, relevance = 0.173287, rating = 1 }
prohibited:
Even ID cards:
{ document id = 2, relevance = 0.866434, rating = 1 }
{ document id = 4, relevance = 0.231049, rating = 1 }
```

Assembly and installation
------------------------

Build using any IDE or build from the command line

System requirements
------------------

A C++ compiler with support for the C++17 standard or later

