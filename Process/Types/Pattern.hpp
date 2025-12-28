#pragma once

#include <vector>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <regex>

typedef unsigned char BYTE;

namespace Azoth
{


/**
 * @brief Binary pattern with wildcard support for memory matching.
 *
 */
class Pattern
{
public:
	std::vector<uint8_t> bytes;
	std::vector<uint8_t> mask;

public:
	/**
     * @brief Construct an empty pattern.
     */
	Pattern() = default;

	/**
     * @brief Construct a pattern from a combined signature string.
	 * 
	 * The string should consist of hexadecimal byte value and wildcards ('?'),
	 * for example:
	 * 
	 * "48 8B ? ? 89"
	 * 
     */
	Pattern(const std::string& combo)
	{
		parseCombo(combo);
	}

	/**
     * @brief Construct a pattern from raw data and a mask string.
	 * 
	 * @param data        Pointer to raw byte data.
	 * @param maskString  Mask definition where wildcards are specified.
	 * 
	 * The length of the pattern is determined by the mask string.
	 * Bytes corresponding to wildcard positions will be ignored during matching.
     */
    Pattern(const void* data, const std::string& maskString)
    {
        bytes.resize(maskString.size());
        std::memcpy(bytes.data(), data, maskString.size());
        parseMaskString(maskString);
    }

	/**
     * @brief Number of bytes in the pattern.
     */
    size_t size() const noexcept
    {
	    return bytes.size();
    }

	/**
     * @brief Check whether a specific pattern index is a wildcard.
     *
     * @param i Byte index within the pattern.
     * @return True if the byte at index @p i is ignored during matching.
     */
	bool isWildcard(size_t i) const noexcept
	{
		return i < mask.size() && !mask[i];
	}

	/**
     * @brief Check whether the pattern is empty.
     */
	bool empty() const noexcept
	{
		return bytes.empty();
	}

	/**
     * @brief Match the pattern against a memory region.
     *
     * @param data   Pointer to the start of the memory region.
     * @param length Size of the memory region in bytes.
     *
     * @return True if the pattern matches at the beginning of @p data.
     */
	bool matches(const BYTE* data, size_t length) const noexcept
	{
		if (length < bytes.size()) return false;
		for (size_t i = 0; i < bytes.size(); ++i)
        {
			if (mask[i] && data[i] != bytes[i])
				return false;
		}
		return true;
	}

	/**
     * @brief Match the pattern against a memory range.
     *
     * @param startPos Pointer to the beginning of the range.
     * @param endPos   Pointer to the end of the range.
     *
     * @return True if the pattern matches at @p startPos.
     */
	bool matches(const BYTE* startPos, const BYTE* endPos) const noexcept
	{
		return matches(startPos, endPos - startPos);
	}

	/**
     * @brief Convert the pattern to a human-readable string.
     *
     * Wildcard bytes are represented as "??".
     * Non-wildcard bytes are printed as two-digit hexadecimal values.
     *
     * Example output:
     *   "48 8B ?? ?? 89"
     */
	std::string toString() const
	{
		std::ostringstream oss;
		for (size_t i = 0; i < bytes.size(); ++i)
		{
			if (!mask[i])
				oss << "??";
			else
				oss << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
				<< static_cast<int>(bytes[i]);
			if (i + 1 < bytes.size()) oss << ' ';
		}
		return oss.str();
	}

private:
    void parseMaskString(const std::string& maskString)
    {
        if (bytes.size() != maskString.length())
        {
            throw std::invalid_argument("Size mismatch between bytes and mask.");
        }
        mask.clear();
        mask.reserve(maskString.length());
        for (char c : maskString)
        {
            mask.push_back((c == 'x') ? 0xFF : 0x00);
        }
    }

    void parseCombo(const std::string& combo)
	{
		this->bytes.clear();
		this->mask.clear();

		auto cleanedCombo = cleanPattern(combo);
		if (cleanedCombo.empty())
		{
			return;
		}

		std::stringstream ss(cleanedCombo);
		std::string token;

		while (ss >> token)
		{
			if (token == "?")
			{
				// --- Wildcard Byte ---
				bytes.push_back(0x00);
				mask.push_back(0x00);
			}
			else if (token.length() == 2)
			{
				try
				{
					// Convert the 2-character hex string to an integer
					uint32_t byteValue = std::stoul(token, nullptr, 16);

					if (byteValue > 0xFF)
					{
                        //Should not be possible?
						throw std::out_of_range("Byte value out of range ( > 0xFF).");
					}

					bytes.push_back(static_cast<uint8_t>(byteValue));
					mask.push_back(0xFF);
				}
				catch (const std::exception& e)
				{
					// parsing errors
					std::cerr << "Error parsing token '" << token << "': " << e.what() << std::endl;
					bytes.clear();
					mask.clear();
					return;
				}
			}
			else
			{
				// --- Malformed Token ---
				std::cerr << "Malformed token found: " << token << std::endl;
				bytes.clear();
				mask.clear();
				return;
			}
		}
	}

	std::string cleanPattern(std::string source)
	{
		std::regex space_re(R"(\s+)");
		std::regex quest_re(R"(\?+)");
		std::regex bytes_re(R"(0x)");

		source = std::regex_replace(source, space_re, " ");
		source = std::regex_replace(source, quest_re, "?");
		source = std::regex_replace(source, bytes_re, "");
		return source;
	}
};


}