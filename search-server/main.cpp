#include "search_server.h"

#include <execution>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "log_duration.h"

using namespace std;

int main() {
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
    /*
    "pet  rat  rat  rat"s,
        "nasty rat  curly hair"s,
        */
    const string query = "curly and funny waag op gagag  -not"s;

    {
        const auto [words, status] = search_server.MatchDocument(execution::par, query, 1);
        cout << words.size() << " words for document 1"s << endl;
        // 1 words for document 1
    }

    {
        const auto [words, status] = search_server.MatchDocument(execution::par, query, 2);
        cout << words.size() << " words for document 2"s << endl;
        // 2 words for document 2
    }

    {
        const auto [words, status] = search_server.MatchDocument(execution::par, query, 3);
        cout << words.size() << " words for document 3"s << endl;
        // 0 words for document 3
    }
    {
        const auto [words, status] = search_server.MatchDocument(execution::par, query, 4);
        cout << words.size() << " words for document 4"s << endl;
        // 0 words for document 3
    }
    {
        const auto [words, status] = search_server.MatchDocument(execution::par, query, 5);
        cout << words.size() << " words for document 5"s << endl;
        // 0 words for document 3
    }

    return 0;
}