#include "webserv.hpp"
#include "http/HttpResponse.hpp"
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

static std::string getErrorBody(int code, const ServerConfig* server, const std::string& defaultBody) {
    std::map<int, std::string>::const_iterator it = server->error_pages.find(code);
    if (it != server->error_pages.end()) {
        std::string content = readFile(it->second);
        if (!content.empty()) {
            return content;
        }
    }
    return defaultBody;
}

Method::Method() { }

Method::~Method() { }

std::string Method::processRequest(HttpRequest& request, const ServerConfig* server) {
    const LocationConfig* location = matchLocation(*server, request.uri);
    if (!location) {
        std::cerr << "❌ 404 Location Not Found para: " << request.uri << std::endl;
        std::string body = getErrorBody(404, server, "<html><body><h1>404 Not Found</h1></body></html>");
        return buildResponse(404, "text/html", body);
    }
    if (location->redirect_code != 0) {
        std::cout << "↪️ Redirect " << location->redirect_code << " -> " << location->redirect_target << std::endl;
        return buildResponse(location->redirect_code, "text/html", "", "Location: " + location->redirect_target + "\r\n");
    }
    if (!isMethodAllowed(location, request.method)) {
        std::cerr << "❌ 405 Method Not Allowed: " << request.method << std::endl;
        std::string body = getErrorBody(405, server, "<html><body><h1>405 Method Not Allowed</h1></body></html>");
        return buildResponse(405, "text/html", body);
    }
    std::string real_path = resolvePath(location, request.uri);
    std::cout << "🎯 [Method] Ruta física resuelta: " << real_path << std::endl;
    if (request.method == "GET") {
        return handleGET(location, real_path, server);
    } 
    else if (request.method == "POST") {
        return handlePOST(real_path, request, server);
    } 
    else if (request.method == "DELETE") {
        return handleDELETE(real_path, server);
    }

    return "HTTP/1.1 501 Not Implemented\r\nContent-Length: 0\r\n\r\n";
}

std::string Method::handleGET(const LocationConfig* location, const std::string& real_path, const ServerConfig* server) {
    struct stat fileInfo;

    if (stat(real_path.c_str(), &fileInfo) != 0) {
        std::cerr << "❌ GET: Archivo no encontrado: " << real_path << std::endl;
        std::string body = getErrorBody(404, server, "<html><body><h1>404 Not Found</h1></body></html>");
        return buildResponse(404, "text/html", body);
    }

    std::string targetPath = real_path;

    if (S_ISDIR(fileInfo.st_mode)) {
        bool foundIndex = false;

        for (size_t i = 0; i < location->index_files.size(); i++) {
            std::string candidate = real_path + "/" + location->index_files[i];
            struct stat indexInfo;
            if (stat(candidate.c_str(), &indexInfo) == 0 && !S_ISDIR(indexInfo.st_mode)) {
                targetPath = candidate;
                foundIndex = true;
                break;
            }
        }

        if (!foundIndex) {
            if (location->autoindex) {
                std::string listing = generateDirectoryListing(real_path, location->path);
                if (listing.empty()) {
                    std::string body = getErrorBody(500, server, "<html><body><h1>500 Internal Server Error</h1></body></html>");
                    return buildResponse(500, "text/html", body);
                }
                return buildResponse(200, "text/html", listing);
            } else {
                std::string body = getErrorBody(403, server, "<html><body><h1>403 Forbidden</h1></body></html>");
                return buildResponse(403, "text/html", body);
            }
        }
    }

    std::string content = readFile(targetPath);
    std::string contentType = getContentType(targetPath);
    return buildResponse(200, contentType, content);
}

std::string Method::handlePOST(const std::string& real_path, const HttpRequest& request, const ServerConfig* server) {
    size_t max_body_size = server->client_max_body_size ? server->client_max_body_size : 5242880; // 5MB default
    
    if (request.body.length() > max_body_size) {
        std::cerr << "❌ POST: Body excede client_max_body_size" << std::endl;
        return "HTTP/1.1 413 Payload Too Large\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
    }
    std::ofstream outfile(real_path.c_str(), std::ios::binary);
    if (!outfile.is_open()) {
        std::cerr << "❌ POST: No se pudo crear el archivo en " << real_path << std::endl;
        return "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
    }
    outfile.write(request.body.c_str(), request.body.length());
    outfile.close();
    std::cout << "💾 POST: Archivo creado con éxito" << std::endl;
    return "HTTP/1.1 201 Created\r\nContent-Length: 14\r\n\r\nArchivo Creado";
}

std::string Method::handleDELETE(const std::string& real_path, const ServerConfig* server) {
    struct stat fileInfo;

    if (stat(real_path.c_str(), &fileInfo) != 0) {
        std::cerr << "❌ DELETE: Archivo no existe: " << real_path << std::endl;
        std::string body = getErrorBody(404, server, "<html><body><h1>404 Not Found</h1></body></html>");
        return buildResponse(404, "text/html", body);
    }

    if (S_ISDIR(fileInfo.st_mode)) {
        std::cerr << "❌ DELETE: No se puede borrar un directorio: " << real_path << std::endl;
        std::string body = getErrorBody(403, server, "<html><body><h1>403 Forbidden</h1></body></html>");
        return buildResponse(403, "text/html", body);
    }

    if (unlink(real_path.c_str()) != 0) {
        std::cerr << "❌ DELETE: Error al eliminar el archivo" << std::endl;
        std::string body = getErrorBody(500, server, "<html><body><h1>500 Internal Server Error</h1></body></html>");
        return buildResponse(500, "text/html", body);
    }

    std::cout << "🗑️ DELETE: Archivo eliminado con éxito: " << real_path << std::endl;
    return buildResponse(200, "text/html", "<html><body><h1>File deleted successfully</h1></body></html>");
}