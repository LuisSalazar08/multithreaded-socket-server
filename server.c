#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>

#define PORT 5555
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

typedef struct {
    int socket;
    char username[50];
    int active;
} Client;

Client clients[MAX_CLIENTS];
int active_clients = 0;

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
FILE *log_file = NULL;

void remove_newline(char *str) {
    str[strcspn(str, "\r\n")] = '\0';
}

void write_to_log(const char *message) {
    pthread_mutex_lock(&log_mutex);
    if (log_file != NULL) {
        fprintf(log_file, "%s\n", message);
        fflush(log_file);
    }
    pthread_mutex_unlock(&log_mutex);
}

void broadcast(const char *message, int sender_socket) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i].active && clients[i].socket != sender_socket) {
            send(clients[i].socket, message, strlen(message), 0);
            send(clients[i].socket, "\n", 1, 0);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void *handle_client(void *arg) {
    int client_socket = *(int *)arg;
    free(arg);
    char buffer[BUFFER_SIZE];
    char username[50];

    int bytes_received = recv(client_socket, username, sizeof(username) - 1, 0);
    if (bytes_received <= 0) {
        close(client_socket);
        pthread_exit(NULL);
    }
    username[bytes_received] = '\0';
    remove_newline(username);

    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && strcmp(clients[i].username, username) == 0) {
            char *err = "ERROR: Nombre de usuario ya en uso. Desconectando...\n";
            send(client_socket, err, strlen(err), 0);
            pthread_mutex_unlock(&clients_mutex);
            close(client_socket);
            pthread_exit(NULL);
        }
    }

    int slot = -1;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i].active) {
            slot = i;
            clients[i].socket = client_socket;
            strcpy(clients[i].username, username);
            clients[i].active = 1;
            active_clients++;
            break;
        }
    }

    if (active_clients == 1 && log_file == NULL) {
        pthread_mutex_lock(&log_mutex); // Precedency rule applied: clients_mutex -> log_mutex
        time_t t = time(NULL);
        struct tm *tm_info = localtime(&t);
        char filename[100];
        strftime(filename, sizeof(filename), "chatlog_%Y%m%d_%H%M%S.txt", tm_info);
        log_file = fopen(filename, "w");
        pthread_mutex_unlock(&log_mutex);
    }

    pthread_mutex_unlock(&clients_mutex);

    if (slot == -1) {
        char *err = "ERROR: Servidor lleno.\n";
        send(client_socket, err, strlen(err), 0);
        close(client_socket);
        pthread_exit(NULL);
    }

    snprintf(buffer, sizeof(buffer), "SERVIDOR: %s ha entrado al chat!", username);
    broadcast(buffer, client_socket);
    write_to_log(buffer);

    while (1) {
        bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            break;
        }
        buffer[bytes_received] = '\0';
        remove_newline(buffer);

        if (strcmp(buffer, "Bye") == 0) {
            break;
        }

        char formatted_msg[BUFFER_SIZE + 60];
        snprintf(formatted_msg, sizeof(formatted_msg), "%s: %s", username, buffer);
        broadcast(formatted_msg, client_socket);
        write_to_log(formatted_msg);
    }

    snprintf(buffer, sizeof(buffer), "SERVIDOR: %s ha abandonado el chat!", username);
    broadcast(buffer, client_socket);
    write_to_log(buffer);

    pthread_mutex_lock(&clients_mutex);
    clients[slot].active = 0;
    active_clients--;

    if (active_clients == 0 && log_file != NULL) {
        pthread_mutex_lock(&log_mutex);
        fclose(log_file);
        log_file = NULL;
        pthread_mutex_unlock(&log_mutex);
    }
    pthread_mutex_unlock(&clients_mutex);

    close(client_socket);
    pthread_exit(NULL);
}

int main() {
    int server_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    listen(server_socket, MAX_CLIENTS);
    printf("Servidor iniciado en puerto %d...\n", PORT);

    while (1) {
        int *client_sock = malloc(sizeof(int));
        *client_sock = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);

        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, (void *)client_sock);
        pthread_detach(tid);
    }

    close(server_socket);
    return 0;
}