#pragma once
#include "search_server.h"
#include <deque>



class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server) :search_server_t(search_server),
                                                              num_zero_requests(0),
                                                              time(0) {}

    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string& raw_query);

    int GetNoResultRequests() const;
private:
    struct QueryResult {
        int num_results;
        uint64_t requests_time;
    };
    std::deque<QueryResult> requests_;
    const static uint64_t min_in_day_ = 1440;
    const SearchServer& search_server_t;
    int num_zero_requests;
    uint64_t time;
    void AddRequests(int this_num_results);
};

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    auto res = search_server_t.FindTopDocuments(raw_query, document_predicate);
    AddRequests(res.size());
    return res;
}