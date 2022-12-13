#include "TCPServer.cpp"

int main() {
    using namespace http;

    // create server socket on localhost:8080
    TCPServer server = TCPServer("127.0.0.1", 8080);
    // start listening on the socket
    server.startListen();

    return 0;
}