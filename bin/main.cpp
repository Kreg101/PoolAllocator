#include "../allocator/pools_allocator.h"
#include <chrono>
#include <fstream>

int main() {
    return 0;
}

// Дает возможность замерять время выполнения чего-либо
class Timer {
public:
    explicit Timer(std::ofstream* file_out) {
        this->file_out = file_out;
        start_time_point = std::chrono::high_resolution_clock::now();
    }

    ~Timer() {
        Stop();
    }

    void Stop() {
        auto end_time_point = std::chrono::high_resolution_clock::now();

        auto start = std::chrono::time_point_cast<std::chrono::microseconds>(start_time_point).time_since_epoch().count();
        auto end = std::chrono::time_point_cast<std::chrono::microseconds>(end_time_point).time_since_epoch().count();

        auto duration = end - start;
        //double ms = duration * 0.001;
        *file_out << duration << ' ';
    }
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time_point;
    std::ofstream* file_out;
};