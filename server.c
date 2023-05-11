#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <time.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <errno.h>
#include <zlib.h>
#include <fcntl.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_TOKEN_LEN 50
#define MAX_PATH_LENGTH 1024
#define TAR_FILE "temp.tar"
#define GZ_FILE "temp.tar.gz"
#define PATH_MAX 80



char G_BUFFER[1000];

char *filePaths[100];
char *fileNames[100];
int noOfFileNames;
int noOfFiles;
int clients_count;

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


void search_files_in_directory(const char *directory_path, const char *filename) {
    DIR *dir = opendir(directory_path);
    struct dirent *dp;

    while ((dp = readdir(dir)) != NULL) {
        if (dp->d_type == DT_REG && strcmp(dp->d_name, filename) == 0) {
            struct stat file_stats;
            char fpath[100];
            strcpy(fpath, directory_path);
            strcat(fpath, "/");
            strcat(fpath, filename);
            if (stat(fpath, &file_stats) == -1) {
                printf("ERROR : No File\n");
                fprintf(stderr, "Error: %s\n", strerror(errno));
            }
            memset(fpath,0,strlen(fpath));
            strcat(G_BUFFER, "\nFound at Path ");
            strcat(G_BUFFER, directory_path);
            strcat(G_BUFFER, "\nSize of the file is : ");
            char size_str[20];
            sprintf(size_str, "%ld", file_stats.st_size);
            strcat(G_BUFFER, size_str);
            strcat(G_BUFFER, "\nDate Created is : ");
            char date_created[26];
            strftime(date_created, 26, "%Y-%m-%d %H:%M:%S", localtime(&file_stats.st_birthtimespec.tv_sec));
            strcat(G_BUFFER, date_created);

            strcat(G_BUFFER, "\n\n");
        }

        if (dp->d_type == DT_DIR && strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0) {
            char path[1024];
            snprintf(path, sizeof(path), "%s/%s", directory_path, dp->d_name);
            search_files_in_directory(path, filename); // recursive call
        }
    }

    closedir(dir);
}

int file_type_matches(char* filename, char* filetype) {
    int filename_len = strlen(filename);
    int filetype_len = strlen(filetype);

    if (filename_len <= filetype_len) {
        return 0; // file name is too short to match file type
    }

    char* extension = filename + filename_len - filetype_len;
    return strcmp(extension, filetype) == 0;
}

void list_files_by_size(const char *path, off_t size1, off_t size2) {
    DIR *dir = opendir(path);

    if (dir == NULL) {
        perror("opendir");
        return;
    }
    struct dirent *dp;
    while ((dp = readdir(dir)) != NULL) {
        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) {
            continue;
        }
        char filename[1024];
        sprintf(filename, "%s/%s", path, dp->d_name);
        struct stat st;
        stat(filename, &st);
        if (S_ISREG(st.st_mode)) {
            off_t size = st.st_size;
            if (size >= size1 && size <= size2) {
                filePaths[noOfFiles++] = (char*) malloc(strlen(filename) * sizeof(char));
                strcpy(filePaths[noOfFiles-1], filename);
                filePaths[noOfFiles-1][strlen(filename)]='\0';
                printf("wf %s\n", filePaths[noOfFiles-1]);
                memset(filename,0,strlen(filename));
            }
        } else if (S_ISDIR(st.st_mode)) {
            list_files_by_size(filename, size1, size2);
        }
    }
    closedir(dir);
}

void liste_files_by_date(char *start_date_str, char *end_date_str, char *path) {
    struct tm start_date, end_date;
    memset(&start_date, 0, sizeof(struct tm));
    memset(&end_date, 0, sizeof(struct tm));
    strptime(start_date_str, "%Y-%m-%d", &start_date);
    strptime(end_date_str, "%Y-%m-%d", &end_date);
    time_t start_time = mktime(&start_date);
    time_t end_time = mktime(&end_date);

    DIR *dir = opendir(path);
    if (dir == NULL) {
        perror("Error opening directory");
        exit(1);
    }

    struct dirent *dp;
    while ((dp = readdir(dir)) != NULL) {
        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) {
            continue;
        }

        char filename[1024];
        sprintf(filename, "%s/%s", path, dp->d_name);

        struct stat st;
        stat(filename, &st);

        if (S_ISREG(st.st_mode)) {
            if (st.st_mtime >= start_time && st.st_mtime <= end_time) {
                filePaths[noOfFiles++] = (char*) malloc(strlen(filename) * sizeof(char));
                strcpy(filePaths[noOfFiles-1], filename);
                filePaths[noOfFiles-1][strlen(filename)]='\0';
                printf("wf %s\n", filePaths[noOfFiles-1]);
                memset(filename,0,strlen(filename));
            }
        } else if(S_ISDIR(st.st_mode)) {
            liste_files_by_date(start_date_str, end_date_str, filename);
        }
    }
    closedir(dir);
}

