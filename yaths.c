#include <dirent.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUF_SIZE 8192
#define PORT 8000

void send_response(int client, const char *status, const char *content_type, const char *body, int body_len) {
    char header[512];
    int header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n\r\n",
        status, content_type, body_len);
    write(client, header, header_len);
    if (body) write(client, body, body_len);
}

void send_file(int client, const char *path) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        send_response(client, "404 Not Found", "text/html", "<h1>404 Not Found</h1>", 22);
        return;
    }
    
    struct stat st;
    fstat(fd, &st);
    
    const char *content_type = "application/octet-stream";
    int path_len = strlen(path);

    if (path_len >= 5 && strcmp(path + path_len - 5, ".html") == 0) content_type = "text/html";
    else if (path_len >= 4 && strcmp(path + path_len - 4, ".css") == 0) content_type = "text/css";
    else if (path_len >= 3 && strcmp(path + path_len - 3, ".js") == 0) content_type = "application/javascript";
    else if (path_len >= 5 && strcmp(path + path_len - 5, ".json") == 0) content_type = "application/json";
    else if (path_len >= 4 && strcmp(path + path_len - 4, ".png") == 0) content_type = "image/png";
    else if (path_len >= 4 && strcmp(path + path_len - 4, ".jpg") == 0) content_type = "image/jpeg";
    else if (path_len >= 5 && strcmp(path + path_len - 5, ".jpeg") == 0) content_type = "image/jpeg";
    else if (path_len >= 4 && strcmp(path + path_len - 4, ".gif") == 0) content_type = "image/gif";
    else if (path_len >= 4 && strcmp(path + path_len - 4, ".txt") == 0) content_type = "text/plain";
    else content_type = "text/plain";
    
    char header[512];
    int header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n\r\n",
        content_type, st.st_size);
    write(client, header, header_len);
    
    off_t offset = 0;
    sendfile(client, fd, &offset, st.st_size);

    close(fd);
}

void send_directory(int client, const char *path, int show_hidden) {
    DIR *dir = opendir(path);
    if (!dir) {
        send_response(client, "404 Not Found", "text/html", "<h1>404 Not Found</h1>", 22);
        return;
    }

    struct {
        char name[256];
        int is_dir;
        long size;
    } entries[1024];

    int entry_count = 0;
    struct dirent *entry;
    struct stat st;
    char full_path[1024];
    
    while ((entry = readdir(dir)) != NULL && entry_count < 1024) {
        if (entry->d_name[0] == '.' && strlen(entry->d_name) == 1) continue;
        if (!show_hidden && entry->d_name[0] == '.' && strcmp(entry->d_name, "..") != 0) continue;
        
        if (strcmp(path, ".") == 0) {
            snprintf(full_path, sizeof(full_path), "%s", entry->d_name);
        } else {
            snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        }
        
        int is_dir = 0;
        long size = 0;
        if (stat(full_path, &st) == 0) {
            is_dir = S_ISDIR(st.st_mode);
            size = st.st_size;
        }
        
        strncpy(entries[entry_count].name, entry->d_name, 255);
        entries[entry_count].name[255] = '\0';
        entries[entry_count].is_dir = is_dir;
        entries[entry_count].size = size;
        entry_count++;
    }
    closedir(dir);
    
    for (int i = 0; i < entry_count - 1; i++) {
        for (int j = i + 1; j < entry_count; j++) {
            int swap = 0;
            if (entries[i].is_dir < entries[j].is_dir) {
                swap = 1;
            } else if (entries[i].is_dir == entries[j].is_dir) {
                if (strcasecmp(entries[i].name, entries[j].name) > 0) {
                    swap = 1;
                }
            }
            if (swap) {
                typeof(entries[0]) tmp = entries[i];
                entries[i] = entries[j];
                entries[j] = tmp;
            }
        }
    }
    
    char *html = malloc(768000);
    int len = sprintf(html, 
        "<!DOCTYPE html><html><head><title>Index of %s</title>"
        "<meta charset=\"utf-8\">"
        "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1,maximum-scale=5\">"
        "<style>"
        "*{-webkit-text-size-adjust:none;}"
        "body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;margin:0;padding:16px;background:#fafafa;font-size:17px;}"
        ".container{max-width:1000px;margin:0 auto;background:white;border-radius:8px;box-shadow:0 1px 3px rgba(0,0,0,0.1);}"
        "h1{margin:0;padding:18px 20px;border-bottom:1px solid #e0e0e0;font-size:22px;font-weight:500;}"
        ".list{list-style:none;margin:0;padding:0;}"
        ".item{display:flex;align-items:center;padding:12px 12px;border-bottom:1px solid #f0f0f0;transition:background 0.15s;}"
        ".item:hover{background:#f5f5f5;}"
        ".item a{text-decoration:none;color:#1a73e8;flex:1;display:flex;align-items:center;min-height:18px;font-size:17px;}"
        ".item a:hover{text-decoration:underline;}"
        ".icon{margin-right:14px;font-size:18px;flex-shrink:0;}"
        ".name{word-break:break-word;line-height:1.0;}"
        ".size{color:#5f6368;font-size:15px;min-width:80px;text-align:right;margin-left:12px;flex-shrink:0;}"
        "@media (max-width:600px){"
        "body{padding:0;background:white;}"
        ".container{border-radius:0;box-shadow:none;}"
        "h1{font-size:18px;padding:14px 14px;}"
        ".item{padding:14px;}"
        ".item a{min-height:42px;}"
        ".icon{font-size:24px;margin-right:14px;}"
        "}"
        "</style></head><body><div class=\"container\">"
        "<h1>Index of /%s</h1><ul class=\"list\">",
        path, strcmp(path, ".") == 0 ? "" : path);
    
    for (int i = 0; i < entry_count; i++) {
        char size_str[32] = "";
        if (!entries[i].is_dir) {
            long s = entries[i].size;
            if (s < 1024) sprintf(size_str, "%ld B", s);
            else if (s < 1024*1024) sprintf(size_str, "%.1f KB", s/1024.0);
            else if (s < 1024*1024*1024) sprintf(size_str, "%.1f MB", s/(1024.0*1024.0));
            else sprintf(size_str, "%.1f GB", s/(1024.0*1024.0*1024.0));
        }
        
        char encoded_name[768];
        int enc_idx = 0;
        for (int j = 0; entries[i].name[j] && enc_idx < 760; j++) {
            unsigned char c = entries[i].name[j];
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || 
                (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
                encoded_name[enc_idx++] = c;
            } else {
                enc_idx += sprintf(encoded_name + enc_idx, "%%%02X", c);
            }
        }
        encoded_name[enc_idx] = '\0';

        if (len > 260000) break;
        
        len += sprintf(html + len, 
            "<li class=\"item\"><a href=\"%s%s\">"
            "<span class=\"icon\">%s</span><span class=\"name\">%s</span></a>"
            "<span class=\"size\">%s</span></li>",
            encoded_name,
            entries[i].is_dir ? "/" : "",
            entries[i].is_dir ? "&#128193;" : "&#128196;",
            entries[i].name,
            size_str);
    }
    
    len += sprintf(html + len, "</ul></div></body></html>");
    
    send_response(client, "200 OK", "text/html; charset=utf-8", html, len);
    free(html);
}

