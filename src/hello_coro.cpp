//
// Created by Henrik Wachowitz on 06.07.20.
//

#include <iostream>
#include <iterator>
#include <string>
#include <experimental/coroutine>
#include "async_task.hpp"
#include <chrono>
#include <thread>
#include "cppcoro/task.hpp"
#include "cppcoro/when_all_ready.hpp"
#include "cppcoro/when_all.hpp"
#include "cppcoro/sync_wait.hpp"
#include "cppcoro/generator.hpp"

enum Message {HELLO, WORLD};

class resumable {
public:
    struct promise_type;
    using coro_handle = std::experimental::coroutine_handle<promise_type>;
    resumable(coro_handle handle) : handle_(handle) { assert(handle); }
    resumable(resumable&) = delete;
    resumable(resumable&&) = delete;
    bool resume() {
        if (not handle_.done())
            handle_.resume();
        return not handle_.done();
    }
    ~resumable() { handle_.destroy(); }
private:
    coro_handle handle_;
};

struct resumable::promise_type {
    using coro_handle = std::experimental::coroutine_handle<promise_type>;
    auto get_return_object() {
        return coro_handle::from_promise(*this);
    }
    auto initial_suspend() { return std::experimental::suspend_always(); }
    auto final_suspend() { return std::experimental::suspend_always(); }
    void return_void() {}
    void unhandled_exception() {
        std::terminate();
    }
};

auto foo() -> resumable {
    std::cout << "Hello" << std::endl;
    co_await std::experimental::suspend_always();
    std::cout << "World" << std::endl;
}

cppcoro::task<std::string> bar(ssize_t st, Message m) {
    std::this_thread::sleep_for(std::chrono::milliseconds(st));
    switch (m) {
        case HELLO:
            co_return "Hello\n";
            break;
        case WORLD:
            co_return "World\n";
            break;
    }
}

task<std::string> build_hello_world() {
    auto [world, hello] = co_await cppcoro::when_all_ready(bar(50, WORLD), bar(50, HELLO));
    co_return (hello.result() + world.result());
}

cppcoro::generator<const uint64_t> fibgen()
{
    std::uint64_t a = 0, b = 1;
    while (true)
    {
        co_yield b;
        auto tmp = a;
        a = b;
        b += tmp;
    }
}

int main (void) {
    resumable res = foo();
    std::cout << "foo() is now suspended, resuming...\n";
    res.resume();
    std::cout << "foo() is now suspended again, resuming...\n";
    res.resume();

    std::cout << "Now showing awaitables:\n";
    auto start = std::chrono::high_resolution_clock::now();
    auto hw_builder = build_hello_world();
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Task creation took: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms\n";

    auto hello_world = cppcoro::sync_wait(build_hello_world());
    std::cout << hello_world << std::endl;
    end = std::chrono::high_resolution_clock::now();
    std::cout << "Retrival took: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms\n";

    std::cout << "Now generating some fibonacci numbers:\n";
    auto gen = fibgen();
    auto it = gen.begin();
    auto oit = std::ostream_iterator<const uint64_t>(std::cout, ", ");
    for(int i = 0; i<10 ; *oit++ = *it, ++it, i++);
}
