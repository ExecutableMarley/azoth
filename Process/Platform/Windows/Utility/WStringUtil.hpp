#pragma once

#include <string>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#else
#include <locale>
#include <codecvt>
#endif

namespace Azoth
{


namespace StringUtil
{
    /**
     * @brief Convert a UTF-16 wide string to a UTF-8 encoded string.
     * 
     * @param[in] wstr Input wide string.
     * @return UTF-8 encoded string.
     */
	inline std::string toUtf8(const std::wstring& wstr)
	{
#ifdef _WIN32
		if (wstr.empty()) return {};
		int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
		std::string result(sizeNeeded, 0);
		WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), result.data(), sizeNeeded, nullptr, nullptr);
		return result;
#else
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
		return conv.to_bytes(wstr);
#endif
	}

    /**
     * @brief Convert a UTF-8 encoded string to a UTF-16 wide string.
     * 
     * @param[in] str Input UTF-8 encoded string.
     * @return UTF-16 wide string. 
     */
	inline std::wstring toUtf16(const std::string& str)
	{
#ifdef _WIN32
		if (str.empty()) return {};
		int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), nullptr, 0);
		std::wstring result(sizeNeeded, 0);
		MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), result.data(), sizeNeeded);
		return result;
#else
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
		return conv.from_bytes(str);
#endif
	}

    /**
     * @brief Convert a wide string to lowercase.
     * 
     * @param[in] str Input wide string.
     * @return Lowercase copy of the input string.
     */
	inline std::wstring to_lower(const std::wstring& str)
	{
		std::wstring lower_str = str;
		std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(),
			[](wchar_t c) { return std::tolower(c); });
		return lower_str;
	}
}


}