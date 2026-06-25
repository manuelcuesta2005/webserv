#include "webserv.hpp"

Method::Method() { }

Method::~Method() { }

std::string Method::processRequest(HttpRequest& request, const ServerConfig* server) {
    const LocationConfig* location = matchLocation(*server, request.uri);
    if (!location) {
        std::cerr << "❌ 404 Location Not Found para: " << request.uri << std::endl;
        return "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
    }
    if (!isMethodAllowed(location, request.method)) {
        std::cerr << "❌ 405 Method Not Allowed: " << request.method << std::endl;
        return "HTTP/1.1 405 Method Not Allowed\r\nContent-Length: 0\r\n\r\n";
    }
    std::string real_path = resolvePath(location, request.uri);
    std::cout << "🎯 [Method] Ruta física resuelta: " << real_path << std::endl;
    if (request.method == "GET") {
        return handleGET(real_path);
    } 
    else if (request.method == "POST") {
        return handlePOST(real_path, request, server);
    } 
    else if (request.method == "DELETE") {
        return handleDELETE(real_path);
    }

    return "HTTP/1.1 501 Not Implemented\r\nContent-Length: 0\r\n\r\n";
}

std::string Method::handleGET(const std::string& real_path) {
    std::ifstream infile(real_path.c_str(), std::ios::binary);
    if (!infile.is_open()) {
        std::cerr << "❌ GET: Archivo no encontrado: " << real_path << std::endl;
        return "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
    }
    std::string content((std::istreambuf_iterator<char>(infile)), std::istreambuf_iterator<char>());
    infile.close();
    std::stringstream ss;
    ss << "HTTP/1.1 200 OK\r\nContent-Length: " << content.length() << "\r\n\r\n" << content;
    return ss.str();
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

std::string Method::handleDELETE(const std::string& real_path) {
    std::ifstream file(real_path.c_str());
    if (!file.good()) {
        std::cerr << "❌ DELETE: Archivo no existe: " << real_path << std::endl;
        return "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
    }
    file.close();
    if (std::remove(real_path.c_str()) != 0) {
        std::cerr << "❌ DELETE: Error al eliminar el archivo" << std::endl;
        return "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
    }

    std::cout << "🗑️ DELETE: Archivo eliminado con éxito: " << real_path << std::endl;
    return "HTTP/1.1 200 OK\r\nContent-Length: 19\r\n\r\nArchivo Eliminado";
}