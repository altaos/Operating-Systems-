#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include<pthread.h>

#define RESPONSE_LENGTH 1024
#define COMMAND_LENGTH 1024

struct Command
{
    char command[COMMAND_LENGTH];
};

struct Inf
{
    int socket;
    int count;
    bool *finish;
};

int Proc(int sock, int count)
{
    int i = 0;
    struct sockaddr_in addr;

    if(sock < 0)
    {
        perror("socket");
        exit(1);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(3425); // порт
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if(connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("connect");
        exit(2);
    }

    struct Inf inf;
    inf.socket = sock;
    bool finish = false;
    inf.finish = &finish;
    inf.count = count;

    const int n = 5;
    int command_count = 0;
    struct Command com[n];
    strcpy(com[0].command,"ls");
    strcpy(com[1].command,"cd JDK");
    strcpy(com[2].command,"ls");
    strcpy(com[3].command,"cd jdk1.7.0_09");
    strcpy(com[4].command,"cd ..");

    while(i < count)
    {
        int bytes;
        int bytes_sent = 0;

        char buf[COMMAND_LENGTH];
        while(bytes_sent < sizeof(char)*COMMAND_LENGTH)
        {
            printf("ind = %d\n", i%n);
            memset(buf, '\0', sizeof(buf)/sizeof(char));
            strcpy(buf, com[i%n].command);
            bytes = send(sock, &buf + bytes_sent, sizeof(struct Command) - bytes_sent, MSG_NOSIGNAL);

            if(bytes > 0)
            {
                bytes_sent += bytes;
            }
        }
        printf("%d sent: %s\n", getpid(), buf);

        i++;
        command_count++;

        int bytes_read = 0;
        int resp_count;
        int bytes_left = sizeof(int);

        while(bytes_left > 0)
        {
            int bytes = recv(sock, &resp_count + bytes_read, bytes_left, MSG_WAITALL | MSG_DONTWAIT);

            if(bytes > 0)
            {
                bytes_read += bytes;
                bytes_left -= bytes;
            }
            else if(bytes == 0)
            {
                printf("Server closed connection.\n");
                break;
            }
        }

        if (bytes_left > 0)
        {
            break;
        }

        bytes_read = 0;
        bytes_left = sizeof(char) * resp_count * RESPONSE_LENGTH;
        char* f = (char *)malloc(bytes_left);

        while(bytes_left > 0)
        {
            int bytes = recv(sock, f + bytes_read, bytes_left, MSG_WAITALL | MSG_DONTWAIT);

            if(bytes > 0)
            {
                bytes_read += bytes;
                bytes_left -= bytes;
            }
            else if(bytes == 0)
            {
                printf("Server closed connection.\n");
                free(f);
                break;
            }
        }

        if (bytes_read < sizeof(char) * resp_count * RESPONSE_LENGTH)
        {
            continue;
        }


        printf("Proc № %d count = %d %s\n", getpid(), resp_count, f);

        free(f);
    }

    close(sock);

    return 0;
}

int main(int argc, char** argv)
{
    int sock;
    struct sockaddr_in addr;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0)
    {
        perror("Socket error\n");
        exit(1);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(3425);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    Proc(sock, 5);

    printf("THE END \n");

    return 0;
}
