#include "utils.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <iomanip>
#include <sys/stat.h>

#if defined(WIN32) 
#include <Windows.h>
#include <process.h>
#include <mmsystem.h>
#include <objbase.h>
#else
#include <dlfcn.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <limits.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <uuid/uuid.h>
#endif

#ifdef __APPLE__
#include <mach/mach_time.h>
#include <mach-o/dyld.h> 
#endif

int StrToInt( const char* str )
{
    int v = 0;
    if(str != NULL && strlen(str) > 0){
        sscanf(str, "%d", &v);
    }
    return v;
}

int64_t StrToInt64( const char* str )
{
    int64_t v = 0;
    if(str != NULL && strlen(str) > 0){
        sscanf(str, "%lld", &v);
    }
    return v;
}

std::string StrUpper( const string& text )
{
    string r;
    r.append(text);
    strupr((char*)r.c_str());
    return r;
}

std::string StrLower( const string& text )
{
    string r;
    r.append(text);
    strlwr((char*)r.c_str());
    return r;
}


#ifdef WIN32
///convert wide chars to multibytes
static char* wide_to_mtbytes(int c, const wchar_t* pWideText)
{
    assert(pWideText);

    int size = WideCharToMultiByte(c, 0, (LPCWSTR)pWideText, -1, NULL, 0, NULL, NULL);
    if (size == 0) {
        assert(false);
        return NULL;
    }
    char* pText = new char[size+1];
    if (WideCharToMultiByte(c, 0, (LPCWSTR)pWideText, -1, pText, size, NULL, NULL) == 0) {
        delete []pText;
        assert(false);
        return NULL;
    }
    pText[size] = '\0';
    return pText;
}

///convert multibytes to wide chars
static wchar_t* mtbytes_to_wide(int c, const char* pText)
{
    assert(pText);

    wchar_t* pWideText=NULL;
    int size = MultiByteToWideChar(c, 0, pText, -1, NULL, 0);
    if (size == 0) {
        assert(false);
        return pWideText;
    } else {
        pWideText = new wchar_t[size+1];
        if (MultiByteToWideChar(c, 0, pText, -1, (LPWSTR)pWideText, size) == 0) {
            delete []pWideText;
            pWideText = NULL;
            assert(false);
            return pWideText;
        } else {
            pWideText[size] = 0;
            return pWideText;
        }
    }
}

string utf8_to_local(const char* pText)
{
    assert(pText);

    wstring ws = utf8_to_wide(pText);
    char* pANSI = wide_to_mtbytes(CP_ACP, ws.c_str());
    if (pANSI == NULL) {        
        assert(false);
        return "";
    }

    string r = pANSI;
    delete []pANSI;
    return r;    
}

string local_to_utf8(const char* pText)
{
    assert(pText);

    wchar_t* pWideText = mtbytes_to_wide(CP_ACP, pText);
    if (pWideText == NULL) {
        assert(false);
        return "";
    }
    char* pUTF8 = wide_to_mtbytes(CP_UTF8, pWideText);
    if (pUTF8 == NULL) {
        assert(false);
        delete []pWideText;
        return "";
    }
    string r = pUTF8;
    delete []pUTF8;
    delete []pWideText;
    return r;
}

wstring utf8_to_wide(const char* pText)
{
    assert(pText);
    wchar_t* pWide = mtbytes_to_wide(CP_UTF8, pText);
    assert(pWide);
    wstring s = (wchar_t*)pWide;
    delete []pWide;
    return s;
}


string wide_to_utf8(const wchar_t* pText)
{
    assert(pText);
    char* pUTF8 = wide_to_mtbytes(CP_UTF8, pText);
    if (pUTF8 == NULL) {
        assert(false);
        return "";
    } else {
        string r = pUTF8;
        delete []pUTF8;
        return r;
    }
}

