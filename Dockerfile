FROM gcc:4.9
WORKDIR .
COPY . .
RUN g++ server.cpp -std=c++11
CMD ["./a.out"]
EXPOSE 8080