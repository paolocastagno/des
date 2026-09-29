#ifndef MESSAGE_H
#define MESSAGE_H

#include <iostream>
#include <string>
#include <utility>

#include "libdes_const.hpp"
#include "libdes_tag.hpp"

using namespace std;

namespace des
{
	class message;
}

/**
 * @brief Fields delivered to observers, addressed by des::tag.
 */
class des::message
{
    public:
        /**
         * @brief Construct an empty message
         *
         */
        message(){}
        /**
         * @brief Construct a message reading the fields of @p s without copying them
         *
         * @p s must outlive the message. Copies of the message own their data, and adding or
         * removing a field makes the message take its own copy first.
         *
         * @param s
         */
        static message view_of(const tag_store& s)
        {
            // Returning a prvalue guarantees no copy, which would own the data
            return message(&s);
        }
        message(const message& other) : fields(other.data()), view(nullptr)
        {}
        message& operator=(const message& other)
        {
            if(this != &other)
            {
                fields = other.data();
                view = nullptr;
            }
            return *this;
        }
        /**
         * @brief Destroy the message object
         *
         */
        ~message(){}
        /**
         * @brief add a field to the message, replacing any previous value
         *
         * @param t
         * @param value
         */
        inline void add(tag t, double value)
        {
            own();
            fields.set(t, value);
        }
        /**
         * @brief Returns the value of field @p t, or 0 if the message does not hold it
         *
         * @param t
         * @return double
         */
        inline double get_value(tag t) const
        {
            return data().value(t);
        }
        /**
         * @brief Tells whether the message holds field @p t
         */
        inline bool has(tag t) const
        {
            return data().has(t);
        }
        /**
         * @brief Removes field @p t
         *
         * @param t
         */
        inline void remove(tag t)
        {
            own();
            fields.remove(t);
        }
        /**
         * @brief Serializes the message to string, e.g. for logging: fields are written as
         * name MESSAGE_KEYVALUE_SEPARATOR value MESSAGE_PAIR_SEPARATOR, named after des::tag_registry
         *
         * @return string
         */
        inline string serialize() const
        {
            string message = "";
            data().for_each([&message](tag t, double v)
            {
                message += tag_registry::name(t) + MESSAGE_KEYVALUE_SEPARATOR + to_string(v) + MESSAGE_PAIR_SEPARATOR;
            });
            return message;
        }
    protected:
        /**
         * @brief The fields the message reads: the viewed store, if any, or its own
         *
         */
        inline const tag_store& data() const
        {
            return view != nullptr ? *view : fields;
        }
        /**
         * @brief Replace the view, if any, with an owned copy of the viewed fields
         *
         */
        inline void own()
        {
            if(view != nullptr)
            {
                fields = *view;
                view = nullptr;
            }
        }
        tag_store fields;
        /**
         * @brief Store read without copying when the message was built by view_of(), nullptr otherwise
         *
         */
        const tag_store* view = nullptr;
    private:
        explicit message(const tag_store* s) : fields(), view(s)
        {}
};
#endif
