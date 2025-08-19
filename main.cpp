#include "coroutine_switcher.h"

#include <iostream>

Job<int> get_job_1() {
    std::cout << "running job_1\n";
    co_return 1;
}


Job<int> get_job_2() {
    std::cout << "running job_2\n";
    co_return 2;
}


Job<int> get_job_progenitor() {
    std::cout << "running job_progenitor\n";

    auto job_1 = get_job_1();
    auto job_2 = get_job_2();

    Coroutine_Switcher::enqueue(&job_1);
    Coroutine_Switcher::enqueue(&job_2);

    auto $val1 = co_await job_1.get_result();
    auto $val2 = co_await job_2.get_result();

    std::cout << "val1 + val2 = " << *$val1 + *$val2 << "\n";

    co_return 0;
}


int main() {
    Coroutine_Switcher::initialize(1000);

    auto job_progenitor = get_job_progenitor();

    std::cout << "running Coroutine_Switcher::run\n";
    Coroutine_Switcher::run(&job_progenitor);

    return 0;
}