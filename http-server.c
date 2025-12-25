#include <stdint.h>   
#include "http-server.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>   
#include <ctype.h>




#define MAX_CHATS       1000
#define MAX_REACTIONS   100
#define MAX_USER_LEN    15
#define MAX_CHAT_MSG    255
#define MAX_REACT_MSG   15

typedef struct {
    char user[MAX_USER_LEN + 1];
    char message[MAX_REACT_MSG + 1];
} Reaction;

typedef struct {
    int id;
    struct tm timestamp;
    char user[MAX_USER_LEN + 1];
    char message[MAX_CHAT_MSG + 1];
    int num_reactions;
    Reaction reactions[MAX_REACTIONS];
} Chat;

static Chat chats[MAX_CHATS];
static int num_chats = 0;


uint8_t add_chat(const char *username, const char *message) {
    if (!username || !message) return 1;

    size_t ulen = strlen(username);
    size_t mlen = strlen(message);

    if (ulen == 0 || ulen > MAX_USER_LEN) return 2;
    if (mlen == 0 || mlen > MAX_CHAT_MSG) return 3;
    if (num_chats >= MAX_CHATS) return 4;

    time_t now = time(NULL);
    struct tm *lt = localtime(&now);
    if (!lt) return 5;

    Chat *c = &chats[num_chats];
    c->id = num_chats + 1;
    c->timestamp = *lt;
    strcpy(c->user, username);
    strcpy(c->message, message);
    c->num_reactions = 0;
    num_chats++;
    return 0;
}

uint8_t add_reaction(const char *username, const char *message, int id) {
    if (!username || !message) return 1;

    size_t ulen = strlen(username);
    size_t mlen = strlen(message);

    if (ulen == 0 || ulen > MAX_USER_LEN) return 2;
    if (mlen == 0 || mlen > MAX_REACT_MSG) return 3;
    if (id <= 0 || id > num_chats) return 4;

    Chat *c = &chats[id - 1];
    if (c->num_reactions >= MAX_REACTIONS) return 5;

    Reaction *r = &c->reactions[c->num_reactions];
    strcpy(r->user, username);
    strcpy(r->message, message);
    c->num_reactions++;
    return 0;
}

void respond_with_error(int client_fd, int code, const char *reason, const char *body) {
    char buf[512];
    int n = snprintf(buf, sizeof(buf),
        "HTTP/1.0 %d %s\r\n"
        "Content-Type: text/plain; charset=utf-8\r\n"
        "\r\n"
        "%s\n",
        code, reason, body ? body : "");
    write(client_fd, buf, n);
}

void respond_with_chats(int client_fd) {
    const char *header =
        "HTTP/1.0 200 OK\r\n"
        "Content-Type: text/plain; charset=utf-8\r\n"
        "\r\n";
    write(client_fd, header, strlen(header));

    char line[512];
    for (int i = 0; i < num_chats; i++) {
        Chat *c = &chats[i];

        char timebuf[32];
        strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", &c->timestamp);

        int n = snprintf(line, sizeof(line),
                         "[#%d %s] %15s: %s\n",
                         c->id, timebuf, c->user, c->message);
        write(client_fd, line, n);

        for (int j = 0; j < c->num_reactions; j++) {
            Reaction *r = &c->reactions[j];
            n = snprintf(line, sizeof(line),
                         "                            (%s)  %s\n",
                         r->user, r->message);
            write(client_fd, line, n);
        }
    }
}



