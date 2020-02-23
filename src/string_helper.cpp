#include "string_helper.h"
#include <algorithm>
#include <cctype>

std::string& strHelper::trim(std::string& str, const char thechar)
{
    if (str.empty()) {
        return str;
    }

    std::string::size_type pos = str.find_first_not_of(thechar);
    if (pos != std::string::npos) {
        str.erase(0, pos);
    }
    pos = str.find_last_of(thechar);
    if (pos != std::string::npos) {
        str.erase(str.find_last_not_of(thechar) + 1);
    }
    return str;
}

