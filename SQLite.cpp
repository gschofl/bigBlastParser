#include "SQLite.hpp"


std::vector<std::string> split(const std::string& s,
                               const std::string& delim = " ",
                               const bool keep_empty = true)
{
    std::vector<std::string> elems;
    std::string::const_iterator start = s.begin(), end;
    while (true) {
        end = search(start, s.end(), delim.begin(), delim.end());
        std::string temp(start, end);
        if (keep_empty || !temp.empty()) {
            elems.push_back(temp);
        }
        if (end == s.end()) {
            break;
        }
        start = end + delim.size();
    }
    return elems;
}

