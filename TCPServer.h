#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <cstring>

#ifndef TCPSERVER_H
#define TCPSERVER_H

namespace http {
    class TCPServer {
        public:
            /**
             * TCPServer constructor and deconstructor.
             * startListen method to start listening on the socket that is created and
             * handle all data being sent and responses.
             */
            TCPServer(std::string ip_address, int port);
            ~TCPServer();
            void startListen();
        
        private:
            /**
             * Private member variables that keep track of the socket information.
             * Private methods that are used to create the socket, accept and handle connections.
            */
            int port;
            int socket_descriptor;
            int new_socket;
            struct sockaddr_in socketAddress;
            int addrlen;

            int startServer();
            void closeServer();
            void acceptConnection();
            void handleConnection();
            void respond(std::string, std::string);
    };
}

#endif