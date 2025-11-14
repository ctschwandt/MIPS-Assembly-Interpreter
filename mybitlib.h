// File  : mybitlib.h
// Author: Cole Schwandt

#ifndef MYBITLIB_H
#define MYBITLIB_H

#include <cstdlib>
#include <string>

inline
uint32_t mask_bits(unsigned width)
{
    return ((1u << width) - 1u);
}

inline
std::string to_binary(uint32_t x, int width)
{
    std::string b(width, '0');
    for (int i = width - 1; i >= 0; --i)
    {
        if (x & 1u)
            b[i] = '1';
        x >>= 1;
    }
    return b;
}

inline
std::string to_binary32(uint32_t x)
{
    return to_binary(x, 32);
}

inline
std::string to_octal(uint32_t x, int width)
{
    std::string o(width, '0');
    for (int i = width - 1; i >= 0; --i)
    {
        uint32_t digit = x & 0x7u; // 3 bits
        o[i] = static_cast<char>('0' + digit);
        x >>= 3;
    }
    return o;
}

inline
std::string to_octal32(uint32_t x)
{
    return to_octal(x, 11);
}

inline
std::string to_hex(uint32_t x, int width)
{
    std::string h(width, '0');
    for (int i = width - 1; i >= 0; --i)
    {
        uint32_t digit = x & 0xFu; // 4 bits
        char c;
        if (digit < 10)
            c = static_cast<char>('0' + digit);
        else
            c = static_cast<char>('A' + (digit - 10));
        h[i] = c;
        x >>= 4;
    }
    return h;
}

inline
std::string to_hex32(uint32_t x)
{
    return to_hex(x, 8);
}

#endif // MYBITLIB_H
