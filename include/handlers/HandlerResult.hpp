#ifndef HANDLER_RESULT_HPP
#define HANDLER_RESULT_HPP

#include "webserv.hpp"

struct HandlerResult {
    int          statusCode;
    std::string  contentType;
    std::string  body;
};

#endif