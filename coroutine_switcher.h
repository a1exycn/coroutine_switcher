#ifndef COROUTINE_SWITCHER_H
#define COROUTINE_SWITCHER_H

#include <cstddef>
#include <memory>

#include <iostream>

#include <deque>

#include <functional>
#include <coroutine>

class Job_Base {
    friend class Coroutine_Switcher;
public:

    struct promise_base {

        promise_base* parent;
        size_t num_children;

        promise_base();

    };

protected:

    virtual std::coroutine_handle<void> get_handle() = 0;

};


template <typename T>
class Job : public Job_Base {
public:

    struct promise_type : public Job_Base::promise_base {

        std::shared_ptr<T> $value;

        void unhandled_exception() noexcept {}
        

        Job get_return_object() noexcept {
            using Handle = std::coroutine_handle<promise_base>;
            Handle handle = Handle::from_address(
                Coroutine_Switcher::get_current_handle().address()
            );
            promise_base& promise = handle.promise();

            this->parent = &promise;

            return Job{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        

        std::suspend_always initial_suspend() noexcept {
            return {};
        }


        void return_value(T v) noexcept {
            *$value = std::move(v);
        }


        std::suspend_always final_suspend() noexcept {
            parent->num_children -= 1;

            if (parent->num_children == 0)
                // TODO: create friend interface within Coroutine_Switcher to enqueue parent context

            return {};
        }

    }; /* end struct promise_type */

private:

    std::coroutine_handle<promise_type> handle;
    std::shared_ptr<T> ret;

    std::coroutine_handle<void> get_handle() override {
        return handle;
    }

public:

    Job(std::coroutine_handle<promise_type> handle) :
        handle{handle},
        ret{std::make_shared<T>()}
    {
        handle.promise().$value = ret;
    }

    Job(const Job&) = delete;

    Job(Job&& other) :
        handle{other.handle},
        ret{std::move(other.ret)}
    {
        other.handle = {};
    }


    Job& operator=(Job&& other) noexcept {
        if (this != &other) {
            handle = other.handle;
            ret = std::move(other.ret);
            other.handle = {};
        }
        return *this;
    }

    Job& operator=(const Job&) = delete;

    std::shared_ptr<T> get_return_value() const {
        return ret;
    }




}; /* end class Job */


class Coroutine_Switcher {
private:

    size_t size_queue_max;

    std::deque<
        std::coroutine_handle<void>
    > queue_handles;

    std::coroutine_handle<void> current_handle;

    static Coroutine_Switcher* instance;

    Coroutine_Switcher(size_t size_queue_max);

    /**
     * @brief Enqueue a new coroutine job.
     * @param handle the coroutine handle of the job
     * @return true upon success, false if the queue is full
     */
    bool enqueue_internal(std::coroutine_handle<void> handle);

    /**
     * @brief Run the coroutine with the given handle.
     * @param handle_progenitor the coroutine handle of the progenitor job
     */
    void run_internal(std::coroutine_handle<void> handle_progenitor);

public:

    ~Coroutine_Switcher();

    static void initialize(size_t size_queue_max = 100);

    static bool enqueue(Job_Base& job);

    static void run(Job_Base& job);

    static std::coroutine_handle<void> get_current_handle();

}; /* end class Coroutine_Switcher */


#endif /* COROUTINE_SWITCHER_H */