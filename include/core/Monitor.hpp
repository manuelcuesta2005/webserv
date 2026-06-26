#ifndef MONITOR_HPP
#define MONITOR_HPP

#include "webserv.hpp"

class Monitor
{
    private:
        int                             _epoll_fd;
        std::vector<ListeningSocket>    _listeners;
        std::map<int, HttpRequest>      _requests;
        std::map<int, std::string>      _responses;

        const ServerConfig* findServerConfig(int client_fd);
    public:
        Monitor(/* args */);
        ~Monitor();

        void    init(const std::vector<ListeningSocket>& listeners);
        void    runServer();
        void	handleNewConnection(int listen_fd);
        void	handleClientRead(int client_fd);
        void	handleClientWrite(int client_fd);
        void	disconnectClient(int client_fd);
};


#endif