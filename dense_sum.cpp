#define ANKERL_NANOBENCH_IMPLEMENT
#include "nanobench.h"

#include "CLI/App.hpp"
#include "CLI/Formatter.hpp"
#include "CLI/Config.hpp"

#include <random>
#include <cmath>
#include <algorithm>
#include <vector>

int main(int argc, char* argv[]) {
    CLI::App app{"Expanded testing checks"};
    int nr;
    app.add_option("-r,--nrow", nr, "Number of rows")->default_val(100000);
    int nc;
    app.add_option("-c,--ncol", nc, "Number of columns")->default_val(2000);
    CLI11_PARSE(app, argc, argv);

    std::cout << "Testing a " << nr << " x " << nc << " matrix" << std::endl;

    std::mt19937_64 rng(123456);
    std::vector<double> stuff(nc * nr);
    std::normal_distribution<double> ndist;
    for (auto& s : stuff) {
        s = ndist(rng);
    }

    double expected = -1;
    ankerl::nanobench::Bench().run("naive", [&](){
        std::vector<double> buffer(nr);

        for (int c = 0; c < nc; ++c) {
            auto start = stuff.data() + c * nr;
            for (int r = 0; r < nr; ++r) {
                buffer[r] += start[r];
            }
        }

        double sum = std::accumulate(buffer.begin(), buffer.end(), static_cast<double>(0));
        if (expected < 0) {
            expected = sum;
        } else if (std::abs(sum - expected) > 0.00001) {
            std::cerr << "WARNING: different result from naive summation (" << sum << ")" << std::endl;
        }
    });

    ankerl::nanobench::Bench().run("blocked", [&](){
        std::vector<double> buffer(nr);
        std::vector<const double*> ptrs;

        constexpr int block_size = 16;
        for (int c = 0; c < nc; c += block_size) {
            int to_use = std::min(static_cast<int>(block_size), nc - c);
            auto start = stuff.data() + c * nr;
            ptrs.clear();
            for (int u = 0; u < to_use; ++u, start += nr) {
                ptrs.push_back(start);
            }

            for (int r = 0; r < nr; r += block_size) {
                int rlimit = r + std::min(static_cast<int>(block_size), nr - r);
                for (int x = 0; x < to_use; ++x) {
                    auto current = ptrs[x];
                    for (int y = r; y < rlimit; ++y) {
                        buffer[y] += current[y];
                    }
                }
            }
        }

        double sum = std::accumulate(buffer.begin(), buffer.end(), static_cast<double>(0));
        if (std::abs(sum - expected) > 0.00001) {
            std::cerr << "WARNING: different result from naive summation (" << sum << ")" << std::endl;
        }
    });

    return 0;
}
