#include "webserv.hpp"

Socket::Socket() { }

Socket::~Socket() {
    for (std::vector<ListeningSocket>::iterator listener = _listeners.begin(); listener != _listeners.end(); ++listener) {
        if (listener->_fd != -1) {
            close(listener->_fd);
        }
    }
}

void    Socket::setPorts(const GlobalConfig& config) {
    for (std::vector<ServerConfig>::const_iterator server = config.servers.begin(); server != config.servers.end(); ++server) {
        for (std::vector<ListenEndpoint>::const_iterator endpoint = server->listen.begin(); endpoint != server->listen.end(); ++endpoint) {
            bool exist = false;
            for (std::vector<ListeningSocket>::iterator checkPort = _listeners.begin(); checkPort != _listeners.end(); ++checkPort) {
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
                _listeners.push_back(new_socket);
            }
        }
    }
}

void    Socket::configSocket(void) {
    for (std::vector<ListeningSocket>::iterator listener = _listeners.begin(); listener != _listeners.end(); ++listener) {
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

const std::vector<ListeningSocket>& Socket::getListeners() const {
    return _listeners;
}