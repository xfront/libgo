/************************************************
 * libgo sample5
*************************************************/

/***********************************************
 * 结合boost.asio, 使网络编程变得更加简单.
 * 如果你不喜欢boost.asio, 这个例子可以跳过不看.
************************************************/
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include "coroutine.h"
#include "win_exit.h"

static const uint16_t port = 43332;
using namespace boost::asio;
using namespace boost::asio::ip;
using boost::system::error_code;

io_service ios;

tcp::endpoint addr(address::from_string("127.0.0.1"), port);

void echo_server()
{
    tcp::acceptor acc(ios, addr, true);
    for (int i = 0; i < 2; ++i) {
        std::shared_ptr<tcp::socket> s(new tcp::socket(ios));
        acc.accept(*s);
        go [s]{
            char buf[1024];
            error_code ignore_ec;
            for (;;) {
                auto n = s->read_some(buffer(buf), ignore_ec);
                n = s->write_some(buffer(buf, n));
            }
        };
    }
}

void client()
{
    tcp::socket s(ios);
    s.connect(addr);


    std::string msg = "1.提供golang一般功能强大协程，基于corontine编写代码，可以以同步的方式编写简单的代码，同时获得异步的性能，\n"
                      "\n"
                      "2.支持海量协程, 创建100万个协程只需使用4.5GB物理内存.(真实值, 而不是刻意压缩stack得到的测试值)\n"
                      "\n"
                      "3.支持多线程调度协程, 提供高效的负载均衡策略和协程同步机制, 很容易编写高效的多线程程序.\n"
                      "\n"
                      "4.调度线程数支持动态伸缩, 不再有调度慢协程导致头部阻塞效应的问题.\n"
                      "\n"
                      "5.使用hook技术让链接进程序的同步的第三方库变为异步调用，大大提升其性能。再也不用担心某些DB官方不提供异步driver了，比如hiredis、mysqlclient这种客户端驱动可以直接使用，并且可以得到不输于异步driver的性能。\n"
                      "\n"
                      "6.动态链接和全静态链接均支持，便于使用C++11的用户静态链接生成可执行文件并部署至低版本的linux系统上。\n"
                      "\n"
                      "7.提供Channel, 协程锁(co_mutex), 协程读写锁(co_rwmutex), 定时器等特性, 帮助用户更加容易地编写程序.\n"
                      "\n"
                      "8.支持协程局部变量(CLS), 并且完全覆盖TLS的所有使用场景(详见教程代码sample13_cls.cpp).\n"
                      "";
    for (;;) {
        int n = s.write_some(buffer(msg));
        printf("client send msg [%d] %s\n", (int) msg.size(), msg.c_str());
        char buf[msg.length()];
        n = s.receive(buffer(buf, n));
        printf("client recv msg [%d] %.*s\n", n, n, buf);
    }
}

int main()
{
    go echo_server;
    go client;
    go client;

    // 2000ms后安全退出
    std::thread([]{ co_sleep(2000); co_sched.Stop(); }).detach();

    // 单线程执行
    co_sched.Start();
    return 0;
}

