#include<iostream>
#include<string>
#include<stdio.h>
#include<stdlib.h>
#include<vector>
#include<fstream>
#include<pthread.h>
#include<math.h>

using namespace std;

struct inf{
  string *m;
  int first;
  int last;
  //int current;
  string *resmas;
  int *countres;
  int *finished_tasks_count;
  int *found_index;
  int *ins_index;
  int *active_threads_count;
};

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


pthread_mutex_t stage_finished_mutex;////////////////////////////////////////////////////
pthread_cond_t stage_finished_cond;//////////////////////////////////////////////
pthread_mutex_t search_finished_mutex;////////////////////////////////////////////////////
pthread_cond_t search_finished_cond;//////////////////////////////////////////////
pthread_mutex_t insertion_finished_mutex;////////////////////////////////////////////////////
pthread_cond_t insertion_finished_cond;//////////////////////////////////////////////

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

void* ThreadProc(void* argv)
{
  inf* arg = (inf*)argv;
  QuickSort(arg->m, arg->first, arg->last);

  //return NULL;
  int i = 0, j=0;
  int *countres = arg->countres;
  int *finished_tasks_count = arg->finished_tasks_count;
  int *active_threads_count = arg->active_threads_count;
  int *found_index = arg->found_index;
  int *ins_index = arg->ins_index;
  string *mas = arg->m;
  string *res = arg->resmas;
  
  pthread_mutex_lock(&mutex);
  (*(arg->finished_tasks_count))++;
  pthread_mutex_unlock(&mutex);

  pthread_mutex_lock(&stage_finished_mutex);
  
  pthread_cond_wait(&stage_finished_cond, &stage_finished_mutex);
  pthread_mutex_unlock(&stage_finished_mutex);

  while((arg->first) <= (arg->last))
  {
      pthread_mutex_lock(&mutex);

      printf("found_index: %d", *found_index);
      cout<<"first: "<<arg->first<<" last: "<<arg->last<<endl;
      cout<<"mas[first]: "<<mas[(arg->first)]<<" mas[*found_index]: "<<mas[*found_index]<<endl;
      if(mas[(arg->first)]<=mas[*found_index])
      {
          printf("OK\n");
          *found_index = (arg->first);
      }

      (*(arg->finished_tasks_count))++;

      pthread_mutex_unlock(&mutex);

      pthread_mutex_lock(&search_finished_mutex);
      pthread_cond_wait(&search_finished_cond, &search_finished_mutex);
      pthread_mutex_unlock(&search_finished_mutex);
      printf("Finished waiting search: first: %d last: %d\n", arg->first, arg->last);

      string tmp;
      int tmp_index;
      int insertion_index = *ins_index;
      int curr_index = *found_index;
      printf("insertion_index = %d\n",*ins_index);
      printf("curr_index = %d\n",*found_index);

      if((arg->last) < curr_index)
      {
          tmp = mas[(arg->last)];
          tmp_index = (arg->last)+1;
          for(i = (arg->last); i>(arg->first); i--)
              mas[i]=mas[i-1];
      }
      else if((arg->first) == curr_index)
      {
          tmp = mas[curr_index];
          tmp_index = insertion_index;
      }
      pthread_mutex_lock(&mutex);
      (*finished_tasks_count)++;
      pthread_mutex_unlock(&mutex);

      pthread_mutex_lock(&insertion_finished_mutex);
      //printf("Waiting insertion, curr_index = %d, tmp_index = %d\n", curr_index, tmp_index);
      pthread_cond_wait(&insertion_finished_cond, &insertion_finished_mutex);////////////////////////////
      pthread_mutex_unlock(&insertion_finished_mutex);
      //printf("Finished waiting insertion: first: %d last: %d\n", first, last);


      //pthread_mutex_lock(&mutex);
      if((arg->last) < curr_index)
      {
          mas[tmp_index] = tmp;
          arg->first++;
          arg->last++;
          //(*found_index) = (arg->first);
          //arg->current++;
      }
      else if((arg->first) == curr_index)
      {
          mas[tmp_index] = tmp;
          arg->first++;
          if(arg->first > arg->last)
          {
              (*active_threads_count)--;
          }
          (*ins_index)++;
      }

      (*found_index) = (*ins_index);
      pthread_mutex_lock(&mutex);
      (*finished_tasks_count)++;
      pthread_mutex_unlock(&mutex);

      pthread_mutex_lock(&stage_finished_mutex);
      pthread_cond_wait(&stage_finished_cond, &stage_finished_mutex);
      pthread_mutex_unlock(&stage_finished_mutex);
  }

  pthread_exit(NULL);
}

