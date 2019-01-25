#ifndef __UTILS_H__
#define __UTILS_H__
#include <string>
#include <vector>

using std::string;
using std::wstring;
using std::vector;

typedef unsigned char uint8_t;
typedef signed char int8_t;

typedef unsigned short uint16_t;
typedef signed short int16_t;

typedef unsigned int uint32_t;
typedef signed int int32_t;

typedef unsigned long long uint64_t;
typedef signed long long int64_t;
typedef int64_t i64;

int StrToInt(const char* str);
int64_t StrToInt64(const char* str);

string StrUpper(const string& text);
string StrLower(const string& text);

int StrPos(const string& text, unsigned int start, const string& needle);
vector<string> StrSplit(const string& src, const string& split);


	/*!
    将UTF8字符串转换成本地字符串
    @param pText UTF8字符串
    @return 本地字符串
    */
    string utf8_to_local(const char* pText);

    /*!
    @param pText 本地字符串
    @return UTF8字符串
    */
    string local_to_utf8(const char* pText);

    /*!
    将UTF8字符串转换成UNICODE字符串
    @param pText UTF8字符串
    @return UNICODE字符串
    */
    wstring utf8_to_wide(const char* pText);

    /*!
    @param pText UNICODE字符串
    @return UTF8字符串
    */
    string wide_to_utf8(const wchar_t* pText);

    string TrimLeft(const string& text, const string& chars);

    string TrimRight(const string& text, const string& chars);

    string Trim(const string& text, const string& chars);

#endif
