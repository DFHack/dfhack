#include "Internal.h"
#include "MiscUtils.h"

#include <iostream>
#include <string>
using namespace std;

int compare_result(const vector<string> &expect, const vector<string> &result)
{
    if (result == expect)
    {
        cout << "ok\n";
        return 0;
    }
    else {
        cout << "not ok\n";
        auto e = expect.begin();
        auto r = result.begin();
        cout << "# " << setw(20) << left << "Expected" << " " << left << "Got\n";
        while (e < expect.end() || r < result.end())
        {
            cout
                << "# "
                << setw(20) << left << ":" + (e < expect.end() ? *e++ : "") + ":"
                << " "
                << setw(20) << left << ":" + (r < result.end() ? *r++ : "") + ":"
                << "\n";
        }
        return 1;
    }
}

int main()
{
    int fails = 0;
#define TEST_WORDWRAP(label, expect, args) \
    { \
        vector<string> result; \
        cout << label << "... "; \
        word_wrap args; \
        fails += compare_result(expect, result); \
    }

    TEST_WORDWRAP("Short line, no wrap", vector<string>({"12345"}), (&result, "12345", 15));

    return fails == 0 ? 0 : 1;
}
