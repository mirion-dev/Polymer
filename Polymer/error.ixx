module;

#include <wil/resource.h>

export module polymer.error;

import std;
import polymer.core;

namespace polymer {

    export std::string format_error_code(DWORD code) {
        wchar_t* buffer{};
        auto _{ wil::scope_exit([&] { LocalFree(buffer); }) }; // no failure handling
        DWORD size{
            FormatMessageW(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                nullptr,
                code,
                MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                reinterpret_cast<wchar_t*>(&buffer),
                0,
                nullptr
            )
        };

        std::string result{ to_string({ buffer, size }) };
        if (result.ends_with("\r\n")) {
            result.resize(result.size() - 2);
        }
        if (result.empty()) {
            result = "Unknown error.";
        }
        return result;
    }

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

    export std::string format_error(std::string_view message, DWORD code, const std::source_location& loc) {
        return format_error(std::format("{} ({})", message, format_error_code(code)), loc);
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
    public:
        SystemError(
            std::string_view message,
            DWORD code = GetLastError(),
            const std::source_location& loc = std::source_location::current()
        ) :
            std::runtime_error{ format_error(message, code, loc) } {}
    };

    export void show_error(std::string_view message) {
        MessageBoxW(nullptr, to_wstring(message).data(), L"Error", MB_ICONERROR); // no failure handling
    }

    export void fatal_error(
        std::string_view message,
        const std::source_location& loc = std::source_location::current()
    ) {
        show_error(format_error(message, loc));
        std::abort();
    }

    export void fatal_error_with_code(
        std::string_view message,
        DWORD code = GetLastError(),
        const std::source_location& loc = std::source_location::current()
    ) {
        show_error(format_error(message, code, loc));
        std::abort();
    }

}
