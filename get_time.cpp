#include <pthread.h>
#include <cstdlib>
#include <iostream>
#include <cstring>
#include <cmath>
#include <chrono>
 
using namespace std;
 
#define err_exit(code, str)                   \
    {                                         \
        cerr << str << ": " << strerror(code) \
             << endl;                         \
        exit(EXIT_FAILURE);                   \
    }
 
// Типы синхронизации потоков
enum SYNK_TYPES { MUTEX, SPINLOCK };  
 
// Тип синхронизации, который мы используем 
const SYNK_TYPES sync_type = SYNK_TYPES::MUTEX;
 
pthread_mutex_t mutex;
pthread_spinlock_t spinlock;
 
int current_task = 0;
 
void do_task()
{
    int res_arr[1000];
    int count;
 
    for (int i = 0; i < 1000; i++) {
        res_arr[i] = i + 1000;
    }
}
 

void *thread_job(void *arg) 
{
    int task_no; 
    int err;
    pthread_t thread_id = pthread_self(); // получаем id потока

    // Захватываем мьютекс для исключительного доступа
        // Блокируем участок используемым типом синхронизации
    if (sync_type == SYNK_TYPES::MUTEX) {
        err = pthread_mutex_lock(&mutex);
        if (err != 0)
            err_exit(err, "Cannot lock mutex");
    }     
    else if (sync_type == SYNK_TYPES::SPINLOCK) {
        err = pthread_spin_lock(&spinlock);
        if (err != 0)
          err_exit(err, "Cannot lock spinlock");
    }
        
    // Запоминаем номер текущего задания, которое будем исполнять 
    task_no = current_task;
    // Сдвигаем указатель текущего задания на следующее
    current_task++;
    // do_task();

    if (sync_type == SYNK_TYPES::MUTEX) {
        err = pthread_mutex_unlock(&mutex);
        if (err != 0)
            err_exit(err, "Cannot unlock mutex");    
    }
    else if (sync_type == SYNK_TYPES::SPINLOCK) {
            err = pthread_spin_unlock(&spinlock);
            if (err != 0)
                err_exit(err, "Cannot unlock spinlock");
        }
    return NULL;
}

 
int main(int argc, char* argv[])
{
    if (argc < 2) {
        cout << "Not enough arguments" << endl;
 
        exit(0);
    }
 
    // Количество потоков считываем как аргумент программы
    int threads_count = atoi(argv[1]);
    if (threads_count <= 0) {
        cout << "Count of threads must be > 0" << endl;
 
        exit(0);
    }
 
    // Код ошибки
    int err;
    // Идентификаторы потоков
    pthread_t* threads = new pthread_t[threads_count];
 
    // Инициализируем мьютекс
    err = pthread_mutex_init(&mutex, NULL);
    if (err != 0)
        err_exit(err, "Cannot initialize mutex");
 
    // Инициализируем спинлок    
    err = pthread_spin_init(&spinlock, PTHREAD_PROCESS_PRIVATE);
    if (err != 0)
        err_exit(err, "Cannot initialize spinlock");
    
    auto begin = chrono::steady_clock::now();
 
    // Создаём потоки
    for (int i = 0; i < threads_count; i++) {
        err = pthread_create(threads + i, NULL, &thread_job, NULL);
        if(err != 0) 
        err_exit(err, "Cannot create thread 2");
    }
 
    // Дожидаемся завершения всех потоков
    for (int i = 0; i < threads_count; i++) {
        pthread_join(threads[i], NULL);
    }
 
    // Заканчиваем замер времени после завершения всех потоков
    auto end = chrono::steady_clock::now();
    
    auto diff_time = chrono::duration_cast<std::chrono::microseconds>(end - begin);
    cout << "Program running time:" << diff_time.count() << " mcs" <<  endl;
 
    // Освобождаем ресурсы, связанные с мьютексом и спинлоком
    pthread_mutex_destroy(&mutex);
    pthread_spin_destroy(&spinlock);
 
    // Освобождаем ресурсы связанные с хранением информации о потоках
    delete[] threads;
}
