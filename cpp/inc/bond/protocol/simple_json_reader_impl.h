// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

#include <bond/core/config.h>

#include "simple_json_reader.h"
#include <sstream>

namespace bond
{

template <typename BufferT>
inline const typename SimpleJsonReader<BufferT>::Field*
SimpleJsonReader<BufferT>::FindField(uint16_t id, const Metadata& metadata, BondDataType type, bool is_enum)
{
    rapidjson::Value::ConstMemberIterator it = MemberBegin();

    if (it != MemberEnd())
    {
        const char* name = detail::FieldName(metadata).c_str();
        detail::JsonTypeMatching jsonType(type, type, is_enum);

        // Match member by type of value and either metadata name, or string reprentation of id
        for (rapidjson::Value::ConstMemberIterator end = MemberEnd(); it != end; ++it)
        {
            if (jsonType.TypeMatch(it->value))
            {
                if (strcmp(it->name.GetString(), name) == 0)
                {
                    // metadata name match
                    return &it->value;
                }

                uint16_t parsedId;
                if (detail::try_lexical_convert(it->name.GetString(), parsedId) && id == parsedId)
                {
                    // string id match
                    return &it->value;
                }
            }
        }
    }

    return NULL;
}

// deserialize std::vector<bool>
template <typename Protocols, typename A, typename T, typename Buffer>
inline void DeserializeContainer(std::vector<bool, A>& var, const T& /*element*/, SimpleJsonReader<Buffer>& reader)
{
    rapidjson::Value::ConstValueIterator it = reader.ArrayBegin();
    resize_list(var, reader.ArraySize());

    for (enumerator<std::vector<bool, A> > items(var); items.more(); ++it)
    {
        items.next() = it->IsTrue();
    }
}


// deserialize blob
template <typename Protocols, typename T, typename Buffer>
inline void DeserializeContainer(blob& var, const T& /*element*/, SimpleJsonReader<Buffer>& reader)
{
    if (uint32_t size = reader.ArraySize())
    {
        boost::shared_ptr<char[]> buffer = boost::make_shared_noinit<char[]>(size);
        uint32_t i = 0;

        for (rapidjson::Value::ConstValueIterator it = reader.ArrayBegin(), end = reader.ArrayEnd(); it != end && i < size; ++it)
            if (it->IsInt())
                buffer[i++] = static_cast<blob::value_type>(it->GetInt());

        var.assign(buffer, i);
    }
    else
        var.clear();
}


// deserialize list
template <typename Protocols, typename X, typename T, typename Buffer>
inline typename boost::enable_if<is_list_container<X> >::type
DeserializeContainer(X& var, const T& element, SimpleJsonReader<Buffer>& reader)
{
    detail::JsonTypeMatching type(get_type_id<typename element_type<X>::type>::value,
                                  GetTypeId(element),
                                  std::is_enum<typename element_type<X>::type>::value);

    rapidjson::Value::ConstValueIterator it = reader.ArrayBegin();
    resize_list(var, reader.ArraySize());

    for (enumerator<X> items(var); items.more(); ++it)
    {
        if (type.ComplexTypeMatch(*it))
        {
            SimpleJsonReader<Buffer> input(reader, *it);
            DeserializeElement<Protocols>(var, items.next(), detail::MakeValue(input, element));
        }
        else if (type.BasicTypeMatch(*it))
        {
            SimpleJsonReader<Buffer> input(reader, *it);
            DeserializeElement<Protocols>(var, items.next(), value<typename element_type<X>::type, SimpleJsonReader<Buffer>&>(input));
        }
        else
        {
            items.next();
        }
    }
}


// deserialize set
template <typename Protocols, typename X, typename T, typename Buffer>
inline typename boost::enable_if<is_set_container<X> >::type
DeserializeContainer(X& var, const T& element, SimpleJsonReader<Buffer>& reader)
{
    detail::JsonTypeMatching type(get_type_id<typename element_type<X>::type>::value,
                                  GetTypeId(element),
                                  std::is_enum<typename element_type<X>::type>::value);
    clear_set(var);

    typename element_type<X>::type e(make_element(var));

    for (rapidjson::Value::ConstValueIterator it = reader.ArrayBegin(), end = reader.ArrayEnd(); it != end; ++it)
    {
        if (type.BasicTypeMatch(*it))
        {
            detail::Read(*it, e);
            set_insert(var, e);
        }
    }
}

// Helper function to convert string keys back to their original types
template <typename Key>
bool ConvertStringToKey(const char* str, Key& key)
{
    return ConvertStringToKeyImpl(str, key, typename is_string<Key>::type());
}

template <typename Key>
bool ConvertStringToKeyImpl(const char* str, Key& key, std::true_type)
{
    // Key is already a string type
    key = str;
    return true;
}

template <typename Key>
bool ConvertStringToKeyImpl(const char* str, Key& key, std::false_type)
{
    // Key needs to be converted from string
    std::istringstream iss(str);
    return !(iss >> key).fail();
}


// deserialize map
template <typename Protocols, typename X, typename T, typename Buffer>
inline typename boost::enable_if<is_map_container<X> >::type
DeserializeMap(X& var, BondDataType keyType, const T& element, SimpleJsonReader<Buffer>& reader)
{
    detail::JsonTypeMatching value_type(
        get_type_id<typename element_type<X>::type::second_type>::value,
        GetTypeId(element),
        std::is_enum<typename element_type<X>::type::second_type>::value);

    clear_map(var);

    typename element_type<X>::type::first_type key(make_key(var));

    // Iterate over JSON object members
    for (rapidjson::Value::ConstMemberIterator it = reader.ObjectBegin(), end = reader.ObjectEnd(); it != end; ++it)
    {
        // Convert string key back to the appropriate key type
        if (!ConvertStringToKey(it->name.GetString(), key))
        {
            bond::InvalidKeyTypeException();
        }

        SimpleJsonReader<Buffer> input(reader, it->value);

        if (value_type.ComplexTypeMatch(it->value))
            detail::MakeValue(input, element).template Deserialize<Protocols>(mapped_at(var, key));
        else
            value<typename element_type<X>::type::second_type, SimpleJsonReader<Buffer>&>(input).Deserialize(mapped_at(var, key));
    }
}

}
