#include <string>

namespace cvasgmt
{
namespace utils
{
std::string ltrim(const std::string &s, char ch)
{
    auto it = s.begin();
    while (it != s.end() && *it == ch)
        ++it;
    return std::string(it, s.end());
}

std::string rtrim(const std::string &s, char ch)
{
    auto it = s.end();
    while (it != s.begin() && *(it - 1) == ch)
        --it;
    return std::string(s.begin(), it);
}

std::string trim(const std::string &s, char ch)
{
    return ltrim(rtrim(s, ch), ch);
}
}
}