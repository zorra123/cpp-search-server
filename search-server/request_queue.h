#pragma once
#include "search_server.h"
#include <stack>




class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server) :search_server_t(search_server) {
        // напишите реализацию
    }

    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string& raw_query);

    int GetNoResultRequests() const;
private:
    struct QueryResult {
        std::vector<Document> resul;
        // определите, что должно быть в структуре
    };
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer& search_server_t;
    // возможно, здесь вам понадобится что-то ещё
};


template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    // напишите реализацию
    if (requests_.size() == min_in_day_) {
        requests_.pop_front();
    }
    auto res = search_server_t.FindTopDocuments(raw_query, document_predicate);
    requests_.push_back({ res });
    return res;
}