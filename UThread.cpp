#include "UThread.h"

typedef unsigned long address_t;

address_t translate_address(address_t addr) {
    address_t ret;
    asm volatile("xor    %%fs:0x30,%0\n"
                 "rol    $0x11,%0\n"
            : "=g"(ret)
            : "0"(addr));
    return ret;
}

UThread::UThread(int tid, thread_entry_point entry_point) {
    this->tid = tid;
    this->state = READY;
    is_sleeping = false;
    is_blocked = true;
    quantum_while_running_count = 0;
    sleep_duration = 0;
    address_t sp = (address_t) stack + STACK_SIZE - sizeof(address_t);
    address_t pc = (address_t) entry_point;
    sigsetjmp(env, 1);
    (env->__jmpbuf)[JB_SP] = translate_address(sp);
    (env->__jmpbuf)[JB_PC] = translate_address(pc);
    sigemptyset(&env->__saved_mask);
}

void UThread::sleep(int quantums) {
    state = BLOCKED;
    sleep_duration = quantums;
    is_sleeping = true;
}

void UThread::block() {
    state = BLOCKED;
}

void UThread::notify_sleep() {
    sleep_duration = std::max(0, sleep_duration - 1);
}

bool UThread::has_finished_sleeping() const {
    return is_sleeping && sleep_duration == 0;
}

int UThread::get_tid() const {
    return tid;
}

int UThread::get_quantum_count() const {
    return quantum_while_running_count;
}

void UThread::increment_quantum_count() {
    quantum_while_running_count += 1;
}
