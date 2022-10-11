#pragma once
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <iostream>
#include "document.h"
#include "string_processing.h"
#include <execution>
#include <string_view>
#include <list>

const int MAX_RESULT_DOCUMENT_COUNT = 5;

const double CALCULATING_ERROR = 1e-6;

    class SearchServer {
    public:
        using MyTulpe = std::tuple<std::vector<std::string_view>, DocumentStatus>;

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

        int GetDocumentCount() const;

        MyTulpe MatchDocument(std::execution::parallel_policy par,const std::string_view raw_query, int document_id) const;
        MyTulpe MatchDocument(std::execution::sequenced_policy seq, const std::string_view raw_query, int document_id) const {
            return MatchDocument(raw_query, document_id);
        }
       
        MyTulpe MatchDocument(const std::string_view raw_query, int document_id) const;

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
            //std::string str;
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
        struct Query {
            std::set<std::string_view> plus_words;
            std::set<std::string_view> minus_words;
        };

       // const std::set<std::string,std::less<>> stop_words_;
        const std::set<std::string,std::less<>> stop_words_;

        std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
        std::map<int, std::map<std::string_view, double>> word_to_document_freqs_new;


        std::map<int, DocumentData> documents_;
        std::set<int> document_ids_;


        bool IsStopWord(const std::string_view word) const;

        static bool IsValidWord(const std::string_view word); // A valid word must not contain special characters
        std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view text) const;

        static int ComputeAverageRating(const std::vector<int>& ratings);

        QueryWord ParseQueryWord(const std::string_view text) const;
        VecQuery ParseQuery(std::execution::parallel_policy par, const std::string_view text) const;
        Query ParseQuery(const std::string_view text) const;

        // Existence required
        double ComputeWordInverseDocumentFreq(const std::string_view word) const;

        template <typename DocumentPredicate>
        std::vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const;

    };

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words))  // Extract non-empty stop words
    //: stop_words_(stop_words)
{
    using namespace std::literals::string_literals;
    if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
        throw std::invalid_argument("Some of stop words are invalid"s);
    }
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentPredicate document_predicate) const {
    const auto query = ParseQuery(raw_query);

    auto matched_documents = FindAllDocuments(query, document_predicate);

    sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
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

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;
    for (const std::string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto &[document_id, term_freq] : word_to_document_freqs_.at(word)) {
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
        for (const auto &[document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    for (const auto &[document_id, relevance] : document_to_relevance) {
        matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}

void RemoveDuplicates(SearchServer& search_server);
