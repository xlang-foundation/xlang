/*
Copyright (C) 2024 The XLang Foundation
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

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