#include "webserv.hpp"

Socket::Socket() : _epoll_fd(-1) { } // Buena práctica inicializar en C++98

Socket::Socket(const Socket& other) {
    (void)other;
}

Socket& Socket::operator=(const Socket& other) {
    (void)other;
    return *this;
}

Socket::~Socket() {
    for (std::vector<ListeningSocket>::iterator listener = _listenters.begin(); listener != _listenters.end(); ++listener) {
        if (listener->_fd != -1) {
            close(listener->_fd);
        }
    }
    for (std::map<int, HttpRequest>::iterator it = _requests.begin(); it != _requests.end(); ++it) {
        close(it->first)
    }
    if (_epoll_fd != -1)
        _requests.erase(_epoll_fd);
}

void    Socket::setPorts(const GlobalConfig& config) {
    for (std::vector<ServerConfig>::const_iterator server = config.servers.begin(); server != config.servers.end(); ++server) {
        for (std::vector<ListenEndpoint>::const_iterator endpoint = server->listen.begin(); endpoint != server->listen.end(); ++endpoint) {
            bool exist = false;
            for (std::vector<ListeningSocket>::iterator checkPort = _listenters.begin(); checkPort != _listenters.end(); ++checkPort) {
                if (checkPort->_host == endpoint->host && checkPort->_port == endpoint->port) {
                    checkPort->_servers.push_back(&(*server)); // Guardamos la dirección del ServerConfig
                    exist = true;
                    break;
                }
            }
            if (!exist) {
                ListeningSocket new_socket;
                new_socket._fd = -1;
                new_socket._host = endpoint->host;
                new_socket._port = endpoint->port;
                new_socket._servers.push_back(&(*server));
                _listenters.push_back(new_socket);
            }
        }
    }
}

void    Socket::configSocket(void) {
    for (std::vector<ListeningSocket>::iterator listener = _listenters.begin(); listener != _listenters.end(); ++listener) {
        listener->_fd = socket(AF_INET, SOCK_STREAM, 0);
        int enable = 1;
        if (listener->_fd == -1) {
            throw std::runtime_error("Error Socket: " + std::string(strerror(errno)));
        }
        if (setsockopt(listener->_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
            std::cerr << "⚠️ Warning setsockopt: " << strerror(errno) << std::endl;
        }
        if (fcntl(listener->_fd, F_SETFL, O_NONBLOCK) < 0) {
            throw std::runtime_error("Error NON_BLOCK: " + std::string(strerror(errno)));
        }
        std::memset(&listener->_adreess, 0, sizeof(listener->_adreess));
        listener->_adreess.sin_family = AF_INET;
        listener->_adreess.sin_port = htons(listener->_port);
        if (listener->_host == "127.0.0.1" || listener->_host == "localhost") {
            listener->_adreess.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        } else {
            listener->_adreess.sin_addr.s_addr = htonl(INADDR_ANY);
        }
        if (bind(listener->_fd, (struct sockaddr *)&listener->_adreess, sizeof(listener->_adreess)) < 0) {
            throw std::runtime_error("Error bind: " + std::string(strerror(errno)));
        }
        if (listen(listener->_fd, 128) < 0) {
            throw std::runtime_error("Error listen: " + std::string(strerror(errno)));
        }
        std::cout << "📡 Servidor escuchando con exito en " << listener->_host << ":" << listener->_port << std::endl;
    }
}

void    Socket::disconnectClient(int client_fd) {
    std::cout << "Cliente desconectado en el fd: " << std::endl;
    epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
    close(client_fd);
    _requests.erase(client_fd);
    _responses.erase(client_fd);
}

void    Socket::handleNewConnection(int listen_fd) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd < 0) {
        std::cerr << "Error accept: " << strerror(errno) << std::endl;
        return ;
    }
    if (fcntl(client_fd, F_SETFL, O_NONBLOCK) < 0) {
        std::cerr << "Error fcntl: " << strerror(errno) << std::endl;
        close(client_fd);
        return ;
    }
    struct epoll_event client_event;
    std::memset(client_event, 0, sizeof(client_event));
    client_event.events = EPOLLIN;
    client_event.data.fd = client_fd;
    if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, client_fd, &client_event) < 0) {
        std::cerr << "Error epoll ctl: " << strerror(errno) << std::endl;
        close(client_fd);
        return ;
    }
    std::cout << "Nuevo Cliente conectado con exito en el fd: " << client_fd << std::endl;
}

