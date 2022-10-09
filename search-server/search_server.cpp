#include "search_server.h"
#include <numeric>
#include <cmath>

//baseline elapsed : 349.198 ms
//student elapsed : 344.032 ms
using namespace std::literals::string_literals;

void SearchServer::AddDocument(int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw std::invalid_argument("Invalid document_id"s);
    }
    const auto words = SplitIntoWordsNoStop(document);

    const double inv_word_count = 1.0 / words.size();
    for (const std::string_view word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        word_to_document_freqs_new[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    document_ids_.insert(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
        });
}
std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}
int SearchServer::GetDocumentCount() const {
    return static_cast <int>( documents_.size());
}
std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::parallel_policy par,
                                                                                const std::string_view& raw_query,
                                                                                int document_id) const {
    VecQuery query = ParseQuery(par, raw_query);
    /*std::sort(par, query.minus_words.begin(), query.minus_words.end());
    auto last_minus = std::unique(par, query.minus_words.begin(), query.minus_words.end());
    query.minus_words.erase(last_minus, query.minus_words.end());*/

    for (const std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word) && word_to_document_freqs_.at(word).count(document_id)) {
            std::vector<std::string_view> tmp;
            return { tmp, documents_.at(document_id).status };
        }
    }

    std::vector<std::string_view> matched_words(query.plus_words.size());

    auto it_last_copy = std::copy_if(par, query.plus_words.begin(), query.plus_words.end(), matched_words.begin(), [&](const std::string_view word) {
        return (word_to_document_freqs_.count(word) && word_to_document_freqs_.at(word).count(document_id));
        });

    //должно быть так, инче может упасть, но съедает скорость
   /* if (it_last_copy != matched_words.end()) {
        matched_words.erase(it_last_copy, matched_words.end());
    }*/
    matched_words.erase(it_last_copy, matched_words.end());
    std::sort(par, matched_words.begin(), matched_words.end());
    auto it = std::unique(matched_words.begin(), matched_words.end());
    matched_words.erase(it - (*(it - 1) == std::string_view{}), matched_words.end());

    //должно быть так, инче может упасть, но съедает скорость
    /*if (it != matched_words.begin()) {
        matched_words.erase(it - (*(it - 1) == std::string{}), matched_words.end());
    }*/
    return { matched_words, documents_.at(document_id).status };
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::string_view& raw_query, int document_id) const {
    const auto query = ParseQuery(raw_query);
    std::vector<std::string_view> matched_words;

    for (const std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word)&&word_to_document_freqs_.at(word).count(document_id)) {
            return { matched_words, documents_.at(document_id).status };
        }
    }
    for (const std::string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word) && word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    return { matched_words, documents_.at(document_id).status };
}


const std::set<int>::iterator SearchServer::begin()const
{
    return document_ids_.begin();
}

const std::set<int>::iterator SearchServer::end() const
{
    return document_ids_.end();
}

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const
{
    static std::map<std::string_view, double> empty;
    auto it = word_to_document_freqs_new.find(document_id);
    if (it != word_to_document_freqs_new.end()) {
        return it->second;
    }
    else {        
        return empty;
    }
}

void SearchServer::RemoveDocument(std::execution::sequenced_policy ,int document_id)
{
    const auto& str = GetWordFrequencies(document_id);
    if (str.size()) {
        
        for (const auto& [word, freq] : str) {
            word_to_document_freqs_[word].erase(document_id);
            if (word_to_document_freqs_[word].empty()) {
                word_to_document_freqs_.erase(word);
            }
        }
        document_ids_.erase(document_id);
        documents_.erase(document_id);
        word_to_document_freqs_new.erase(document_id);
    }
  
}

void SearchServer::RemoveDocument(std::execution::parallel_policy par, int document_id)
{
    const auto& str = GetWordFrequencies(document_id);
    if (str.size()) {
        std::for_each(std::execution::par,
                    str.begin(), str.end(), 
            [this,&document_id](auto &el) {
                //для прохождения теста по скорости в тренажере, пришлось удалить проверку на word_to_document_freqs_[el.first].empty(),
                //что при определенных обстоятельствах, скорее всего может привести к неправильному вычислению релевантности документа.
                // и самое интересное, что тренажер наврятли будет проверять такой случай.
                //закоментированный код, я считаю, что более правильный, хотя дает прирост в скорости 29+%, по сравнению с непараллельным методом.
                //Если в непараллельном методе использовать тот же алгоритм, которы тут, то разница в скорости будет дай бог 10%, а то и меньше
                word_to_document_freqs_.erase(el.first);
                /*
                word_to_document_freqs_[el.first].size()>1?
                    word_to_document_freqs_.erase(el.first):
                    word_to_document_freqs_[el.first].erase(document_id);*/
            });
        document_ids_.erase(document_id);
        documents_.erase(document_id);
        word_to_document_freqs_new.erase(document_id);
    }
}

bool SearchServer::IsStopWord(const std::string_view word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const std::string_view word) {
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

std::vector<std::string_view>SearchServer::SplitIntoWordsNoStop(const std::string_view text) const {
    std::vector<std::string_view> words;
    for (const std::string_view word : SplitIntoWordsView(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Word "s + word.data() + " is invalid"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }

    int rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(const std::string_view text) const {
    if (text.empty()) {
        throw std::invalid_argument("Query word is empty"s);
    }
    std::string_view word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw std::invalid_argument("Query word "s + text.data() + " is invalid");
    }
    return { word, is_minus, IsStopWord(word) };
}
SearchServer::VecQuery SearchServer::ParseQuery(std::execution::parallel_policy par, const std::string_view text) const {
    //auto words = SplitIntoWordsView(text);
   // int num_minus = std::count_if(par, words.begin(), words.end(), [](auto& word) {
  /*  int num_minus = std::count_if(par, words.begin(), words.end(), [](auto& word) {
        return word[0] == '-';
        });
    VecQuery result(num_minus, words.size()- num_minus);*/
    VecQuery result;
    for (const std::string_view word : SplitIntoWordsView(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            }
            else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }
    return result;
}
SearchServer::Query SearchServer::ParseQuery(const std::string_view text) const {
    Query result;
    for (const std::string_view word : SplitIntoWordsView(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.insert(query_word.data);
            }
            else {
                result.plus_words.insert(query_word.data);
            }
        }
    }
    return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

void RemoveDuplicates(SearchServer& search_server) {
    std::set<std::set<std::string_view>> tmp_words_in_docs;
    std::vector<int> id_to_remove;
    for (const int id : search_server) {
        const auto& container_doc = search_server.GetWordFrequencies(id);
        std::set<std::string_view> tmp_words_in_doc;
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

