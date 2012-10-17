#include<fstream>
#include<iostream>
#include<stdio.h>
#include <stdlib.h>
#include<string>
#include<vector>
#include<math.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <wait.h>

using namespace std;

static struct sembuf sop_unlock2 = {
    0, 1, 0
};


struct Task
{
    int first;
    int last;
    bool finish;
};

int PrintVector(string **m, int count);

//Быстрая сортировка
int QuickSort(string **mas, int first, int last)
{
    int i = first, j = last;
    string mid = *(mas[(i + j)/2]);
    string *temp;

    do
    {
        while ((*(mas[i])) < mid) ++i;
        while ((*(mas[j])) > mid) --j;

        if (i <= j)
        {
            temp = mas[i];
            mas[i] = mas[j];
            mas[j] = temp;
            i++; j--;
        }
    } while (i <= j);

    if (first < j) QuickSort(mas, first, j);
    if (i < last) QuickSort(mas, i, last);

    return 0;
}

void Proc(int mas_shmid, int tasks_pipe_fd, int proc_num, int semid)
{
    printf("Process %d started\n", proc_num);

    string **mas;

    if((mas = (string **)shmat(mas_shmid, 0, 0)) < (string **)0)
    {
        perror("Segment 'mas' attaching failed\n");
        exit(EXIT_FAILURE);
    }

    while(1)
    {
        Task task;

        printf("Proc %d reading from pipe\n", proc_num);
        read(tasks_pipe_fd, &task, sizeof(Task));
        printf("Proc %d read from pipe %d\n", proc_num, task.finish);

        if(task.finish)
            break;

        QuickSort(mas, task.first, task.last);

        semop(semid, &sop_unlock2, 1);
    }

    printf("Proc %d finished\n", getpid());
    shmdt(mas);
}

//Считывание из файла
int ReadFromFile(char *fileName, int *count)
{
    string *str = new string();
    vector<string *> vsrt;
    ifstream input_file;

    input_file.open(fileName, ios::in);

    if (!input_file)
    {
        perror("Error: Open for reading");
        exit(1);
    }

    while(std::getline(input_file,*str))
    {
        vsrt.push_back(str);
        str = new string();
    }

    *count = vsrt.size();

    int mas_shmid;
    if((mas_shmid = shmget(IPC_PRIVATE, sizeof(string *) * vsrt.size(), IPC_CREAT|0666)) < 0)
    {
        perror("Segment 'mas_shmid' creation failed\n");
        exit(EXIT_FAILURE);
    }

    string **mas = (string **)shmat(mas_shmid, 0, 0);

    for(int i = 0; i < *count; i++)
    {
        mas[i] = vsrt[i];
    }
    shmdt(mas);
    input_file.close();

    return mas_shmid;
}

//Запись в файл
int WriteToFile(char* fname, string **m, int count)
{
    ofstream output_file;

    output_file.open(fname, ios::out);

    if (!output_file)
    {
        perror("Error: Open for writing");
        exit(1);
    }

    for(int i = 0; i < count; i++)
        output_file<<*(m[i])<<endl;

    output_file.close();

    return 0;
}

//Вывод массива в консоль
int PrintVector(string **m, int count)
{
    for(int i = 0; i < count; i++)
        cout<<*(m[i])<<endl;

    return 0;
}

