#include "ast_search.h"
#include <regex>
#include <iostream>

std::vector<SearchPattern> ParsePattern(const std::string& pattern) 
{
    std::vector<SearchPattern> results;

    std::regex partRegex("([=~]?)([^=~.]+)([=~]?)");
    std::sregex_token_iterator iter(pattern.begin(), pattern.end(), partRegex, 0);
    std::sregex_token_iterator end;
    while (iter != end) {
        std::string part = *iter;
        SearchPattern sp;
        sp.wholematch = (part[0] != '~');
        if (!sp.wholematch) part = part.substr(1);

        if (part.front() == '=') {
            sp.actionType = SearchActionType::SearchRightOnly;
            part = part.substr(1);
        }
        else if (part.back() == '=') {
            sp.actionType = SearchActionType::SearchLeftOnly;
            part.pop_back();
        }
        else {
            sp.actionType = SearchActionType::SearchBoth;
        }
        sp.name = part;

        results.push_back(sp);
        ++iter;
    }

    return results;
}

static void test() 
{
    auto patterns = ParsePattern("name1=.=~name2.name3");
    for (const auto& pattern : patterns) 
    {
        std::cout << "Name: " << pattern.name << ", ActionType: " << static_cast<int>(pattern.actionType)
            << ", WholeMatch: " << pattern.wholematch << std::endl;
    }
}