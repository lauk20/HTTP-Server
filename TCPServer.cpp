#include <iostream>
#include <sstream>
#include <unistd.h>

#include "TCPServer.h"

namespace {
    /**
     * Parse line to find the path, which is the second token as delimited by " " in HTTP requests
    */
    char * find_path(char * line, const char * delim) {
        char * token = strtok(line, delim);

        if (token != NULL) {
            token = strtok(NULL, delim);
        }

        printf("PATH: %s\n", token);
        return token;
    }
}

namespace http {
    TCPServer::TCPServer(std::string ip_address, int port) {
        this->port = port;
        startServer();
    };

    TCPServer::~TCPServer() {
        closeServer();
    };

    /**
     * Create the socket and bind it to the given port.
    */
    int TCPServer::startServer() {
        // create socket
        socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_descriptor < 0) {
            perror("Error creating socket");
            return 1;
        }

        // set socket to be reuseable immediately
        int option = 1;
        setsockopt(socket_descriptor, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

        // set sockaddr_in data
        socketAddress.sin_family = AF_INET;
        socketAddress.sin_addr.s_addr = INADDR_ANY;
        socketAddress.sin_port = htons(port);
        memset(socketAddress.sin_zero, '\0', sizeof(socketAddress.sin_zero));

        // bind socket
        if (bind(socket_descriptor, (struct sockaddr *) &socketAddress, sizeof(socketAddress))) {
            perror("Error binding socket");
            exit(1);
        }

        addrlen = sizeof(socketAddress);

        return 0;
    }

    /**
     * Start listening on the socket and accept any connections.
     */
    void TCPServer::startListen() {
        // start listening on socket
        if (listen(socket_descriptor, 10) < 0) {
            perror("Error on socket listen");
            exit(1);
        }

        while (1) {
            acceptConnection();
        }
    }

    /**
     * Accept connections and call method to handle the connection.
     * Closes the socket when finished handling the connection.
     */
    void TCPServer::acceptConnection() {
        // accept socket connections
        new_socket = accept(socket_descriptor, (struct sockaddr *) &socketAddress, (socklen_t *) &addrlen);

        if (new_socket < 0) {
            perror("Error on socket accept");
            exit(1);
        }

        // handle the new socket connection
        handleConnection();

        // done with new socket, close it
        close (new_socket);
    }

    /**
     * Handles the connection. Reads any data and responds as needed.
    */
    void TCPServer::handleConnection() {
        // read what the client socket has sent
        char buffer[16000] = {0};
        int bytes_read = read(new_socket, buffer, sizeof(buffer));

        printf("%s", buffer);
        find_path(buffer, " ");
    }

    /**
     * Closes the server and client sockets and exits.
    */
    void TCPServer::closeServer() {
        close(socket_descriptor);
        close(new_socket);
        exit(0);
    }
}