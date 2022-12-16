# HTTP-Server
*HTTP Server Written in C++*

## How To Run
```docker compose up -d``` will create the image and start a container using that image.
The server will be running on localhost:8080 and will be listening on the container on port 8080.

## Files
In order to get the server working, your web files must be inside the ```/src``` directory.
The web server will listen on localhost:8080 and will serve the needed files.

## Limitations
The server assumes that all incoming requests are HTTP-GET requests. It will only serve files, and only those specified inside TCPServer.cpp.
