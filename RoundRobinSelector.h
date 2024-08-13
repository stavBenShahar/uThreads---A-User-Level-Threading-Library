#ifndef _ROUND_ROBIN_SELECTOR_H_
#define _ROUND_ROBIN_SELECTOR_H_

#include <list>
#include "UThread.h"

class RoundRobinSelector {
    private:
        std::list<thread_ptr> pool;
    public:
        /**
         * Pops the first item of the list and returns it to the caller.
         * @return A pointer to the thread who is currently RUNNING.
         */
        thread_ptr front();

        /**
         * Pushes the given pointer to the back of the list.
         * @param v - The thread pointer the callers wants to add to the list.
         */
        void push_back(thread_ptr v);

        /**
         * @return A boolean value whether the list is empty or not.
         */
        bool is_empty() const;

        /**
         * Removes a given pointer from the list.
         * @param v  - The thread pointer the callers wants to remove.
         */
        void remove(thread_ptr v);

        /**
         * Frees all the memory the instance is currently using.
         */
        void clear();

};

#endif //_ROUND_ROBIN_SELECTOR_H_