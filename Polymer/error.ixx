module;

#include <Windows.h>

export module polymer.error;

import std;

namespace polymer {

    static std::wstring to_os_string(std::string_view str) {
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

    static std::string from_os_string(std::wstring_view str) {
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

    export void fatal_error(std::string_view message) {
        MessageBoxW(nullptr, to_os_string(message).data(), L"Fatal Error", MB_ICONERROR);
        std::abort();
    }

    static std::string format_os_error(DWORD error) {
        wchar_t* buffer{};
        DWORD size{
            FormatMessageW(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                nullptr,
                error,
                0,
                reinterpret_cast<wchar_t*>(&buffer),
                0,
                nullptr
            )
        };
        if (size == 0) {
            return "Unknown error";
        }

        std::string result{ from_os_string({ buffer, size }) };
        if (LocalFree(buffer) != nullptr) {
            fatal_error("Failed to free buffer.");
        }
        return result;
    }

    static std::string format_error(std::string_view message, const std::stacktrace& trace) {
        std::string result{ message };
        for (auto& entry : trace) {
            std::string function_name{ entry.description() };
            function_name.resize(function_name.find_last_of('+'));

            std::string filename{ std::filesystem::path{ entry.source_file() }.filename().string() };
            if (filename.empty()) {
                filename = "unknown";
            }

            result += std::format("\n    at {}({}:{})", function_name, filename, entry.source_line());
        }
        return result;
    }

    export class LogicError : public std::logic_error {
    public:
        LogicError(std::string_view message, const std::stacktrace& trace = std::stacktrace::current()) :
            std::logic_error{ format_error(message, trace) } {}
    };

    export class RuntimeError : public std::runtime_error {
    public:
        RuntimeError(std::string_view message, const std::stacktrace& trace = std::stacktrace::current()) :
            std::runtime_error{ format_error(message, trace) } {}
    };

    export class SystemError : public std::runtime_error {
    public:
        SystemError(
            std::string_view message,
            DWORD error = GetLastError(),
            const std::stacktrace& trace = std::stacktrace::current()
        ) :
            std::runtime_error{ format_error(std::format("{}: {}", message, format_os_error(error)), trace) } {}
    };

}
