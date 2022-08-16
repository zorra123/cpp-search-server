//Вставьте сюда своё решение из урока «‎Очередь запросов».‎
#include "document.h"

using namespace std::literals::string_literals;
std::ostream& operator<<(std::ostream& out, const Document& document) {
	out << "{ "s
		<< "document_id = "s << document.id << ", "s
		<< "relevance = "s << document.relevance << ", "s
		<< "rating = "s << document.rating << " }"s;
	return out;
}