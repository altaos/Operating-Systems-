#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include<pthread.h>
#include <string.h>
#include "HashTable2.h"
#include <stdbool.h>
#include <sys/sem.h>
#include <linux/limits.h>
#include <errno.h>

#define COMMAND_LENGTH 128
#define RESPONSE_LENGTH 40
#define PATH_LENGTH 128
#define HOME_DIR "/home/alina" //свой домашний директорий

struct Command
{
    char command[COMMAND_LENGTH];
};

struct InterSt
{
    char com[4];
    char dir[PATH_LENGTH];
};

struct QUEUE_TYPE
{
    int clientfd;
    int count;
    char resp[1024];
};

struct TASK
{
    int clientfd;
    struct HashTable *clients;
    struct Command command;
    int command_size;
    bool finish;
};

#include "QueueForTasks.h"

struct ServerData
{
    int threads_count;
    bool *finish;
};

int semid;

static struct sembuf sop_lock = {
    0, -1, 0
};

static struct sembuf sop_unlock = {
    0, 1, 0
};

pthread_mutex_t mutex;

struct InterSt Interpret(char str[COMMAND_LENGTH])
{
    int i = 0;
    while (str[i] ==  ' ' && i < COMMAND_LENGTH)
        i++;

    struct InterSt newCom;
    memset(newCom.dir, '\0', sizeof(newCom.dir)/sizeof(char));
    memset(newCom.com, '\0', sizeof(newCom.dir)/sizeof(char));

    switch (str[i])
    {
    case 'l' : if (str[i+1] == 's') strncpy(newCom.com, str + i, 2); break;
    case 'c' : if (str[i+1] == 'd')
        {
            strncpy(newCom.com, str + i, 2);
            i += 2;
            while (str[i] ==  ' ' && i < COMMAND_LENGTH)
                i++;
            strncpy(newCom.dir, str + i, COMMAND_LENGTH - i - 2);
        }
        break;
    default: ;
    }

    return newCom;
}

struct QUEUE_TYPE LsCommand(struct HashTable *table, int clientfd)
{
    struct QUEUE_TYPE response;
    memset(response.resp, '\0', sizeof(response.resp)/sizeof(char));

    char dirc[128];
    strcpy(dirc, GetDirect(*table, FindIndex(table, clientfd)));
    int result = chdir(dirc);

    if(result == 0)
    {
        printf("directory changed\n");
    }
    else
    {
        switch(result)
        {
        case EACCES: perror("Permission denied\n");
            break;
        case EIO:	 perror("An input output error occured\n");
            break;
        case ENAMETOOLONG: perror("Path is to long\n");
            break;
        case ENOTDIR: perror("A component of path not a directory\n");
            break;
        case ENOENT: perror("No such file or directory\n"); printf("enoent\n");
            break;
        default: printf("Couldn't change directory");
        }
    }

    char path[RESPONSE_LENGTH];
    response.count = 0;
    FILE *fp = popen("ls", "r");

    while (fgets(path, RESPONSE_LENGTH, fp ) != NULL)
    {
        strcat(response.resp, path);
        (response.count)++;
    }

    pclose(fp);
    response.clientfd = clientfd;

    return response;
}

struct QUEUE_TYPE CdCommand(char dir[128], struct HashTable *table, int clientfd)
{
    char newdir[PATH_LENGTH];
    struct QUEUE_TYPE response;
    memset(response.resp, '\0', sizeof(response.resp)/sizeof(char));
    int index = FindIndex(table, clientfd);
    int result;

