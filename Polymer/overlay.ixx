module;

#include <Windows.h>
#include <DbgHelp.h>

export module polymer.overlay;

import std;
import polymer.error;
import polymer.env;

namespace polymer {

    struct FileData {
        std::uint32_t offset, size;
        std::filesystem::path name;
    };

    static bool read_binary(auto& stream, auto& dest) {
        return static_cast<bool>(stream.read(reinterpret_cast<char*>(&dest), sizeof(dest)));
    }

    static bool read_binary(auto& stream, FileData& dest) {
        std::uint16_t name_size;
        if (!read_binary(stream, dest.offset) || !read_binary(stream, dest.size) || !read_binary(stream, name_size)) {
            return false;
        }

        std::wstring buffer(name_size, '\0');
        if (!stream.read(reinterpret_cast<char*>(buffer.data()), name_size * sizeof(wchar_t))) {
            return false;
        }

        dest.name = std::move(buffer);
        return !dest.name.has_parent_path() && dest.name.has_filename() && dest.name != "." && dest.name != "..";
    }

    class Overlay {
        friend const Overlay& overlay();

    public:
        std::filesystem::path patcher_dir{
            std::filesystem::temp_directory_path() / std::format("polymer_{}", env().current_process_id)
        };
        std::filesystem::path patches_dir{ patcher_dir / "patches" };
        std::filesystem::path patcher_name;
        std::vector<std::filesystem::path> patches_name;

    private:
        Overlay() {
            auto header{ ImageNtHeader(env().current_module) };
            if (header == nullptr) {
                throw SystemError{ "Failed to get the PE header." };
            }

            auto section{ IMAGE_FIRST_SECTION(header) };
            DWORD offset{};
            for (WORD i{}; i < header->FileHeader.NumberOfSections; ++i) {
                offset = std::max(offset, section[i].PointerToRawData + section[i].SizeOfRawData);
            }

            std::ifstream file{ env().current_path, std::ios::binary };
            if (!file.is_open()) {
                throw RuntimeError{ "Failed to open the file." };
            }
            file.seekg(offset);

            static constexpr std::array POLYMER_SIGN{ 'P', 'L', 'M', '\0', '\1', '\0' };
            std::array<char, POLYMER_SIGN.size()> sign;
            if (!read_binary(file, sign) || sign != POLYMER_SIGN) {
                throw RuntimeError{ "Wrong signature." };
            }

            FileData patcher;
            if (!read_binary(file, patcher)) {
                throw RuntimeError{ "Failed to read the patcher's metadata." };
            }

            std::uint32_t patches_num;
            if (!read_binary(file, patches_num)) {
                throw RuntimeError{ "Failed to read patches' metadata." };
            }

            std::vector<FileData> patches(patches_num);
            for (auto& patch : patches) {
                if (!read_binary(file, patch)) {
                    throw RuntimeError{ "Failed to read patches' metadata." };
                }
            }

            if (!std::filesystem::create_directories(patcher_dir) ||
                !std::filesystem::create_directories(patches_dir)) {
                throw RuntimeError{ "Failed to create temporary directories." };
            }

            std::string buffer;
            buffer.resize(patcher.size);
            file.seekg(patcher.offset);
            if (!file.read(buffer.data(), patcher.size)) {
                throw RuntimeError{ "Failed to read the patcher." };
            }
            if (!std::ofstream{ patcher_dir / patcher.name, std::ios::binary }.write(buffer.data(), patcher.size)) {
                throw RuntimeError{ "Failed to extract the patcher." };
            }
            patcher_name = std::move(patcher.name);

            patches_name.resize(patches_num);
            for (auto&& [name, patch] : std::views::zip(patches_name, patches)) {
                buffer.resize(patch.size);
                file.seekg(offset);
                if (!file.read(buffer.data(), patch.size)) {
                    throw RuntimeError{ "Failed to read patches." };
                }
                if (!std::ofstream{ patches_dir / patch.name, std::ios::binary }.write(buffer.data(), patch.size)) {
                    throw RuntimeError{ "Failed to extract patches." };
                }
                name = std::move(patch.name);
            }
        }

        Overlay(const Overlay&) = delete;
        Overlay& operator=(const Overlay&) = delete;

    public:
        ~Overlay() {
            std::error_code error;
            std::filesystem::remove_all(patcher_dir, error);
            std::filesystem::remove_all(patches_dir, error);
        }
    };

    export const Overlay& overlay() {
        static Overlay instance;
        return instance;
    }

}
