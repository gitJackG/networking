# networking
Project to dive deep into robust networking.
Implemented HTTP/1.0 in C.
Implemented HTTP/1.0 in C++.

## features
- Receives and parses http requests.
- Handles http errors.
- Serves files from the www/ folder.
- Allows for multiple connections.

## todo
- Optimize connection creation/handling.
- Implement header functionalitites.
- Adress security issues.
- Implement other mehtods functionalities (not only GET).
- Implement in Rust?

## setup
1. Clone the repository to your local machine.
   ```bash
   git clone https://github.com/gitJackG/networking.git
   ```

2. Make sure clang & make are installed in your local machine.

3. Run the server with:
   ```bash
   make && ./http_server
   ```

4. The server will be listening on PORT 8080.

## additional information
Server written in C can be found in [./c/http-server/http_server.c](./c/http-server/http_server.c).

Server written in C++ can be found in [./cpp/http-server/http_server.cpp](./cpp/http-server/http_server.cpp).
