#ifndef OBJECT_H
#define OBJECT_H

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// #include <experimental/source_location>

using namespace std;
// using namespace experimental;

namespace des
{
	class object;
}

class des::object{
    public:
        /**
         * @brief Prints location and message
         * 
         * @param message 
         * @param location 
         */
        // inline void log(const string& message, 
        //         const source_location& location = source_location::current())
        // {
        //     cout << "info:"
        //         << location.file_name() << ":"
        //         << location.line() << " "
        //         << location.function_name() << " "
        //         << message << '\n';         
        // }
        // template <typename ...Args>
        /**
         * @brief Prints location and arguments
         * 
         * @tparam Args 
         * @param location 
         * @param args 
         */
        // inline void TraceLoc(const source_location& location, Args&& ...args)
        // {
        //     std::ostringstream stream;
        //     stream << location.file_name() << "(" << location.line() << ") : ";
        //     (stream << ... << std::forward<Args>(args)) << '\n';

        //     std::cout << stream.str();
        // }
        /**
         * @brief Construct a new object object
         * 
         * @param i 
         */
        object(int i)
        {
            id = static_cast<unsigned int>(i);
        }
        /**
         * @brief Construct a new object object
         * 
         * @param i 
         */
        object(unsigned int i)
        {
            id = i;
        }
        /**
         * @brief Destroy the object object
         * 
         */
        virtual ~object(){}
        /**
         * @brief Set the id object
         * 
         * @param i 
         */
        inline void set_id(int i)
        {
            id = static_cast<unsigned int>(i);
        }
        /**
         * @brief Set the id object
         * 
         * @param i 
         */
        inline void set_id(unsigned int i)
        {
            id = i;
        }
        /**
         * @brief Get the id object
         * 
         * @return unsigned int 
         */
        inline unsigned int get_id() const
        {
            return id;
        }
        virtual string to_string() const = 0;
        /**
         * @brief Resets the state of the object to param
         * 
         * @param param 
         */
        virtual void reset(double param, vector<string> keys = vector<string>(), bool newrun = false)
        {}
        /**
         * @brief Resets the state of the object to param
         * 
         * @param param 
         */
        virtual void reset(vector<string> keys = vector<string>(), bool newrun = false)
        {}
        /**
         * @brief Resets the current state of the object to the initial state
         * 
         */
        virtual void clear() = 0;
    private:
        unsigned int id;
};
#endif