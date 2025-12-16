module;

#include <Windows.h>

export module polymer.core;

import std;

namespace polymer {

    export std::wstring to_wstring(std::string_view str) {
        if (str.empty()) {
            return {};
        }

        int size{
            MultiByteToWideChar(
                CP_UTF8,
                MB_ERR_INVALID_CHARS,
                str.data(),
                static_cast<int>(str.size()),
                nullptr,
                0
            )
        };
        if (size == 0) {
            return {};
        }

        std::wstring result(size, '\0');
        MultiByteToWideChar(
            CP_UTF8,
            0,
            str.data(),
            static_cast<int>(str.size()),
            result.data(),
            size
        );
        return result;
    }

    export std::string to_string(std::wstring_view str) {
        if (str.empty()) {
            return {};
        }

        int size{
            WideCharToMultiByte(
                CP_UTF8,
                WC_ERR_INVALID_CHARS,
                str.data(),
                static_cast<int>(str.size()),
                nullptr,
                0,
                nullptr,
                nullptr
            )
        };
        if (size == 0) {
            return {};
        }

        std::string result(size, '\0');
        WideCharToMultiByte(
            CP_UTF8,
            0,
            str.data(),
            static_cast<int>(str.size()),
            result.data(),
            size,
            nullptr,
            nullptr
        );
        return result;
    }

}
