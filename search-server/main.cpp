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


void TestExcludeStopWordsFromAddedDocumentContent() {//for 1&2
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
        //assert(found_docs.size() == 1);
        const Document& doc0 = found_docs[0];
        ASSERT_HINT(doc0.id == doc_id, "Failed by adding doc (doc.id isn't correct)");
        //assert(doc0.id == doc_id);
    }

    // Затем убеждаемся, что поиск этого же слова, входящего в список стоп-слов,
    // возвращает пустой результат
    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Failed by searching doc (doc contain stop words)");
        //assert(server.FindTopDocuments("in"s).empty());
    }
}

/*
Разместите код остальных тестов здесь
*/

void TestMinusWords() {//for 3
    SearchServer server;

    server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    auto found_docs = server.FindTopDocuments("пушистый ухоженный -кот"s);
    ASSERT_HINT(found_docs.size() == 0, "Failed by found doc with minus words");
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    found_docs = server.FindTopDocuments("пушистый ухоженный -кот"s);
    ASSERT_HINT(found_docs.size() == 1, "Failed by found doc with minus words");
}
void TestMatchingWords() {//for 4
    SearchServer server;
    server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    auto [first, second] = server.MatchDocument("пушистый ухоженный кот"s, 1);
    ASSERT_HINT(first.size() == 2 && first.at(0) == "кот"s && first.at(1) == "пушистый"s && first.size() == 2, "Something wrong with matching docs :)");
    auto [first2, second2] = server.MatchDocument("пушистый ухоженный -кот"s, 1);
    ASSERT_HINT(first2.empty(), "Something wrong with matching docs while search contains minus word ");
}
void TestSortRel() {//for 5
    SearchServer server;
    server.AddDocument(12, "белый кот модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    auto res = server.FindTopDocuments("пушистый ухоженный кот"s);
    ASSERT_HINT(abs(res.at(0).relevance - 0.65067242136109593) < 1e-6 && abs(res.at(1).relevance - 0.27465307216702745) < 1e-6 && abs(res.at(2).relevance - 0.10136627702704110) < 1e-6,
        "Problems with relevance (uncorrect calculating or sort");
    //assert(abs(res.at(0).relevance - 0.86643397569993164) < 1e-6 && abs(res.at(1).relevance - 0.17328679513998632) < 1e-6 && abs(res.at(2).relevance - 0.17328679513998632) < 1e-6);
    ASSERT_HINT(res.at(0).rating == 5 && res.at(1).rating == -1 && res.at(2).rating == 2, " Problems with rating. It must be arithmetic mean");
    //assert(res.at(0).rating == 5 && res.at(1).rating == 2 && res.at(2).rating == -1); //for 5 rating
    server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });
    res = server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED);
    ASSERT_HINT(res.at(0).id == 3, "Problems with search by status of doc");
    //assert(res = server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; }));
    //for 7 predicate
    res = server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; });

    ASSERT_HINT(res.size() == 2, "Problems with search by predicate");
}

// Функция TestSearchServer является точкой входа для запуска тестов
/*
void TestSearchServer() {
    TestExcludeStopWordsFromAddedDocumentContent();
    TestMinusWords();
    TestMatchingWords();
    TestRat();
    // Не забудьте вызывать остальные тесты здесь
}*/
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestMinusWords);
    RUN_TEST(TestMatchingWords);
    RUN_TEST(TestSortRel);


    // Не забудьте вызывать остальные тесты здесь
}
// --------- Окончание модульных тестов поисковой системы -----------

int main() {
    TestSearchServer();
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
}