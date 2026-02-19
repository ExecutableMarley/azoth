/*
 * Copyright (c) Marley Arns
 * Licensed under the MIT License.
*/

#pragma once

#include <stdint.h>

namespace Azoth
{

namespace MemIn
{
    typedef uint8_t BYTE;

    inline size_t findCodeCaveSize(const void* ptr, size_t bufferSize)
    {
        if (!ptr || bufferSize == 0)
            return 0;

        const unsigned char* bytes = static_cast<const unsigned char*>(ptr);
        const unsigned char* end = bytes + bufferSize;
        BYTE caveByte = *(BYTE*)ptr;
        size_t length = 0;

        for (const BYTE* i = (BYTE*)ptr; i < end; i++)
        {
            if (*i == caveByte)
                length++;
            else
                break;
        }
        return length;
    }

    inline size_t findAsciiStringLength(const void* ptr, size_t bufferSize)
    {
        if (!ptr || bufferSize == 0)
            return 0;

        const unsigned char* bytes = static_cast<const unsigned char*>(ptr);
        const unsigned char* end = bytes + bufferSize;
        size_t length = 0;

        for (const unsigned char* i = bytes; i < end; ++i)
        {
            unsigned char c = *i;
            if (c == '\0' || c < ' ' || c > '~')
                break;
            ++length;
        }
        return length;
    }

    inline size_t findAsciiStringUTF16Length(const void* ptr, size_t bufferSize)
    {
        if (!ptr || bufferSize == 0)
            return 0;

        const uint8_t* bytes = static_cast<const uint8_t*>(ptr);
        const uint8_t* end = bytes + bufferSize;
        size_t length = 0;

        for (const uint8_t* i = bytes; i + 1 < end; i += sizeof(char16_t))
        {
            const uint8_t lo = i[0];
            const uint8_t hi = i[1];

            if (lo == 0x00 && hi == 0x00)
                break;

            if (lo >= ' ' && lo <= '~' && hi == 0x00)
                length += sizeof(char16_t);
            else
                break;
        }

        return length;
    }
}

}