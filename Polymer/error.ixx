module;

#include <Windows.h>

export module polymer.error;

import std;
import polymer.core;

namespace polymer {

    static std::string format_error(std::string_view message, const std::stacktrace& trace) {
        std::string result{ message };
        for (auto& entry : trace) {
            std::string function_name{ entry.description() };
            std::size_t pos{ function_name.find_last_of('+') };
            if (pos != -1) {
                function_name.resize(pos);
            }

            std::string filename{ std::filesystem::path{ entry.source_file() }.filename().string() };
            if (filename.empty()) {
                filename = "unknown";
            }

            result += std::format("\n    at {} ({}:{})", function_name, filename, entry.source_line());
        }
        return result;
    }

    export void show_error(std::string_view message) {
        MessageBoxW(nullptr, to_wstring(message).data(), L"Error", MB_ICONERROR);
    }

    export void fatal_error(std::string_view message, const std::stacktrace& trace = std::stacktrace::current()) {
        show_error(format_error(message, trace));
        std::abort();
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

            std::string result;
            if (size == 0) {
                result = "Unknown error";
            }
            else {
                while (size-- != 0) {
                    wchar_t ch{ buffer[size] };
                    if (ch != ' ' && ch != '\r' && ch != '\n') {
                        break;
                    }
                }
                result = to_string({ buffer, ++size });
            }

            if (LocalFree(buffer) != nullptr) {
                fatal_error("Failed to free the buffer.");
            }
            return result;
        }

    public:
        SystemError(
            std::string_view message,
            DWORD error = GetLastError(),
            const std::stacktrace& trace = std::stacktrace::current()
        ) :
            std::runtime_error{ format_error(std::format("{} ({})", message, _format_code(error)), trace) } {}
    };

}
