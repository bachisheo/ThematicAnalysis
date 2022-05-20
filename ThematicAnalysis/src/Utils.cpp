﻿#include "Utils.h"

#include <algorithm>
#include <numeric>
#include <string>


size_t Utils::calculateHashCode(std::wstring const& text)
{
	static std::hash<std::wstring> _hashCalculator;
	return _hashCalculator(text);
}

std::wstring Utils::sortAndConcatWords(std::vector<std::wstring>  words)
{
	std::sort(words.begin(), words.end());
	return std::accumulate(words.begin(), words.end(), std::wstring(L""));
}