    printf("dir = %s\n", dir);
    if (strcmp(dir, "") == 0)
    {
        strcpy(newdir, HOME_DIR);
        result = chdir(newdir);
    }
    else
    {
        if (dir[0]== '/')
        {
            strcpy(newdir, dir);
            result = chdir(newdir);
        }
        else if(dir[0] == '.' && dir[1] == '.')
        {
            int i, k;
            char tmp[PATH_LENGTH];
            strcpy(tmp, GetDirect(*table, index));
            for(i = 0; i < PATH_LENGTH; i++)
                if(tmp[i] == '/') k = i;
            strncpy(newdir, tmp, k);
            result = chdir(newdir);
        }
        else
        {
            strcpy(newdir, GetDirect(*table, index));
            printf("GetDirect = %s\n", newdir);
            strcat(newdir, "/");
            printf("cur_dir %s\n", dir);
            strcat(newdir, dir);
            printf("new_dir %s\n", newdir);
            result = chdir(newdir);
        }
    }
    if(result == 0)
    {
        printf("directory changed\n");

        if (dir != NULL)
        {
            strcpy(response.resp, newdir);
            ChangeHashTable(table, index, newdir);
            printf("dir in HashTable = %s\n", GetDirect(*table, index));
        }
        else
        {
            printf("dir is NULL\n");
            strcpy(response.resp, "Error\n");
        }
    }
    else
    {
        switch(result)
        {
        case EACCES: perror("Permission denied\n");           
            strcpy(response.resp, "Permission denied\n");
            break;
        case EIO:	 perror("An input output error occured\n");
            strcpy(response.resp, "An input output error occured\n");
            break;
        case ENAMETOOLONG: perror("Path is too long\n");
            strcpy(response.resp, "Path is too long\n");
            break;
        case ENOTDIR: perror("A component of path is not a directory\n");
            strcpy(response.resp, "A component of path is not a directory\n");
            break;
        case ENOENT: perror("No such file or directory\n"); printf("enoent\n");
            strcpy(response.resp, "No such file or directory\n");
            break;
        default: perror("Couldn't change directory");
            strcpy(response.resp, "Couldn't change directory");
        }
    }

    response.count = 1;
    response.clientfd = clientfd;

    return response;
}

void *ThreadProc(void *data)
{
    printf("Hahahaha\n");
    struct QueueTask *queue = (struct QueueTask *)data;
    struct TASK task;
    while(1)
    {
        semop(semid, &sop_lock, 1);
        pthread_mutex_lock(&mutex);

        task = GetFromQueueTask(queue);

        pthread_mutex_unlock(&mutex);

        if (task.finish)
        {
            printf("Thread finished\n");
            break;
        };

        struct InterSt st = Interpret(task.command.command);
        struct QUEUE_TYPE resp;
        printf("!!!!!!!!!!command = %s!!!!!!!!!!\n", st.com);
        if (strcmp(st.com, "ls") == 0)
        {
            resp = LsCommand(task.clients, task.clientfd);
        }
        else
            if (strcmp(st.com, "cd") == 0)
            {
                printf("cd newdir = %s\n", st.dir);
                resp = CdCommand(st.dir, task.clients, task.clientfd);
            }
            else
            {
                printf("%s : command not found\n", st.com);
                resp.clientfd = task.clientfd;
                resp.count = 1;
                strcpy(resp.resp, "Command not found\n");
            }

//        int bytes;
//        int bytes_sent = 0;

        send(task.clientfd, &(resp.count), sizeof(int), 0);
        send(task.clientfd, resp.resp, sizeof(char) * resp.count * RESPONSE_LENGTH, 0);

//        char *buf = (char *)malloc(sizeof(char) * resp.count * RESPONSE_LENGTH);
//        while(bytes_sent < sizeof(char) * resp.count * RESPONSE_LENGTH)
//        {
//            memset(buf, '\0', sizeof(buf)/sizeof(char));
//            strcpy(buf, resp.resp);
//            bytes = send(task.clientfd, &buf + bytes_sent, sizeof(char) * resp.count * RESPONSE_LENGTH - bytes_sent, MSG_NOSIGNAL);

//            if(bytes > 0)
//            {
//                bytes_sent += bytes;
//            }
//        }
//        free(buf);
    }
    return NULL;
}