int get_param(const char *query, const char *key,
              char *dst, size_t dst_size) {
    if (!query || !key || !dst || dst_size == 0) return 0;

    size_t keylen = strlen(key);
    const char *p = query;

    while (*p) {
        const char *eq = strchr(p, '=');
        if (!eq) break;

        const char *amp = strchr(eq + 1, '&');
        if (!amp) amp = p + strlen(p);

        if ((size_t)(eq - p) == keylen && strncmp(p, key, keylen) == 0) {
            size_t vlen = amp - (eq + 1);
            if (vlen >= dst_size) {
				return 0;
		}
            memcpy(dst, eq + 1, vlen);
            dst[vlen] = '\0';
            return 1;
        }

        if (*amp == '\0') break;
        p = amp + 1;
    }
     return 0;
}
static int hexval(char c) {
    if ('0' <= c && c <= '9') return c - '0';
    if ('a' <= c && c <= 'f') return 10 + (c - 'a');
    if ('A' <= c && c <= 'F') return 10 + (c - 'A');
    return -1;
}
static void url_decode(char *s) {
    char *src = s;
    char *dst = s;
    while (*src) {
        if (*src == '%' && isxdigit((unsigned char)src[1]) &&
                          isxdigit((unsigned char)src[2])) {
            int hi = hexval(src[1]);
            int lo = hexval(src[2]);
            if (hi >= 0 && lo >= 0) {
                *dst++ = (char)((hi << 4) | lo);
                src += 3;
                continue;
            }
        }
        if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}




void handle_request(char *buffer, int client_fd) {
    // First line: "GET /path?query HTTP/1.1"
    char *line_end = strstr(buffer, "\r\n");
    if (!line_end) {
        respond_with_error(client_fd, 400, "Bad Request", "Malformed request\n");
        return;
    }
    *line_end = '\0';

    char method[8], path[1024], version[16];
    if (sscanf(buffer, "%7s %1023s %15s", method, path, version) != 3) {
        respond_with_error(client_fd, 400, "Bad Request", "Malformed request line\n");
        return;
    }

    if (strcmp(method, "GET") != 0) {
        respond_with_error(client_fd, 405, "Method Not Allowed", "Only GET supported\n");
        return;
    }

    // Separate path and query
    char *qmark = strchr(path, '?');
    char *query = NULL;
    if (qmark) {
        *qmark = '\0';
        query = qmark + 1;
    
    url_decode(query);


	}

    if (strcmp(path, "/chats") == 0) {
        respond_with_chats(client_fd);

    } else if (strcmp(path, "/post") == 0) {
        char user[MAX_USER_LEN + 1];
        char msg[MAX_CHAT_MSG + 1];

        if (!query ||
            !get_param(query, "user", user, sizeof(user)) ||
            !get_param(query, "message", msg, sizeof(msg))) {
            respond_with_error(client_fd, 400, "Bad Request", "Missing parameter\n");
            return;
        }

		 url_decode(user);
        url_decode(msg);

        uint8_t err = add_chat(user, msg);
        if (err != 0) {
            respond_with_error(client_fd, 500, "Server Error", "Could not add chat\n");
            return;
        }
        respond_with_chats(client_fd);

    } else if (strcmp(path, "/react") == 0) {
        char user[MAX_USER_LEN + 1];
        char msg[MAX_REACT_MSG + 1];
        char id_str[16];

        if (!query ||
            !get_param(query, "user", user, sizeof(user)) ||
            !get_param(query, "message", msg, sizeof(msg)) ||
            !get_param(query, "id", id_str, sizeof(id_str))) {
            respond_with_error(client_fd, 400, "Bad Request", "Missing parameter\n");
            return;
        }


		        url_decode(user);
                url_decode(msg);


        int id = atoi(id_str);
        if (id <= 0) {
            respond_with_error(client_fd, 400, "Bad Request", "Invalid id\n");
            return;
        }

        uint8_t err = add_reaction(user, msg, id);
        if (err != 0) {
            respond_with_error(client_fd, 500, "Server Error", "Could not add reaction\n");
            return;
        }
        respond_with_chats(client_fd);

    } else {
        respond_with_error(client_fd, 404, "Not Found", "Unknown path\n");
    }
}



void start_server(void(*handler)(char*, int), int port) {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    

    // Create a socket
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    int enable = 1; 
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)  
        perror("setsockopt(SO_REUSEADDR) failed");                                


    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);  
    socklen_t addr_len = sizeof(server_addr);
    if (bind(server_sock, (struct sockaddr *)&server_addr, addr_len) < 0) {
        perror("bind failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_sock, 10) < 0) {
        perror("listen failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }
    // get port number
    if (getsockname(server_sock, (struct sockaddr *)&server_addr, &addr_len) == -1) {
        perror("getsockname failed");
        exit(EXIT_FAILURE);
    }

    printf("Server started on port %d\n", ntohs(server_addr.sin_port));
    char buffer[BUFFER_SIZE];
    // Main server loop
    while (1) {
        if ((client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len)) < 0) {
            perror("accept failed");
            close(server_sock);
            exit(EXIT_FAILURE);
        }

        // Receive the request
        int bytes = recv(client_sock, buffer, BUFFER_SIZE - 1, 0);
        if(bytes < 0) {printf("error receiving\n");close(client_sock);continue;}
        buffer[bytes] = '\0';  

        (*handler)(buffer, client_sock);

        // Close the connection with the client
        close(client_sock);
    }

    close(server_sock);
}

extern void handle_request(char *buffer, int client_fd);

int main(int argc, char *argv[]) {
    int port = 0;
    if (argc >= 2) {
        port = atoi(argv[1]);
    }
    start_server(handle_request, port);
    return 0; // start_server never returns
}





