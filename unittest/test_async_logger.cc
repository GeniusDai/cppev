#include <thread>
#include <vector>
#include <random>
#include "cppev/sysconfig.h"
#include "cppev/async_logger.h"

namespace cppev {

class TestAsyncLogger {
public:
    int thr_num = 50;
    int loop_num = 100;

    void test_output() {
        sysconfig::buffer_outdate = 1;
        auto output = [this](int i) -> void {
            for (int j = 0; j < this->loop_num; ++j) {
                log::info << "testing logger " << i << " round" << log::endl;
                std::this_thread::sleep_for(std::chrono::microseconds(1));
                log::error << "testing logger " << i << " round" << log::endl;
            }
            std::random_device rd;
            std::default_random_engine rde(rd());
            std::uniform_int_distribution<int> dist(0, 1500);
            std::this_thread::sleep_for(std::chrono::milliseconds(dist(rde)));
            for (int j = 0; j < this->loop_num; ++j) {
                log::info << "testing logger " << i << " round" << log::endl;
                std::this_thread::sleep_for(std::chrono::microseconds(1));
                log::error << "testing logger " << i << " round" << log::endl;
            }
        };
        std::vector<std::thread> thrs;
        for (int i = 0; i < thr_num; ++i) {
            thrs.emplace_back(output, i);
        }
        for (int i = 0; i < thr_num; ++i) {
            thrs[i].join();
        }
    }
};

}

int main(int argc, char **argv) {
    cppev::TestAsyncLogger t;
    t.test_output();
    return 0;
}