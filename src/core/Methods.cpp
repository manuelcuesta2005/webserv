#include "webserv.hpp"

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

static std::map<std::string, std::string> sessions; // session_id -> username

static std::string generateSessionId() {
    std::string charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::string id;
    for (int i = 0; i < 32; i++) {
        id += charset[rand() % charset.length()];
    }
    return id;
}

static std::string getCookieValue(const HttpRequest& request, const std::string& key) {
    std::map<std::string, std::string>::const_iterator it = request.headers.find("cookie");
    if (it == request.headers.end()) return "";
    std::string cookies = it->second;
    size_t pos = cookies.find(key + "=");
    if (pos == std::string::npos) return "";
    size_t start = pos + key.length() + 1;
    size_t end = cookies.find(';', start);
    return cookies.substr(start, end - start);
}

static std::string handleLogin(const HttpRequest& request, const ServerConfig* server) {
    (void)server;
    std::string body = request.body;
    std::string usuario, clave;

    size_t userPos = body.find("usuario=");
    size_t clavePos = body.find("clave=");
    if (userPos != std::string::npos) {
        size_t end = body.find('&', userPos);
        usuario = body.substr(userPos + 8, end - (userPos + 8));
    }
    if (clavePos != std::string::npos) {
        size_t end = body.find('&', clavePos);
        clave = body.substr(clavePos + 6, end - (clavePos + 6));
    }

    if (usuario == "admin" && clave == "1234") {
        std::string sessionId = generateSessionId();
        sessions[sessionId] = usuario;
        std::string headers = "Set-Cookie: session_id=" + sessionId + "\r\n";
        headers += "Location: /\r\n";
        return buildResponse(302, "text/html", "", headers);
    }
    return buildResponse(401, "text/html", "<html><body><h1>Credenciales incorrectas</h1></body></html>");
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
    if (request.uri == "/login") {
        if (request.method == "POST") {
            return handleLogin(request, server);
        } else {
            std::string content = readFile(location->root + "/login.html");
            if (content.empty()) {
                std::string body = getErrorBody(404, server, "<html><body><h1>404 Not Found</h1></body></html>");
                return buildResponse(404, "text/html", body);
            }
            return buildResponse(200, "text/html", content);
        }
    }
    if (location->redirect_code != 0) {
        std::cout << "↪️ Redirect " << location->redirect_code << " -> " << location->redirect_target << std::endl;
        return buildResponse(location->redirect_code, "text/html", "", "Location: " + location->redirect_target + "\r\n");
    }
    if (location->requires_auth && request.uri != "/login") {
        std::string sessionId = getCookieValue(request, "session_id");
        if (sessionId.empty() || sessions.find(sessionId) == sessions.end()) {
            return buildResponse(302, "text/html", "", "Location: /login\r\n");
        }
    }
    if (!isMethodAllowed(location, request.method)) {
        std::cerr << "❌ 405 Method Not Allowed: " << request.method << std::endl;
        std::string body = getErrorBody(405, server, "<html><body><h1>405 Method Not Allowed</h1></body></html>");
        return buildResponse(405, "text/html", body);
    }
    std::string real_path = resolvePath(location, request.uri);
    std::cout << "🎯 [Method] Ruta física resuelta: " << real_path << std::endl;
    size_t  dot_pos = real_path.find_last_of('.');
    if (dot_pos != std::string::npos) {
        std::string extension = real_path.substr(dot_pos);
        if (extension == ".py" || extension == ".php" || extension == ".sh") {
            try {
                HandlerCGI cgi(real_path, extension);
                int pipe_fd = cgi.execute(request);
                std::stringstream ss;
                ss << "CGI_TRIGGERED:" << pipe_fd;
                return ss.str();
            } catch (const std::exception& e) {
                std::cerr << "❌ Error ejecutando CGI: " << e.what() << std::endl;
                std::string body = getErrorBody(500, server, "<html><body><h1>500 Internal Server Error (CGI)</h1></body></html>");
                return buildResponse(500, "text/html", body);
            }
        }
    }
    if (request.method == "GET") {
        return handleGET(location, real_path, server);
    } 
    else if (request.method == "POST") {
        return handlePOST(real_path, request, server, location);
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

std::string Method::handlePOST(const std::string& real_path, const HttpRequest& request, const ServerConfig* server, const LocationConfig* location) {
    size_t max_body_size = (location && location->client_max_body_size > 0) 
                        ? location->client_max_body_size
                        : (server->client_max_body_size ? server->client_max_body_size : 5242880);
    if (request.body.length() > max_body_size) {
        std::cerr << "❌ POST: Body excede el limite permitido." << std::endl;
        return "HTTP/1.1 413 Payload Too Large\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
    }
    if (!location || !location->upload_enabled) {
        std::cerr << "❌ POST: Upload deshabilitado para esta location" << std::endl;
        return "HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\n\r\n";
    }
    std::string final_path = real_path;
    std::string upload_dir = location->upload_store;

    if (!upload_dir.empty()) {
        if (upload_dir[upload_dir.length() - 1] != '/') {
            upload_dir += '/';
        }
        size_t last_slash = request.uri.find_last_not_of('/');
        std::string filename = (last_slash != std::string::npos) ? request.uri.substr(last_slash + 1) : "uploaded_file";
        final_path = upload_dir + filename;
        if (mkdir(upload_dir.c_str(), 0775) < 0) {
            if (errno != EEXIST) {
                std::cerr << "❌ POST: No se pudo crear el directorio upload_store: " << strerror(errno) << std::endl;
                return "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
            }
        }
    }
    std::ofstream outfile(final_path.c_str(), std::ios::binary);
    if (!outfile.is_open()) {
        std::cerr << "❌ POST: No se pudo crear el archivo final en " << final_path << std::endl;
        return "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
    }
    outfile.write(request.body.c_str(), request.body.length());
    outfile.close();
    std::cout << "💾 POST: Archivo creado con éxito en zona segura: " << final_path << std::endl;
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