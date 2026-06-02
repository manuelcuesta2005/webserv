#ifndef HTTP_REQUEST_PARSER_HPP
#define HTTP_REQUEST_PARSER_HPP

#include "HttpRequest.hpp"

ParseResult feedData(HttpRequest& req, const char* data, size_t len);

ParseResult parseRequestLine(HttpRequest& req);
ParseResult parseHeaders(HttpRequest& req);
ParseResult parseBody(HttpRequest& req);

#endif