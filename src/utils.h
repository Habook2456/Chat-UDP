#ifndef _UTILS_H_
#define _UTILS_H_

#include <iostream>
#include <string>
using std::string;

string complete_digits(int t, int bytesNum)
{
    string result = std::to_string(t);

    if (result.length() < bytesNum)
    {
        result = string(bytesNum - result.length(), '0') + result;
    }

    return result;
}

#endif