#include <cstdlib>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <experimental/filesystem>
#include <iomanip>
#include <cstring>
#include <pthread.h>
 
#define err_exit(code, str)                   \
    {                                         \
        cerr << str << ": " << strerror(code) \
             << endl;                         \
        exit(EXIT_FAILURE);                   \
    }
 
using namespace std;
namespace fs = std::experimental::filesystem;
 
// Расширение файлов для просмотра
const string file_format = ".txt";
 
// Класс для работы с пулом потоков
class ThreadPool {
 
private:
    // Идентификаторы потоков в пуле
    vector<pthread_t> threads;
 
    // Очередь задач
    queue<void*> task_queue;
 
    // Указатель на задачу
    void* (*task_ptr) (void*);
 
    pthread_mutex_t mutex;
 
    // Функция для потоков из пула
    void* threadJob(void* classContext) {
        int err;
        int queue_size;
        void* task;
 
        while(true) {
            // Захватываем мьютекс, чтобы синхронизировать доступ к очереди задач
            err = pthread_mutex_lock(&mutex);
            if (err != 0)
                err_exit(err, "Cannot lock mutex"); 
            
            queue_size = task_queue.size();
            // Если очередь не пуста, то поток берёт аргументы новой задачи
            if (queue_size > 0) {
                task = task_queue.front();
                task_queue.pop();
            }
            
            // Освобождаем мьютекс, т.к. вся необходимая информация получена
            err = pthread_mutex_unlock(&mutex);
            if (err != 0)
                err_exit(err, "Cannot unlock mutex");
 
            if (queue_size > 0) {
                task_ptr(task);
            } else {
                return NULL;
            }
        }
    }
 
public:
    // В конструкторе задаём указатель на функцию-задачу и число потоков
    ThreadPool(void* (*taskPtr) (void*), const int threads_count) {
 
        int err = pthread_mutex_init(&mutex, NULL);
        if (err != 0)
            err_exit(err, "Cannot initialize mutex");
 
        task_ptr = taskPtr;
 
        for (int i = 0; i < threads_count; i++) {
            pthread_t thread;
            threads.push_back(thread);
        }
    }
 
    // Добавляем задачу в пул
    void add_task(void* task) {
        task_queue.push(task);
    }
 
    // Запускаем работу потоков
    void start() {
        int err;
        for (int i = 0; i < threads.size(); i++) {
            err = pthread_create(&threads[i], NULL, (void*(*)(void*))&ThreadPool::threadJob, (void*) this);
            if (err != 0)
                err_exit(err, "Cannot create thread");            
        }
 
        // Дожидаемся завершения всех потоков
        for (int i = 0; i < threads.size(); i++) {
            err = pthread_join(threads[i], NULL);
            if (err != 0)
                err_exit(err, "Cannot join thread");
        }
    }
};
 

void get_paths(const string &root, const string &format, vector<string> &file_paths) {
    // Используем рекурсивный итератор, чтобы обойти все внутренние директории
    for (auto itEntry = fs::recursive_directory_iterator(root);
         itEntry != fs::recursive_directory_iterator(); 
         ++itEntry) {
 
        const auto file_path = itEntry->path();
        const string fileExtension = file_path.extension().string();
        if (fileExtension == format) {
            file_paths.push_back(file_path.string());
        }
    }
}
 
// Структура, которая соержит параметры, необходимые для выполненния одной
// конкретной задачи пулом потоков
struct ThreadParams {
    // Путь к файлу
    string *file_path;
    // Искомая подстрока
    string *find_string;
    // Вектор, в который будут записаны номера строк со вхождениями подстроки
    vector<int> *line_numbers;
};
 
// Функция-задача, которая выполняется потоком из пула
void* Find_line(void *arg) {
    // Распаковываем параметры из аргумента функции
    ThreadParams* params = (ThreadParams*) arg;
    string file_path = *params->file_path;
    string find_string = *params->find_string;
    vector<int> *line_numbers = params->line_numbers;
 
    ifstream fileInput(file_path, ios_base::in);
    if (fileInput.is_open() == false) {
        return NULL;
    }
 
    int current_line = 0;
    string line;
    // Пока можем считать строку из файла
    while(getline(fileInput, line)) {
        // Увеличиваем счётчик строк
        current_line++;
 
        // Если нашли вхождение подстроки в текущей строке, 
        // то добавляем номер текущей строки
        if (line.find(find_string, 0) != string::npos) {
            (*line_numbers).push_back(current_line);
        }
    }
 
    fileInput.close();
 
    return NULL;
}
 
int main(int argc, char* argv[]) {
    // Необходимо как минимум 3 аргумента
    if (argc < 3) {
        cout << "Not enough arguments" << endl;
        exit(0);
    }
    // Искомая подстрока
    string search_string = argv[1];    
    
    // Директория, в которой будем искать файлы
    string path = argv[2];

    if (!fs::exists(path)) {
        cout << path << " directory doesn't exist." << endl;
        exit(0);
    }
 
    // Количество потоков в пуле
    int threads_count = 1;
    if (argc > 3) threads_count = atoi(argv[3]);
    
    // Ищем нужные нам файлы
    vector<string> file_paths;
    get_paths(path, file_format, file_paths);
 
    vector<vector<int>> result;
    result.resize(file_paths.size());
 
    // Инициализируем наш пул потоков
    ThreadPool threadPool(&Find_line, threads_count);
 
    ThreadParams* params = new ThreadParams[file_paths.size()];
    for (int i = 0; i < file_paths.size(); i++) {
        // Инициализируем параметры каждой задачи
        params[i].file_path = &file_paths[i];
        params[i].find_string = &search_string;
        params[i].line_numbers = &result[i];
 
        // Добавляем задачу в пул
        threadPool.add_task((void*) &params[i]);
    }
 
    // Запускаем пул потоков
    threadPool.start();
 
    // Выводим информацию о вхождениях подстроки в каждом подходящем файле
    for (int i = 0; i < result.size(); i++) {
        cout << file_paths[i] << ":" << endl;
        
        if (!result[i].empty()) {
            for (int j = 0; j < result[i].size(); j++) {
                cout << result[i][j] << endl;
            }
        } else {
            cout << "No search string." << endl;
        }
 
        cout << endl;
    }
 
    delete[] params;
}
