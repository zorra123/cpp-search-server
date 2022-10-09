#include "string_processing.h"

std::vector<std::string_view> SplitIntoWordsView(std::string_view str) {
    std::vector<std::string_view> result;
    //int64_t pos = str.find_first_not_of(" ");
    const int64_t pos_end = str.npos;
    while (str.size()) {
        int64_t word_begin = str.find_first_not_of(' ');
        if (word_begin == pos_end) {
            break;
        }
        else {
            str.remove_prefix(word_begin);
            int64_t space = str.find(' ');
            if (space != pos_end) {
                result.push_back(str.substr(0, space));
                str.remove_prefix(space);
            }
            else {
                result.push_back(str);
                break;
            }
        }
    }

    return result;
}
std::vector<std::string> SplitIntoWords(const std::string& text) {
    std::vector<std::string> words;
    std::string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }
    return words;
}