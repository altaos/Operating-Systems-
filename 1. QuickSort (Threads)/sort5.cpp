#include<fstream>
#include<iostream>
#include<stdio.h>
#include <stdlib.h>
#include<string>
#include<vector>
#include<queue>
#include<pthread.h>
#include<math.h>
#include <sys/sem.h>////////////////////////////////////////////

using namespace std;

static struct sembuf sop_lock = {
    0, -1, 0
};


struct Task
{
    string *mas;
    int first;
    int last;
};

struct TaskManager
{
    pthread_t *threads;
    vector<Task> tasks;
    int active_threads_count;
    int completed_tasks_count;//////////////////////////////////////////
    int count_tasks_ended;
    bool finish;
};

struct ForThread
{
    int number;
    TaskManager *task_manager;
    int semid;
};

pthread_cond_t finished_task;
pthread_cond_t start_task;

pthread_mutex_t start_task_mutex;
pthread_mutex_t mutex;

int PrintVector(string *m, int count);

//Быстрая сортировка
int QuickSort(string *mas, int first, int last)
{
  int i = first, j = last;
  string mid = mas[(i + j)/2];
  string temp;

  do
    {
      while (mas[i] < mid) ++i;
      while (mas[j] > mid) --j;

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

void* ThreadProc(void *data)
{
    ForThread *ft = (ForThread *)data;
    TaskManager *tm = ft->task_manager;

    printf("Thread %d working\n", ft->number);

    while(!tm->finish)
    {
        semop(ft->semid, &sop_lock, 1);

        printf("THREAD %d IS ACTIVE\n", ft->number);

        if(tm->tasks.size() > 0)
        {
            pthread_mutex_lock(&mutex);

            Task task;
            printf("Thread %d locked mutex!!!!!!!!!!!@@@@@@@@@@@@@@@@@@@@@@@@2\n", ft->number);
            bool ok = false;

            if(tm->tasks.size() > 0)
            {
                task = tm->tasks.front();
                ok = true;
                printf("Thread %d @@@@@@@@@@@@@@@@@@@@@@@@2\n", ft->number);
                    tm->tasks.erase(tm->tasks.begin() + 0);


                printf("Thread %d got task!!!!!!!!!!!@@@@@@@@@@@@@@@@@@@@@@@@2\n", ft->number);

                printf("thread %d task: first = %d, last = %d\n", ft->number, task.first, task.last);

                printf("\n\n");

                printf("\n\n");

                printf("Thread %d unlocked mutex\n", ft->number);
            }
            pthread_mutex_unlock(&mutex);

            if(ok)
                QuickSort(task.mas, task.first, task.last);

        }

            pthread_mutex_lock(&mutex);

            printf("====================================THREAD %d FINISHED TASK=======================\n", ft->number);

            (tm->completed_tasks_count)++;
            pthread_mutex_unlock(&mutex);
    }

    printf("\nTHREAD IS GOING AWAY %d\n\n", ft->number);
    pthread_exit(NULL);
}

//Считывание из файла
string *ReadFromFile(char *fileName, int *count)
{
    string str;
    vector<string> vsrt;
    ifstream input_file;

    input_file.open(fileName, ios::in);

    if (!input_file)
      {
        perror("Error: Open for reading");
        exit(1);
      }

    while(std::getline(input_file,str))
      {
        vsrt.push_back(str);
      }

    *count = vsrt.size();

    string* mas = new string[*count];

    for(int i = 0; i < *count; i++)
      {
        mas[i] = vsrt[i];
    }

    input_file.close();

    return mas;
}

//Запись в файл
int WriteToFile(char* fname, string *m, int count)
{
  ofstream output_file;

  output_file.open(fname, ios::out);

  if (!output_file)
    {
      perror("Error: Open for writing");
      exit(1);
    }

  for(int i = 0; i < count; i++)
    output_file<<m[i]<<endl;

  output_file.close();

  return 0;
}

//Вывод массива в консоль
int PrintVector(string *m, int count)
{
  for(int i = 0; i < count; i++)
    cout<<m[i]<<endl;

  return 0;
}

int CreateTasks(vector<Task> *q, string *mas, int count, int elc, int th_count, bool *created)
{
    printf("th_count = %d\n", th_count);
    if (th_count > 0)
    {
        if(!(*created))
        {
            printf("Creating\n");

            for(int i = 0; i < th_count; i++)
            {
                struct Task inf;
                inf.mas = mas;
                inf.first= i * elc;
                printf("count = %d\n", count);
                if((i+1)*elc - 1 < count)
                {
                    inf.last = (i + 1) * elc - 1;
                }
                else
                    inf.last = count - 1;
                printf("new task: first: %d last: %d\n", inf.first, inf.last);

                (*q).push_back(inf);
             }
            (*created)=true;
           CreateTasks(q, mas, count, elc, th_count, created);
        }
        else if(th_count > 1)
        {
            int th_c, j;

            th_c = 0;

            int first_index = (*q).size() - th_count;//индекс первого задания предыдущего поколения в очереди
           for(j = first_index; j < first_index + th_count - 1; j += 2)
           {
               struct Task inf;
               inf.first = (*q)[j].first;
               inf.mas = (*q)[j].mas;
               if(j+2 == first_index + th_count - 1)
                   inf.last = (*q)[j+2].last;
               else
                   inf.last = (*q)[j+1].last;
               printf("new task: first: %d last: %d\n", inf.first, inf.last);

               th_c++;
               (*q).push_back(inf);
           }

           CreateTasks(q, mas, count, elc, th_c, created);
        }
    }

    return 0;
}

void ThreadPool(string *mas, int count, int th_count)
{
    vector<Task> tasks;

    int elc = ceil((double)count / th_count);
    int threads_count = ceil((double)count / elc);
    bool createdTasks = false;

    CreateTasks(&tasks, mas, count, elc, threads_count, &createdTasks);

    printf("tasks = %d\n", tasks.size());

    printf("Tasks created\n");

    struct TaskManager task_manager;

    int i;

    task_manager.finish = false;
    task_manager.count_tasks_ended = 0;
    task_manager.threads = (pthread_t *)malloc(sizeof(pthread_t) * th_count);
    task_manager.tasks = tasks;
    task_manager.active_threads_count = threads_count;
    task_manager.completed_tasks_count = 0;

    int semid;

    semid = semget(IPC_PRIVATE, 1, 0666|IPC_CREAT);

    if(semid==-1)
    {
        perror("Semaphore creation failed\n");
        exit(EXIT_FAILURE);
    }

    printf("SEMVAL = %d\n",semctl(semid, 1, GETVAL, 0));
    printf("SEMID = %d\n",semid);

    int error;
    struct ForThread *forThread = new struct ForThread[threads_count];

    for(i = 0; i < threads_count; i++)
    {
        printf("i = %d\n", i);
        forThread[i].number = i;
        printf("number = %d\n", forThread[i].number);
        forThread[i].task_manager = &task_manager;
        forThread[i].semid = semid;

        printf("Creating thread %d\n", forThread[i].number);

        error = pthread_create(&(task_manager.threads[i]), NULL, ThreadProc, (void*)(&forThread[i]));

        if(error)
            cout<<"Error!\n";
    }

    int active_threads_count;

    printf("Threads_count = %d\n", threads_count);
    int step = 1;

    printf("task_manager.tasks.size = %d\n",task_manager.tasks.size());

    struct sembuf sop_unlock;
    active_threads_count = threads_count;
    sop_unlock.sem_num = 0;
    sop_unlock.sem_flg = 0;

    while(!task_manager.finish)
    {
        sop_unlock.sem_op = active_threads_count;
        semop(semid, &sop_unlock, 1);

        printf("\nSTEP %d\n\n", step++);

        while(task_manager.completed_tasks_count < active_threads_count)
        {
            //pthread_cond_wait(&finished_task, &task_mutex);
        }
        task_manager.completed_tasks_count = 0;
        task_manager.active_threads_count = 0;

        active_threads_count = active_threads_count / 2;//число заданий с каждым поколением уменьшается до разделить нацело на два раз                                                                                 //

       printf("============================================================================\n==================================================================\n");

        if(task_manager.tasks.empty())
        {
            task_manager.finish = true;

            printf("finish = true>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
        }

    }

    sop_unlock.sem_op = threads_count;
    semop(semid, &sop_unlock, 1);


    printf("YEAAAAAAAAAAAAAAAAH\n");

    task_manager.finish = true;

    for(int i = 0; i < threads_count; i++)
        pthread_join(task_manager.threads[i], NULL);

    semctl (semid, 0, IPC_RMID, 0);
}

int main(int argc, char**argv)
{
    string *arr;

    if(argc != 4)
    {
        cout<<"Argc != 4\n";
        exit(1);
    }

    int count;
    int threads_count = atoi(argv[3]);

    arr = ReadFromFile(argv[1], &count);

    pthread_cond_init(&finished_task, NULL);
    pthread_cond_init(&start_task, NULL);

    pthread_mutex_init(&start_task_mutex, NULL);
    pthread_mutex_init(&mutex, NULL);

    PrintVector(arr, count);

    cout<<"==============================start sort==============================\n";
    ThreadPool(arr, count, threads_count);
    cout<<"==================sssswwwsss============finish sort=================================\n";

    WriteToFile(argv[2], arr, count);


    pthread_cond_destroy(&finished_task);
    pthread_cond_destroy(&start_task);
    pthread_mutex_destroy(&start_task_mutex);
    pthread_mutex_destroy(&mutex);

}