#else
// length of a utf8 (multibyte)char
// follows signature of mblen( const char *, size_t count ) but always behaves as if count==1
// does not check validity of trailing bytes (u[1]&0xc0)==0x80
int u8len( const char *u, size_t count )
{
    if( 0==count ) return 0;

    if( NULL==u ) return 0;
    else if( 0==*u ) return 0;
    else if( !(*u&~0x7f) ) return 1;
    else if( (*u&0xe0)==0xc0 ) return 2;
    else if( (*u&0xf0)==0xe0 ) return 3;
    else if( (*u&0xf8)==0xf0 ) return 4;
    else /* error */ return -1;
}

// convert utf8 (multibyte)char to wchar_t
// follows signature of mbtowc( wchar_t *, const char *, size_t count ) but always behaves as if count==1
int u8towc( wchar_t *w, const char *u, size_t count )
{
    /* assert */ if( NULL==w ) return -1;

    int len=u8len( u,1 );

    if( len<1 ) return len;
    else if( 1==len ) { w[0]=u[0]&0x7f; return len; }
    else if( 2==len ) { if( (u[1]&0xc0)!=0x80 ) /* error */ return -1;
    w[0]=((u[0]&0x1f)<<6)|(u[1]&0x3f);
    return 2; }
    else if( 3==len ) { if( (u[1]&0xc0)!=0x80 ) /* error */ return -1;
    if( (u[2]&0xc0)!=0x80 ) /* error */ return -1;
    w[0]=((u[0]&0x0f)<<12)|((u[1]&0x3f)<<6)|(u[2]&0x3f);
    return 3; }
    else if( 4==len ) { if( (u[1]&0xc0)!=0x80 ) /* error */ return -1;
    if( (u[2]&0xc0)!=0x80 ) /* error */ return -1;
    if( (u[3]&0xc0)!=0x80 ) /* error */ return -1;
    w[0]=((u[0]&0x07)<<18)|((u[1]&0x3f)<<12)|((u[2]&0x3f)<<6)|(u[3]&0x3f);
    return 4; }
    else /* error */ return -1;
}

// number of wchar_t required to represent a utf8 string
// follows signature of mbstowcs( NULL, char*, )
int u8swcslen( const char* pu )
{
    int len=0;
    char c;

    while( (c = *pu) )
    {
        if( !(c&0x80) ) { len++; pu+=1; }
        else if( (c&0xe0)==0xc0 ) { len++; pu+=2; }
        else if( (c&0xf0)==0xe0 ) { len++; pu+=3; }
        else if( (c&0xf8)==0xf0 ) { len++; pu+=4; }
        else /* error: add width of single byte character entity &#xFF; */ { len+=6; pu+=1; }
    }
    return len;
}

// convert a utf8 string to a wchar_t string
// follows signature of size_t mbstowcs( wchar_t *, const char *, size_t count )
size_t u8stowcs( wchar_t *pw, const char *pu, size_t count )
{
    size_t clen=0;

    if( NULL==pw ) return u8swcslen( pu );

    while( *pu && clen<count )
    {
        int ulen=u8towc( &pw[clen], pu, 1 );
        if( ulen<0 ) return (size_t)-1;
        else { clen++; pu+=ulen; }
    }
    if( '\0'==*pu && clen<count) pw[clen++]=L'\0';
    return clen;
}

