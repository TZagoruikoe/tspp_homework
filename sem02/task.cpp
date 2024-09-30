#include <iostream>
#include <cstdlib>
#include <pthread.h>

#define TEST
#define N 20

struct MyConcurrentQueue {
    int queue[N];
    int first_pos, last_pos;
    bool isempty;

    pthread_mutex_t mutex;
    pthread_cond_t condput, condget;

    void init();
    void put(int elem);
    int get();
};

typedef struct {
    int count;
    MyConcurrentQueue* q;
} pthrData;

void MyConcurrentQueue::init() {
    this->isempty = true;
    this->first_pos = 0;
    this->last_pos = -1;

    pthread_mutex_init(&this->mutex, NULL);
    pthread_cond_init(&this->condput, NULL);
    pthread_cond_init(&this->condget, NULL);
}

void MyConcurrentQueue::put(int elem) {
    pthread_mutex_lock(&this->mutex);

    while (true) {
        if (this->isempty || (this->first_pos != 0 && 
                              this->last_pos != this->first_pos - 1) ||
                             (this->first_pos == 0 && 
                              this->last_pos != N - 1)) {
            this->last_pos++;
            if (this->last_pos > N - 1) {
                this->last_pos = 0;
            }
            this->queue[last_pos] = elem;
    
            this->isempty = false;
    
            pthread_cond_signal(&this->condget);
            break;
        }
        else {
            pthread_cond_wait(&this->condput, &this->mutex);
        }
    }

#ifdef TEST
    std::cerr << "Put elem = " << elem << "\n";
#endif

    pthread_mutex_unlock(&this->mutex);
}

int MyConcurrentQueue::get() {
    pthread_mutex_lock(&this->mutex);

    int elem;

    while(true) {
        if (!this->isempty) {
            elem = this->queue[this->first_pos];
            if (this->first_pos == this->last_pos) {
                this->isempty = true;
            }
            
            this->first_pos++;
            if (this->first_pos > N - 1) {
                this->first_pos = 0;
            }
    
            pthread_cond_signal(&this->condput);

#ifdef TEST
            std::cerr << "Got elem = " << elem << "\n";
#endif

            pthread_mutex_unlock(&this->mutex);
    
            return elem;
        }
        else {
            pthread_cond_wait(&this->condget, &this->mutex);
        }
    }
}

void* run_get(void* thread_data) {
    pthrData* data = (pthrData*) thread_data;
    int count_g = data->count;
    MyConcurrentQueue* q_g = data->q;

    for (int i = 0; i < count_g; i++) {
        q_g->get();
    }

    return NULL;
}

void* run_put(void* thread_data) {
    pthrData* data = (pthrData*) thread_data;
    int count_p = data->count;
    MyConcurrentQueue* q_p = data->q;

    for (int i = 0; i < count_p; i++) {
        q_p->put(rand() % 1000);
    }

    return NULL;
}

void test1(MyConcurrentQueue* q, int count) {
    int threads_count = 4;

    pthread_t* threads = (pthread_t*)malloc(threads_count * sizeof(pthread_t));
    pthrData* threadData = (pthrData*)malloc(threads_count * sizeof(pthrData));

    threadData[0].q = q;
    threadData[0].count = count;

    pthread_create(&(threads[0]), NULL, run_put, &threadData[0]);

    for (int i = 1; i < threads_count; i++) {
        threadData[i].q = q;
        threadData[i].count = 1;
        pthread_create(&(threads[i]), NULL, run_get, &threadData[i]);
    }

    for (int i = 0; i < threads_count; i++) {
        pthread_join(threads[i], NULL);
    }

    free(threads);
    free(threadData);
}

void test2(MyConcurrentQueue* q, int count) {
    int threads_count = 4;

    pthread_t* threads = (pthread_t*)malloc(threads_count * sizeof(pthread_t));
    pthrData* threadData = (pthrData*)malloc(threads_count * sizeof(pthrData));

    threadData[0].q = q;
    threadData[0].count = count;

    pthread_create(&(threads[0]), NULL, run_get, &threadData[0]);

    for (int i = 1; i < threads_count; i++) {
        threadData[i].q = q;
        threadData[i].count = 2;
        pthread_create(&(threads[i]), NULL, run_put, &threadData[i]);
    }

    for (int i = 0; i < threads_count; i++) {
        pthread_join(threads[i], NULL);
    }

    free(threads);
    free(threadData);
}

void test3(MyConcurrentQueue* q, int count) {
    int threads_count = 2;

    pthread_t* threads = (pthread_t*)malloc(threads_count * sizeof(pthread_t));
    pthrData* threadData = (pthrData*)malloc(threads_count * sizeof(pthrData));

    threadData[0].q = q;
    threadData[0].count = count;
    threadData[1].q = q;
    threadData[1].count = count;

    pthread_create(&(threads[0]), NULL, run_put, &threadData[0]);
    pthread_create(&(threads[1]), NULL, run_get, &threadData[1]);

    for (int i = 0; i < threads_count; i++) {
        pthread_join(threads[i], NULL);
    }

    free(threads);
    free(threadData);
}

void test4(MyConcurrentQueue* q, int count) {
    int threads_count = 8;

    pthread_t* threads = (pthread_t*)malloc(threads_count * sizeof(pthread_t));
    pthrData* threadData = (pthrData*)malloc(threads_count * sizeof(pthrData));

    for (int i = 0; i < 5; i++) {
        threadData[i].q = q;
        threadData[i].count = count;
        pthread_create(&(threads[i]), NULL, run_put, &threadData[i]);
    }

    for (int i = 5; i < threads_count; i++) {
        threadData[i].q = q;
        threadData[i].count = 3;
        pthread_create(&(threads[i]), NULL, run_get, &threadData[i]);
    }

    for (int i = 0; i < threads_count; i++) {
        pthread_join(threads[i], NULL);
    }

    free(threads);
    free(threadData);
}

int main() {
#ifdef TEST
    MyConcurrentQueue q1;
    MyConcurrentQueue q2;
    MyConcurrentQueue q3;
    MyConcurrentQueue q4;
    q1.init();
    q2.init();
    q3.init();
    q4.init();

    std::cout << "Test 1\n\n";
    test1(&q1, 4);

    std::cout << "\nTest 2\n\n";
    test2(&q2, 3);

    std::cout << "\nTest 3\n\n";
    test3(&q3, 4);

    std::cout << "\nTest 4\n\n";
    test4(&q4, 2);
#endif

    return 0;
}
