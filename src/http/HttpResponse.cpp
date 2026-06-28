#include "http/HttpResponse.hpp"
#include <map>
#include <sstream>

std::string buildResponse(int statusCode, const std::string& contentType, const std::string& body, const std::string& extraHeader) {
    static std::map<int, std::string> statusTexts;
    if (statusTexts.empty()) {
        statusTexts[200] = "OK";
        statusTexts[201] = "Created";
        statusTexts[301] = "Moved Permanently";
        statusTexts[302] = "Found";
        statusTexts[404] = "Not Found";
        statusTexts[403] = "Forbidden";
        statusTexts[405] = "Method Not Allowed";
        statusTexts[500] = "Internal Server Error";
    }

    std::string statusText = "Unknown";
    std::map<int, std::string>::iterator it = statusTexts.find(statusCode);
    if (it != statusTexts.end()) {
        statusText = it->second;
    }

    std::ostringstream oss;
    oss << statusCode;
    std::string codeStr = oss.str();

    std::ostringstream lenOss;
    lenOss << body.length();
    std::string lengthStr = lenOss.str();

    std::string response;
    response += "HTTP/1.1 " + codeStr + " " + statusText + "\r\n";
    response += "Content-Type: " + contentType + "\r\n";
    response += "Content-Length: " + lengthStr + "\r\n";
    response += extraHeader;
    response += "\r\n";
    response += body;

    return response;
}