void get_all_files(char *path) {
    DIR *dir = opendir(path);

    if (dir == NULL) {
        perror("opendir");
        return;
    }
    struct dirent *dp;
    while ((dp = readdir(dir)) != NULL) {
        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) {
            continue;
        }
        char filename[1024];
        sprintf(filename, "%s/%s", path, dp->d_name);
        struct stat st;
        stat(filename, &st);
        if (S_ISREG(st.st_mode)) {
            //printf("fn --- %s path --- %s\n", filename, path);

            for(int i=0;i<noOfFileNames;i++) {
                char temps[100];
                strcat(temps, path);
                strcat(temps, "/");
                strcat(temps, fileNames[i]);
                //printf("temps %s\n", temps);
                if(strcmp(temps, filename) == 0) {
                    printf("file name matched %s\n", filename);
                    filePaths[noOfFiles++] = (char*) malloc(strlen(filename) * sizeof(char));
                    strcpy(filePaths[noOfFiles-1], filename);
                    filePaths[noOfFiles-1][strlen(filename)]='\0';
                    //printf("wf %s\n", filePaths[noOfFiles-1]);
                    memset(filename,0,strlen(filename));
                }
                memset(temps,0,strlen(temps));
            }
        } else if (S_ISDIR(st.st_mode)) {
            get_all_files(filename);
        }
    }
    closedir(dir);
}

void get_all_files_types(char *path) {
    DIR *dir = opendir(path);

    if (dir == NULL) {
        perror("opendir");
        return;
    }
    struct dirent *dp;
    while ((dp = readdir(dir)) != NULL) {
        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) {
            continue;
        }
        char filename[1024];
        sprintf(filename, "%s/%s", path, dp->d_name);
        struct stat st;
        stat(filename, &st);
        if (S_ISREG(st.st_mode)) {
            for(int i=0;i<noOfFileNames;i++) {

                if(file_type_matches(filename, fileNames[i])) {
                    printf("file type matched %s\n", filename);
                    filePaths[noOfFiles++] = (char*) malloc(strlen(filename) * sizeof(char));
                    strcpy(filePaths[noOfFiles-1], filename);
                    filePaths[noOfFiles-1][strlen(filename)]='\0';
                    memset(filename,0,strlen(filename));
                }
            }
        } else if (S_ISDIR(st.st_mode)) {
            get_all_files_types(filename);
        }
    }
    closedir(dir);
}

