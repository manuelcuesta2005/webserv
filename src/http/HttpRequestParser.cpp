#include "http/HttpRequestParser.hpp"

ParseResult feedData(HttpRequest& req, const char* data, size_t len) {
    (void)req; (void)data; (void)len;
    return PARSE_INCOMPLETE;
}

ParseResult parseRequestLine(HttpRequest& req) {
    (void)req;
    return PARSE_INCOMPLETE;
}

ParseResult parseHeaders(HttpRequest& req) {
    (void)req;
    return PARSE_INCOMPLETE;
}

ParseResult parseBody(HttpRequest& req) {
    (void)req;
    return PARSE_INCOMPLETE;
}