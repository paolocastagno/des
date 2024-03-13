#ifndef MESSAGE_H
#define MESSAGE_H

#include <iostream>
#include <string>
#include <utility>
#include <unordered_map>

#include "libdes_const.hpp"

using namespace std;

namespace des
{
	class message;
}

class des::message 
{
    public:
        /**
         * @brief Construct a new message object
         * 
         */
        message(){}
        /**
         * @brief Construct a new message object
         * 
         * @param kv 
         */
        message(unordered_map<string, double> kv)
        {
            key_value = kv;
        }
        message(string m)
        {
            deserialize(m);
        }
        /**
         * @brief Destroy the message object
         * 
         */
        ~message(){}
        /**
         * @brief add a key-value pair to the message 
         * 
         * @param key 
         * @param value 
         */
        inline void add(string key, double value)
        {
            key_value[key] = value;
        }
        /**
         * @brief add a key-value pair to the message, with a default value equal to zero 
         * 
         * @param key 
         * @param value 
         */
        inline void add(string key)
        {
            key_value[key] = 0;
        }
        /**
         * @brief Get the value object
         * 
         * @param key 
         * @return double 
         */
        inline double get_value(string key)
        {
            unordered_map<string,double>::iterator it = key_value.find(key);
            if(it != key_value.end())
            {
                return it -> second;
            }
            return 0;
        }
        /**
         * @brief set key_value object
         * 
         * @param um 
         */
        inline void set_keyvalue(unordered_map<string, double> um)
        {
            key_value = um;
        }
        /**
         * @brief Removes a key-value pair
         * 
         * @param key 
         */
        inline void remove(string key)
        {
            unordered_map<string, double>::iterator element = key_value.find(key);
            if(element != key_value.end())
            {
                key_value.erase(element);
            }
        }
        /**
         * @brief Serializes the message to string
         * 
         * @return string 
         */
        inline string serialize()
        {
            unordered_map<string, double>::iterator it = key_value.begin();
            string message = "";
            while(it != key_value.end())
            {
                message += it -> first + MESSAGE_KEYVALUE_SEPARATOR + to_string(it -> second) + MESSAGE_PAIR_SEPARATOR;
                ++it;
            }
            return message;
        }
        /**
         * @brief Deserializes  a message from string
         * 
         * @param msg 
         */
        inline void deserialize(string msg)
        {
            size_t kv_start = 0, kv_end = 0, kv_sep = 0,length = 0;
            key_value.clear();
			// Read pairs
			length = msg.length();
			do
			{
                string k;
                double v;
                kv_sep = msg.find(MESSAGE_KEYVALUE_SEPARATOR, kv_start);
                kv_end = msg.find(MESSAGE_PAIR_SEPARATOR, kv_start);
                if(kv_end == string::npos)
                {
                    kv_end = length;
                }
                // There is only the key
                if(kv_sep == string::npos)
                {
                    k = msg.substr(kv_start, kv_end-kv_start);
                    v = 0.0;
                }
                // There are both key and value
                else
                {
                    k = msg.substr(kv_start, kv_sep-kv_start);
                    v = stod(msg.substr(kv_sep+1, kv_end-kv_sep-1));
                }
                key_value.emplace(k,v);
                kv_start = kv_end+1;
			}
			while(kv_end != length);

        }
    protected:
        unordered_map<string, double> key_value;
};
#endif