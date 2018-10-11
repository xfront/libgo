/************************************************
 * libgo sample5
*************************************************/

/***********************************************
 * 结合boost.asio, 使网络编程变得更加简单.
 * 如果你不喜欢boost.asio, 这个例子可以跳过不看.
************************************************/
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <getopt.h>
#include <chrono>
#include "coroutine.h"
#include "win_exit.h"

static const uint16_t port = 43332;
using namespace boost::asio;
using namespace boost::asio::ip;
using boost::system::error_code;

io_service ios;

tcp::endpoint addr(address::from_string("127.0.0.1"), port);

struct Option {
    int bufSize;
    int conn;

    bool run;
    std::atomic_long all_pkg;
    std::atomic_long all_flow;
    std::atomic_long all_conn;
};

static void readcmdline(struct Option *opt, int argc, char *argv[]) {
    static char const *optstring = "s:c:";
    static struct option const options[] = {
            {"size", required_argument, 0, 's'},
            {"conn", required_argument, 0, 'c'},
            {0, 0,                      0, 0}
    };

    int n;
    opt->conn = 100;
    opt->bufSize = 1024 * 4;
    opt->run = true;
    while ((n = getopt_long(argc, argv, optstring, options, NULL)) != EOF) {
        switch (n) {
            case 's':
                opt->bufSize = atoi(optarg);
                break;
            case 'c':
                opt->conn = atoi(optarg);
                break;
        }
    }
}

Option gOpt;

void echo_server() {
    tcp::acceptor acc(ios, addr, true);
    while (gOpt.run) {
        std::shared_ptr<tcp::socket> s(new tcp::socket(ios));
        acc.accept(*s);
        int id = ++gOpt.all_conn;
        go co_stack(1024 * 32) [id, s] {
            std::vector<char> buf(gOpt.bufSize);
            error_code ignore_ec;
            for (;;) {
                auto n = s->read_some(buffer(buf), ignore_ec);
                //printf("server[%d] recv msg [%d]\n", id, n);
                n = s->write_some(buffer(buf, n), ignore_ec);
                //printf("server[%d] send msg [%d]\n", id, n);
            }
        };
    }
}

void client() {
    tcp::socket s(ios);
    boost::system::error_code ignore_ec;
    s.connect(addr, ignore_ec);
    if (ignore_ec) {
        std::cout << "can't connect to: " << addr << std::endl;
    }

    std::vector<char> buf(gOpt.bufSize, 'x');
    long count = 0;
    long flow = 0;
    while (gOpt.run) {
        int n = s.write_some(buffer(buf), ignore_ec);
        n = s.read_some(buffer(buf, n), ignore_ec);
        ++count;
        flow += n;
    }
    gOpt.all_pkg += count;
    gOpt.all_flow += flow;
}

// save some typing
namespace cr = std::chrono;

// you can replace this with steady_clock or system_clock
typedef cr::high_resolution_clock my_clock;

int main(int argc, char *argv[]) {
    readcmdline(&gOpt, argc, argv);

    go echo_server;

    auto start_time = my_clock::now();
    for (int i = 0; i < gOpt.conn; ++i)
        go co_stack(1024 * 32) client;

    //执行
    std::thread([] { co_sched.Start(1, std::thread::hardware_concurrency()); }).detach();

    //输入任意字符退出
    char c;
    std::cin >> c;
    gOpt.run = false;
    sleep(1);
    co_sched.Stop();


    // get the clock time after the operation
    auto end_time = my_clock::now();

    // get the elapsed time
    auto diff = end_time - start_time;

    // convert from the clock rate to a millisecond clock
    auto milliseconds = cr::duration_cast<cr::milliseconds>(diff);

    // get the clock count (i.e. the number of milliseconds)
    auto millisecond_count = milliseconds.count() - 1000;

    long con = gOpt.all_conn;
    printf("conn=%d, count=%.02f/s, flow=%.02fMB/s\n", con, gOpt.all_pkg * 1000.0 / millisecond_count,
           (gOpt.all_flow >> 20) * 1000.0 / millisecond_count);

    return 0;
}

