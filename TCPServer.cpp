#include <iostream>
#include <sstream>
#include <unistd.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

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
            printf("Accepting Connections...\n");
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

        printf("%s\n", buffer);
        char * path = find_path(buffer, " ");
        char * localpath = (char *)calloc(500, sizeof(char));
        char * http_header = (char *)calloc(100, sizeof(char));
        strcat(localpath, ".");
        strcat(http_header, "HTTP/1.1 200 OK\r\n");

        if (strcmp(path, "/") == 0) {
            strcat(localpath, "/index.html");
            strcat(http_header, "Content-Type: text/html\r\n");
        }

        respond(localpath, http_header);
        free(localpath);
        free(http_header);
    }

    /**
     * Respond with the requested file if found.
     *
     * @param localpath Relative path of the file requested
     * @param http_header HTTP Header to respond with
    */
    void TCPServer::respond(char * localpath, char * http_header) {
        int file = open(localpath, O_RDONLY);

        // Could not open file or path is "." then return
        if (file < 0 || strcmp(localpath, ".") == 0) {
            //printf("FILE: %s\n", localpath);
            strcpy(http_header, "HTTP/1.1 404 Not Found\r\n");
            write(new_socket, http_header, strlen(http_header));
            return;
        }

        // Get file info
        struct stat file_info;
        fstat(file, &file_info);
        int size = file_info.st_size;
        int block_size = file_info.st_blksize;
        char char_size[10];
        sprintf(char_size, "%d", size);

        // Add Content-Length to HTTP Header
        strcat(http_header, "Content-Length: ");
        strcat(http_header, char_size);
        // End Header, Starting HTTP Body
        strcat(http_header, "\r\n\r\n");
        
        // Write HTTP Header to client socket
        write(new_socket, http_header, strlen(http_header));

        // Write the requested file to client socket
        while (size > 0) {
            int sent_bytes = sendfile(new_socket, file, NULL, block_size);
            size = size - sent_bytes;
        }

        // Close the opened file
        close(file);
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