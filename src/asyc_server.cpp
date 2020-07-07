//
// Created by Henrik Wachowitz on 06.07.20.
//

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/write.hpp>
#include <cstdio>
#include <iostream>
#include <cstring>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <functional>

#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

#include <cppcoro/task.hpp>
#include <cppcoro/generator.hpp>
#include <cppcoro/sync_wait.hpp>
#include <random>

using boost::asio::ip::tcp;
using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::use_awaitable;
namespace this_coro = boost::asio::this_coro;
namespace json = rapidjson;

static std::unordered_map<std::string, cppcoro::task<std::string> (*)(int)> handle_lookup;

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

cppcoro::generator<const uint64_t> dicegen()
{
    std::mt19937_64 engine{0};
    std::uniform_int_distribution<uint16_t> distr(1, 6);

    auto rand = [&](){return distr(engine);};
    while (true)
    {
        co_yield rand();
    }
}

auto handle_fib(int n) -> cppcoro::task<std::string> {
    std::stringstream out;

    auto gen = fibgen();
    auto it = gen.begin();

    auto oit = std::ostream_iterator<const uint64_t>(out, ", ");
    for(int i = 0; i<n-1 ; *oit++ = *it, ++it, i++);
    ++it;
    out << *it << std::endl;
    co_return out.str();
}

auto handle_rand(int n) -> cppcoro::task<std::string> {
    std::stringstream out;

    auto gen = dicegen();
    auto it = gen.begin();

    auto oit = std::ostream_iterator<const uint64_t>(out, ", ");
    for(int i = 0; i<n-1 ; *oit++ = *it, ++it, i++);
    ++it;
    out << *it << std::endl;
    co_return out.str();
}

awaitable<void> echo(tcp::socket socket)
{
    try
    {
        char data_buffer[4096];
        char* data_buf_begin = data_buffer;
        char data[1024];
        size_t read_total = 0;
        for (;;)
        {
            std::size_t n = co_await socket.async_read_some(boost::asio::buffer(data), use_awaitable);
            read_total += n;
            std::memcpy(data_buf_begin, data, n);
            data_buf_begin+=n;
            if (read_total < 3) {
                continue;
            } else {
                char num[4] = {'\0'};
                char* end;
                std::memcpy(num, data_buffer, 3);
                auto _len = std::strtoul(num, &end, 10);
                if ( read_total < _len + 4) {
                    continue;
                }

                // Json Read completely
                std::memmove(data_buffer, data_buffer+3, sizeof(data_buffer)-3);

                json::Document document;
                bool was_parsed = false;
                try {
                    document.Parse(data_buffer);
                    was_parsed = true;
                }
                catch (json::ParseErrorCode e) {
                    std::cerr << "Parse error" << std::endl;
                    was_parsed = false;
                }

                if (not was_parsed || not document.IsObject() || not document.HasMember("function") || not document.HasMember("arg")){
                    // NO Parse echo back

                    char send_buffer[4096] = {'\0'};
                    std::strcpy(send_buffer, data_buffer);
                    co_await async_write(socket, boost::asio::buffer(data_buffer, std::strlen(data_buffer)), use_awaitable);
                    read_total = 0;
                    data_buf_begin = data_buffer;
                    std::memset(data_buffer, 0, 4096);
                    continue;
                }

                int arg = document["arg"].GetInt();
                std::string f_name = document["function"].GetString();
                auto ret = cppcoro::sync_wait(handle_lookup[f_name](arg));
                co_await async_write(socket, boost::asio::buffer(ret, ret.size()), use_awaitable);
            }
        }
    }
    catch (std::exception& e)
    {
        std::printf("echo Exception: %s\n", e.what());
    }
}

awaitable<void> listener(const unsigned short port)
{
    auto executor = co_await this_coro::executor;
    tcp::acceptor acceptor(executor, {tcp::v4(), port});
    for (;;)
    {
        tcp::socket socket = co_await acceptor.async_accept(use_awaitable);
        co_spawn(executor,
                 [socket = std::move(socket)]() mutable
                 {
                     return echo(std::move(socket));
                 },
                 detached);
    }
}

int main(int argc, char* argv[]) {

    handle_lookup["fib"] = &handle_fib;
    handle_lookup["rand"] = &handle_rand;

    try {

        if (argc < 2)
        {
            std::cerr << "Usage: async_server <port>\n";
            return 1;
        }

        boost::asio::io_context io_context(1);

        boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&](auto, auto) { io_context.stop(); });

        unsigned short port = std::atol(argv[1]);

        co_spawn(io_context, [port]() {
            return listener(port);
            }, detached);

        io_context.run();
    }
    catch (std::exception& e) {
        std::printf("Exception: %s\n", e.what());
    }
}