#pragma once
#include "log_duration.h"
#include "document.h"
#include "string_processing.h"
#include "concurrent_map.h"
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <iostream>
#include <execution>
#include <string_view>
#include <list>
#include <execution>
#include <type_traits>
#include <atomic>
#include <mutex>
#include <future>
#include <functional>

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double CALCULATING_ERROR = 1e-6;
const int THREAD_COUNT = 6;
const int NUM_SUB_CONTAINERS = 6000;

class SearchServer {
public: 
    using MatchWords = std::tuple<std::vector<std::string_view>, DocumentStatus>;

    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);  // Extract non-empty stop words
    explicit SearchServer(const std::string& stop_words_text)  // Invoke delegating constructor from string container 
        : SearchServer(static_cast<std::string_view>(stop_words_text)) {};

    explicit SearchServer(std::string_view stop_words)
        :SearchServer(SplitIntoWordsView(stop_words)) {};
    void AddDocument(int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentPredicate document_predicate) const;

    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const;
    std::vector<Document> FindTopDocuments(const std::string_view raw_query) const;

    template <typename DocumentPredicate, typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy& policy, const std::string_view raw_query, DocumentPredicate document_predicate) const;

    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy& policy, const std::string_view raw_query, DocumentStatus status) const;

    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy& policy, const std::string_view raw_query) const;

    int GetDocumentCount() const;

    MatchWords MatchDocument(std::execution::parallel_policy par, const std::string_view raw_query, int document_id) const;

    MatchWords MatchDocument(std::execution::sequenced_policy seq, const std::string_view raw_query, int document_id) const {
        return MatchDocument(raw_query, document_id);
    }

    MatchWords MatchDocument(const std::string_view raw_query, int document_id) const;

    const std::set<int>::iterator begin()const;
    const std::set<int>::iterator end()const;
    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;
    void RemoveDocument(int document_id) {
        RemoveDocument(std::execution::seq, document_id);
    }
    void RemoveDocument(std::execution::sequenced_policy seq, int document_id);
    void RemoveDocument(std::execution::parallel_policy par, int document_id);



private:
    std::list<std::string> docs_str;
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };
    struct VecQuery {
        VecQuery() = default;
        VecQuery(int num_minus, int num_plus) :minus_words(num_minus), plus_words(num_plus) {};
        std::vector<std::string_view> minus_words;
        std::vector<std::string_view> plus_words;
    };

    const std::set<std::string, std::less<>> stop_words_;

    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::map<int, std::map<std::string_view, double>> word_to_document_freqs_new;


    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;


    bool IsStopWord(const std::string_view word) const;

    static bool IsValidWord(const std::string_view word); // A valid word must not contain special characters
    std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    QueryWord ParseQueryWord(const std::string_view text) const;
    VecQuery ParseQueryVec(const std::string_view text, bool need_sort = 0) const;

    // Existence required
    double ComputeWordInverseDocumentFreq(const std::string_view word) const;

    template<typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(std::execution::parallel_policy, const VecQuery& query, DocumentPredicate document_predicate) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const VecQuery& query, DocumentPredicate document_predicate) const;
    void SortQuery(VecQuery& query) const;

    template <typename ForwardRange, typename Function>
    void ForEach(const std::execution::parallel_policy&, ForwardRange& range, Function function) const;
};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words))  // Extract non-empty stop words
{
    using namespace std::literals::string_literals;
    if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
        throw std::invalid_argument("Some of stop words are invalid"s);
    }
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentPredicate document_predicate) const {
    const auto query = ParseQueryVec(raw_query,1);

    auto matched_documents = FindAllDocuments(query, document_predicate);

    std::sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
        if (std::abs(lhs.relevance - rhs.relevance) < CALCULATING_ERROR) {
            return lhs.rating > rhs.rating;
        }
        else {
            return lhs.relevance > rhs.relevance;
        }
        });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;
}

template<typename DocumentPredicate, typename ExecutionPolicy>
inline std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy& policy, const std::string_view raw_query, DocumentPredicate document_predicate) const
{
    if constexpr (std::is_same_v<std::decay_t<ExecutionPolicy>, std::execution::sequenced_policy>) {
        return FindTopDocuments(raw_query, document_predicate);
    }
    else {
        const auto query = ParseQueryVec(raw_query,1);
        auto matched_documents = FindAllDocuments(policy, query, document_predicate);

        std::sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
            return std::abs(lhs.relevance - rhs.relevance) < CALCULATING_ERROR ?
                lhs.rating > rhs.rating:
            lhs.relevance > rhs.relevance;
            });


        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }
}

template<typename ExecutionPolicy>
inline std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy& policy, const std::string_view raw_query, DocumentStatus status) const
{
    return FindTopDocuments(policy, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
                return document_status == status;
            });
}

template<typename ExecutionPolicy>
inline std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy& policy, const std::string_view raw_query) const
{
    return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}


template<typename DocumentPredicate>
inline std::vector<Document> SearchServer::FindAllDocuments(std::execution::parallel_policy, const VecQuery& query,
    DocumentPredicate document_predicate) const {
    ConcurrentMap<int, double> cm(NUM_SUB_CONTAINERS);

    std::for_each(std::execution::par, query.plus_words.begin(), query.plus_words.end(), [this, &cm, document_predicate](std::string_view word)
        {
            if (word_to_document_freqs_.count(word) == 0)
            {
                return;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word))
            {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating))
                {
                    cm[document_id].ref_to_value += term_freq * inverse_document_freq;
                }
            }
        });
    std::for_each(std::execution::par, query.minus_words.begin(), query.minus_words.begin(), [this, &cm](std::string_view word)
        {
            if (word_to_document_freqs_.count(word) == 0)
            {
                return;
            }
            for (const auto& [document_id, _] : word_to_document_freqs_.at(word))
            {
                cm.erase(document_id);
            }
        });

    std::vector<Document> matched_documents;
    for (const auto& [document_id, relevance] : cm.BuildOrdinaryMap())
    {
        matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
    }

    return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const VecQuery& query, DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;
    for (const std::string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }
    for (const std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto& [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    for (const auto& [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}



template <typename ForwardRange, typename Function>
void SearchServer::ForEach(const std::execution::parallel_policy&, ForwardRange& range, Function function) const {
    if constexpr (std::is_same_v<typename std::iterator_traits<typename ForwardRange::iterator>::iterator_category,
        std::random_access_iterator_tag>)
    {
        for_each(range.begin(), range.end(), function);
    }
    else
    {
        const int size = static_cast<int>(range.size()) / THREAD_COUNT;
        auto it_begin = range.begin();
        auto it_end = std::next(it_begin, size);

        std::vector<std::future<void>> vector_futures;
        int num_threads = 0;
        while (num_threads < THREAD_COUNT)
        {
            vector_futures.push_back(std::async(
                [function, it_begin, it_end]
                {
                    std::for_each(it_begin, it_end, function);
                }));

            ++num_threads;
            it_begin = it_end;
            num_threads == THREAD_COUNT - 1 ? it_end = range.end() : it_end = std::next(it_begin, size);
        }
    }
}

void RemoveDuplicates(SearchServer& search_server);
