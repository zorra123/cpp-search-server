#include "remove_duplicates.h"

using namespace std::literals::string_literals;

void RemoveDuplicates(SearchServer& search_server) {
    std::set<std::set<std::string>> tmp_words_in_docs;
    std::vector<int> id_to_remove;
    for (const int id : search_server) {
        const auto& container_doc = search_server.GetWordFrequencies(id); 
        std::set<std::string> tmp_words_in_doc;
        for (const auto& [word, value] : container_doc) {
            tmp_words_in_doc.insert(word);
        }
        if (tmp_words_in_docs.empty() || tmp_words_in_docs.find(tmp_words_in_doc) == tmp_words_in_docs.end())
            tmp_words_in_docs.insert(tmp_words_in_doc);
        else id_to_remove.push_back(id);
    }
    for (const auto& id : id_to_remove) {
        std::cout << "Found duplicate document id "s << id << std::endl;
        search_server.RemoveDocument(id);
    }
}