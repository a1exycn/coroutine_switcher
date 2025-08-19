#include "coroutine_switcher.h"

/* start Coroutine_Switcher definitions */

std::unordered_map<
    std::coroutine_handle<void>, size_t
> Job_Base::handle_to_num_children = {};

Coroutine_Switcher* Coroutine_Switcher::instance = nullptr;

Coroutine_Switcher::Coroutine_Switcher(size_t size_queue_max) :
    size_queue_max(size_queue_max),
    queue_handles(),
    current_handle{nullptr}
{}


bool Coroutine_Switcher::enqueue_internal(
    std::coroutine_handle<void> handle
) {
    if (this->queue_handles.size() < this->size_queue_max) {
        this->queue_handles.push_back(handle);
        return true;
    }

    else
        return false;
}


void Coroutine_Switcher::run_internal(
    std::coroutine_handle<void> handle_progenitor
) {
    this->enqueue_internal(handle_progenitor);

    while(!this->queue_handles.empty()) {
        current_handle = this->queue_handles.front();
        this->queue_handles.pop_front();

        current_handle.resume();

        if(!current_handle.done())
            this->enqueue_internal(current_handle);

        current_handle = nullptr;
    }
}


Coroutine_Switcher::~Coroutine_Switcher() {}


void Coroutine_Switcher::initialize(size_t size_queue_max = 100) {
    if (!instance)
        instance = new Coroutine_Switcher(size_queue_max);
}

bool Coroutine_Switcher::enqueue(Job_Base& job) {
    return instance->enqueue_internal(job.get_handle());
}

void Coroutine_Switcher::run(Job_Base& job) {
    instance->run_internal(job.get_handle());
}

/* end Coroutine_Switcher definitions */