void    Socket::handleClientRead(int client_fd) {
    char buffer[4096];
    std::memset(buffer, 0, sizeof(buffer));
    
    int socket_bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (socket_bytes == 0) {
        disconnectClient(client_fd);
        return;
    } else if (socket_bytes < 0) {
        std::cerr << "Error recv: " << strerror(errno) << std::endl;
        disconnectClient(client_fd);
        return;
    }

    ParseResult result = feedData(_requests[client_fd], buffer, socket_bytes);
    
    if (result == PARSE_COMPLETE) {
        std::cout << "✨ Petición HTTP recibida por completo" << std::endl;
        HttpRequest& request = _requests[client_fd];

        // --- AQUÍ LLAMARÁN AL ROUTER DE TU COMPAÑERO ---
        // Tu compañero generará el string del response builder. Por ahora simulamos:
        _responses[client_fd] = "HTTP/1.1 200 OK\r\nContent-Length: 11\r\n\r\nHello World";

        // Cambiamos el evento a EPOLLOUT para indicarle a epoll que queremos escribir de forma asíncrona
        struct epoll_event ev;
        std::memset(&ev, 0, sizeof(ev));
        ev.events = EPOLLOUT;
        ev.data.fd = client_fd;
        epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, client_fd, &ev);

    } else if (result == PARSE_ERROR) {
        std::cerr << "❌ Error HTTP detectado en el parseo" << std::endl;
        _responses[client_fd] = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";

        struct epoll_event ev;
        std::memset(&ev, 0, sizeof(ev));
        ev.events = EPOLLOUT;
        ev.data.fd = client_fd;
        epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, client_fd, &ev);
    }
}

void    Socket::handleClientWrite(int client_fd) {
    std::string& response = _responses[client_fd];
    
    if (response.empty()) return;
    int bytes_sent = send(client_fd, response.c_str(), response.length(), 0);
    if (bytes_sent < 0) {
        std::cerr << "Error send: " << strerror(errno) << std::endl;
        disconnectClient(client_fd);
        return;
    }
    if (static_cast<size_t>(bytes_sent) < response.length()) {
        response = response.substr(bytes_sent);
        return;
    }
    HttpRequest& request = _requests[client_fd];
    if (request.headers["connection"] == "close") {
        disconnectClient(client_fd);
    } else {
        // Si es Keep-Alive, limpiamos buffers y volvemos a poner el socket en modo lectura (EPOLLIN)
        _requests[client_fd].reset();
        _responses.erase(client_fd);

        struct epoll_event ev;
        std::memset(&ev, 0, sizeof(ev));
        ev.events = EPOLLIN;
        ev.data.fd = client_fd;
        epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, client_fd, &ev);
    }
}

void    Socket::runServer() {
    _epoll_fd = epoll_create(1);
    if (_epoll_fd == -1) {
        throw std::runtime_error("Error epoll_create: " + std::string(strerror(errno)));
    }
    struct epoll_event ev;
    std::memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN;
    for (std::vector<ListeningSocket>::iterator listener = _listenters.begin(); listener != _listenters.end(); ++listener) {
        ev.data.fd = listener->_fd;
        if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, listener->_fd, &ev) < 0) {
            throw std::runtime_error("Error epoll ctl: " + std::string(strerror(errno)));
        }
    }
    struct epoll_event event_list[128];
    std::cout << "🚀 Core Network activado de forma asíncrona total." << std::endl;
    while(true) {
        int num_events = epoll_wait(_epoll_fd, event_list, 128, -1);
        if (num_events < 0) {
            throw std::runtime_error("Error epoll_wait: " + std::string(strerror(errno)));
        }
        for (int i = 0; i < num_events; i++) {
            int current_fd = event_list[i].data.fd;
            bool is_listener = false;
            for (std::vector<ListeningSocket>::iterator listener = _listenters.begin(); listener != _listenters.end(); ++listener) {
                if (current_fd == listener->_fd) {
                    is_listener = true;
                    break;
                }
            }
            if (is_listener) {
                handleNewConnection(current_fd);
            }
            else if (event_list[i].events & EPOLLIN) {
                handleClientRead(current_fd);
            }
            else if (event_list[i].events & EPOLLOUT) {
                handleClientWrite(current_fd);
            }
        }
    }
}