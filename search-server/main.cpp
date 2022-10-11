#include "search_server.h"
#include "process_queries.h"
#include <execution>
#include <iostream>
#include <random>
#include <string>
#include <vector>
#include "search_server.h" 

#include "paginator.h" 

#include "request_queue.h" 
#include "log_duration.h"

using namespace std;
void PrintMatchDocumentResult(int document_id, const std::vector<std::string>& words, DocumentStatus status) {

    std::cout << "{ "s

        << "document_id = "s << document_id << ", "s

        << "status = "s << static_cast<int>(status) << ", "s

        << "words ="s;

    for (const std::string& word : words) {

        std::cout << ' ' << word;

    }

    std::cout << "}"s << std::endl;

}



void AddDocument(SearchServer& search_server, int document_id, const std::string& document, DocumentStatus status,

    const std::vector<int>& ratings) {

    try {

        search_server.AddDocument(document_id, document, status, ratings);

    }

    catch (const std::invalid_argument& e) {

        std::cout << "Ошибка добавления документа "s << document_id << ": "s << e.what() << std::endl;

    }

}



void FindTopDocuments(const SearchServer& search_server, const std::string& raw_query) {

    std::cout << "Результаты поиска по запросу: "s << raw_query << std::endl;

    try {

        for (const Document& document : search_server.FindTopDocuments(raw_query)) {

            std::cout << document;

        }

    }

    catch (const std::invalid_argument& e) {

        std::cout << "Ошибка поиска: "s << e.what() << std::endl;

    }

}






int main() {

    setlocale(LC_ALL, "Russian");
    SearchServer search_server("and with"s);
    int id = 0;
    for (
        const string& text : {
            "funny pet and nasty rat"s,
            "funny pet with curly hair"s,
            "funny pet and not very nasty rat"s,
            "pet with rat and rat and rat"s,
            "nasty rat with curly hair"s,
        }
        ) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, { 1, 2 });
    }
    const vector<string> queries = {
        "nasty rat -not"s,
        "not very funny nasty pet"s,
        "curly hair"s
    };
    id = 0;
    for (const Document& document : ProcessQueriesJoined(search_server, queries)) {
        cout << "Document "s << document.id << " matched with relevance "s << document.relevance << endl;
    }
  
    //string stop_w = "   и      в                на             "s;
    //SearchServer search_server("   и      в                на             "s);
   // SearchServer search_server("и в на"s);
    /*string zapr1 = "пушистый кот пушистый хвост"s;
    string zapr2 = "пушистый пёс и модный ошейник"s;
    string zapr3 = "большой кот модный ошейник "s;
    string zapr4 = "большой пёс скворец евгений"s;
    string zapr5 = "большой пёс скворец василий"s;*/
    /*
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });

    search_server.AddDocument(2, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2, 3 });

    search_server.AddDocument(3, "большой кот модный ошейник "s, DocumentStatus::ACTUAL, { 1, 2, 8 });

    search_server.AddDocument(4, "большой пёс скворец евгений"s, DocumentStatus::ACTUAL, { 1, 3, 2 });

    search_server.AddDocument(5, "большой пёс скворец василий"s, DocumentStatus::ACTUAL, { 1, 1, 1 });



    const auto search_results = search_server.FindTopDocuments("пушистый пёс"s);

    int page_size = 2;

    const auto pages = Paginate(search_results, page_size);



    // Выводим найденные документы по страницам 

    for (auto page = pages.begin(); page != pages.end(); ++page) {

        std::cout << *page << std::endl;

        std::cout << "Разрыв страницы"s << std::endl;

    }


    /*
    {
        const auto [words, status] = search_server.MatchDocument( query, 1);
        cout << words.size() << " words for document 1"s << endl;
        // 1 words for document 1
    }

    {
        const auto [words, status] = search_server.MatchDocument( query, 2);
        cout << words.size() << " words for document 2"s << endl;
        // 2 words for document 2
    }

    {
        const auto [words, status] = search_server.MatchDocument( query, 3);
        cout << words.size() << " words for document 3"s << endl;
        // 0 words for document 3
    }
    /*
    {
        const auto [words, status] = search_server.MatchDocument(execution::par, query, 4);
        cout << words.size() << " words for document 4"s << endl;
        // 0 words for document 3
    }
    {
        const auto [words, status] = search_server.MatchDocument(execution::par, query, 5);
        cout << words.size() << " words for document 5"s << endl;
        // 0 words for document 3
    }*/
    
    return 0;
}