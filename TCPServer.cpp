#include <iostream>
#include <sstream>
#include <vector>
#include <unistd.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>

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

    /**
     * Split the string into a list, as delimited by delim.
     */
    std::vector<std::string> split(std::string s, std::string delim) {
        size_t start = 0, end;
        std::string token;
        std::vector<std::string> result;

        while ((end = s.find(delim, start)) != std::string::npos) {
            token = s.substr(start, end - start);
            start = end + delim.size();
            result.push_back(token);
        }

        result.push_back(s.substr(start));

        return result;
    }
    
    /**
     * Determines whether we are staying in our allowed directory.
     * @param path The path we are given.
     */
    bool validate_directory(std::string path) {
        std::vector<std::string> dir_levels = split(path, "/");
        int level = -1; // let 0 = our current level "./"

        for (std::string s : dir_levels) {
            if (s == "..") {
                level = level - 1;
            } else {
                level = level + 1;
            }
        }

        return level > 0;
    }
}

namespace http {
    /**
     * Content-Type header format
     */
    static std::unordered_map<std::string, std::string> content_type = {
        {"html", "text/html"},
        {"css", "text/css"},
        {"csv", "text/csv"},
        {"xml", "text/xml"},
        {"js", "application/javascript"},
        {"gif", "image/gif"},
        {"jpeg", "image/jpeg"},
        {"png", "image/png"},
        {"svg", "image/svg+xml"}
    };

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

        int pid = fork();

        if (pid < 0) {
            perror("Forking error");
            exit(1);
        }

        if (pid == 0) { // Child process
            // handle the new socket connection
            handleConnection();

            // done with new socket, close it
            close (new_socket);

            // child process completed job, exit child process
            exit(0);
        } else { // Parent process
            // done with this socket descriptor from client, close it and move onto next accept.
            close(new_socket);
        }
    }

    /**
     * Handles the connection. Reads any data and responds as needed.
     */
    void TCPServer::handleConnection() {
        // read what the client socket has sent
        char buffer[16000] = {0};
        int bytes_read = read(new_socket, buffer, sizeof(buffer));

        printf("%s\n", buffer);
        std::string path = find_path(buffer, " ");
        std::string localpath = "./src"; // Entry Directory is ./src
        std::string http_header = "HTTP/1.1 200 OK\r\n";

        // Path leads outside of ./src, security risk. Respond with 404 and return.
        // This does not seem to be a problem as tested through using Microsoft Edge since it seems
        // to clean the directory before sending a HTTP-GET request but just in case.
        if (!validate_directory(localpath + path)) {
            printf("Client tried to access file outside of allowed directory: %s\n", (localpath + path).c_str());
            http_header = "HTTP/1.1 404 Not Found\r\n";
            write(new_socket, http_header.c_str(), http_header.size());

            return;
        }

        if (path == "/") {
            localpath = localpath + "/index.html";
            http_header = http_header + "Content-Type: text/html\r\n";
        } else {
            std::vector<std::string> args = split(path, ".");
            std::string file_extension = args.back();
            localpath = localpath + path;

            if (content_type.find(file_extension) != content_type.end()) {
                http_header = http_header + "Content-Type: " + content_type.at(file_extension) + "\r\n";
            } else {
                http_header = http_header + "Content-Type: text/plain\r\n";
            }
        }

        respond(localpath, http_header);
    }

    /**
     * Respond with the requested file if found.
     *
     * @param localpath Relative path of the file requested
     * @param http_header HTTP Header to respond with
     */
    void TCPServer::respond(std::string localpath, std::string http_header) {
        int file = open(localpath.c_str(), O_RDONLY);

        // Could not open file or path is "." then return
        if (file < 0 || localpath == ".") {
            //printf("FILE: %s\n", localpath);
            http_header = "HTTP/1.1 404 Not Found\r\n";
            write(new_socket, http_header.c_str(), http_header.size());
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
        http_header = http_header + "Content-Length: ";
        http_header = http_header + char_size;

        // End Header, Starting HTTP Body
        http_header = http_header + "\r\n\r\n";
        
        // Write HTTP Header to client socket
        write(new_socket, http_header.c_str(), http_header.size());

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