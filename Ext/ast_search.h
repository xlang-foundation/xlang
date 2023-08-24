#pragma once

#include <string>
#include <vector>

enum class SearchActionType 
{
    SearchLeftOnly,
    SearchRightOnly,
    SearchBoth
};

struct SearchPattern 
{
    std::string name;
    SearchActionType actionType;
    bool wholematch;
};


std::vector<SearchPattern> ParsePattern(const std::string& pattern);
