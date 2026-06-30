#include "webserv.hpp"

Monitor::Monitor() : _epoll_fd(-1) { }

Monitor::~Monitor() {
    if (_epoll_fd != -1) close(_epoll_fd);
    for (std::map<int, HttpRequest>::iterator it = _requests.begin(); it != _requests.end(); ++it) {
        close(it->first);
    }
}

void    Monitor::init(const std::vector<ListeningSocket>& listeners) {
    _listeners = listeners;
    _epoll_fd = epoll_create(1);
    if (_epoll_fd == -1) {
        throw std::runtime_error("Error epoll_create: " + std::string(strerror(errno)));
    }

    struct epoll_event events;
    std::memset(&events, 0, sizeof(events));
    events.events = EPOLLIN;

    for (std::vector<ListeningSocket>::const_iterator it = _listeners.begin(); it != _listeners.end(); ++it) {
        events.data.fd = it->_fd;
        if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, it->_fd, &events) < 0) {
            throw std::runtime_error("Error epoll_ctl en listener: " + std::string(strerror(errno)));
        }
    }
}

const ServerConfig* Monitor::findServerConfig(int client_fd) {
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    if (getsockname(client_fd, (struct sockaddr*)&addr, &addr_len) < 0) {
        return NULL;
    }
    uint16_t connected_port = ntohs(addr.sin_port);
    for (std::vector<ListeningSocket>::iterator listener = _listeners.begin(); listener != _listeners.end(); ++listener) {
        if (listener->_port == connected_port && !listener->_servers.empty()) {
            return listener->_servers[0];
        }
    }
    return NULL;
}

void    Monitor::disconnectClient(int client_fd) {
    std::cout << "Cliente desconectado en el fd: " << std::endl;
    epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
    close(client_fd);
    _requests.erase(client_fd);
    _responses.erase(client_fd);
}

void Monitor::handleNewConnection(int listen_fd) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd < 0) 
        return;
    fcntl(client_fd, F_SETFL, O_NONBLOCK);
    struct epoll_event client_event;
    std::memset(&client_event, 0, sizeof(client_event));
    client_event.events = EPOLLIN;
    client_event.data.fd = client_fd;
    epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, client_fd, &client_event);
    std::cout << "Nuevo Cliente conectado con exito en el fd: " << client_fd << std::endl;
}

void Monitor::handleClientRead(int client_fd) {
    char buffer[4096];
    std::memset(buffer, 0, sizeof(buffer));
    
    int socket_bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (socket_bytes <= 0) {
        disconnectClient(client_fd);
        return;
    }

    ParseResult result = feedData(_requests[client_fd], buffer, socket_bytes);
    
    if (result == PARSE_COMPLETE) {
        std::cout << "✨ Petición HTTP recibida por completo" << std::endl;
        HttpRequest& request = _requests[client_fd];
        const ServerConfig* server = findServerConfig(client_fd);
        
        if (!server) {
            _responses[client_fd] = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
            struct epoll_event ev;
            std::memset(&ev, 0, sizeof(ev));
            ev.events = EPOLLOUT;
            ev.data.fd = client_fd;
            epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, client_fd, &ev);
            return;
        }
        std::string result_str = Method::processRequest(request, server);
        if (result_str.find("CGI_TRIGGERED:") == 0) {
            int pipe_fd = std::atoi(result_str.substr(14).c_str());
            _pipe_to_client[pipe_fd] = client_fd;
            struct epoll_event ev;
            std::memset(&ev, 0, sizeof(ev));
            ev.events = EPOLLIN;
            ev.data.fd = pipe_fd;
            epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, pipe_fd, &ev);
            return;
        } else {
            _responses[client_fd] = result_str;
            
            struct epoll_event ev;
            std::memset(&ev, 0, sizeof(ev));
            ev.events = EPOLLOUT;
            ev.data.fd = client_fd;
            epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, client_fd, &ev);
        }
    } else if (result == PARSE_ERROR) {
        _responses[client_fd] = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
        struct epoll_event ev;
        std::memset(&ev, 0, sizeof(ev));
        ev.events = EPOLLOUT;
        ev.data.fd = client_fd;
        epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, client_fd, &ev);
    }
}

void Monitor::handleClientWrite(int client_fd) {
    std::string& response = _responses[client_fd];
    if (response.empty()) return;
    int bytes_sent = send(client_fd, response.c_str(), response.length(), 0);
    if (bytes_sent < 0) {
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
        _requests[client_fd].reset();
        _responses.erase(client_fd);

        struct epoll_event ev;
        std::memset(&ev, 0, sizeof(ev));
        ev.events = EPOLLIN;
        ev.data.fd = client_fd;
        epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, client_fd, &ev);
    }
}

void Monitor::runServer() {
    struct epoll_event event_list[128];
    std::cout << "🚀 Core Network activado de forma asíncrona total." << std::endl;
    while(true) {
        int num_events = epoll_wait(_epoll_fd, event_list, 128, -1);
        if (num_events < 0) throw std::runtime_error("Error epoll_wait");
        for (int i = 0; i < num_events; i++) {
            int current_fd = event_list[i].data.fd;
            bool is_listener = false;
            for (std::vector<ListeningSocket>::iterator listener = _listeners.begin(); listener != _listeners.end(); ++listener) {
                if (current_fd == listener->_fd) {
                    is_listener = true;
                    break;
                }
            }
            if (is_listener) 
                handleNewConnection(current_fd);
            else if (_pipe_to_client.find(current_fd) != _pipe_to_client.end()) {
                int client_fd = _pipe_to_client[current_fd];
                char cgi_buffer[4096];
                std::memset(cgi_buffer, 0, sizeof(cgi_buffer));
                int bytes_read = read(current_fd, cgi_buffer, sizeof(cgi_buffer) - 1);

                if (bytes_read > 0){
                    std::stringstream ss;
                    ss << "HTTP/1.1 200 OK\r\n" << cgi_buffer;
                    _responses[client_fd] = ss.str();
                } else {
                    _responses[client_fd] = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
                }
                epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, current_fd, NULL);
                close(current_fd);
                _pipe_to_client.erase(current_fd);
                struct epoll_event ev;
                std::memset(&ev, 0, sizeof(ev));
                ev.events = EPOLLOUT;
                ev.data.fd = client_fd;
                epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, client_fd, &ev);
            }
            else if (event_list[i].events & EPOLLIN) 
                handleClientRead(current_fd);
            else if (event_list[i].events & EPOLLOUT) 
                handleClientWrite(current_fd);
        }
    }
}