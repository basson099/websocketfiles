#ifndef STRING_HELPER_HEAD_FILE_
#define STRING_HELPER_HEAD_FILE_

#include <string>

class strHelper {
public:

    // split the string to array
    template <typename TYPE>
    static int splitStr(TYPE& list,
                 const std::string& str, const char* delim);

    // trim the specify char
    static std::string& trim(std::string& str, const char thechar = ' ');
    // convert type T to string
    template <typename T, typename S> 
    static const T valueOf(const S &a);

};

#include "string_helper.inl"

#endif /* STRING_HELPER_HEAD_FILE_ */
