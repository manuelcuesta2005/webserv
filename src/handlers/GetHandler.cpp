#include "handlers/GetHandler.hpp"
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

static std::string readFile(const std::string& path) {
    int fd = open(path.c_str(), O_RDONLY);
    if (fd == -1) {
        return "";
    }
    std::string content;
    char buffer[4096];
    ssize_t bytesRead;
    while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0) {
        content.append(buffer, bytesRead);
    }
    close(fd);
    return content;
}

static std::string getContentType(const std::string& path) {
    size_t dotPos = path.find_last_of('.');
    if (dotPos == std::string::npos) {
        return "application/octet-stream";
    }
    std::string ext = path.substr(dotPos);
    if (ext == ".html" || ext == ".htm") return "text/html";
    if (ext == ".css")  return "text/css";
    if (ext == ".js")   return "application/javascript";
    if (ext == ".png")  return "image/png";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".gif")  return "image/gif";
    if (ext == ".txt")  return "text/plain";
    return "application/octet-stream";
}

static std::string generateDirectoryListing(const std::string& path, const std::string& uri) {
    DIR* dir = opendir(path.c_str());
    if (dir == NULL) {
        return "";
    }
    std::string html = "<html><body><h1>Index of " + uri + "</h1><ul>";
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;
        if (name == ".") {
            continue;
        }
        html += "<li><a href=\"" + uri + "/" + name + "\">" + name + "</a></li>";
    }
    closedir(dir);
    html += "</ul></body></html>";
    return html;
}

HandlerResult handleGet(const LocationConfig* location, const std::string& fullPath) {
    HandlerResult result;
    struct stat fileInfo;

    if (stat(fullPath.c_str(), &fileInfo) != 0) {
        result.statusCode = 404;
        result.contentType = "text/html";
        result.body = "<html><body><h1>404 Not Found</h1></body></html>";
        return result;
    }

    std::string targetPath = fullPath;

    if (S_ISDIR(fileInfo.st_mode)) {
        bool foundIndex = false;

        for (size_t i = 0; i < location->index_files.size(); i++) {
            std::string candidate = fullPath + "/" + location->index_files[i];
            struct stat indexInfo;
            if (stat(candidate.c_str(), &indexInfo) == 0 && !S_ISDIR(indexInfo.st_mode)) {
                targetPath = candidate;
                foundIndex = true;
                break;
            }
        }

        if (!foundIndex) {
            if (location->autoindex) {
                std::string listing = generateDirectoryListing(fullPath, location->path);
                if (listing.empty()) {
                    result.statusCode = 500;
                    result.contentType = "text/html";
                    result.body = "<html><body><h1>500 Internal Server Error</h1></body></html>";
                } else {
                    result.statusCode = 200;
                    result.contentType = "text/html";
                    result.body = listing;
                }
                return result;
            } else {
                result.statusCode = 403;
                result.contentType = "text/html";
                result.body = "<html><body><h1>403 Forbidden</h1></body></html>";
                return result;
            }
        }
    }

    std::string content = readFile(targetPath);
    result.statusCode = 200;
    result.contentType = getContentType(targetPath);
    result.body = content;
    return result;
}