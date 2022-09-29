#include "process_queries.h"
#include <execution>

std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {
    std::vector<std::vector<Document>> res(queries.size());
    //res.resize(queries.size());
    std::transform(
        std::execution::par,
        queries.begin(),
        queries.end(),
        res.begin(),
        [&search_server](const std::string& query) {
            return search_server.FindTopDocuments(query);
        });
    return res;
}

std::list<Document> ProcessQueriesJoined(const SearchServer& search_server, const std::vector<std::string>& queries)
{
    std::list<Document> res;
    for (auto& el : ProcessQueries(search_server, queries)) {//ProcessQueries возвращает временный объект, копирования не происходит
        for (auto& doc : el) {
            res.push_front(std::move(doc));//происходит перемещение элемента временного объекта, копирования нет
        }
    }
    res.reverse();
    return res;
}
