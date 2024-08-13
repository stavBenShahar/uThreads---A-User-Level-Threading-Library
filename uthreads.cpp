#include "UThreadsManager.h"
#include "uthreads.h"

int uthread_init(int quantum_usecs) {
    return UThreadsManager::init(quantum_usecs);
}

int uthread_spawn(thread_entry_point entry_point) {
    return UThreadsManager::getInstance().uthread_spawn(entry_point);
}

int uthread_terminate(int tid) {
    return UThreadsManager::getInstance().uthread_terminate(tid);
}

int uthread_block(int tid) {
    return UThreadsManager::getInstance().uthread_block(tid);
}

int uthread_resume(int tid) {
    return UThreadsManager::getInstance().uthread_resume(tid);
}

int uthread_sleep(int num_quantums) {
    return UThreadsManager::getInstance().uthread_sleep(num_quantums);
}

int uthread_get_tid() {
    return UThreadsManager::getInstance().uthread_get_tid();
}

int uthread_get_total_quantums() {
    return UThreadsManager::getInstance().uthread_get_total_quantums();
}

int uthread_get_quantums(int tid) {
    return UThreadsManager::getInstance().uthread_get_quantums(tid);
}



