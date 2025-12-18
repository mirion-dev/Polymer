module;

#include <Windows.h>

export module polymer.error;

import std;
import polymer.core;

namespace polymer {

    export std::string format_error(std::string_view message, const std::source_location& loc) {
        std::string filename{ loc.file_name() };
        return std::format(
            "{}\n    at {} ({}:{})",
            message,
            loc.function_name(),
            filename.substr(filename.find_last_of('\\') + 1),
            loc.line()
        );
    }

    export void show_error(std::string_view message) {
        MessageBoxW(nullptr, to_wstring(message).data(), L"Error", MB_ICONERROR);
    }

    export void fatal_error(
        std::string_view message,
        const std::source_location& loc = std::source_location::current()
    ) {
        show_error(format_error(message, loc));
        std::abort();
    }

    export class LogicError : public std::logic_error {
    public:
        LogicError(std::string_view message, const std::source_location& loc = std::source_location::current()) :
            std::logic_error{ format_error(message, loc) } {}
    };

    export class RuntimeError : public std::runtime_error {
    public:
        RuntimeError(std::string_view message, const std::source_location& loc = std::source_location::current()) :
            std::runtime_error{ format_error(message, loc) } {}
    };

    export class SystemError : public std::runtime_error {
        static std::string _format_code(DWORD error) {
            wchar_t* buffer{};
            DWORD size{
                FormatMessageW(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                    nullptr,
                    error,
                    MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                    reinterpret_cast<wchar_t*>(&buffer),
                    0,
                    nullptr
                )
            };

            wchar_t* end{ buffer + size };
            wchar_t* last{
                std::ranges::find_last_if_not(
                    buffer,
                    end,
                    [](wchar_t ch) { return ch == ' ' || ch == '\r' || ch == '\n'; }
                ).begin()
            };
            std::string result{ last == end ? "Unknown error" : to_string({ buffer, ++last }) };
            if (LocalFree(buffer) != nullptr) {
                fatal_error("Failed to free the buffer.");
            }

            return result;
        }

    public:
        SystemError(
            std::string_view message,
            DWORD error = GetLastError(),
            const std::source_location& loc = std::source_location::current()
        ) :
            std::runtime_error{ format_error(std::format("{} ({})", message, _format_code(error)), loc) } {}
    };

}