void send_tar_gz_size(int client_socket, char *cmd) {
    FILE *fp;
    char buffer[1024];
    int file_size, bytes_sent;

    fp = fopen("size.tar.gz", "rb");

    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    send(client_socket, &file_size, sizeof(file_size), 0);

    
    while ((bytes_sent = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        send(client_socket, buffer, bytes_sent, 0);
    }
    fclose(fp);
}

void send_tar_gz_date(int client_socket, char *cmd) {
    FILE *fp;
    char buffer[1024];
    int file_size, bytes_sent;
    
    fp = fopen("date.tar.gz", "rb");

    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    send(client_socket, &file_size, sizeof(file_size), 0);

    
    while ((bytes_sent = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        send(client_socket, buffer, bytes_sent, 0);
    }
    fclose(fp);
}

void send_tar_gz_files(int client_socket, char *cmd) {
    FILE *fp;
    char buffer[1024];
    int file_size, bytes_sent;
    
    fp = fopen("files.tar.gz", "rb");

    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    send(client_socket, &file_size, sizeof(file_size), 0);

    
    while ((bytes_sent = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        send(client_socket, buffer, bytes_sent, 0);
    }
    fclose(fp);
}

void send_tar_gz_types(int client_socket, char *cmd) {
    FILE *fp;
    char buffer[1024];
    int file_size, bytes_sent;
    
    fp = fopen("types.tar.gz", "rb");

    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    send(client_socket, &file_size, sizeof(file_size), 0);

    
    while ((bytes_sent = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        send(client_socket, buffer, bytes_sent, 0);
    }
    fclose(fp);
}

void sendZipFileSize(int client_sock, char *cmd) {

    FILE* pipe = popen("tar czf size.tar.gz -T -", "w");
    
    for(int i=0;i<noOfFiles;i++) {
        //printf("filepath---- %s\n", filePaths[i]);
        fprintf(pipe, "%s\n", filePaths[i]);
    }

    pclose(pipe);
    send_tar_gz_size(client_sock, cmd);
}

void sendZipFileDate(int client_sock, char *cmd) {

    FILE* pipe = popen("tar czf date.tar.gz -T -", "w");
    
    for(int i=0;i<noOfFiles;i++) {
        fprintf(pipe, "%s\n", filePaths[i]);
    }

    pclose(pipe);
    send_tar_gz_date(client_sock, cmd);
}

void sendZipFileFiles(int client_sock, char *cmd) {

    FILE* pipe = popen("tar czf files.tar.gz -T -", "w");
    
    for(int i=0;i<noOfFiles;i++) {
        fprintf(pipe, "%s\n", filePaths[i]);
    }

    pclose(pipe);
    send_tar_gz_files(client_sock, cmd);
}

void sendZipFileTypes(int client_sock, char *cmd) {

    FILE* pipe = popen("tar czf types.tar.gz -T -", "w");
    
    for(int i=0;i<noOfFiles;i++) {
        fprintf(pipe, "%s\n", filePaths[i]);
    }

    pclose(pipe);
    send_tar_gz_types(client_sock, cmd);
}

void handle_client(int client_sock) {
    char buffer[BUFFER_SIZE] = {0};
    int valread;

    char *welcome_msg = "Hello from server";
    char *puka = "entra puka";
    send(client_sock, welcome_msg, strlen(welcome_msg), 0);

    while ((valread = read(client_sock, buffer, BUFFER_SIZE)) > 0) {
        printf("Received message from client: %s\n", buffer);

        char* result1 = getNthString(buffer, 1);
        if(strcmp(result1, "cli_cnt") == 0) {
            memset(G_BUFFER,0,strlen(G_BUFFER));
            char cc[20];
            sprintf(cc, "%d", clients_count);
            strcat(G_BUFFER, cc);
            send(client_sock, G_BUFFER, strlen(G_BUFFER), 0);
            memset(G_BUFFER,0,strlen(G_BUFFER));
        }
        if(strcmp(result1, "findfile") == 0) {
            memset(G_BUFFER,0,strlen(G_BUFFER));
            G_BUFFER[0]='\0';
            char *filename = getNthString(buffer, 2);
            search_files_in_directory("/Users/chaitanyabhavanam/AAA/", filename);
            send(client_sock, G_BUFFER, strlen(G_BUFFER), 0);
            memset(G_BUFFER,0,strlen(G_BUFFER));
            printf("\nDone Searching\n");
        } else if (strcmp(result1, "sgetfiles") == 0) {
            char *result2 = getNthString(buffer, 2);
            char *result3 = getNthString(buffer, 3);
            int size1 = atoi(result2);
            int size2 = atoi(result3);
            noOfFiles = 0;
            list_files_by_size("/Users/chaitanyabhavanam/AAA/", (off_t)size1, (off_t)size2);
            sendZipFileSize(client_sock, result1);
            sleep(2);
            remove("size.tar.gz");
            printf("\nDone...\n");
        } else if(strcmp(result1, "dgetfiles") == 0) {
            noOfFiles = 0;
            liste_files_by_date(getNthString(buffer, 2), getNthString(buffer, 3), "/Users/chaitanyabhavanam/AAA/");
            sendZipFileDate(client_sock, result1);
            sleep(2);
            remove("date.tar.gz");
            printf("\nDone...\n");
        } else if(strcmp(result1, "getfiles") == 0) {
            noOfFiles = 0;
            noOfFileNames = 0;
            int noOfArgs,i;
            for(i=2;i<9;i++) {
                if(getNthString(buffer, i) == NULL) {
                    noOfArgs = i-1;
                    break;
                }
                char *ts = getNthString(buffer, i);
                fileNames[noOfFileNames++] = (char*) malloc(strlen(ts) * sizeof(char));
                strcpy(fileNames[noOfFileNames-1], ts);
            }
            // printf("noa %d\n", noOfArgs);
            get_all_files("/Users/chaitanyabhavanam/AAA/");
            sendZipFileFiles(client_sock, result1);
            sleep(2);
            remove("files.tar.gz");
            printf("\nDone...\n");
        } else if(strcmp(result1, "gettargz") == 0) {
            noOfFiles = 0;
            noOfFileNames = 0;
            int noOfArgs,i;
            for(i=2;i<9;i++) {
                if(getNthString(buffer, i) == NULL) {
                    noOfArgs = i-1;
                    break;
                }
                char *ts = getNthString(buffer, i);
                fileNames[noOfFileNames++] = (char*) malloc(strlen(ts) * sizeof(char));
                strcpy(fileNames[noOfFileNames-1], ts);
            }
            get_all_files_types("/Users/chaitanyabhavanam/AAA/");
            sendZipFileTypes(client_sock, result1);
            sleep(2);
            remove("types.tar.gz");
            printf("\nDone...\n");
        } else if(strcmp(result1, "quit") == 0) {
            clients_count = clients_count-1;
            kill(getpid(), SIGTERM);
            close(client_sock);
        }

        memset(buffer,0,strlen(buffer));
    }

    // close(client_sock);
}

int main() {
    int server_sock, client_sock, valread;
    struct sockaddr_in serv_addr, cli_addr;
    char buffer[BUFFER_SIZE] = {0};
    int opt = 1;
    int addrlen = sizeof(serv_addr);

    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR , &opt, sizeof(opt))) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_sock, 5) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server started on port %d\n", PORT);

    int clients[10] = {0};
    clients_count = 0;

    while (1) {
        if ((client_sock = accept(server_sock, (struct sockaddr *)&cli_addr, (socklen_t *)&addrlen)) < 0) {
            perror("accept failed");
            exit(EXIT_FAILURE);
        }

        printf("New client connected: %s:%d\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

        clients[clients_count++] = client_sock;

        if (fork() == 0) {
            handle_client(client_sock);
            exit(0);
        }
    }

    return 0;
}
