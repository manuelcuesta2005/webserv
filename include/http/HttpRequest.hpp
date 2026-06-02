#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include "webserv.hpp"

enum RequestState {
    READING_REQUEST_LINE,
    READING_HEADERS,
    READING_BODY,
    COMPLETE,
    ERROR_STATE
};

enum BodyType {
    BODY_NONE,
    BODY_CONTENT_LENGTH,
    BODY_CHUNKED
};

enum ParseResult {
    PARSE_INCOMPLETE,
    PARSE_COMPLETE,
    PARSE_ERROR
};

struct HttpRequest {
    RequestState                        state;
    std::string                         buffer;

    std::string                         method;
    std::string                         uri;
    std::string                         version;

    std::map<std::string, std::string>  headers;

    BodyType                            bodyType;
    size_t                              contentLength;
    std::string                         body;

    HttpRequest();
    void reset();
};

ParseResult feedData(HttpRequest& req, const char* data, size_t len);

#endif