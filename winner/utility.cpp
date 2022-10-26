#include "utility.h"
#include <cctype>
#include <codecvt>
#include <locale>
#include <sstream>

std::wstring s2ws(const std::string& str)
{
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.from_bytes(str);
}
std::string ws2s(const std::wstring& wstr)
{
	using convert_type = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_type, wchar_t> converter;
	std::string ret = converter.to_bytes(wstr);
	return ret;
}

