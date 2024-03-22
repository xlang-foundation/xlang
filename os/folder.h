#include <iostream>
#include <string>
#include <filesystem>
#include <vector>
#include "xpackage.h"
#include "xlang.h"

namespace X {

    class Folder {
    public:
        BEGIN_PACKAGE(Folder)
            APISET().AddPropWithType<std::string>("Path", &Folder::folderPath);
            APISET().AddFunc<1>("BuildPath", &Folder::BuildPath);
            APISET().AddFunc<0>("Scan", &Folder::Scan);
            APISET().AddFunc<0>("List", &Folder::List);
            APISET().AddFunc<1>("CopyFolder", &Folder::CopyFolder);
            APISET().AddFunc<2>("CopyFile", &Folder::CopyFile);
            APISET().AddFunc<1>("CreateFolder", &Folder::CreateFolder);
            APISET().AddFunc<2>("RemoveFolder", &Folder::RemoveFolder);
            APISET().AddFunc<2>("Rename", &Folder::Rename);
            APISET().AddFunc<1>("DeleteFile", &Folder::DeleteFile);
            APISET().AddFunc<1>("MoveFolder", &Folder::MoveFolder);
            APISET().AddFunc<0>("ParentPath", &Folder::ParentPath);
            APISET().AddFunc<1>("RelativePath", &Folder::RelativePath);
            APISET().AddFunc<1>("IsAbsolutePath", &Folder::IsAbsolutePath);
        END_PACKAGE

    public:
        Folder(const std::string& path) {
            // Normalize the input path
            folderPath = path;
#if (WIN32)
            std::replace(folderPath.begin(), folderPath.end(), '/', '\\');
#else
            std::replace(folderPath.begin(), folderPath.end(), '\\', '/');
#endif
            // Remove trailing separator if present
            if (!folderPath.empty() && (folderPath.back() == '\\' 
                || folderPath.back() == '/')) {
                folderPath.pop_back();
            }
        }


        std::string BuildPath(const std::string& subPath) {
            std::string fullPath = folderPath;

            // Normalize the input subPath
            std::string normalizedSubPath = subPath;
#if (WIN32)
            std::replace(normalizedSubPath.begin(), normalizedSubPath.end(), '/', '\\');
#else
            std::replace(normalizedSubPath.begin(), normalizedSubPath.end(), '\\', '/');
#endif

            // Ensure the folder path ends with the correct separator
#if (WIN32)
            if (fullPath.back() != '\\') {
                fullPath += '\\';
            }
#else
            if (fullPath.back() != '/') {
                fullPath += '/';
            }
#endif

            fullPath += normalizedSubPath;
            return fullPath;
        }

        X::List List() {
            X::List resultList;
            for (const auto& entry : std::filesystem::directory_iterator(folderPath)) {
                resultList += entry.path().filename().string();
            }
            return resultList;
        }
        X::List Scan() {
            X::List resultList;
            for (const auto& entry : std::filesystem::directory_iterator(folderPath)) {
                X::Dict fileInfo;
                fileInfo->Set("Name", entry.path().filename().string());
                fileInfo->Set("IsDirectory", entry.is_directory() ? "true" : "false");

                if (!entry.is_directory()) {
                    fileInfo->Set("Size", std::to_string(std::filesystem::file_size(entry.path())));
                }

                auto ftime = std::filesystem::last_write_time(entry.path());
                auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    ftime - std::filesystem::file_time_type::clock::now()
                    + std::chrono::system_clock::now());
                std::time_t cftime = std::chrono::system_clock::to_time_t(sctp);
                fileInfo->Set("LastModified", std::asctime(std::localtime(&cftime)));

                resultList += fileInfo;
            }
            return resultList;
        }

        bool CopyFolder(const std::string& targetPath) {
            try {
                std::filesystem::copy(folderPath, targetPath, std::filesystem::copy_options::recursive);
                return true;
            }
            catch (std::filesystem::filesystem_error& e) {
                std::cerr << "Error: " << e.what() << std::endl;
                return false;
            }
        }

        bool CopyFile(const std::string& filePath, const std::string& targetPath) {
            try {
                std::filesystem::copy(filePath, targetPath);
                return true;
            }
            catch (std::filesystem::filesystem_error& e) {
                std::cerr << "Error: " << e.what() << std::endl;
                return false;
            }
        }

        bool CreateFolder(const std::string& path) {
            try {
                std::filesystem::create_directories(path);
                return true;
            }
            catch (std::filesystem::filesystem_error& e) {
                std::cerr << "Error: " << e.what() << std::endl;
                return false;
            }
        }

        bool RemoveFolder(const std::string& path, bool recursive = false) {
            try {
                if (recursive) {
                    std::filesystem::remove_all(path);
                }
                else {
                    std::filesystem::remove(path);
                }
                return true;
            }
            catch (std::filesystem::filesystem_error& e) {
                std::cerr << "Error: " << e.what() << std::endl;
                return false;
            }
        }

        bool Rename(const std::string& srcPath, const std::string& targetPath) {
            try {
                std::filesystem::rename(srcPath, targetPath);
                return true;
            }
            catch (std::filesystem::filesystem_error& e) {
                std::cerr << "Error: " << e.what() << std::endl;
                return false;
            }
        }

        bool DeleteFile(const std::string& filePath) {
            try {
                std::filesystem::remove(filePath);
                return true;
            }
            catch (std::filesystem::filesystem_error& e) {
                std::cerr << "Error: " << e.what() << std::endl;
                return false;
            }
        }
        bool MoveFolder(const std::string& targetPath) {
            try {
                std::filesystem::rename(folderPath, targetPath);
                folderPath = targetPath; // Update the folderPath to the new location
                return true;
            }
            catch (std::filesystem::filesystem_error& e) {
                std::cerr << "Error: " << e.what() << std::endl;
                return false;
            }
        }

        std::string ParentPath() const {
            return std::filesystem::path(folderPath).parent_path().string();
        }

        std::string RelativePath(const std::string& basePath) const {
            return std::filesystem::relative(folderPath, basePath).string();
        }
        bool IsAbsolutePath(const std::string& path) const {
#if (WIN32)
            return path.size() > 2 && path[1] == ':' && (path[2] == '\\' || path[2] == '/');
#else
            return !path.empty() && path[0] == '/';
#endif
        }

    private:
        std::string folderPath;
    };

} // namespace X
