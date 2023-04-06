#include <cstdlib>
#include <iostream>
#include <cstring>
#include <cmath>
#include <vector>
#include <map>
#include <queue>
#include <pthread.h>
 
using namespace std;
 
#define err_exit(code, str)                   \
    {                                         \
        cerr << str << ": " << strerror(code) \
             << endl;                         \
        exit(EXIT_FAILURE);                   \
    }
 
// Мьютекс для синхронного доступа к очереди в reduce-функции
pthread_mutex_t q_mutex;
// Мьютекс для синхронизации работы с контейнерами
pthread_mutex_t k_mutex;
// Очередь, в которую помещаем ключи для reduce-функции
queue<char> key_queue;
 
// Структура, передаваемая в качестве аргумента потока map-функции
struct MapParams {
    // Указатель на элемент массива, с которого поток начнёт применять функцию поэлементно
    int* arr_ptr;
    
    // Количество элементов на поток
    int array_block;
    
    // Указатель на контейнер, в который будет записан результат map-функции
    multimap<char, int> *pContainer;
};
 
// Структура, передаваемая в качестве аргумента потока reduce-функции
struct ReduceParams {
    // Контейнер, к которому применяем reduce-функцию
    multimap<char, int> *pContainer;
 
    // Контейнер, в который записываем результат reduce-функции
    map<char, vector<int>> *pResult;
};
 
// Map-функция
void *Map_func(void *arg) {
    MapParams* params = (MapParams *)arg;
    int val;
    int err;
 
    for (int i = 0; i < params->array_block; i++) {
        val = (params->arr_ptr)[i];
        // Если число делится на 2 без остатка
        err = pthread_mutex_lock(&k_mutex);
        if(err != 0)
            err_exit(err, "Cannot lock k_mutex");

        if (val % 2 == 0) {
            // Записываем в контейнер с ключом 'e (even)'
            params->pContainer->insert(pair<char, int>('e', val));
            
        } else {
            // Иначе записываем с ключом 'o' (odd)
            params->pContainer->insert(pair<char, int>('o', val));
        }
        err = pthread_mutex_unlock(&k_mutex);
        if(err != 0)
            err_exit(err, "Cannot unlock k_mutex");

    }
    return NULL;
}
 
// Reduce-функция
void *Reduce_func(void *arg) {
    int err;
    ReduceParams* params = (ReduceParams *) arg;
 
    int queue_size;
    vector<int> values;
 
    // Ключ по которому будет применена reduce-функция
    char key;
    
    while(true) {
        // Захватываем мьютекс, чтобы синхронизировать доступ к очереди
        err = pthread_mutex_lock(&q_mutex);
        if(err != 0)
            err_exit(err, "Cannot lock q_mutex");

        
        queue_size = key_queue.size();
        // Если очередь не пуста, то поток берёт новый ключ
        if (queue_size > 0) {
            key = key_queue.front();
            key_queue.pop();
        }
        
        // Освобождаем мьютекс, т.к. вся необходимая информация получена
       err =  pthread_mutex_unlock(&q_mutex);
       if(err != 0)
            err_exit(err, "Cannot unlock q_mutex");
 
        if (queue_size > 0) {
            // Определяем диапазон элементов с текущим ключом
            auto range = params->pContainer->equal_range(key);
            
            for (auto it = range.first; it != range.second; it++) 
                values.push_back(it->second);
 
            err = pthread_mutex_lock(&k_mutex);
            if(err != 0)
                err_exit(err, "Cannot lock k_mutex");
 
            // Записываем (ключ, значение) в контейнер-результат
            params->pResult->insert(pair<char, vector<int>>(key, values));
            values.clear();
            err = pthread_mutex_unlock(&k_mutex);
            if(err != 0)
                err_exit(err, "Cannot unlock k_mutex");
 
        } else {
            return NULL;
        }
    }
}
 
// Основная функия map-reduce
std::map<char, vector<int>> mapReduce(int *test_array, size_t arr_len, void *(*Map_func)(void *), void *(*Reduce_func)(void *), size_t threads_count) {
    int err;
    // Блок для обработки одним потоком
    int array_block = arr_len / threads_count;
    // Остаток присваивается последнему потоку
    int remainder = arr_len % threads_count;
 
    // Потоки
    vector<pthread_t> threads(threads_count);
    
    // Здесь будет храниться результат map-функции
    multimap<char, int> res_map;
 
    // Параметры для map-функции на каждый поток
    MapParams *mapParams = new MapParams[threads_count];
 
    // Запускаем map-функции в потоках с заданными параметрами
    for (int i = 0; i < threads.size(); i++) {
 
        mapParams[i].arr_ptr = &test_array[i * array_block];
        mapParams[i].array_block = array_block;
        if (i == threads_count - 1) {
            mapParams[i].array_block += remainder;
        }
        mapParams[i].pContainer = &res_map;
 
        err = pthread_create(&threads[i], NULL, Map_func, &mapParams[i]);
        if (err != 0)
            err_exit(err, "Cannot create thread");
    }
 
    // Ждём завершения map-функций
    for (int i = 0; i < threads.size(); i++) {
        pthread_join(threads[i], NULL);
    }
 
    cout << "Multimap content:" << endl;
    for (const auto &i: res_map) {
        cout << "{ Key: " << i.first << "; Value = " << i.second << " }" << endl;
    }
 
    delete[] mapParams;
    
    // Добавляем ключи для reduce-функции
    key_queue.push('e');
    key_queue.push('o');
 
    // Итоговый результат после выполнения Reduce_func
    map<char, vector<int>> resReduce;

    // Параметры для всех потоков
    ReduceParams reduceParams;
    reduceParams.pContainer = &res_map;
    reduceParams.pResult = &resReduce;
 
    // Запускаем reduce-функции в потоках
    for (int i = 0; i < threads_count; i++) {
        err = pthread_create(&threads[i], NULL, Reduce_func, &reduceParams);
        if (err != 0)
            err_exit(err, "Cannot create thread");
    }
 
    // ждём завершения reduce-функции
    for (int i = 0; i < threads.size(); i++) {
        pthread_join(threads[i], NULL);
    }

    return resReduce;
}
 
 
int main(int argc, char *argv[]) {
    if (argc < 3) {
        cout << "Not enough arguments" << endl;
        exit(0);
    }
 
    // Размер массива
    unsigned int arr_len = atoi(argv[1]);    
    // Кол-во потоков
    unsigned int threads_count = atoi(argv[2]);

    int err = pthread_mutex_init(&q_mutex, NULL);
    if (err != 0)
        err_exit(err, "Cannot initialize q_mutex");
 
    err = pthread_mutex_init(&k_mutex, NULL);
    if (err != 0)
        err_exit(err, "Cannot initialize k_mutex");

    if (threads_count > arr_len) {
        cout << "Array length is less than the number of threads" << endl;
        exit(-1);
    }
    
    // Создаём и заполняем массив
    int* test_array = new int[arr_len];
    for (int i = 0; i < arr_len; i++) {
        test_array[i] = i;
    }
    
    map<char, vector<int>> result = mapReduce(test_array, arr_len, &Map_func, &Reduce_func, threads_count);
 
    cout << "Result map:" << endl;

    for (auto const& x : result) {
        string out_res;
        out_res.push_back(x.first);
        out_res += " : {";
        for (auto const& el : x.second){
            out_res += std::to_string(el) + ", ";
        }

        out_res.pop_back();
        out_res.pop_back();
        cout << out_res + "} " << endl;
    }
    
 
    pthread_mutex_destroy(&q_mutex);
    pthread_mutex_destroy(&k_mutex);
    delete[] test_array; 
}
