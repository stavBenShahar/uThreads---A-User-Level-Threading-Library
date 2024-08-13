#ifndef _USER_THREAD_H_
#define _USER_THREAD_H_

#include "ThreadState.cpp"
#include <csetjmp>
#include "uthreads.h"
#include <csignal>
#include <memory>

#define MAIN_THREAD_ID 0
#define JB_SP 6
#define JB_PC 7

class UThread {
    private:
        int tid;
        int quantum_while_running_count;
        int sleep_duration;
    public:
        ThreadState state;
        char stack[STACK_SIZE];
        bool is_sleeping;
        bool is_blocked;
        sigjmp_buf env;

        UThread(int tid, thread_entry_point entry_point);


        /**
         * The method makes the thread "sleep" and changes its sleep_duration to
         * the given quantums.
         * @param quantums - The amount of quantums the caller wants the thread to
         * "sleep".
         */
        void sleep(int quantums);

        /**
         * Changes the state of the thread to BLOCKED.
         */
        void block();

        /**
         *  Updates the sleep duration of the thread by reducing it by one (Minimum
         *  value is 0).
         */
        void notify_sleep();

        /**
         * @return A boolean value whether the thread is "sleeping" or not.
         */
        bool has_finished_sleeping() const;

        /**
         * @return The ID of the thread.
         */
        int get_tid() const;

        /**
         * @return The amount of quantums the thread has been running this far.
         */
        int get_quantum_count() const;

        /**
         * Increases the amount of quantums the thread has been running by one.
         */
        void increment_quantum_count();

};

typedef std::shared_ptr<UThread> thread_ptr;

#endif //_USER_THREAD_H_