#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <sys/select.h>

static volatile sig_atomic_t sighup_flag = 0;

void handle_sighup(int sig) {
    (void)sig;
    sighup_flag = 1;
}

int main() 
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(1);
    }

    int optval = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(12345);
    server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        exit(1);
    }

    if (listen(server_fd, 5) == -1) {
        perror("listen");
        close(server_fd);
        exit(1);
    }

    struct sigaction sa = {0};
    sa.sa_handler = handle_sighup;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGHUP, &sa, NULL) == -1) {
        perror("sigaction");
        close(server_fd);
        exit(1);
    }

    
    sigset_t block_mask, orig_mask;
    sigemptyset(&block_mask);
    sigaddset(&block_mask, SIGHUP);
    if (sigprocmask(SIG_BLOCK, &block_mask, &orig_mask) == -1) {
        perror("sigprocmask");
        close(server_fd);
        exit(EXIT_FAILURE);
    }



    int client_fd = -1;
    const int MAX_BUFF = 256;
    char buffer[MAX_BUFF];


    printf("Server running on 127.0.0.1:12345\n");

    while (1) {
        fd_set read_set;
        FD_ZERO(&read_set);
        FD_SET(server_fd, &read_set);
        int max_fd = server_fd;

        if (client_fd != -1) {
            FD_SET(client_fd, &read_set);
            if (client_fd > max_fd)
                max_fd = client_fd;
        }

        int ready = pselect(max_fd + 1, &read_set, NULL, NULL, NULL, &orig_mask);
        if (ready == -1) {
            if (errno == EINTR) {
                if (sighup_flag) {
                    printf("Received SIGHUP\n");
                    sighup_flag = 0;
                }
                continue;
            }
            else {
                perror("pselect");
                break;
            }
        }

        if (FD_ISSET(server_fd, &read_set)) {
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            int new_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
            if (new_fd == -1) {
                perror("accept");
                continue;
            }

            if (client_fd == -1) {
                client_fd = new_fd;
                printf("Accepted client (fd=%d)\n", client_fd);
            }
            else {
                close(new_fd);
            }
        }

        // Данные от клиента
        if (client_fd != -1 && FD_ISSET(client_fd, &read_set)) {
            ssize_t nbytes = recv(client_fd, buffer, sizeof(buffer), 0);
            if (nbytes > 0) {
                printf("Received %zd bytes from client\n", nbytes);
            }
            else if (nbytes == 0) {
                printf("Client disconnected\n");
                close(client_fd);
                client_fd = -1;
            }
            else {
                perror("recv");
                close(client_fd);
                client_fd = -1;
            }
        }
    }

    if (client_fd != -1)
        close(client_fd);
    close(server_fd);
    return 0;
}
