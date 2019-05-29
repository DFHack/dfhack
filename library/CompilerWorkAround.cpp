#include <regex>

namespace DFHack {
namespace neverCalled {

/**
 * gcc/linstdc++ seems to generate code that links libstdc++ back to first
 * shared object using std::regex. To allow plugins unload with std::regex in
 * the code we need the std::regex functions inside libdfhack.so.
 *
 * If your plugin decides to use any overloads that aren't listed here it may
 * stay in memory after dlclose.
 */
std::regex stdRegexPluginUnloadWorkaround()
{
    std::regex fake("foo");
    std::string haystack("bar is foo in the world");
    std::regex fake2(std::string("bar"));
    if (std::regex_match(haystack, fake))
        std::swap(fake, fake2);
    if (std::regex_search(haystack, fake))
        std::swap(fake, fake2);
    const char* haystack2 = "foo";
    if (std::regex_match(haystack2, fake))
        std::swap(fake, fake2);
    if (std::regex_search(haystack2, fake))
        std::swap(fake, fake2);
    return fake;
}

}
}
