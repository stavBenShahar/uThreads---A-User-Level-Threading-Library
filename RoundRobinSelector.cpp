#include "RoundRobinSelector.h"

thread_ptr RoundRobinSelector::front() {
    thread_ptr res = pool.front();
    pool.pop_front();
    return res;
}

void RoundRobinSelector::push_back(thread_ptr v) {
    pool.push_back(v);
}

bool RoundRobinSelector::is_empty() const {
    return pool.empty();
}

void RoundRobinSelector::remove(thread_ptr v) {
    pool.remove(v);
}

void RoundRobinSelector::clear() {
    pool.clear();
}

