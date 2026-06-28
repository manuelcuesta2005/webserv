#ifndef HTTP_RESPONSE_HPP
#define HTTP_RESPONSE_HPP

#include "webserv.hpp"

std::string buildResponse(int statusCode, const std::string& contentType, const std::string& body, const std::string& extraHeader = "");

#endif