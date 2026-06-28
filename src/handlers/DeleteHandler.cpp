#include "handlers/DeleteHandler.hpp"
#include <sys/stat.h>
#include <unistd.h>

HandlerResult handleDelete(const std::string& fullPath) {
    HandlerResult result;
    struct stat fileInfo;

    if (stat(fullPath.c_str(), &fileInfo) != 0) {
        result.statusCode = 404;
        result.contentType = "text/html";
        result.body = "<html><body><h1>404 Not Found</h1></body></html>";
        return result;
    }

    if (S_ISDIR(fileInfo.st_mode)) {
        result.statusCode = 403;
        result.contentType = "text/html";
        result.body = "<html><body><h1>403 Forbidden</h1></body></html>";
        return result;
    }

    if (unlink(fullPath.c_str()) != 0) {
        result.statusCode = 403;
        result.contentType = "text/html";
        result.body = "<html><body><h1>403 Forbidden</h1></body></html>";
        return result;
    }

    result.statusCode = 200;
    result.contentType = "text/html";
    result.body = "<html><body><h1>File deleted successfully</h1></body></html>";
    return result;
}