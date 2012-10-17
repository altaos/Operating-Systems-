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

    pthread_mutex_lock(&mutex);
    (tm->completed_tasks_count)++;
    pthread_mutex_unlock(&mutex);

    while(!tm->finish)
    {
        /*printf("Thread %d working inside while\n", ft->number);

        pthread_mutex_lock(&start_task_mutex);
        printf("Thread %d locked start_task_mutex\n", ft->number);

        pthread_cond_wait(&start_task, &start_task_mutex);

        printf("Thread %d unlocked start_task_mutex\n", ft->number);
        pthread_mutex_unlock(&start_task_mutex);*/

        semop(ft->semid, &sop_lock, 1);

        printf("THREAD %d IS ACTIVE\n", ft->number);

        /*if(!tm->finish)
        {*/

        //(tm->active_threads_count)++;

        //printf("active_threads_count = %d\n", tm->active_threads_count);

        //if(!tm->tasks.empty())
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
            //PrintVector(task.mas, 8);
            printf("\n\n");

            printf("Thread %d unlocked mutex\n", ft->number);
            }
            pthread_mutex_unlock(&mutex);

            if(ok)
                QuickSort(task.mas, task.first, task.last);
            //(fm->active_threads_count)--;

            //pthread_cond_signal(&finished_task);

        }

            //pthread_cond_signal(&finished_task);
            //printf("Thread %d sent signal\n", ft->number);

            pthread_mutex_lock(&mutex);

            //printf("Thread %d locked mutex again\n", ft->number);


            printf("====================================THREAD %d FINISHED TASK=======================\n", ft->number);

            /*printf("finished tasks: %d\n",tm->completed_tasks_count);

            printf("Thread %d unlocked mutex again\n", ft->number);*/
            (tm->completed_tasks_count)++;
            pthread_mutex_unlock(&mutex);
        //}

    }
    //sleep(0);





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

/*void* ThreadProc(void* argv)
{
    threadInf* arg = (threadInf*)argv;
    QuickSort(arg->m, arg->first, arg->last);

    return NULL;
}*/

int CreateTasks(vector<Task> *q, string *mas, int count, int elc, int th_count, bool *created)
{
    //vector<Task> (*q0) = *q0;
    printf("th_count = %d\n", th_count);
    /*while*/if (th_count > 0)///////////////////// не while, а if, рекурсия же, больше нуля, а не единицы - чтобы предусмотреть случай с одной нитью
    {
        if(!(*created))
        {
            printf("Creating\n");
            //int elc = ceil((double)count / threads_count);
            //int th_count = ceil((double)count / elc);

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
            (*created)=true;///////////////////////////////////////////////////////////////////////////////////////
           CreateTasks(q, mas, count, elc, th_count, created);
        }
        else if(th_count > 1)//т.к. ранее мы предусмотрели случай с одной нитью, задание для него уже создалось там, и здесь мы его уже должны отсечь, чтобы не попасть в бесконечную рекурсию
        {
            //if(th_count<3)printf("Yet created, th_count = %d\n", th_count);
            int th_c, j;
            /*if(th_count / 2 != 0)
                th_c = th_count / 2 + 1;
            else
                th_c = th_count / 2;*/
            th_c = 0;////////////////////////////////

            int first_index = (*q).size() - th_count;//индекс первого задания предыдущего поколения в очереди
           for(j = first_index; j < first_index + th_count - 1; j += 2)
           {
               struct Task inf;
               inf.first = (*q)[j].first;
               inf.mas = (*q)[j].mas;
               if(j+2 == first_index + th_count - 1)///////////////////
                   inf.last = (*q)[j+2].last;///////////////////////
               else////////////////////////////////////////
                   inf.last = (*q)[j+1].last;
               printf("new task: first: %d last: %d\n", inf.first, inf.last);

               th_c++;//думаю, так попроще и покрасивее будет
               (*q).push_back(inf);
           }

           /*if (j == th_count - 1)
           {
               struct Task inf = q[j]; нет смысла отправлять на сортировку уже отсортированный массив в одиночку

               q.push_back(inf);
           }*/

           //printf("th_c = %d\n", th_c);

           CreateTasks(q, mas, count, elc, th_c, created);
        }
    }

    //////////////////////////////////////

    //printf("Exiting CreateTasks\n");
    return 0;
}

void CreateThreads(int threads_count, pthread_t **threads, struct TaskManager *m)//если передать просто массив, он дублируется для функции и не возвращается
{
    int error;

    for(int i = 0; i < threads_count; i++)
    {
        struct ForThread forThread;
        printf("i = %d\n", i);
        forThread.number = i;
        printf("number = %d\n", forThread.number);
        forThread.task_manager = m;

        error = pthread_create(&(*threads)[i], NULL, ThreadProc, (void*)(&forThread));//*threads
        printf("thread %d created\n", forThread.number);
        if(error)
            cout<<"Error!\n";
        //////////////////////////////////////
    }
}

