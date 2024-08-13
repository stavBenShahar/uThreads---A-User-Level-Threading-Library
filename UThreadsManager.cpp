#include "UThreadsManager.h"

#define IS_DIRECT_INVOCATION(code) code==0
#define SYSCALL_FAIL(str_msg) fprintf(stderr,"System error: " str_msg "\n")
#define UTHREADS_FAIL(str_msg) fprintf(stderr,"Thread library error: " str_msg "\n")

//added 0 at the end so to enforce using this as a function call requiring semicolon at the end of the invocation
#define GUARD(predicate, message) if(predicate){ message; return FAILURE;}

//defining BLOCK_SIGNALS and UNBLOCK_SIGNALS like this enforces us to use BLOCK_SIGNALS if we want to use UNBLOCK_SIGNALS otherwise
// sigset won't be declared, and also this allows multiple UNBLOCK_SIGNALS statements for the same sigset.
//essentially creating the desired one-many relationship between BLOCK_SIGNALS and UNBLOCK_SIGNALS respectively
#define BLOCK_SIGNALS() sigset_t sigset;sigemptyset(&sigset);sigaddset(&sigset, \
SIGVTALRM); sigprocmask(SIG_BLOCK,&sigset,NULL)
#define UNBLOCK_SIGNALS() sigprocmask (SIG_UNBLOCK,&sigset,NULL)

/*
 * library function
 * */
int UThreadsManager::uthread_init(int quantum) {
    BLOCK_SIGNALS();
    GUARD(
            quantum <= 0,
            UTHREADS_FAIL("Quantum must be a non negative integer.")
    );
    quantum_length = quantum;

    //Initialize all legal tids in the available_thread_ids DS.
    for (int i = 0; i < MAX_THREAD_NUM; i++)
        available_thread_ids.push(i);

    //handle "main" thread
    try {
        thread_ptr main_thread_ptr = std::make_shared<UThread>(available_thread_ids.pop(), nullptr);
        main_thread_ptr->state = RUNNING;
        thread_map.insert({0, main_thread_ptr});
        running_thread = main_thread_ptr;
        //Start the virtual timer, counts the executing time of the process.
        init_itimer(quantum);
        increment_overall_quantum_count();
        running_thread->increment_quantum_count();
        UNBLOCK_SIGNALS();
        return SUCCESS;
    }
    catch (std::bad_alloc &e) {
        SYSCALL_FAIL("bad alloc");
        free_all_memory();
        UNBLOCK_SIGNALS();
        exit(1);
//      return FAILURE;
    }
}

int UThreadsManager::uthread_spawn(thread_entry_point entry_point) {
    BLOCK_SIGNALS();
    // check that we can still create thread and didn't pass the limit MAX_THREAD_NUM
    GUARD(
            entry_point == nullptr,
            UTHREADS_FAIL("entry point must be not null")
    );

    GUARD(
            available_thread_ids.is_empty(),
            UTHREADS_FAIL("can't create new threads as limit has been reached")
    );
    //get lowest pid available
    int new_tid = available_thread_ids.pop();
    //Make sure std::make_shared doesn't fail
    try {
        thread_ptr new_thread = std::make_shared<UThread>(new_tid, entry_point);
        thread_map.insert({new_tid, new_thread});
        threads_scheduler.push_back(thread_map.at(new_tid));
        UNBLOCK_SIGNALS();
        return new_tid;
    }
    catch (std::bad_alloc &e) {
        SYSCALL_FAIL("bad alloc");
        free_all_memory();
        UNBLOCK_SIGNALS();
        exit(1);
    }
    UNBLOCK_SIGNALS();
}

int UThreadsManager::uthread_terminate(int tid) {
    BLOCK_SIGNALS();
    // if not tid exists return FAILURE
    GUARD(
            this->thread_map.find(tid) == this->thread_map.end(),
            UTHREADS_FAIL("Can't terminate a none existing thread.")
    );
    // if tid==0 release all memory and exit(0)
    if (tid == 0) {
        free_all_memory();
        UNBLOCK_SIGNALS();
        exit(0);
    }
    //return tid number to the available pool of values
    if (tid != running_thread->get_tid()) {
        threads_scheduler.remove(thread_map.find(tid)->second);
        //Remove from the sleeping there if it's there.
        for (auto &thread_ptr: this->sleeping_threads) {
            if (thread_ptr->get_tid() == tid) {
                this->sleeping_threads.erase(thread_ptr);
                break;
            }
        }
    }
    thread_map.erase(tid);
    available_thread_ids.push(tid);
    if (tid == running_thread->get_tid()) {
        UNBLOCK_SIGNALS();
        handle_sleeping_threads();
        jmp_to_next_thread();
    }
    UNBLOCK_SIGNALS();
    return SUCCESS;
}

int UThreadsManager::uthread_block(int tid) {
    BLOCK_SIGNALS();
    GUARD(
            thread_map.count(tid) <= 0,
            UTHREADS_FAIL("Can't block non-existing thread")
    );
    GUARD(
            tid == 0,
            UTHREADS_FAIL("Can't block main thread")
    );
    thread_map.at(tid)->block();
    if (tid == uthread_get_tid()) {
        //About to block itself so save current state
        int ret_val = sigsetjmp(running_thread->env, 1);
        if (ret_val == 1) {
            return SUCCESS;
        }
        handle_sleeping_threads();
        jmp_to_next_thread();
    } else
        threads_scheduler.remove(thread_map.at(tid));
    UNBLOCK_SIGNALS();
    return SUCCESS;
}

