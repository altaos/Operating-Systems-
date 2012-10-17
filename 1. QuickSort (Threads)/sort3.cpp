#include<fstream>
#include<iostream>
#include<stdio.h>
#include <stdlib.h>
#include<string>
#include<vector>
#include<queue>
#include<pthread.h>
#include<math.h>

using namespace std;

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
    int count_tasks_ended;
    bool finish;
};

struct ForThread
{
    int number;
    TaskManager *task_manager;
};

pthread_cond_t finished_task;
pthread_cond_t start_task;
pthread_mutex_t task_mutex;

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

    while(!tm->finish)
    {
        pthread_mutex_lock(&task_mutex);

        pthread_cond_wait(&start_task, &task_mutex);

        pthread_mutex_unlock(&task_mutex);

        if(!tm->finish)
        {
            pthread_mutex_lock(&task_mutex);

            tm->active_threads_count++;

            if(!tm->tasks.empty())
            {
                Task task = tm->tasks.front();
                tm->tasks.erase(tm->tasks.begin() + 0);

                pthread_mutex_unlock(&task_mutex);

                QuickSort(task.mas, task.first, task.last);
                //(fm->active_threads_count)--;

                pthread_cond_signal(&finished_task);
            }
            else
                pthread_mutex_unlock(&task_mutex);
        }
    }
    //sleep(0);
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
    if(th_count > 0)/////////////////////
    {
        if(!(*created))
        {
            //int elc = ceil((double)count / threads_count);
            //int th_count = ceil((double)count / elc);
            cout<<"Count mas = "<<count<<endl;
            for(int i = 0; i < th_count; i++)
            {
                struct Task inf;
                inf.mas = mas;
                inf.first= i * elc;
                if((i+1)*elc - 1 < count)
                {
                    inf.last = (i + 1) * elc - 1;
                }
                else
                    inf.last = count - 1;

                printf("new task: first: %d last: %d\n", inf.first, inf.last);

                (*q).push_back(inf);
             }
            (*created) = true;
            CreateTasks(q, mas, count, elc, th_count, created);
        }
        else if(th_count > 1)
        {
           int th_c, j;

           th_c = 0;
           cout<<"th_c = "<<th_c<<endl;

           int first_index = (*q).size() - th_count;
           for(j = first_index; j < first_index + th_count - 1; j += 2)
           {
               struct Task inf;
               inf.first = (*q)[j].first;
               inf.mas = (*q)[j].mas;
               if(j+2 == first_index + th_count - 1)
                   inf.last = (*q)[j+2].last;
               else
                   inf.last = (*q)[j+1].last;
               //inf.last = (*q)[j+1].last;
               printf("new task: first: %d last: %d\n", inf.first, inf.last);

               th_c++;
               (*q).push_back(inf);

           }

           CreateTasks(q, mas, count, elc, th_c, created);
        }
    }
    //////////////////////////////////////
    return 0;
}

void CreateThreads(int threads_count, pthread_t *threads, struct TaskManager *m)
{
    int error;

    for(int i = 0; i < threads_count; i++)
    {
        struct ForThread forThread;

        forThread.number = i;
        forThread.task_manager = m;

        error = pthread_create(&threads[i], NULL, ThreadProc, (void*)(&forThread));
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
    int th_c = threads_count;
    bool createdTasks = false;

    CreateTasks(&tasks, mas, count, elc, threads_count, &createdTasks);

    cout<<"Size = "<<tasks.size()<<endl;
    for(int i = 0; i < tasks.size(); i++)
    {
        cout<<"First = "<<tasks[i].first<<" Last = "<<tasks[i].last<<endl;
    }

    /*struct TaskManager task_manager;

    task_manager.finish = false;
    task_manager.count_tasks_ended = 0;
    task_manager.threads = (pthread_t *)malloc(sizeof(pthread_t) * th_c);

    CreateThreads(th_c, task_manager.threads, &task_manager);

    int task_completed = 0;

    while(task_manager.tasks.size() >= 1)
    {
        while(task_completed < task_manager.active_threads_count)
        {
            pthread_cond_wait(&finished_task, &task_mutex);
            task_completed++;
        }

        task_completed = 0;
        task_manager.active_threads_count = 0;

        pthread_cond_signal(&start_task);
    }

    task_manager.finish = true;

    for(int i = 0; i < th_c; i++)
        pthread_join(task_manager.threads[i], NULL);

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

    pthread_mutex_init(&task_mutex, NULL);

    //PrintVector(arr, count);

    cout<<"==============================start sort==============================\n";
    ThreadPool(arr, count, threads_count);
    cout<<"==================sssswwwsss============finish sort=================================\n";

    WriteToFile(argv[2], arr, count);

    pthread_cond_destroy(&finished_task);
    pthread_cond_destroy(&start_task);
    pthread_mutex_destroy(&task_mutex);

}
