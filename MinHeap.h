#ifndef _MINIMUM_HEAP_H__
#define _MINIMUM_HEAP_H__

#include <queue>

class MinHeap {
    private:
        std::priority_queue<int, std::vector<int>, std::greater<int> > pq;
    public:
        /**
         * Pop the top of the heap and returns its value to the caller.
         * @return lowest integer in the heap.
         */
        int pop();

        /**
         * Pushes the given integer to the heap and places it according to its size.
         * @param v - The integer the callers wants to add to the heap.
         */
        void push(int v);

        /**
         * @return A boolean whether the heap is empty or not.
         */
        bool is_empty() const;

        /**
         * Clears all the memory of the heap.
         */
        void clear();
};

#endif //_MINIMUM_HEAP_H__