int UThreadsManager::uthread_resume(int tid) {
    BLOCK_SIGNALS();
    GUARD(
            thread_map.count(tid) <= 0,
            UTHREADS_FAIL("Can't resume non-existing thread")
    );
    //they said in the forum that there is no test for this, but it doesn't hurt
    // to leave it here anyway
    GUARD(
            tid == MAIN_THREAD_ID,
            UTHREADS_FAIL("Can't resume main thread.")
    );
    thread_ptr selected_thread = thread_map.at(tid);
    if (selected_thread->state == BLOCKED) {
        selected_thread->is_blocked = false;
        if (!selected_thread->is_sleeping) {
            selected_thread->state = READY;
            threads_scheduler.push_back(selected_thread);
        }
    }
    UNBLOCK_SIGNALS();
    return SUCCESS;
}

int UThreadsManager::uthread_sleep(int num_quantums) {
    BLOCK_SIGNALS();
    GUARD(
            num_quantums <= 0,
            UTHREADS_FAIL("Can only put to sleep to a positive amount of "
                          "quantums.")
    );
    GUARD(
            running_thread->get_tid() == MAIN_THREAD_ID,
            UTHREADS_FAIL("Can't put main thread to sleep.")
    );
    //The "plus 1" is because of the "handle_sleeping_threads" after.
    running_thread->sleep(num_quantums + 1);
    sleeping_threads.insert(running_thread);
    //Saves the thread context before putting it to sleep
    int ret_val = sigsetjmp(running_thread->env, 1);
    if (ret_val == 1) {
        return SUCCESS;
    }
    handle_sleeping_threads();
    jmp_to_next_thread();
    UNBLOCK_SIGNALS();
    return SUCCESS;
}

int UThreadsManager::uthread_get_tid() {
    return running_thread->get_tid();
}

int UThreadsManager::uthread_get_total_quantums() { return overall_quantum_count; }

int UThreadsManager::uthread_get_quantums(int tid) {
    GUARD(
            thread_map.count(tid) <= 0,
            UTHREADS_FAIL("Can't get quantums count for non-existing thread")
    );
    return thread_map.at(tid)->get_quantum_count();
}


/*
 * internal funcitons
 * */
UThreadsManager::UThreadsManager() {}

void UThreadsManager::free_all_memory() {
    thread_map.clear();
    threads_scheduler.clear();
    available_thread_ids.clear();
    running_thread.reset();
}

void UThreadsManager::increment_overall_quantum_count() {
    overall_quantum_count += 1;
}

void UThreadsManager::switch_threads() {
    BLOCK_SIGNALS();
    //Only main thread exists
    if (threads_scheduler.is_empty()) {
        running_thread->increment_quantum_count();
        increment_overall_quantum_count();
        UNBLOCK_SIGNALS();
        return;
    }
    running_thread->state = READY;
    threads_scheduler.push_back(running_thread);
    bool did_jut_save_bookmark = 0 == sigsetjmp(running_thread->env, MASK_KEEP);
    if (did_jut_save_bookmark) {
        jmp_to_next_thread();
    }
}


/*
 * static functions
 * */
int UThreadsManager::init(int quantum) {
    if (UThreadsManager::getInstance().uthread_init(quantum) == FAILURE)
        return FAILURE;
    return init_itimer(quantum);
}

UThreadsManager &UThreadsManager::getInstance() {
    static UThreadsManager instance;
    return instance;
}

void UThreadsManager::timer_handler(int sig) {
    BLOCK_SIGNALS();
    //Wake threads that finished their "sleep"
    handle_sleeping_threads();
    reset_timer(getInstance().quantum_length);
    UNBLOCK_SIGNALS();
    //Switch threads according to Round-Robin algorithm.
    getInstance().switch_threads();
}

int UThreadsManager::init_itimer(int quantum) {
    struct sigaction sa = {nullptr};

    // Install timer_handler as the signal handler for SIGVTALRM.
    sa.sa_handler = timer_handler;
    if (sigaction(SIGVTALRM, &sa, NULL) < 0) {
        SYSCALL_FAIL("sigaction error.");
        getInstance().free_all_memory();
        exit(1);
    }
    return reset_timer(quantum);

}

int UThreadsManager::reset_timer(int quantum) {
    BLOCK_SIGNALS();
    struct itimerval timer;
    constexpr const int MILLION = 1000000;
    int seconds = quantum / MILLION;
    int microseconds = quantum % MILLION;
    timer.it_value.tv_sec = seconds;
    timer.it_value.tv_usec = microseconds;
    timer.it_interval.tv_sec = seconds;
    timer.it_interval.tv_usec = microseconds;

    if (setitimer(ITIMER_VIRTUAL, &timer, NULL) < 0) {
        SYSCALL_FAIL("setitimer error.");
        getInstance().free_all_memory();
        UNBLOCK_SIGNALS();
        exit(1);
    }
    UNBLOCK_SIGNALS();
    return SUCCESS;


}

void UThreadsManager::handle_sleeping_threads() {
    UThreadsManager &instance = UThreadsManager::getInstance();
    for (auto &ptr: instance.sleeping_threads) {
        ptr->notify_sleep();
        if (ptr->has_finished_sleeping()) {
            ptr->is_sleeping = false;
            if (!ptr->is_blocked) {
                ptr->state = READY;
                instance.threads_scheduler.push_back(ptr);
            }
        }
    }
}

void UThreadsManager::jmp_to_next_thread() {
    BLOCK_SIGNALS();
    UThreadsManager &instance = UThreadsManager::getInstance();
    running_thread = instance.threads_scheduler.front();
    running_thread->increment_quantum_count();
    instance.increment_overall_quantum_count();
    running_thread->state = RUNNING;
    UNBLOCK_SIGNALS();
    siglongjmp(running_thread->env, MASK_KEEP);
}
