#pragma once

#include <windows.h>
#include <algorithm>
#include <unordered_map>
#include <strsafe.h>
#include <string>
#include <memory>
#include <fstream>

#include "yagent.string.h"

#define SETTING_STRING          0
#define SETTING_INTEGER         1
#define SETTING_PAIR            2
#define SETTING_INTEGER_PAIR    3
#define SETTING_SCALABLE_INTEGER_PAIR   4

namespace Setting {
    enum Type
    {
        STRING,
        INTEGER,
        PAIR,
        INTEGER_PAIR,          //  Integer Pair
        SCALABLE_INTEGER_PAIR
    };
    typedef struct IntegerPair
    {
        LONG    x;
        LONG    y;
    } IntegerPair;
    typedef struct ScalableIntegerPair
    {
        union {
            IntegerPair     pair;
            struct {
                LONG        x;
                LONG        y;
            };
        };
        ULONG               scale;
    } ScalableIntegerPair;
    typedef struct Rect
    {
        union {
            IntegerPair     position;
            struct {
                LONG        left;
                LONG        top;
            };
        };
        union {
            IntegerPair     size;
            struct {
                LONG        width;
                LONG        height;
            };
        };
    } Rect;

    typedef struct Color
    {
        COLORREF text;
        COLORREF focus;
        COLORREF selection;
        // Add new fields here.
    } Color;


};
class CSetting
{
public:
    CSetting() 
    {
        type        = Setting::Type::INTEGER;
        ivalue      = 0;
        ipair.x     = 0;
        ipair.y     = 0;
        ZeroMemory(&spair, sizeof(spair));
    }
    ~CSetting() {
    }
    Setting::Type           type;
    CWSTR                   name;
    CWSTR                   ovalue;
    int64_t                 ivalue;
    CWSTR                   svalue;
    Setting::IntegerPair    ipair;
    Setting::ScalableIntegerPair    spair;

};
typedef std::shared_ptr<CSetting>   CSettingPtr;


#define ADD_SETTING_WRAPPER(Type, Name, DefaultValue) \
{ \
    static PH_STRINGREF name = PH_STRINGREF_INIT(Name); \
    static PH_STRINGREF defaultValue = PH_STRINGREF_INIT(DefaultValue); \
    PhAddSetting(Type, &name, &defaultValue); \
}

#define SETTINGS_FILE       L"settings"

