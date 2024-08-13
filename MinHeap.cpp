#include "MinHeap.h"

int MinHeap::pop() {
    int res = pq.top();
    pq.pop();
    return res;
}

void MinHeap::push(int v) {
    pq.push(v);
}

bool MinHeap::is_empty() const {
    return pq.empty();
}

void MinHeap::clear() {
    //This clears the queue
    pq = std::priority_queue<int, std::vector<int>, std::greater<int> >();
}