void ThreadPool(string *mas, int count, int th_count)
{
    vector<Task> tasks;

    int elc = ceil((double)count / th_count);
    int threads_count = ceil((double)count / elc);
    bool createdTasks = false;

    CreateTasks(&tasks, mas, count, elc, threads_count, &createdTasks);//передаем вектор по указателю, иначе внутри функции

    printf("tasks = %d\n", tasks.size());

    printf("Tasks created\n");

    struct TaskManager task_manager;

    int i;

    task_manager.finish = false;
    task_manager.count_tasks_ended = 0;
    task_manager.threads = (pthread_t *)malloc(sizeof(pthread_t) * th_count);
    task_manager.tasks = tasks;//самое главное и забыла инициализировать
    task_manager.active_threads_count = threads_count;////////////////////////////////////
    task_manager.completed_tasks_count = 0;

    //CreateThreads(th_count, &(task_manager.threads), &task_manager);







    int semid;


    static struct sembuf sop_unlock = {
        0, threads_count, 0
    };


    semid = semget(IPC_PRIVATE, 1, 0666|IPC_CREAT);

    if(semid==-1)
    {
        perror("Semaphore creation failed\n");
        exit(EXIT_FAILURE);
    }

    printf("SEMVAL = %d\n",semctl(semid, 1, GETVAL, 0));
    printf("SEMID = %d\n",semid);

    union semun
    {
        int val;
        struct semid_ds *buf;
        ushort * array;
    } argument;

    argument.val = 0;

    if(semctl(semid, 0, SETVAL, argument) == -1)
    {
        printf("Cannot set semaphore value.\n");
    }
    else
    {
        printf("Semaphore initialized.\n");
    }

    semop(semid, &sop_unlock, 1);












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

        //printf("thread %d created\n", forThread.number);
        if(error)
            cout<<"Error!\n";
        //////////////////////////////////////
    }





    //printf("Threads created\n");
    int active_threads_count;
    int task_completed = 0;

    printf("Threads_count = %d\n", threads_count);
    int step = 1;

    //printf("\nSTEP %d\n\n", step++);
    /*for (i = 0; i < threads_count; i++)
    {
        printf("Sending signal 'start task'\n");
        pthread_cond_signal(&start_task);
    }*/
    //pthread_cond_broadcast(&start_task);

    printf("task_manager.tasks.size = %d\n",task_manager.tasks.size());


    while(!task_manager.finish/*task_manager.tasks.size() >= 1*/)
    {
        printf("\nSTEP %d\n\n", step++);
        //active_threads_count = task_manager.active_threads_count;
        /*if(active_threads_count>1)  */
        while(task_manager.completed_tasks_count < threads_count/*task_manager.active_threads_count*/)
        {
            //printf("completed_tasks: %d; threads_count = %d\n",task_manager.completed_tasks_count, threads_count);
            /*printf("Waiting for signal 'finished_task'");
            pthread_cond_wait(&finished_task, &task_mutex);
            task_completed++;*/
        }
        task_manager.completed_tasks_count = 0;
        //task_completed = 0;
        task_manager.active_threads_count = 0;

       printf("============================================================================\n==================================================================\n");
        /*PrintVector(tasks[0].mas, 8);
        printf("\n\n");*/

        if(task_manager.tasks.empty())
        {
            task_manager.finish = true;//это нужно здесь, чтобы нити при пустой очереди не пошли на еще один круг и не уперлись в условную переменную
        //(а то из while при этом мы потом вышли бы, и нити так и остались бы висеть, дожидаясь сигнала)
            printf("finish = true>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
        }


        semop(semid, &sop_unlock, 1);
        /*for (i = 0; i < threads_count; i++)
        {
            printf("Sending signal 'start task' again\n");
            pthread_cond_signal(&start_task);
        }*/
        //pthread_cond_broadcast(&start_task);

    }
    printf("YEAAAAAAAAAAAAAAAAH\n");

    task_manager.finish = true;

    for(int i = 0; i < threads_count; i++)
        pthread_join(task_manager.threads[i], NULL);

    semctl (semid, 0, IPC_RMID, 0);
    /*if (!(*are_thr_created))
    {
        int elc = ceil((double)count / th_count);
        int threads_count = ceil((double)count / elc);

        cur_t = new struct threadInf*[threads_count];

        pthread_t thread;
        //Содание заданий
        CreateTasks(tasks, mas, count, tasks_created, elc, threads_count);

        //Создание нитей
        for(int i = 0; i < threads_count; i++)
        {
            tasks[i].num = i;
            cur_t[i] = &(tasks[i]);
            tasks.erase(tasks.begin() + 0);
            pthread_create(&thread, NULL, ThreadProc, (void*)(cur_t[i]));
            act_threads.push_back(thread);
        }

        ThreadPool(cur_t, mas, count, th_count,are_thr_created, tasks_created, act_threads, tasks);
    }
    else
    {
        int finished_tasks_count = 0;
        int act_threads_count = 0;
        //Пока очередь (вектор заданий) не пуста
        while(!tasks.empty())
        {
            while(finished_tasks_count < act_threads_count)
            {
                pthread_cond_wait(&finished_task, &mutex);
            }


        }
    }*/
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
    //bool *are_threads_created = false;
    //bool *tasks_created = false;
    //vector<pthread_t> active_threads;
    //vector<threadInf*> tasks;
    //struct threadInf **curr_tasks;

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