class CSettings
    :
    virtual public  CAppLog
{
public:
	CSettings() 
        :
        CAppLog(L"settings.log")
    {
	    Log(NULL);
        Load(SETTINGS_FILE);
	}
	~CSettings() {
	    //Save(SETTINGS_FILE);
	}
    int64_t        GetInteger(PCWSTR pName)
    {
        auto    t   = m_table.find(pName);
        if( m_table.end() != t && Setting::INTEGER == t->second->type )
            return t->second->ivalue;
        return 0;
    }
    Setting::IntegerPair    GetIntegerPair(PCWSTR pName)
    {
        Setting::IntegerPair    value   = {0,0};
        auto    t   = m_table.find(pName);
        if( m_table.end() != t && Setting::INTEGER_PAIR == t->second->type ) {
            return t->second->ipair;
        }
        return value;
    }
    Setting::ScalableIntegerPair    GetScalableIntegerPair(PCWSTR pName)
    {
        Setting::ScalableIntegerPair    value   = {0,0};
        auto    t   = m_table.find(pName);
        if( m_table.end() != t && Setting::SCALABLE_INTEGER_PAIR == t->second->type ) {
            value   = t->second->spair;

            ULONG currentScale  = GetCurrentScale();
            if (value.scale != currentScale && value.scale != 0)
            {
                value.x = MultiplyDivideSigned(value.x, currentScale, value.scale);
                value.y = MultiplyDivideSigned(value.y, currentScale, value.scale);
                value.scale = currentScale;
            }
        }
        return value;
    }
    ULONG MultiplyDivide(
        _In_ ULONG Number,
        _In_ ULONG Numerator,
        _In_ ULONG Denominator
    )
    {
        return (ULONG)(((ULONG64)Number * (ULONG64)Numerator + Denominator / 2) / (ULONG64)Denominator);
    }
    LONG MultiplyDivideSigned(
        _In_ LONG Number,
        _In_ ULONG Numerator,
        _In_ ULONG Denominator
    )
    {
        if (Number >= 0)
            return MultiplyDivide(Number, Numerator, Denominator);
        else
            return -(LONG)MultiplyDivide(-Number, Numerator, Denominator);
    }
    bool        Load(PCWSTR pFileName)
    {
        bool    bParsed = false;
        std::string     str, buf;
        std::ifstream   file(__utf8(pFileName));
        
        while( std::getline(file, buf))
            str += buf;
        file.seekg(0, std::ios::end);
        if( 0 == file.tellg() ) {
            //  파일 내용 없음. 
            Log("%-32s no settings", __FUNCTION__);
        }
        else {
            try {
                Json::CharReaderBuilder builder;
                Json::CharReader        *reader = NULL;

                Json::Value     doc;
                Json::String    err;

                reader  = builder.newCharReader();
                if( reader->parse(str.c_str(), str.c_str() + str.size(), &doc, &err) ) {
                    CSettingPtr setting;
                    for( auto t : doc ) {
                        setting = std::make_shared<CSetting>();

                        setting->type   = (Setting::Type)t["type"].asInt();
                        setting->name.Set(__utf16(t["name"].asCString()));
                        setting->ovalue.Set(__utf16(t["default"].asCString()));

                        switch( t["type"].asInt() ) {
                            case Setting::INTEGER:
                                setting->ivalue = t["value"].asInt64();
                            break;

                            case Setting::STRING:
                                setting->svalue.Set(__utf16(t["value"].asCString()));
                            break;

                            case Setting::INTEGER_PAIR:
                                setting->ipair.x    = t["value"]["x"].asInt();
                                setting->ipair.y    = t["value"]["y"].asInt();
                                break;

                            case Setting::SCALABLE_INTEGER_PAIR:
                                setting->spair.x    = t["value"]["x"].asInt();
                                setting->spair.y    = t["value"]["y"].asInt();
                                setting->spair.scale= t["value"]["scale"].asInt();
                                break;                        
                        }  
                        Add(setting);
                    }
                    Log("%-32s %d loaded.", __FUNCTION__, (int)m_table.size());
                    bParsed = true;
                }
                else {
                    Log("%-32s not parse settings", __FUNCTION__);
                
                }
            }
            catch( std::exception & e) {
                Log("%-32s %s", __FUNCTION__, e.what());
            }
        }
        file.close();
        return bParsed;
    }
    bool        Save(PCWSTR pFileName) 
    {
        Json::Value doc;

        for( auto t : m_table ) {
            Json::Value row;
            
            row["type"]     = t.second->type;
            row["name"]     = __utf8(t.first);
            row["default"]   = __utf8(t.second->ovalue);
            switch( t.second->type) {
                case Setting::Type::STRING:
                    row["value"]   = __utf8(t.second->svalue);
                    break;
                case Setting::Type::INTEGER:
                    row["value"]  = t.second->ivalue;
                    break;
                case Setting::Type::INTEGER_PAIR:
                    row["value"]["x"]        = (int)t.second->ipair.x;
                    row["value"]["y"]        = (int)t.second->ipair.y;
                    break;
                case Setting::Type::SCALABLE_INTEGER_PAIR:
                    row["value"]["x"]        = (int)t.second->spair.x;
                    row["value"]["y"]        = (int)t.second->spair.y;
                    row["value"]["scale"]    = (int)t.second->spair.scale;
                    break;                    
            }
            doc[__utf8(t.first)]    = row;
            row.clear();
        }
        std::string     str;
        Json::StreamWriterBuilder   builder;
        builder["indentation"]  = "  ";
        str = Json::writeString(builder, doc);

        std::ofstream   file(__utf8(pFileName));
        file << str;
        file.close();

        Log("%-32s %d saved.", __FUNCTION__, (int)m_table.size());
        return true;
    }
    void        Add(CSettingPtr setting) {
        auto    t   = m_table.find((PCWSTR)setting->name);
        if( m_table.end() == t )
            m_table.insert(make_pair((PCWSTR)setting->name, setting));
        else
            m_table[(PCWSTR)setting->name]  = setting;
    }
    void        Add(
        Setting::Type   type,
        PCWSTR          pName,
        PCWSTR          pDefaultValue
    ) {
        try {
            CSettingPtr     setting   = std::make_shared<CSetting>();

            setting->type            = type;
            setting->name            = pName;
            setting->ovalue          = pDefaultValue;

            if( SettingFromString(type, pDefaultValue, NULL, setting) )
                Add(setting);
            
        }
        catch( std::exception & e) {
            UNREFERENCED_PARAMETER(e);
        }    
    }
    bool StringToInteger64(
        _In_    PCWSTR      String,
        _In_    ULONG       Base,
        _Out_   PULONG64    Integer
    )
    {
        BOOLEAN valid = TRUE;
        ULONG64 result;
        SIZE_T length;
        SIZE_T i;

        length = wcslen(String);
        result = 0;

        for (i = 0; i < length; i++)
        {
            ULONG value;

            value = PhCharToInteger((UCHAR)String[i]);
            if (value < Base)
                result = result * Base + value;
            else
                valid = FALSE;
        }

        *Integer = result;

        return valid;
    }

    ULONG PhCharToInteger(ULONG n)
    {
        static  short   nzTable[256] =
        {
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 0 - 15 */
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 16 - 31 */
            36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, /* ' ' - '/' */
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, /* '0' - '9' */
            52, 53, 54, 55, 56, 57, 58, /* ':' - '@' */
            10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, /* 'A' - 'Z' */
            59, 60, 61, 62, 63, 64, /* '[' - '`' */
            10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, /* 'a' - 'z' */
            65, 66, 67, 68, -1, /* '{' - 127 */
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 128 - 143 */
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 144 - 159 */
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 160 - 175 */
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 176 - 191 */
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 192 - 207 */
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 208 - 223 */
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 224 - 239 */
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 /* 240 - 255 */
        };
        if( n>= 0 && n < _countof(nzTable)) {
            return nzTable[n];
        }
        return 0;
    };
    BOOLEAN StringToInteger64(
        _In_    PCWSTR      String,
        _In_    ULONG       nLength,
        _In_    ULONG       Base,
        _Out_   PULONG64    Integer
    )
    {
        BOOLEAN valid = TRUE;
        ULONG64 result;
        SIZE_T length;
        SIZE_T i;

        length = nLength;
        result = 0;

        for (i = 0; i < length; i++)
        {
            ULONG value;

            value = PhCharToInteger((UCHAR)String[i]);

            if (value < Base)
                result = result * Base + value;
            else
                valid = FALSE;
        }
        *Integer = result;
        return valid;
    }
    _Success_(return)   bool    StringToInteger64(
            _In_        PCWSTR  pString,
            _In_opt_    ULONG   nBase,
            _Out_opt_   PLONG64 pInteger
        )
    {
        ULONG       base;
        bool        bValid          = false;
        PCWSTR      pOriginString   = pString;
        bool        bNegative       = false;
        ULONG64     nResult         = 0;

        if (nBase > 69)
            return false;

        size_t  nLength = (pString && *pString)? wcslen(pString) : 0;

        if (nLength && (*pString == '-' || *pString == '+'))
        {
            if (*pString == '-')
                bNegative = true;
            pString++, nLength--;
        }

        // If the caller specified a base, don't perform any additional processing.

        if (nBase)
        {
            base = nBase;
        }
        else
        {
            nBase = 10;
            if (nLength >= 2 && *pString == '0')
            {
                switch (*(pString + 1))
                {
                case 'x':
                case 'X':
                    nBase = 16;
                    break;
                case 'o':
                case 'O':
                    nBase = 8;
                    break;
                case 'b':
                case 'B':
                    nBase = 2;
                    break;
                case 't': // ternary
                case 'T':
                    nBase = 3;
                    break;
                case 'q': // quaternary
                case 'Q':
                    nBase = 4;
                    break;
                case 'w': // base 12
                case 'W':
                    nBase = 12;
                    break;
                case 'r': // base 32
                case 'R':
                    nBase = 32;
                    break;
                }
                if (nBase != 10) 
                    pString += 2, nLength -= 2;
            }
        }
        bValid = StringToInteger64(pString, (ULONG)nLength, nBase, &nResult);

        if (pInteger)
            *pInteger = bNegative ? -(LONG64)nResult : nResult;
        return bValid;
    }
    static ULONG GetCurrentScale()
    {
        static ULONG dpi = 96;

        {
            HDC hdc;
            if (hdc = GetDC(NULL))
            {
                dpi = GetDeviceCaps(hdc, LOGPIXELSY);
                ReleaseDC(NULL, hdc);
            }
        }
        return dpi;
    }

    bool    GetIntegerPair(PCWSTR pStr, LONG * px, LONG * py) {
        std::wstring        str = pStr, token;
        std::wstringstream  wss(str);
        LONG64              value;
        int                 i;
        for( i = 0 ; std::getline(wss, token, L',') && i < 2 ; i++ ) {
            if( StringToInteger64(token.c_str(), 10, &value) ) {
                if( 0 == i )
                    *px = (LONG)value;
                else if( 1 == i ) {
                    *py = (LONG)value;
                }
            }                    
        }
        return ( 2 == i);
    }
    bool    SettingFromString(
        _In_        Setting::Type       type,
        _In_        PCWSTR              pDefaultString,
        _In_opt_    PCWSTR              pString,
        _Inout_     CSettingPtr         setting
    )
    {
        if( NULL == pDefaultString || NULL == *pDefaultString ) return false;

        switch (type)
        {
            case Setting::SCALABLE_INTEGER_PAIR:
                if( L'@' == *pDefaultString ) {
                    std::wstring        str     = pDefaultString + 1, token;
                    std::wstringstream  wss(str);
                    for( int i = 0 ; std::getline(wss, token, L'|') && i < 2 ; i++ ) {
                        if( 0 == i ) {
                            LONG64  scale;
                            if( StringToInteger64(token.c_str(), 10, &scale) ) {
                                setting->spair.scale    = (LONG)scale;
                            }
                        }
                        else {
                            if( GetIntegerPair(token.c_str(), &setting->spair.x, &setting->spair.y) ) {
                                //Log("%-32ws %ws %d %d %d", (PCWSTR)setting->name, 
                                //   L"SPAIR", setting->spair.scale, setting->spair.x, setting->spair.y);
                                return true;
                            }
                        }
                    }
                }
                else {
                    setting->spair.scale    = GetCurrentScale();
                }
                break;

            case Setting::INTEGER_PAIR:
                {
                    std::wstring    str = pDefaultString, token;
                    std::wstringstream  wss(str);
                    if( GetIntegerPair(pDefaultString, &setting->ipair.x, &setting->ipair.y) ) {
                        //Log("%-32ws %-10ws %d %d", 
                        //    (PCWSTR)setting->name, L"IPAIR", setting->ipair.x, setting->ipair.y);
                        return true;
                    }
                }
                break;

            case Setting::INTEGER:
                {
                    if( StringToInteger64(pDefaultString, 16, &setting->ivalue)) {
                        //Log("%-32ws %-10ws %016x", 
                        //(PCWSTR)setting->name, L"INTEGER", setting->ivalue);
                        return true;
                    }
                }
                break;


            case Setting::STRING:
                if (pString)
                {
                    //PhSetReference(&Setting->u.Pointer, String);
                }
                else
                {
                    setting->svalue = pDefaultString;
                    //Log("%-32ws %-10ws %ws", 
                    //   (PCWSTR)setting->name, L"STRING", (PCWSTR)setting->svalue);
                    return true;
                }
        }
        return false;
    }
    VOID    CenterRectangle(
        _Inout_ Setting::Rect   *Rectangle,
        _In_    Setting::Rect   *Bounds
    )
    {
        Rectangle->left = Bounds->left + (Bounds->width - Rectangle->width) / 2;
        Rectangle->top = Bounds->top + (Bounds->height - Rectangle->height) / 2;
    }
    RECT    ToRect(Setting::Rect r)
    {
        RECT    rect    = {r.left, r.top, r.left + r.width, r.top + r.height};
        return rect;
    }
    Setting::Rect   ToRect(RECT r)
    {
        Setting::Rect   rect    = {r.left, r.top, r.right-r.left, r.bottom-r.top};
        return rect;
    }
    VOID    AdjustRectToBounds(
        _Inout_ Setting::Rect   *Rectangle,
        _In_    Setting::Rect   *Bounds
    )
    {
        if (Rectangle->left + Rectangle->width > Bounds->left + Bounds->width)
            Rectangle->left = Bounds->left + Bounds->width - Rectangle->width;
        if (Rectangle->top + Rectangle->height > Bounds->top + Bounds->height)
            Rectangle->top = Bounds->top + Bounds->height - Rectangle->height;

        if (Rectangle->left < Bounds->left)
            Rectangle->left = Bounds->left;
        if (Rectangle->top < Bounds->top)
            Rectangle->top = Bounds->top;
    }
    VOID    AdjustRectToWorkingArea(
        _In_opt_    HWND            hWnd,
        _Inout_     Setting::Rect   *Rectangle
    )
    {
        HMONITOR monitor;
        MONITORINFO monitorInfo = { sizeof(monitorInfo) };

        if (hWnd)
        {
            monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
        }
        else
        {
            RECT rect;

            rect    = ToRect(*Rectangle);
            monitor = MonitorFromRect(&rect, MONITOR_DEFAULTTONEAREST);
        }

        if (GetMonitorInfo(monitor, &monitorInfo))
        {
            Setting::Rect   bounds;

            bounds = ToRect(monitorInfo.rcWork);
            AdjustRectToBounds(Rectangle, &bounds);
        }
    }
private:
    std::unordered_map<std::wstring, CSettingPtr>   m_table;
};