void handle_request(int client, int show_hidden) {
    char buf[BUF_SIZE];
    int n = read(client, buf, BUF_SIZE - 1);
    if (n <= 0) return;
    buf[n] = 0;
    
    char method[16], path[1024], proto[16];
    sscanf(buf, "%s %s %s", method, path, proto);
    
    if (strcmp(method, "GET") != 0) {
        send_response(client, "405 Method Not Allowed", "text/html", 
                     "<h1>405 Method Not Allowed</h1>", 31);
        return;
    }
    
    char decoded_path[1024];
    int dec_idx = 0;
    for (int i = 0; path[i] && dec_idx < 1023; i++) {
        if (path[i] == '%' && path[i+1] && path[i+2]) {
            char hex[3] = {path[i+1], path[i+2], 0};
            decoded_path[dec_idx++] = (char)strtol(hex, NULL, 16);
            i += 2;
        } else if (path[i] == '+') {
            decoded_path[dec_idx++] = ' ';
        } else {
            decoded_path[dec_idx++] = path[i];
        }
    }
    decoded_path[dec_idx] = '\0';
    
    char *file_path = decoded_path + 1;
    if (strlen(file_path) == 0) file_path = ".";
    
    if (strstr(file_path, "..")) {
        send_response(client, "403 Forbidden", "text/html", "<h1>403 Forbidden</h1>", 22);
        return;
    }
    
    struct stat st;
    if (stat(file_path, &st) < 0) {
        send_response(client, "404 Not Found", "text/html", "<h1>404 Not Found</h1>", 22);
        return;
    }
    
    if (S_ISDIR(st.st_mode)) {
        char index_path[1100];
        snprintf(index_path, sizeof(index_path), "%s/index.html", file_path);
        if (access(index_path, F_OK) == 0) {
            send_file(client, index_path);
        } else {
            send_directory(client, file_path, show_hidden);
        }
    } else {
        send_file(client, file_path);
    }
}

int main(int argc, char **argv) {
    signal(SIGCHLD, SIG_IGN);
    
    int port = PORT;
    char path[1024] = ".";
    int show_hidden = 0;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Usage: %s [OPTIONS] [PORT]\n\n", argv[0]);
            printf("YaTHS - Yet another Tiny HTTP-Server\n\n");
            printf("Options:\n");
            printf("  --port PORT        Set port number (default: 8000)\n");
            printf("  --dir PATH         Set directory to serve (default: current dir)\n");
            printf("  --show-hidden, -a  Show hidden files (files starting with .)\n");
            printf("  --help, -h         Show this help message\n\n");
            printf("Examples:\n");
            printf("  %s                      # Serve current directory on port 8000\n", argv[0]);
            printf("  %s 3000                 # Serve on port 3000\n", argv[0]);
            printf("  %s --port 8080          # Serve on port 8080\n", argv[0]);
            printf("  %s --dir /var/www       # Serve /var/www directory\n", argv[0]);
            printf("  %s -a --port 3000       # Show hidden files on port 3000\n", argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            port = atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "--dir") == 0 && i + 1 < argc) {
            strncpy(path, argv[i + 1], sizeof(path) - 1);
            path[sizeof(path) - 1] = '\0';
            i++;
        } else if (strcmp(argv[i], "--show-hidden") == 0 || strcmp(argv[i], "-a") == 0) {
            show_hidden = 1;
        } else if (argv[i][0] != '-') {
            port = atoi(argv[i]);
        }
    }
    
    if (strcmp(path, ".") != 0) {
        if (chdir(path) != 0) {
            fprintf(stderr, "Error: cannot change to directory '%s'\n", path);
            return 1;
        }
    }
    
    int server = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = INADDR_ANY
    };
    
    bind(server, (struct sockaddr*)&addr, sizeof(addr));
    listen(server, 10);
    
    printf("Serving HTTP on 0.0.0.0 port %d (http://0.0.0.0:%d/) ...\n", port, port);
    
    while (1) {
        int client = accept(server, NULL, NULL);
        if (client < 0) continue;
        
        if (fork() == 0) {
            close(server);
            handle_request(client, show_hidden);
            close(client);
            exit(0);
        }
        close(client);
    }
    
    return 0;
}