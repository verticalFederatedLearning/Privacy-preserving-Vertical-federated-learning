#ifndef SERIALIZATION_H
#define SERIALIZATION_H
#include "jsonParser.h"
#include "type_traits"

#include "../../bloom/bloom_filter.hpp"
class serialize
{
private:
    /* data */
public:
    serialize(/* args */);
    template <typename T>
    struct isVector
    {
        static constexpr auto value=false;
    };
    template <typename T>
    struct  isVector<std::vector<T>>
    {
        static constexpr auto value=true;
    };
    template <typename T>
    struct  isMap
    {
        static constexpr auto value=false;
    };
    template <typename U,typename V>
    struct  isMap<std::map<U,V>>
    {
        static constexpr auto value=true;
    };
    template <typename T>
    struct  isPair
    {
        static constexpr auto value=false;
    };
    template <typename U,typename V>
    struct  isPair<std::pair<U,V>>
    {
        static constexpr auto value=true;
    }; 
    template <typename T>
    struct  isInteger
    {
        using rT=typename std::remove_const<T>::type;
        static constexpr auto value=std::__is_integer<rT>::__value;
    };
    
    template <typename T>
    struct isString
    {
        private:
            template <typename U,typename=decltype(std::string(U()))>
            static char _isString(void*);
            template <typename>
            static int _isString(...);
        public:
            static constexpr auto value=std::is_same<decltype(_isString<T>(nullptr)),char>::value&&!isInteger<T>::value;
    };
    template <typename T>
    static JsonParser doSerialize(T data){   
        if constexpr(isString<T>::value)
        {
            return JsonParser(std::make_shared<std::string>(std::string(data)));
        }  
        else if constexpr(std::is_same<T,bloom_filter>::value)
        {
            return (JsonParser)data;
        }
        else if constexpr(isInteger<T>::value)
        {
            return JsonParser(new std::string(std::to_string(data)),JsonParser::INT);
        }
        else if constexpr(isVector<T>::value)
        {
            std::string arr="[]";
            JsonParser jsonArray(&arr,JsonParser::ARRAY);
            for (auto &&each:data)
                jsonArray.add(doSerialize(each));
            return jsonArray;
        }
        else if constexpr(isMap<T>::value)
        {
            static_assert(isString<typename T::value_type::first_type>::value,"key should be string");
            JsonParser result;
            for (auto &&[u,v]:data)
            {
                result[std::string(u)]=doSerialize(v);
            }
            return result;
        }
        else if constexpr(isPair<T>::value)
        {
            auto &&[u,v]=data;
            std::string arr="[]";
            JsonParser jsonArray(&arr,JsonParser::ARRAY);
            jsonArray.add(doSerialize(u));
            jsonArray.add(doSerialize(v));
            return jsonArray;
        }
        else
            throw std::invalid_argument("unsupported type");
    }
    template <typename T> 
    static T doUnSerialize(JsonParser data)
    {
        if constexpr(isString<T>::value)
        {
            return data.toString();
        }
        else if constexpr(std::is_same<T,bloom_filter>::value)
        {
            return  bloom_filter(data);
        }
        else if constexpr(isInteger<T>::value)
        {
            return data.toInt();
        }
        else if constexpr(isVector<T>::value)
        {
            T result;
            data.foreach([&](JsonParser& json){ 
                result.push_back(doUnSerialize<typename T::value_type>(json));
            });
            return result;
        }
        else if constexpr(isMap<T>::value)
        {
            T result;
            for (auto &&[u,v]:data.getIterator())
            {
                result[u]=doUnSerialize<typename T::value_type::second_type>(v);
            }
            return result;
        }
        else if constexpr(isPair<T>::value)
        {
            std::vector<JsonParser> result;
            data.foreach([&](JsonParser &json){
                result.push_back(json);
            });
            return std::make_pair(doUnSerialize<typename T::first_type>(result[0]),doUnSerialize<typename T::second_type>(result[1]));
        }
        else
            throw std::invalid_argument("unsupported type");
    }
    ~serialize();
};


#endif