int CreateTasks(vector<Task> *q, int pipe_write_fd, int array_count, int elc, int tasks_count, bool *created, int proc_count)
{
    if (tasks_count > 0)
    {
        if(!(*created))
        {
            for(int i = 0; i < tasks_count; i++)
            {
                struct Task inf;
                inf.first= i * elc;
                if((i+1)*elc - 1 < array_count)
                {
                    inf.last = (i + 1) * elc - 1;
                }
                else
                    inf.last = array_count - 1;
                printf("New task: first: %d last: %d\n", inf.first, inf.last);
                inf.finish = false;
                write(pipe_write_fd, &inf, sizeof(Task));
                (*q).push_back(inf);
            }
            (*created)=true;
        }
        else if(tasks_count > 1)
        {
            int j;

            int first_index = (*q).size() - tasks_count;//индекс первого задания предыдущего поколения в очереди
            for(j = first_index; j < first_index + tasks_count - 1; j += 2)
            {
                struct Task inf;
                inf.first = (*q)[j].first;
                if(j+2 == first_index + tasks_count - 1)
                    inf.last = (*q)[j+2].last;
                else
                    inf.last = (*q)[j+1].last;
                printf("New task: first: %d last: %d\n", inf.first, inf.last);
                inf.finish = false;
                write(pipe_write_fd, &inf, sizeof(Task));
                (*q).push_back(inf);
            }
        }
        else
        {
            struct Task inf;
            inf.first = 0;
            inf.last = array_count-1;
            inf.finish = false;
            printf("New task: first: %d last: %d\n", inf.first, inf.last);
            write(pipe_write_fd, &inf, sizeof(Task));
            (*q).push_back(inf);

            for(int i = 0; i < proc_count; i++)
            {
                struct Task inf;
                inf.finish = true;
                printf("New task: 'finish'\n");
                write(pipe_write_fd, &inf, sizeof(Task));
                (*q).push_back(inf);
            }
        }
    }

    return 0;
}

void TaskManager(int mas_shmid, int count, int pr_count)
{
    vector<Task> tasks;

    int elc = ceil((double)count / pr_count);
    int proc_count = ceil((double)count / elc);
    bool createdTasks = false;

    int processes[proc_count];

    int pfd[2];

    if (pipe(pfd) == -1)
    {
        perror("Pipe creation error\n");
        exit(EXIT_FAILURE);
    }

    printf("Tasks created\n");

    int semid = semget(IPC_PRIVATE, 1, 0666|IPC_CREAT);

    if(semid==-1)
    {
        perror("Semaphore creation failed\n");
        exit(EXIT_FAILURE);
    }

    printf("SEMVAL 0 = %d\n",semctl(semid, 0, GETVAL, 0));

    for(int i = 0; i < proc_count; i++)
    {
        printf("Creating process %d\n", i);
        if((processes[i] = fork()) < 0)
        {
            exit(-1);
        }
        else if(processes[i] == 0)
        {
            close(pfd[1]);
            Proc(mas_shmid, pfd[0], i, semid);
            close(pfd[0]);
            exit(0);
        }
    }

    close(pfd[0]);

    int tasks_count;

    int step = 1;

    tasks_count = proc_count;

    struct sembuf sop_lock2;
    sop_lock2.sem_num = 1;
    sop_lock2.sem_flg = 0;

    bool finish = false;

    printf("\n=================================================STEP %d started================================================Creating tasks:\n\n", step++);
    CreateTasks(&tasks, pfd[1], count, elc, tasks_count, &createdTasks, proc_count);
    while(!finish)
    {
        sop_lock2.sem_op = -1*tasks_count;
        semop(semid, &sop_lock2, 1);
        printf("Step %d tasks completed\n", step);
        printf("\n=================================================STEP %d started================================================\n\nCreating tasks:\n", step++);

        CreateTasks(&tasks, pfd[1], count, elc, tasks_count, &createdTasks, proc_count);

        printf("Task_count = %d\n", tasks_count);

        tasks_count = tasks_count / 2;

        if (tasks_count < 1)
            finish = true;

    }

    close(pfd[1]);

    int status;

    for(int i = 0; i < proc_count; i++)
    {
        waitpid(processes[i], &status, 0);
    }

    semctl (semid, 0, IPC_RMID, 0);
    semctl (semid, 1, IPC_RMID, 0);
}

int main(int argc, char**argv)
{
    if(argc != 4)
    {
        cout<<"Argc != 4\n";
        exit(1);
    }

    int count;
    int proc_count = atoi(argv[3]);
    if(proc_count < 1)
    {
        cout<<"Proc_count < 1\n";
        exit(1);
    }

    int mas_shmid = ReadFromFile(argv[1], &count);

    cout<<"==============================start sort=================================\n";
    TaskManager(mas_shmid, count, proc_count);
    cout<<"==============================finish sort=================================\n";



    string **mas = (string **)shmat(mas_shmid, 0, 0);

    WriteToFile(argv[2], mas, count);


    shmdt(mas);
    shmctl(mas_shmid, IPC_RMID, NULL);

    return 0;
}
