#ifndef METHOD_HPP
#define METHOD_HPP

#include "webserv.hpp"

class Method {
    private:
        static std::string handleGET(const LocationConfig* location, const std::string& real_path, const ServerConfig* server);
        static std::string handlePOST(const std::string& real_path, const HttpRequest& request, const ServerConfig* server);
        static std::string handleDELETE(const std::string& real_path, const ServerConfig* server);
    public:
        Method();
        ~Method();

        static std::string processRequest(HttpRequest& request, const ServerConfig* server);
};

#endif