# C Chat Server (Raw TCP + HTTP)

A fully functional chat server built **from scratch in C** using raw TCP sockets and the HTTP protocol.

This project implements:
- A real HTTP server
- Request routing
- Query string parsing
- In-memory message storage
- Browser + curl clients

No frameworks. No libraries. Just sockets, memory, and protocol handling.

---

##  Features

- `GET /` — Web UI for interacting with the server  
- `GET /chats` — Returns all messages  
- `GET /post?user=NAME&message=TEXT` — Adds a message  
- `GET /react?user=NAME&message=TEXT&id=N` — React to a message  

---

##  Demo

Start the server:

```bash
gcc -Wall -Wextra -O2 http-server.c -o http-server
./http-server

Open in browser:

http://localhost:PORT

Post a message:

http://localhost:PORT/post?user=Ioanna&message=I%20built%20a%20server

View messages:
<img width="824" height="128" alt="Screenshot 2025-12-30 at 02 42 41" src="https://github.com/user-attachments/assets/c4ea237b-52e1-429c-accc-ddb552d87bba" />

http://localhost:PORT/chats
