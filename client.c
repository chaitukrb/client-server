#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>

#define PORT1 8080
#define PORT2 8081
#define BUFFER_SIZE 1024
#define MAX_TOKEN_LEN 50

char* getNthString(char* str, int n) {
    char* token = (char*)malloc(sizeof(char) * MAX_TOKEN_LEN);
    int i = 1, j = 0;

    while (isspace(str[j])) {
        j++;
    }

    while (i < n && str[j] != '\0') {
        if (isspace(str[j])) {
            i++;
            while (isspace(str[j])) {
                j++;
            }
        } else {
            j++;
        }
    }

    int k = 0;
    while (str[j] != '\0' && !isspace(str[j]) && k < MAX_TOKEN_LEN - 1) {
        token[k] = str[j];
        j++;
        k++;
    }
    token[k] = '\0';

    if (k == 0) {
        return NULL;
    } else {
        return token;
    }
}

int should_unzip(char* str) {
    int len = strlen(str);
    if (len < 2) {
        return 0;
    }
    if (str[len-2] == '-' && str[len-1] == 'u') {
        return 1;
    }
    return 0;
}

void receive_tar_gz(int socket, char *cmd) {
    FILE *fp;
    char buffer[BUFFER_SIZE];
    int file_size, bytes_received, total_bytes_received = 0;

    recv(socket, &file_size, sizeof(file_size), 0);

    printf("File size: %d bytes\n", file_size);

    if(strcmp("sgetfiles", cmd)==0) {
        fp = fopen("size_received.tar.gz", "wb");
    }

    if(strcmp("dgetfiles", cmd)==0) {
        fp = fopen("date_received.tar.gz", "wb");
    }

    if(strcmp("getfiles", cmd)==0) {
        fp = fopen("files_received.tar.gz", "wb");
    }

    if(strcmp("gettargz", cmd)==0) {
        fp = fopen("types_received.tar.gz", "wb");
    }



    while (total_bytes_received < file_size) {
        bytes_received = recv(socket, buffer, sizeof(buffer), 0);
        if (bytes_received == -1) {
            perror("Error receiving file data");
            exit(1);
        }
        fwrite(buffer, 1, bytes_received, fp);
        total_bytes_received += bytes_received;
    }
    fclose(fp);
}

void recieve_findfileinfo(int socket) {
    char buffer[BUFFER_SIZE];
    if (recv(socket, buffer, sizeof(buffer), 0) == -1) {
            perror("recv");
            exit(EXIT_FAILURE);
    }
    buffer[113]='\0';
    printf("%s\n", buffer);
    memset(buffer,0,strlen(buffer));

}

int connect_server(int sock, int PORT) {
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    // Create socket file descriptor
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket address parameters
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("inet_pton failed");
        exit(EXIT_FAILURE);
    }

    // Connect to server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect failed");
        exit(EXIT_FAILURE);
    }

    printf("Connected to server\n");

    int valread = read(sock, buffer, BUFFER_SIZE);
    printf("Received message from server: %s\n", buffer);
    return sock;
}

int main() {
    int sock = 0;
    sock = connect_server(sock, PORT1);

    while(1) {
        char msg[50];
        scanf("%[^\n]%*c", msg);
        send(sock, msg, strlen(msg), 0);

        if(strcmp("findfile", getNthString(msg,1))==0) {
            printf("\n");
            recieve_findfileinfo(sock);
        }

        if(strcmp("sgetfiles", getNthString(msg,1))==0) {
            printf("\n");
            receive_tar_gz(sock, getNthString(msg,1));
            if(should_unzip(msg)) {
                char* filename = "size_received.tar.gz";
                char* command = "tar xzf ";
                char* full_command = (char*)malloc(strlen(command) + strlen(filename) + 1);
                strcpy(full_command, command);
                strcat(full_command, filename);
                FILE* pipe = popen(full_command, "r");
                int status = pclose(pipe);
                printf("File %s extracted successfully.\n", filename);
                free(full_command);
            }
            printf("\n");
        }

        if(strcmp("dgetfiles", getNthString(msg,1))==0) {
            printf("\n");
            receive_tar_gz(sock, getNthString(msg,1));
            if(should_unzip(msg)) {
                char* filename = "date_received.tar.gz";
                char* command = "tar xzf ";
                char* full_command = (char*)malloc(strlen(command) + strlen(filename) + 1);
                strcpy(full_command, command);
                strcat(full_command, filename);
                FILE* pipe = popen(full_command, "r");
                int status = pclose(pipe);
                printf("File %s extracted successfully.\n", filename);
                free(full_command);
            }
            printf("\n");
        }

        if(strcmp("getfiles", getNthString(msg,1))==0) {
            printf("\n");
            receive_tar_gz(sock, getNthString(msg,1));
            if(should_unzip(msg)) {
                char* filename = "files_received.tar.gz";
                char* command = "tar xzf ";
                char* full_command = (char*)malloc(strlen(command) + strlen(filename) + 1);
                strcpy(full_command, command);
                strcat(full_command, filename);
                FILE* pipe = popen(full_command, "r");
                int status = pclose(pipe);
                printf("File %s extracted successfully.\n", filename);
                free(full_command);
            }
            printf("\n");
        }

        if(strcmp("gettargz", getNthString(msg,1))==0) {
            printf("\n");
            receive_tar_gz(sock, getNthString(msg,1));
            if(should_unzip(msg)) {
                char* filename = "types_received.tar.gz";
                char* command = "tar xzf ";
                char* full_command = (char*)malloc(strlen(command) + strlen(filename) + 1);
                strcpy(full_command, command);
                strcat(full_command, filename);
                FILE* pipe = popen(full_command, "r");
                int status = pclose(pipe);
                printf("File %s extracted successfully.\n", filename);
                free(full_command);
            }
            printf("\n");
        }

        if(strcmp("quit", msg) == 0) {
            exit(0);
        }

        sleep(1);
    }

    return 0;
}
