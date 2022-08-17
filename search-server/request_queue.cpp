#include "request_queue.h"


std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    return AddFindRequest(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
        });
}
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    return AddFindRequest(raw_query, DocumentStatus::ACTUAL);
}

int RequestQueue::GetNoResultRequests() const {
    return num_zero_requests;
}

void RequestQueue::AddRequests(int this_num_results) {
    ++time;
    requests_.push_back({ this_num_results,time });
    if (this_num_results == 0) {
        ++num_zero_requests;
    }
    while (requests_.size() > min_in_day_) {
        if (requests_.front().num_results == 0) {
            --num_zero_requests;
        }
        requests_.pop_front();
    }
}