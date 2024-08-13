#ifndef _UTHREADSMANAGER_H_
#define _UTHREADSMANAGER_H_

#include "MinHeap.h"
#include "UThread.h"
#include "uthreads.h"
#include "RoundRobinSelector.h"
#include <bits/stdc++.h>
#include <sys/time.h>

#define MASK_KEEP 1
#define FAILURE (-1)
#define SUCCESS 0
#define JB_SP 6
#define JB_PC 7
#define MAIN_THREAD_ID 0


class UThreadsManager {
    private:
        int overall_quantum_count;
        int quantum_length;
        thread_ptr running_thread;
        std::map<int, thread_ptr> thread_map;
        MinHeap available_thread_ids;
        std::set<thread_ptr> sleeping_threads;
        RoundRobinSelector threads_scheduler;
    public:
        static struct itimerval timer;

    private:
        /**
         *  Initializes the thread library and sets the main thread to be running.
         *  Takes the length of a quantum in microseconds as an argument.
         * @param quantum - The measurement for the virtual time.
         * @return - 0 if the method was successful, -1 otherwise.
         */
        int uthread_init(int quantum);


        UThreadsManager();

        /**
         * Increments the quantum count by one.
         */
        void increment_overall_quantum_count();

        /**
         * The method schedules the threads, which one should be currently RUNNING
         * and which one should be READY to run (It schedules each one according
         * to the ROUND ROBIN scheduling algorithm).
         */
        void switch_threads();

        /**
         * The method changes the next thread in the pool to be the running thread
         * and jmp to its PC by using "siglongjmp".
         * Updates the quantum count of the thread and the quantum count of the
         * process. (Serves as a helper function for "switch_threads").
         */
        void jmp_to_next_thread();

        /**
       *  A handler for SIGVTALRM, the handler is charge of what should happen
       *  every quantum that passes.
       * @param sig - A signal
       */
        static void timer_handler(int sig);

        /**
         * The method starts the virtual timer of the process.
         * @param quantum - The length of quantum in micro-seconds.
         * @return 0 if it was successful and exit(1) otherwise.
         */
        static int init_itimer(int quantum);

        /**
         * Resets the virtual timer according to a given quantum.
         * @param quantum - The length of quantum in micro-seconds.
         * @return 0 if it was successful and exit(1) otherwise.
         */
        static int reset_timer(int quantum);

        static void handle_sleeping_threads();

    public:
        UThreadsManager(UThreadsManager const &) = delete;

        void operator=(UThreadsManager const &) = delete;

        static int init(int quantum);

        static UThreadsManager &getInstance();

        /**
         * Creates a new thread and adds it to the end of the READY threads list.
         * Takes a function pointer to the thread entry point as an argument.
         * @param entry_point - The function of the new thread.
         * @return On success, return the ID of the created thread. On failure,
         * return -1.
         */
        int uthread_spawn(thread_entry_point entry_point);

        /**
         * Free all the memory the instance is currently using.
         */
        void free_all_memory();

        /**
         * Terminates a thread with the given ID and deletes it from all relevant control structures.
         * If no thread with the given ID exists, it's considered an error.
         * Terminating the main thread will result in the termination of the entire process
         * @param tid - The ID of the thread the caller wants to terminate.
         * @return  The function returns 0 if the thread was successfully terminated and -1 otherwise.
         */
        int uthread_terminate(int tid);

        /**
         * Blocks a thread with the given ID. If no thread with the given ID exists,
         * it's considered an error. It's also an error to try blocking the main thread.
         * If a thread blocks itself, a scheduling decision should be made.
         * @param tid - The ID of the thread the caller wants to terminate.
         * @return On success, return 0. On failure, return -1.
         */
        int uthread_block(int tid);

        /**
         * Resumes a blocked thread with the given ID and moves it to the READY state.
         * If no thread with the given ID exists, it's considered an error.
         * @param tid - The id of the thread the caller wants to resume to action.
         */
        int uthread_resume(int tid);

        /**
         * Blocks the RUNNING thread for a specified number of quantums.
         * When the sleeping time is over, the thread should go back to the end of the READY queue.
         * It's considered an error if the main thread calls this function.
         * @param num_quantums - The amount of quantums the uthread is going to
         * sleep for.
         * @return On success, return 0. On failure, return -1.
         */
        int uthread_sleep(int num_quantums);

        /**
         * @return The thread ID of the calling thread.
         */
        int uthread_get_tid();

        /**
         * @return The total number of quantums since the
         * library was initialized, including the current quantum.
         */
        int uthread_get_total_quantums();

        /**
         * Returns the number of quantums the thread with the given ID was in RUNNING state.
         * If no thread with the given ID exists, it's considered an error.
         * @param tid - The ID of the thread the callers wants to get its quantums.
         * @return The amount of quantums the thread was in RUNNING state.
         */
        int uthread_get_quantums(int tid);

};

#endif //_UTHREADSMANAGER_H_
