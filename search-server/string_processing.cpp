#include "string_processing.h"



std::vector<std::string_view> SplitIntoWordsView(std::string_view str) {
    std::vector<std::string_view> result;
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