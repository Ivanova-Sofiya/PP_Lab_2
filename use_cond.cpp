#include <cstdlib> 
#include <iostream>
#include <unistd.h> 
#include <cstring> 
#include <pthread.h>
#include <cmath> 
using namespace std;

#define err_exit(code, str)                   \
    {                                         \
        cerr << str << ": " << strerror(code) \
             << endl;                         \
        exit(EXIT_FAILURE);                   \
    }

int status = 0;
const int TASKS_COUNT = 6; 
int task_list[TASKS_COUNT]; // Массив заданий

int current_task = 0; // Указатель на текущее задание
pthread_mutex_t mutex; // Мьютекс
pthread_cond_t cond;

void do_task(int task_num) 
{ 
    int res_arr[1000];
    int count;
 
    for (int i = 0; i < 1000; i++) {
        count = 0;
        for (int j = 0; j < 1000; j++)
            count += j;
        res_arr[i] = i + count;
    }
}

void *cond_thread_job(void *arg) {
    int err;
    int task_num;
    pthread_t thread_id = pthread_self(); // получаем id потока

    
    err = pthread_mutex_lock(&mutex); 
    if (err != 0) 
        err_exit(err, "Cannot lock mutex");


    task_num = current_task;
    current_task++;
    status = 1;

    err = pthread_mutex_unlock(&mutex); 
    if(err != 0) 
        err_exit(err, "Cannot unlock mutex");

    do_task(task_num);
    cout << "Thread with id " << thread_id << " is doing task №" << task_num << endl;  
    
    // err = pthread_cond_wait(&cond, &mutex);
    // if(err != 0) 
    //     err_exit(err, "Cannot wait on condition variable");
    while(status == 1) {

    }
 

    task_num = current_task;
    current_task++;
    do_task(task_num);

    cout << "Thread with id " << thread_id << " is doing task №" << task_num << endl;  
    cout << "And this is the end of the program!" << endl;
 

    return NULL;
}


void *thread_job(void *arg) 
{
    int task_num; 
    int err;
    pthread_t thread_id = pthread_self(); // получаем id потока

    // Перебираем в цикле доступные задания
    while(true) { 
        // Захватываем мьютекс для исключительного доступа
        err = pthread_mutex_lock(&mutex); 
        if (err != 0) 
            err_exit(err, "Cannot lock mutex"); 
        
        // Запоминаем номер текущего задания, которое будем исполнять 
        task_num = current_task;
        
       // Освобождаем мьютекс
        err = pthread_mutex_unlock(&mutex); 
        if(err != 0) 
            err_exit(err, "Cannot unlock mutex"); 
    

        if(task_num < TASKS_COUNT - 1) {
            do_task(task_num);
            // Сдвигаем указатель текущего задания на следующее
            current_task++;
            cout << "Thread with id " << thread_id << " is doing task №" << task_num << endl;  
            }
        else {
            // pthread_cond_signal(&cond);
            status = 0;
            return NULL;
            } 
    } 
}

int main() 
{ 

    pthread_t thread1, thread2; // Идентификаторы потоков
    int err; // Код ошибки
    
    // Инициализируем массив заданий случайными числами
    for(int i=0; i<TASKS_COUNT; ++i) 
        task_list[i] = rand() % TASKS_COUNT;

    // Инициализируем мьютекс
    err = pthread_mutex_init(&mutex, NULL); 
    
    if(err != 0) 
        err_exit(err, "Cannot initialize mutex");

    err = pthread_cond_init(&cond, NULL);
    if (err != 0)
        err_exit(err, "Cannot initialize condition");
 
    // Создаём потоки
    err = pthread_create(&thread1, NULL, cond_thread_job, NULL); 
    if(err != 0) 
        err_exit(err, "Cannot create thread 1"); 

    usleep(5000);
    
    err = pthread_create(&thread2, NULL, thread_job, NULL); 
    if(err != 0) 
        err_exit(err, "Cannot create thread 2"); 
    
    pthread_join(thread1, NULL); 
    pthread_join(thread2, NULL); 
    
    // Освобождаем ресурсы, связанные с мьютексом
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
}