void *Server(void *data)
{
    struct ServerData *sData = (struct ServerData *)data;
    int listenfd;
    struct sockaddr_in addr;

    const int MAX_EPOLL_EVENTS = 100;
    const int BACK_LOG = 100;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd < 0)
    {
        perror("Socket creation error\n");
        exit(1);
    }

    //назначение сокета неблокирующим
    fcntl(listenfd, F_SETFL, O_NONBLOCK);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(3425);
    addr.sin_addr.s_addr = INADDR_ANY;
    if(bind(listenfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("Bind error");
        exit(2);
    }

    //перевод сервера в режим ожидания запросов
    listen(listenfd, BACK_LOG);

    struct HashTable clientsTable = HashInit();

    int efd = epoll_create(MAX_EPOLL_EVENTS);

    struct epoll_event listenev;
    listenev.events = EPOLLIN /*есть данные для чтения*/|EPOLLPRI/*есть срочные данные для чтения*//*|EPOLLET*/;
    listenev.data.fd = listenfd;
    if(epoll_ctl(efd, EPOLL_CTL_ADD, listenfd, &listenev) < 0)
    {
        perror("Epoll fd add\n");
        exit(1);
    }

    socklen_t client;

    struct epoll_event events[MAX_EPOLL_EVENTS], connev; //структуры epoll_event для всех наступивших событий
    struct sockaddr_in cliaddr; //структура для сохранения в ней адреса подключаемого клиента

    int events_count = 1;
    int threads_count = sData->threads_count;

    struct QueueTask queueTask;
    InitializeQueueTask(&queueTask);

    semid = semget(IPC_PRIVATE, 1, 0666|IPC_CREAT);

    if(semid==-1)
    {
        perror("Semaphore creation failed\n");
        exit(EXIT_FAILURE);
    }

    pthread_t *threads = (pthread_t *)malloc(sizeof(pthread_t) * threads_count);

    int i;
    for(i = 0; i < threads_count; i++)////////////////////////////////////////////////////////////////////////
    {
        printf("creating thread %d\n", i);
        int error = pthread_create(&(threads[i]), NULL, ThreadProc, (void*)(&queueTask));

        if(error)
            printf("Thread creation error\n");
    }

    while(!(*(sData->finish)))
    {
        //printf("1\n");
        // Блокирование до готовности одного или нескольких дескрипторов
        int nfds = epoll_wait(efd, events, MAX_EPOLL_EVENTS, /*timeout*/1000);
        if(nfds < 1)
            continue;

        printf("====================================================\n");

        int n;
        for(n = 0; n < nfds; n++)
        {
            //Готов слушающий дескриптор
            if(events[n].data.fd == listenfd)
            {
                client = sizeof(cliaddr);
                //Принять запрос на соединение от клиента
                int connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &client);
                if(connfd < 0)
                {
                    perror("Accept error\n");
                    continue;
                }

                // Недостаточно места в массиве ожидания, закрываем соединение
                if(events_count == MAX_EPOLL_EVENTS - 1)
                {
                    printf("Event array is full\n");
                    close(connfd);
                    continue;
                }

                // Добавление клиентского дескриптора в массив ожидания
                fcntl(connfd, F_SETFL, O_NONBLOCK); //Назначение сокета неблокирующим
                connev.data.fd = connfd;
                connev.events = EPOLLIN/*есть данные для чтения*| EPOLLOUT /*готов для записи*//*| EPOLLET/*edge-triggered*/ | EPOLLRDHUP/*сокет клиента закрыл соединение*/;;
                if(!epoll_ctl(efd, EPOLL_CTL_ADD, connfd, &connev) < 0) //Добавляем, теперь epoll отслеживает события и для этого сокета
                {
                    perror("Epoll fd add error\n");
                    close(connfd);
                    continue;
                }
                printf("%d connected\n", connfd);

                char* ch = "/home/alina\0";
                //strcpy(mas[connfd], ch);
                //Вставляем в хеш таблицу id и текущий директорий данного клиента
                if(InsertToHashTable(&clientsTable, ch, connfd) == -1)
                {
                    printf("Hash is full\n");
                    continue;
                }

                ++events_count;

            }
            //Готов клиентский дескриптор
            else
            {
                printf("2\n");
                //Получаем дескриптор из структур epoll_event массива events
                int fd = events[n].data.fd;

                struct TASK inf;

                if(events[n].events & EPOLLIN)//Поступили данные от клиента
                {
                    printf("3\n");
                    char buf[sizeof(struct Command)];
                    int bytes_read = 0;
                    int bytes_left = sizeof(struct Command);

                    while(bytes_left > 0 && !(*(sData->finish)))
                    {
                        int bytes = recv(fd, buf + bytes_read, bytes_left, MSG_WAITALL);
                        if(bytes > 0)
                        {
                            bytes_read += bytes;
                            bytes_left -= bytes;
                            printf("bytes_read: %d bytes_left: %d\n", bytes_read, bytes_left);
                        }
//                        printf("3.1\n");
                        if(bytes == 0) // Соединение разорвано, удаляем сокет из epoll и списка
                        {
                            epoll_ctl(efd, EPOLL_CTL_DEL, fd, &connev);
                            --events_count;

                            DelFromHashTable(&clientsTable, fd);
                            printf("%d disconnected\n", fd);

                            close(fd);

                            break;
                        }
//                        printf("3.2\n");
                    }

                    printf("Read %d of %d bytes\n", bytes_read, sizeof(struct Command));

                    if (bytes_left > 0)
                    {
                        printf("Continue\n");
                        continue;
                    }

                    inf.command = *((struct Command *)buf);

                    inf.finish = false;
                    inf.clientfd = fd;
                    inf.command_size = bytes_read;
                    inf.clients = &clientsTable;
                    printf("command %s     clientfd %d    com_size %d\n", inf.command.command, inf.clientfd, inf.command_size);
                    pthread_mutex_lock(&mutex);
//                    printf("=========SSSSSSSSSSSSS=============\n");
                    AddToQueueTask(&queueTask, inf);
                    semop(semid, &sop_unlock, 1);
                    pthread_mutex_unlock(&mutex);
                }

                if(events[n].events & EPOLLRDHUP)//Клиент закрыл соединение
                {
                    printf("4\n");
                    epoll_ctl(efd, EPOLL_CTL_DEL, fd, &connev);
                    --events_count;
                    close(fd);
                    DelFromHashTable(&clientsTable, fd);
                    continue;
                }
//                printf("5\n");

            }

        }
    }
    printf("Creating finish tasks\n");

    for(i = 0; i < threads_count; i++)
    {
        struct TASK task;
        task.finish = true;
        pthread_mutex_lock(&mutex);
        AddToQueueTask(&queueTask, task);
        semop(semid, &sop_unlock, 1);
        pthread_mutex_unlock(&mutex);
    }

    printf("Finish tasks created\n");

    void *status;

    for(i = 0; i < threads_count; i++)//Waiting for threads
    {
        pthread_join(threads[i], &status);
    }
    printf("All threads joined\n");

    close(listenfd);
    free(threads);
    DistructHash(&clientsTable);
    printf ("Server stopped\n");

    return NULL;
}

int main(int argc, char** argv)
{
    pthread_mutex_init(&mutex, NULL);

    struct ServerData data;
    bool finish = false;
    data.finish = &finish;
    data.threads_count = 4;
    pthread_t thread;
    int error = pthread_create(&thread, NULL, Server, (void*)(&data));

    if(error)
        printf("Server thread creation error!\n");

    while(!finish)
    {
        char command[5];
        char *pos;
        fgets(command, sizeof(command), stdin);
        if ((pos = strchr(command, '\n')) != NULL)
            *pos = '\0';
        if(strcmp(command, "stop") == 0)
        {
            finish = true;
        }
    }

    void *status;
    pthread_join(thread, &status);

    pthread_mutex_destroy(&mutex);

    return 0;
}
