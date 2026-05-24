#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 5555
#define BUFFER_SIZE 1024

int sock = 0;

void remove_newline(char *str) {
    str[strcspn(str, "\r\n")] = '\0';
}

void *receive_messages(void *arg) {
    char buffer[BUFFER_SIZE];
    while (1) {
        int bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            printf("\nConexión con el servidor cerrada.\n");
            exit(0);
        }
        buffer[bytes_received] = '\0';
        remove_newline(buffer);
        printf("%s\n", buffer);
    }
    return NULL;
}

int main() {
    struct sockaddr_in serv_addr;
    char username[50];
    char buffer[BUFFER_SIZE];

    printf("Escribe el usuario para entrar al chat: ");
    fgets(username, sizeof(username), stdin);
    remove_newline(username);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Fallo al conectar al servidor.\n");
        return -1;
    }

    send(sock, username, strlen(username), 0);

    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, receive_messages, NULL);

    while (1) {
        fgets(buffer, sizeof(buffer), stdin);
        remove_newline(buffer);

        send(sock, buffer, strlen(buffer), 0);

        //disconnect condition
        if (strcmp(buffer, "Bye") == 0) {
            printf("Saliendo del chat...\n");
            break;
        }
    }

    close(sock);
    return 0;
}