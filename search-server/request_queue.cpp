//Вставьте сюда своё решение из урока «‎Очередь запросов».‎
#include "request_queue.h"


std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    // напишите реализацию
    if (requests_.size() == min_in_day_) {
        requests_.pop_front();
    }
    auto res = search_server_t.FindTopDocuments(raw_query, status);
    requests_.push_back({ res });
    return res;
}
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    // напишите реализацию
    if (requests_.size() == min_in_day_) {
        requests_.pop_front();
    }
    auto res = search_server_t.FindTopDocuments(raw_query);
    requests_.push_back({ res });
    return res;

}

int RequestQueue::GetNoResultRequests() const {
    // напишите реализацию
    auto num = std::count_if(requests_.begin(), requests_.end(), [](const auto& el) {
        return el.resul.empty();
        });
    return num;
}
