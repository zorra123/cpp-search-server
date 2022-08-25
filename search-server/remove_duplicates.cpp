#include "remove_duplicates.h"

using namespace std::literals::string_literals;

void RemoveDuplicates(SearchServer& search_server) {
    std::set<std::set<std::string>> tmp;
    std::vector<int> id_to_remove;
    for (const int id : search_server) {
        const auto& str = search_server.GetWordFrequencies(id);
        std::set<std::string> tmp2;
        for (const auto& [key, value] : str) {
            tmp2.insert(key);
        }
        if (tmp.empty() || tmp.find(tmp2) == tmp.end())
            tmp.insert(tmp2);
        else id_to_remove.push_back(id);
    }
    for (const auto& id : id_to_remove) {
        std::cout << "Found duplicate document id "s << id << std::endl;
        search_server.RemoveDocument(id);
    }
}