// convert a wchar_t to a utf8 (multibyte)char
// follows signature of int wctomb( char *m, wchar_t w )
// requires m to point to a buffer of length 4 or more
int wctou8( char *m, wchar_t w )
{
    /* Unicode Table 3-5. UTF-8 Bit Distribution
    Unicode                     1st Byte 2nd Byte 3rd Byte 4th Byte
    00000000 0xxxxxxx           0xxxxxxx
    00000yyy yyxxxxxx           110yyyyy 10xxxxxx
    zzzzyyyy yyxxxxxx           1110zzzz 10yyyyyy 10xxxxxx
    000uuuuu zzzzyyyy yyxxxxxx  11110uuu 10uuzzzz 10yyyyyy 10xxxxxx
    */

    if( !(w&~0x7f) ) { m[0]=w&0x7f;
    m[1]='\0';
    return 1; }
    else if( !(w&~0x7ff) ) { m[0]=((w>>6)&0x1f)|0xc0;
    m[1]=(w&0x3f)|0x80;
    m[2]='\0';
    return 2; }
    else if( !(w&~0xffff) ) { m[0]=((w>>12)&0x0f)|0xe0;
    m[1]=((w>>6)&0x3f)|0x80;
    m[2]=(w&0x3f)|0x80;
    m[3]='\0';
    return 3; }
    else if( !(w&~0x1fffff) ) { m[0]=((w>>18)&0x07)|0xf0;
    m[1]=((w>>12)&0x3f)|0x80;
    m[2]=((w>>6)&0x3f)|0x80;
    m[3]=(w&0x3f)|0x80;
    m[4]='\0';
    return 4; }
    else return -1;
}
// number of char required to represent a wchar_t string in utf8
// follows signature of wcslen( wchar_t* ) or wcstombs( NULL, wchar_t*, )
int wcsu8slen( const wchar_t *pw )
{
    int len=0;
    wchar_t w;

    while( (w = *pw++) )
    {
        if( !(w&~0x7f) ) len+=1;
        else if( !(w&~0x7ff) ) len+=2;
        else if( !(w&~0xffff) ) len+=3;
        else if( !(w&~0x1fffff) ) len+=4;
        else /* error: add width of null character entity &#x00; */ len+=6;
    }
    return len;
}
// number of char required to represent a single wchar_t in utf8
// follows signature of mblen( const wchar_t )
int wcu8len( const wchar_t w )
{
    if( !(w&~0x7f) ) return 1;
    if( !(w&~0x7ff) ) return 2;
    if( !(w&~0xffff) ) return 3;
    if( !(w&~0x1fffff) ) return 4;
    return -1; /* error */
}
// convert a wchar_t string to utf8 string
// follows signature of size_t wcstombs( char *u, const wchar_t *w, size_t count )
size_t wcstou8s( char *pu, const wchar_t *pw, size_t count )
{
    int len=wcsu8slen( pw );

    if( NULL==pu ) return (size_t)len;

    size_t clen=0;
    wchar_t w;
    while( (w = *pw++) )
    {
        int ulen=wcu8len(w);

        if( ulen>=0 )
        {
            if( (clen+wcu8len(w))<=count ) { clen+=wctou8( pu, w ); pu+=ulen; }
            else break;
        }
        else
        {
            // untranslatable character so insert null character entity &#x00;
            if( (clen+6)<=count )
            {
                *pu++='&'; *pu++='#'; *pu++='x';
                *pu++='0'; *pu++='0';
                *pu++=';';
            }
            else break;
        }
    }

    return (size_t)clen;
}

wstring utf8_to_wide(const char* pText)
{
    uint32_t buf_size = u8stowcs(NULL, pText, 0);
    wchar_t *wchar_buf = new wchar_t[buf_size + 1];
    u8stowcs(wchar_buf, pText, buf_size);
    wchar_buf[buf_size] = '\0';
    wstring ret = wchar_buf;
    delete []wchar_buf;
    return ret;
}

string wide_to_utf8(const wchar_t* pText)
{
    uint32_t size = wcstou8s(NULL, pText, 0);
    char *buf = new char[size + 1];
    wcstou8s(buf, pText, size);
    buf[size] = '\0';
    string ret = buf;
    delete []buf;
    return ret;    
}


vector<std::string> StrSplit(const std::string &src, const std::string &split)
{
    vector<string> val;
    string cs = src;
    int pos = StrPos(cs, 0, split);
    while(pos >= 0){
        if(pos > 0){
            string v;
            v.insert(v.begin(), cs.begin(), cs.begin()+pos);
            val.push_back(v);
        }
        size_t ps = (size_t)pos+split.size();
        if(ps >= cs.size()){
            cs.clear();
        }else{
            cs = cs.substr(ps, cs.size()-ps);
        }
        pos = StrPos(cs, 0, split);
    }
    if(cs.size() > 0){
        val.push_back(cs);
    }
    return val;
}

#endif
