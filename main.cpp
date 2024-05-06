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
    size_t nr;
    app.add_option("-r,--nrow", nr, "Number of rows")->default_val(100000);
    size_t nc;
    app.add_option("-c,--ncol", nc, "Number of columns")->default_val(200);
    CLI11_PARSE(app, argc, argv);

    std::mt19937_64 rng(123456);
    std::vector<double> stuff(nc * nr);
    std::normal_distribution<double> ndist;
    for (auto& s : stuff) {
        s = ndist(rng);
    }

    double expected = std::accumulate(stuff.begin(), stuff.end(), 0.0);

    ankerl::nanobench::Bench().run("naive", [&](){
        std::vector<double> buffer(nr);
#ifdef MULTIPLE_RESULTS
        std::vector<double> buffer2(nr);
        std::vector<double> buffer3(nr);
#endif

        for (size_t c = 0; c < nc; ++c) {
            auto start = stuff.data() + c * nr;
            for (size_t r = 0; r < nr; ++r) {
                buffer[r] += start[r];
#ifdef MULTIPLE_RESULTS
                buffer2[r] += start[r];
                buffer3[r] += start[r];
#endif
            }
        }

        double sum = std::accumulate(buffer.begin(), buffer.end(), static_cast<double>(0));
        if (std::abs(sum - expected) > 0.00001) {
            std::cerr << "WARNING: different result from naive summation (" << sum << ")" << std::endl;
        }
    });

    ankerl::nanobench::Bench().run("blocked", [&](){
        std::vector<double> buffer(nr);
#ifdef MULTIPLE_RESULTS
        std::vector<double> buffer2(nr);
        std::vector<double> buffer3(nr);
#endif
        std::vector<double*> ptrs;

        constexpr size_t block_size = 16;
        for (size_t c = 0; c < nc; c += block_size) {
            size_t to_use = std::min(static_cast<size_t>(block_size), nc - c);
            auto start = stuff.data() + c * nr;
            ptrs.clear();
            for (size_t u = 0; u < to_use; ++u, start += nr) {
                ptrs.push_back(start);
            }

            for (size_t r = 0; r < nr; r += block_size) {
                size_t rlimit = r + std::min(static_cast<size_t>(block_size), nr - r);
                for (size_t x = 0; x < to_use; ++x) {
                    auto current = ptrs[x];
                    for (size_t y = r; y < rlimit; ++y) {
                        buffer[y] += current[y];
#ifdef MULTIPLE_RESULTS
                        buffer2[y] += current[y];
                        buffer3[y] += current[y];
#endif
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
