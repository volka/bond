// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

#include <bond/core/config.h>

#include <bond/core/blob.h>
#include <bond/core/containers.h>

#include <exception>
#include <stdio.h>

namespace bond
{


template <typename Buffer, typename T, typename Enable = void> struct
implements_varint_write
    : std::false_type {};


template <typename Buffer, typename T> struct
implements_varint_write<Buffer, T,
#ifdef BOND_NO_SFINAE_EXPR
    typename boost::enable_if<check_method<void (Buffer::*)(T), &Buffer::WriteVariableUnsigned> >::type>
#else
    detail::mpl::void_t<decltype(std::declval<Buffer>().WriteVariableUnsigned(std::declval<T>()))>>
#endif
    : std::true_type {};


template<typename Buffer, typename T>
inline
typename boost::enable_if<implements_varint_write<Buffer, T> >::type
WriteVariableUnsigned(Buffer& output, T value)
{
    BOOST_STATIC_ASSERT(std::is_unsigned<T>::value);

    // Use Buffer's implementation of WriteVariableUnsigned
    output.WriteVariableUnsigned(value);
}


template<typename Buffer, typename T>
inline
typename boost::disable_if<implements_varint_write<Buffer, T> >::type
WriteVariableUnsigned(Buffer& output, T value)
{
    BOOST_STATIC_ASSERT(std::is_unsigned<T>::value);

    // Use generic WriteVariableUnsigned
    GenericWriteVariableUnsigned(output, value);
}


template<typename Buffer, typename T>
BOND_NO_INLINE
void GenericWriteVariableUnsigned(Buffer& output, T value)
{
    T x = value;

    if (value >>= 7)
    {
        output.Write(static_cast<uint8_t>(x | 0x80));
        WriteVariableUnsigned(output, value);
    }
    else
    {
        output.Write(static_cast<uint8_t>(x));
    }
}


template <typename Buffer, typename T, typename Enable = void> struct
implements_varint_read
    : std::false_type {};


template <typename Buffer, typename T> struct
implements_varint_read<Buffer, T,
#ifdef BOND_NO_SFINAE_EXPR
    typename boost::enable_if<check_method<void (Buffer::*)(T&), &Buffer::ReadVariableUnsigned> >::type>
#else
    detail::mpl::void_t<decltype(std::declval<Buffer>().ReadVariableUnsigned(std::declval<T&>()))>>
#endif
    : std::true_type {};


template<typename Buffer, typename T>
inline
typename boost::enable_if<implements_varint_read<Buffer, T> >::type
ReadVariableUnsigned(Buffer& input, T& value)
{
    BOOST_STATIC_ASSERT(std::is_unsigned<T>::value);

    // Use Buffer's implementation of ReadVariableUnsigned
    input.ReadVariableUnsigned(value);
}


template<typename Buffer, typename T>
BOND_NO_INLINE
void GenericReadVariableUnsigned(Buffer& input, T& value)
{
    value = 0;
    uint8_t byte;
    uint32_t shift = 0;

    do
    {
        input.Read(byte);

        T part = byte & 0x7f;
        value += part << shift;
        shift += 7;
    }
    while(byte >= 0x80);
}


template<typename Buffer, typename T>
inline
typename boost::disable_if<implements_varint_read<Buffer, T> >::type
ReadVariableUnsigned(Buffer& input, T& value)
{
    BOOST_STATIC_ASSERT(std::is_unsigned<T>::value);

    // Use generic ReadVariableUnsigned
    GenericReadVariableUnsigned(input, value);
}


// ZigZag encoding
template<typename T>
inline
typename std::make_unsigned<T>::type EncodeZigZag(T value)
{
    return (value << 1) ^ (value >> (sizeof(T) * 8 - 1));
}

// ZigZag decoding
template<typename T>
inline
typename std::make_signed<T>::type DecodeZigZag(T value)
{
    return (value >> 1) ^ (-static_cast<typename std::make_signed<T>::type>((value & 1)));
}


namespace detail
{

// HexDigit
inline char HexDigit(int n)
{
    char d = n & 0xf;
    return d < 10 ? ('0' + d) : ('a' + d - 10);
}

inline int HexDigit(char c)
{
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    else if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    else
        return c - '0';
}

// Base64 encoding
inline std::string EncodeBase64(const char* data, uint32_t length)
{
    static const char base64_chars[] = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    std::string result;
    result.reserve(((length + 2) / 3) * 4);
    
    for (uint32_t i = 0; i < length; i += 3) {
        uint32_t value = 0;
        int count = 0;
        
        for (int j = 0; j < 3 && i + j < length; ++j) {
            value = (value << 8) | static_cast<unsigned char>(data[i + j]);
            ++count;
        }
        
        value <<= (3 - count) * 8;
        
        for (int j = 0; j < 4; ++j) {
            if (j <= count) {
                result.push_back(base64_chars[(value >> (18 - j * 6)) & 0x3F]);
            } else {
                result.push_back('=');
            }
        }
    }
    
    return result;
}

// Base64 decoding
inline std::vector<char> DecodeBase64(const std::string& encoded)
{
    static const int base64_decode_table[256] = {
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,
        52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,
        -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
        15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
        -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
        41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
    };
    
    std::vector<char> result;
    result.reserve((encoded.length() * 3) / 4);
    
    for (size_t i = 0; i < encoded.length(); i += 4) {
        uint32_t value = 0;
        int count = 0;
        
        for (int j = 0; j < 4 && i + j < encoded.length(); ++j) {
            char c = encoded[i + j];
            if (c == '=') break;
            
            int decoded = base64_decode_table[static_cast<unsigned char>(c)];
            if (decoded == -1) continue; // Skip invalid characters
            
            value = (value << 6) | decoded;
            ++count;
        }
        
        for (int j = 0; j < count - 1; ++j) {
            result.push_back(static_cast<char>((value >> (8 * (count - 2 - j))) & 0xFF));
        }
    }
    
    return result;
}

template <typename T, typename Enable = void> struct
string_char_int_type;

template <typename T> struct
string_char_int_type<T, typename boost::enable_if<is_string<T> >::type>
{
    typedef uint8_t type;
};

template <typename T> struct
string_char_int_type<T, typename boost::enable_if<is_wstring<T> >::type>
{
    typedef uint16_t type;
};

template <typename Buffer, typename T>
typename boost::enable_if_c<(sizeof(typename element_type<T>::type) == sizeof(typename string_char_int_type<T>::type))>::type
inline ReadStringData(Buffer& input, T& value, uint32_t length)
{
    resize_string(value, length);
    input.Read(string_data(value), length * sizeof(typename element_type<T>::type));
}

template <typename Buffer, typename T>
typename boost::enable_if_c<(sizeof(typename element_type<T>::type) > sizeof(typename string_char_int_type<T>::type))>::type
inline ReadStringData(Buffer& input, T& value, uint32_t length)
{
    resize_string(value, length);
    typename element_type<T>::type* data = string_data(value);
    typename element_type<T>::type* const data_end = data + length;
    typename string_char_int_type<T>::type ch;
    for (; data != data_end; ++data)
    {
        input.Read(ch);
        *data = static_cast<typename element_type<T>::type>(ch);
    }
}

template <typename Buffer, typename T>
typename boost::enable_if_c<(sizeof(typename element_type<T>::type) == sizeof(typename string_char_int_type<T>::type))>::type
inline WriteStringData(Buffer& output, const T& value, uint32_t length)
{
    output.Write(string_data(value), length * sizeof(typename element_type<T>::type));
}

template <typename Buffer, typename T>
typename boost::enable_if_c<(sizeof(typename element_type<T>::type) > sizeof(typename string_char_int_type<T>::type))>::type
inline WriteStringData(Buffer& output, const T& value, uint32_t length)
{
    const typename element_type<T>::type* data = string_data(value);
    const typename element_type<T>::type* const data_end = data + length;
    typename string_char_int_type<T>::type ch;
    for (; data != data_end; ++data)
    {
        ch = static_cast<typename string_char_int_type<T>::type>(*data);
        output.Write(ch);
    }
}

} // namespace detail

} // namespace bond
