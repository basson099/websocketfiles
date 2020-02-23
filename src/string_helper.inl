/* ***********************************************
 * string helper inline file
 */

#include <string.h>
#include <sstream>

template <typename TYPE> inline
int strHelper::splitStr(TYPE& list,
                const std::string& str, const char* delim)
{
    if (str.empty())
        return 0;
        
    if (delim == NULL){
        list.push_back(str);
        return 1;
    }

    unsigned int size = strlen(delim);

    std::string::size_type prepos = 0;
    std::string::size_type pos = 0;
    int count = 0;

    for(;;) 
    {
        pos = str.find(delim, pos);
        if (pos == std::string::npos){
            if (prepos < str.size()){
                list.push_back(str.substr(prepos));
                count++;
            }
            break;
        }

        list.push_back(str.substr(prepos, pos-prepos));

        count++;
        pos += size;
        prepos = pos;
    }

    return count;
}

template <typename T, typename S> inline
const T strHelper::valueOf(const S& a)
{
    std::stringstream s;
    T t;
    s << a ; 
    s >> t;
    return t;
}


