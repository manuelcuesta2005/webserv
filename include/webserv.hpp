#ifndef WEBSERV_HPP
#define WEBSERV_HPP

// Webserv: Esta libreria será de uso general
// Para crear el servidor web.
// Añadiremos clases, estructuras y librerias
// Que se necesitan para realizar el proyecto

/* Librerias dentro del Lenguaje de C++ */
#include <iostream>
#include <vector>
#include <map>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <sys/socket.h>
#ifdef __linux__
# include <sys/epoll.h>
#else
# include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#endif

/* Clases desarrolladas dentro del Proyecto */

// Parser: Clases usadas para el archivo.conf
#include "parser/ConfigError.hpp"
#include "parser/ConfigParser.hpp"
#include "parser/ConfigTokenizer.hpp"
#include "parser/ConfigTypes.hpp"
#include "parser/ConfigValidator.hpp"
#include "parser/Token.hpp"

// Http: Clases para el manejo de las peticiones HTTP y su parseo
#include "http/HttpRequest.hpp"
#include "http/HttpRequestParser.hpp"
#include "router/Router.hpp"

// Core: Clases usadas para la configuracion e inicializacion del servidor
#include "core/socket.hpp"

#endif