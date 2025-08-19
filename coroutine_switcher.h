#ifndef COROUTINE_SWITCHER_H
#define COROUTINE_SWITCHER_H

#include <cstddef>
#include <memory>

#include <iostream>

#include <deque>
#include <unordered_map>

#include <coroutine>

class Job_Base {
    friend class Coroutine_Switcher;
protected:

    static std::unordered_map<
        std::coroutine_handle<void>, size_t
    > handle_to_num_children;

    virtual std::coroutine_handle<void> get_handle() = 0;

};


template <typename T>
class Job : public Job_Base {
public:

    struct promise_type {

        std::shared_ptr<T> $value;
        std::coroutine_handle<void> handle_parent;


        promise_type() :
            $value{nullptr},
            handle_parent{nullptr}
        {}


        void unhandled_exception() noexcept {}
        

        Job get_return_object() noexcept {
            using Handle = std::coroutine_handle<promise_type>;

            Handle handle = Handle::from_promise(*this);

            Job_Base::handle_to_num_children[handle] = 0;

            return Job(handle);
        }
        

        std::suspend_always initial_suspend() noexcept {
            return {};
        }


        void return_value(T v) noexcept {
            *$value = std::move(v);
        }


        std::suspend_always final_suspend() noexcept {

            if (handle_parent) {
                auto& num_children = Job_Base::handle_to_num_children[handle_parent];

                --num_children;

                if (num_children == 0)
                    Coroutine_Switcher::enqueue_internal(handle_parent);
            }

            Job_Base::handle_to_num_children.erase(
                std::coroutine_handle<void>::from_promise(*this)
            );

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

    ~Job() {
        if (handle) {
            handle.destroy();
        }
    }
    

    /**
     * @brief awaiter for when we need to obtain a result 
     * from another job
     */
    struct Awaiter_Result {
        friend class Job;
    private:

        std::coroutine_handle<promise_type> handle;

        Awaiter_Result(std::coroutine_handle<promise_type> handle) :
            handle{handle}
        {}

    public:

        Awaiter_Result(const Awaiter_Result&) = delete;

        Awaiter_Result(Awaiter_Result&& other) :
            handle{other.handle}
        {
            other.handle = {};
        }

        Awaiter_Result& operator=(const Awaiter_Result&) = delete;

        Awaiter_Result& operator=(Awaiter_Result&& other) noexcept {
            if (this != &other) {
                handle = other.handle;
                other.handle = {};
            }
            return *this;
        }


        bool await_ready() const {
            return handle.done();
        }


        void await_suspend(std::coroutine_handle<void> handle_awaiting) {
            this->handle.promise().handle_parent = handle_awaiting;

            ++Job_Base::handle_to_num_children[handle_awaiting];

            Coroutine_Switcher::enqueue_internal(handle_awaiting);
        }


        std::shared_ptr<T> await_resume() {
            return this->handle.promise().$value;
        }

    }; /* end struct Awaiter_Result */


    Awaiter_Result get_result() {
        return Awaiter_Result(handle);
    }

}; /* end class Job */


class Coroutine_Switcher {
    template<class> friend class Job;
    template<class U> friend struct Job<U>::promise_type;
private:

    size_t size_queue_max;

    std::deque<
        std::coroutine_handle<void>
    > queue_handles;

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

    static bool enqueue(Job_Base* job);

    static void run(Job_Base* job);

}; /* end class Coroutine_Switcher */


#endif /* COROUTINE_SWITCHER_H */