string *ReadFromFile(char *fname, int *count)
{
  string str;
  vector<string> vsrt;
  ifstream input_file;

  input_file.open(fname, ios::in);

  if (!input_file)
    {
      perror("Open for reading");
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

int PrintVector(string *m, int count)
{
  for(int i = 0; i < count; i++)
    cout<<m[i]<<endl;

  return 0;
}

int WriteToFile(char* fname, string *m, int count)
{
  ofstream output_file;

  output_file.open(fname, ios::out);

  if (!output_file)
    {
      perror("Open for writing");
      exit(1);
    }

  for(int i = 0; i < count; i++)
    output_file<<m[i]<<endl;
  
  output_file.close();
  return 0;
}

int main(int argc, char** argv)
{
  
  if (argc != 4)
    {
      perror("Argc != 4");
      exit(1);
    }

  int count;
  string *mas;
  int threads_count = atoi(argv[3]);

  mas = ReadFromFile(argv[1], &count);

  string *resmas = new string[count];

  int elc = ceil((double)count / threads_count);
  int th_count = ceil((double)count / elc);
  pthread_t* threads = new pthread_t[th_count];
  struct inf* infm = new struct inf[th_count];

  int countres = 0;


  pthread_mutex_init(&stage_finished_mutex, NULL);////////////////////////////////////////////////
  pthread_cond_init (&stage_finished_cond, NULL);/////////////////////////////////////////
  pthread_mutex_init(&search_finished_mutex, NULL);////////////////////////////////////////////////
  pthread_cond_init (&search_finished_cond, NULL);/////////////////////////////////////////
  pthread_mutex_init(&insertion_finished_mutex, NULL);////////////////////////////////////////////////
  pthread_cond_init (&insertion_finished_cond, NULL);/////////////////////////////////////////

  int finished_tasks_count = 0;
  int active_threads_count = th_count;
  int found_index = 0;
  int ins_index = 0;
  //Создание нитей
  for(int i = 0; i < th_count; i++)
  {
      infm[i].m =  mas;
      infm[i].first = i * elc;
      infm[i].resmas = resmas;
      infm[i].countres = &countres;
      infm[i].found_index = &found_index;
      infm[i].ins_index = &ins_index;
      infm[i].finished_tasks_count = &finished_tasks_count;
      infm[i].active_threads_count = &active_threads_count;
      if((i+1)*elc - 1 <= count)
      {
          infm[i].last = (i + 1) * elc - 1;
      }
      else
          infm[i].last = count - 1;

      pthread_create(&threads[i], NULL, ThreadProc, (void*)(&infm[i]));

  }
  while(finished_tasks_count < active_threads_count)
  {
      //wait();
  }
  int h;

  finished_tasks_count = 0;
  
  printf("Signal! Sort finished\n");	
  pthread_cond_broadcast(&stage_finished_cond);

  printf("After sort: \n");
  PrintVector(mas, count);
  for(h = 0; h<count; h++)
  {
      printf("                                            STEP %d:\n", h);
      printf("ins_index: %d, %d\n", *(infm[0].ins_index),*(infm[1].ins_index));
      if(h > 0)
      {
          printf("After insertion:\n");
          PrintVector(mas, count);
      }

      int curr_active_threads_count = active_threads_count;
      printf("active threads: %d\n", curr_active_threads_count);
      
      printf("h: %d count: %d\n", h, count);
      
      printf("real_tasks_count: %d tasks_count: %d\n", *(infm[0].finished_tasks_count), finished_tasks_count);
      printf("finished: %d th_count: %d\n", finished_tasks_count, curr_active_threads_count);
      while(finished_tasks_count < curr_active_threads_count)
      {
          //wait();
      }
      printf("After search: ");
      
      PrintVector(mas, count);

      finished_tasks_count = 0;
      for(int i = 0; i < curr_active_threads_count; i++)
      {
          printf("Signal! Search finished\n");
          pthread_cond_signal(&search_finished_cond);/////////////////////////////////////
      }


      while(finished_tasks_count < curr_active_threads_count)
      {
          //wait();
      }
      finished_tasks_count = 0;
      for(int i = 0; i < curr_active_threads_count; i++)
      {
          printf("Signal! Insertion finished\n");
          pthread_cond_signal(&insertion_finished_cond);/////////////////////////////////////
      }


      while(finished_tasks_count < curr_active_threads_count)
      {
          //wait();
      }
      finished_tasks_count = 0;

      for(int i = 0; i < th_count; i++)
      {
          printf("first: %d\n", infm[i].first);
          printf("last: %d\n", infm[i].last);
          printf("ins_index: %d\n", *(infm[i].ins_index));
      }


      for(int i = 0; i < curr_active_threads_count; i++)
      {
          printf("Signal! Stage finished\n");
          pthread_cond_signal(&stage_finished_cond);
      }
  }

  for(int i = 0; i < th_count; i++)
  {
      pthread_join(threads[i], NULL);
  }

  PrintVector(mas, count);
  printf("\noutput:\n");
  PrintVector(mas, count);

  WriteToFile(argv[2], resmas, count);

  delete[] resmas;
  delete[] threads;

  pthread_mutex_destroy(&stage_finished_mutex);////////////////////////////////////
  pthread_cond_destroy(&stage_finished_cond);//////////////////////////////
  pthread_mutex_destroy(&search_finished_mutex);////////////////////////////////////
  pthread_cond_destroy(&search_finished_cond);//////////////////////////////
  pthread_mutex_destroy(&insertion_finished_mutex);////////////////////////////////////
  pthread_cond_destroy(&insertion_finished_cond);//////////////////////////////
  pthread_mutex_destroy(&mutex);
  
  pthread_exit(NULL);
} 
