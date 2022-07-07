#include "SearchServer.h"


template <typename T>
void AssertEqualImpl(const T& t, const string& t_str, const string& file,
    const string& func, unsigned line, const string& hint) {
    if (t != true) {
        cerr << boolalpha;
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT("s << t_str << ") failed."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

#define RUN_TEST(expr) expr() 
#define ASSERT_HINT(expr, hint) AssertEqualImpl((expr),  #expr,  __FILE__, __FUNCTION__, __LINE__, (hint))


void TestFindDocContainWordsFromSearchAndSupportStopWords() {//for 1&2
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    // Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
    // находит нужный документ
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in the"s);
        ASSERT_HINT(found_docs.size() == 1, "Failed by adding doc (not found correct doc)");
        const Document& doc0 = found_docs[0];
        ASSERT_HINT(doc0.id == doc_id, "Failed by adding doc (doc.id isn't correct)");
    }

    // Затем убеждаемся, что поиск этого же слова, входящего в список стоп-слов,
    // возвращает пустой результат
    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Failed by searching doc (doc contain stop words)");
    }
}


void TestFindingDocWhithMinusWords() {
    SearchServer server;

    server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    auto found_docs = server.FindTopDocuments("пушистый ухоженный -кот"s);
    ASSERT_HINT(found_docs.size() == 0, "Failed by found doc with minus words");
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    found_docs = server.FindTopDocuments("пушистый ухоженный -кот"s);
    ASSERT_HINT(found_docs.size() == 1 && found_docs.at(0).id == 2, "Failed by found doc with minus words");
}
void TestMatchingWords() {
    SearchServer server;
    server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    auto [first, second] = server.MatchDocument("пушистый ухоженный кот"s, 1);
    ASSERT_HINT(first.size() == 2 && first.at(0) == "кот"s && first.at(1) == "пушистый"s, "Something wrong with matching docs :)");
    auto [first2, second2] = server.MatchDocument("пушистый ухоженный -кот"s, 1);
    ASSERT_HINT(first2.empty(), "Something wrong with matching docs while search contains minus word ");
}
template <typename T>
bool IsSortByRelevanceCorrect(const T& res) {
    for (auto el = res.begin(); el < res.end() - 1; ++el) {
        if (el->relevance < (el + 1)->relevance)
            return false;
    }
    return true;
}

void TestCalculatingAndSortingRelevance() {
    const vector <string> content = { "белый кот модный ошейник"s, "пушистый кот пушистый хвост"s, "ухоженный пёс выразительные глаза"s };
    const string search = "пушистый ухоженный кот"s;
    const vector<string> search_words = { "пушистый"s, "ухоженный"s, "кот"s };
    SearchServer server;
    server.AddDocument(1, content.at(0), DocumentStatus::ACTUAL, { 8, -3 });
    server.AddDocument(2, content.at(1), DocumentStatus::ACTUAL, { 7, 2, 7 });
    server.AddDocument(3, content.at(2), DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    auto res = server.FindTopDocuments(search);

    ASSERT_HINT(IsSortByRelevanceCorrect(res), "Problems with relevance (uncorrect sort");
    int num_docs = 3;
    map <string, double> num_docs_for_word;
    num_docs_for_word["пушистый"s] = 1;
    num_docs_for_word["ухоженный"s] = 1;
    num_docs_for_word["кот"s] = 2;

    map <string, double> idf;
    for (const auto& el : num_docs_for_word) {
        double tmp = log(static_cast<double>(content.size() / el.second));
        idf.emplace(el.first, tmp);
    }

    double num_words_in_docs = 4;
    map <string, map<string, double>> tf;
    tf["1st doc"]["пушистый"] = 0;
    tf["1st doc"]["ухоженный"] = 0;
    tf["1st doc"]["кот"] = 1.0 / num_words_in_docs;

    tf["2st doc"]["пушистый"] = 2.0 / num_words_in_docs;
    tf["2st doc"]["ухоженный"] = 0;
    tf["2st doc"]["кот"] = 1.0 / num_words_in_docs;

    tf["3st doc"]["пушистый"] = 0;
    tf["3st doc"]["ухоженный"] = 1.0 / num_words_in_docs;
    tf["3st doc"]["кот"] = 0;

    vector <double> relevance = { 0,0,0 };
    int i = 0;
    for (const auto& doc : tf)
    {
        for (const auto& tf_word : doc.second) {
            relevance[i] += tf_word.second * idf.at(tf_word.first);
        }
        ++i;

    }
    sort(relevance.begin(), relevance.end());
    reverse(relevance.begin(), relevance.end());
    auto error_rate = 1e-6;
    i = 0;
    for (const auto& el : res) {
        ASSERT_HINT(abs(el.relevance - relevance.at(i)) < error_rate, "Problems with relevance (uncorrect calculating)");
        ++i;
    }
}

void TestCalculatingRating() {
    const vector <string> content = { "белый кот модный ошейник"s, "пушистый кот пушистый хвост"s, "ухоженный пёс выразительные глаза"s };
    const string search = "пушистый ухоженный кот"s;
    const vector<string> search_words = { "пушистый"s, "ухоженный"s, "кот"s };
    const vector<vector<int>> ratings = { {8,-3}, {7,2,7}, {5, -12, 2, 1,1 } };
    SearchServer server;
    server.AddDocument(0, content.at(0), DocumentStatus::ACTUAL, ratings.at(0));
    server.AddDocument(1, content.at(1), DocumentStatus::ACTUAL, ratings.at(1));
    server.AddDocument(2, content.at(2), DocumentStatus::ACTUAL, ratings.at(2));
    auto res = server.FindTopDocuments(search);
    vector <int> test_ratings;
    for (const auto& rating : ratings) {
        int rat = 0;
        for (const auto& el : rating) {
            rat += el;
        }
        rat /= static_cast<int>(rating.size());
        test_ratings.push_back(rat);
    }
    // el.id соответствует нумеровке элементов векотра test_ratings и id добавленного документа, id поменять - не будет работать, но мы тут и не id тестим ;)
    for (const auto& el : res) {
        ASSERT_HINT(el.rating == test_ratings.at(el.id), "Some problems to calculate ratings. It must be arefmetical mean");
    }

}
void TestFilterByPredicateByUser() {
    SearchServer server;
    server.AddDocument(12, "белый кот модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    auto res = server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; });

    for (const auto& el : res) {
        ASSERT_HINT(el.id % 2 == 0, "Problems with search by predicate");
    }


}

void TestSearchDocsByStatus() {
    SearchServer server;
    server.AddDocument(12, "белый кот модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    auto res = server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED);
    ASSERT_HINT(res.empty(), "Problems with search by status. It must be empty");
    server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });
    res = server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED);
    ASSERT_HINT(res.size() == 1 && res.at(0).id == 3, "Problems with search by status. It must be empty");

}

void TestSearchServer() {
    RUN_TEST(TestFindDocContainWordsFromSearchAndSupportStopWords);
    RUN_TEST(TestFindingDocWhithMinusWords);
    RUN_TEST(TestMatchingWords);
    RUN_TEST(TestCalculatingAndSortingRelevance);
    RUN_TEST(TestCalculatingRating);
    RUN_TEST(TestFilterByPredicateByUser);
    RUN_TEST(TestSearchDocsByStatus);
}
// --------- Окончание модульных тестов поисковой системы -----------

int main() {
    TestSearchServer();
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    std::cout << "Search server testing finished"s << endl;
}