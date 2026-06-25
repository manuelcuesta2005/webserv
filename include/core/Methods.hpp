#ifndef METHOD_HPP
#define METHOD_HPP

#include "webserv.hpp"

class Method {
    private:
        static std::string handleGET(const std::string& real_path);
        static std::string handlePOST(const std::string& real_path, const HttpRequest& request, const ServerConfig* server);
        static std::string handleDELETE(const std::string& real_path);
    public:
        Method();
        ~Method();

        static std::string processRequest(HttpRequest& request, const ServerConfig* server);
};

#endif