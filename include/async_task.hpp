//
// Created by Henrik Wachowitz on 06.07.20.
//

#ifndef ASYNC_DEMO_ASYNC_TASK_HPP
#define ASYNC_DEMO_ASYNC_TASK_HPP

#include <experimental/coroutine>
#include <variant>
#include <condition_variable>
#include <stdio.h>

template <typename T = void>
struct [[nodiscard]] task {
    struct promise_type {
        std::variant<std::monostate, T, std::exception_ptr> result;
        std::experimental::coroutine_handle<> waiter; // who waits on this coroutine

        auto get_return_object() { return task{*this}; }
        void return_value(T value) { result.template emplace<1>(std::move(value)); }
        void unhandled_exception() { result.template emplace<2>(std::current_exception()); }
        std::experimental::suspend_always initial_suspend() { return {}; }
        auto final_suspend() {
            struct final_awaiter {
                bool await_ready() { return false; }
                void await_resume() {}
                auto await_suspend(std::experimental::coroutine_handle<promise_type> me) {
                    return me.promise().waiter;
                }
            };
            return final_awaiter{};
        }
    };

    task(task&& rhs) : h(rhs.h) { rhs.h = nullptr; }
    ~task() { if (h) h.destroy(); }
    explicit task(promise_type& p) : h(std::experimental::coroutine_handle<promise_type>::from_promise(p)) {}

    bool await_ready() { return false; }
    T await_resume() {
        auto& result = h.promise().result;
        if (result.index() == 1) return std::get<1>(result);
        rethrow_exception(std::get<2>(result));
    }
    void await_suspend(std::experimental::coroutine_handle<> waiter) {
        h.promise().waiter = waiter;
        h.resume();
    }
    auto _secret() { return h; } // to help sync_await

private:
    std::experimental::coroutine_handle<promise_type> h;
};

template <typename Whatever> // here is a quick hack of universal sync_await
auto sync_await(Whatever& x) {
    if (!x.await_ready()) {
        std::mutex m;
        std::condition_variable c;
        bool done = false;
        // helper to hookup continuation to whatever x is
        auto helper = [&]()->task<char> {
            { std::lock_guard grab(m); done = true; }
            c.notify_one();
            co_return 1; // I was lazy and did not create task<void> in this sample
        };
        auto helper_task = helper();
        auto helper_handle = helper_task._secret();
        // we wait for helper using mutex + condition variable, no need to resume anything
        // hence, hacking the waiter to helper to be a noop_coroutine
        helper_handle.promise().waiter = std::experimental::noop_coroutine();

        x.await_suspend(helper_handle); // tell x to resume helper when done

        std::unique_lock lk(m); // block until x completes
        if (!done) c.wait(lk, [&]{ return done; });
    }
    return x.await_resume();
}
#endif //ASYNC_DEMO_ASYNC_TASK_HPP
