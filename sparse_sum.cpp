#define ANKERL_NANOBENCH_IMPLEMENT
#include "nanobench.h"

#include "CLI/App.hpp"
#include "CLI/Formatter.hpp"
#include "CLI/Config.hpp"

#include <cmath>
#include <vector>
#include <queue>
#include <random>

int main(int argc, char* argv []) {
    CLI::App app{"Expanded testing checks"};
    double density;
    app.add_option("-d,--density", density, "Density of the expanded sparse matrix")->default_val(0.1);
    int nr;
    app.add_option("-r,--nrow", nr, "Number of rows")->default_val(50000);
    int nc;
    app.add_option("-c,--ncol", nc, "Number of columns")->default_val(10000);
    CLI11_PARSE(app, argc, argv);

    std::cout << "Testing a " << nr << " x " << nc << " matrix with a density of " << density << std::endl;

    // Simulating a set of sparse vectors.
    std::mt19937_64 generator(1234567);
    std::uniform_real_distribution<double> distu;

    std::vector<std::vector<int> > indices(nc);
    std::vector<std::vector<double> > values(nc);
    for (int c = 0; c < nc; ++c) {
        auto& curidx = indices[c];
        auto& curvals = values[c];
        for (int r = 0; r < nr; ++r) {
            if (distu(generator) <= density) {
                curidx.push_back(r);
                curvals.push_back(distu(generator));
            }
        }
    }

    double expected = -1;
    ankerl::nanobench::Bench().run("naive", [&](){
        std::vector<double> buffer(nr);

        for (int c = 0; c < nc; ++c) {
            const auto& ii = indices[c];
            const auto& vv = values[c];
            for (int x = 0, end = ii.size(); x < end; ++x) {
                buffer[ii[x]] += vv[x];
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
        std::vector<const double*> vptrs;
        std::vector<const int*> iptrs;
        std::vector<const int*> terminus;

        constexpr int block_size = 16;
        for (int c = 0; c < nc; c += block_size) {
            int to_use = std::min(static_cast<int>(block_size), nc - c);
            vptrs.clear();
            iptrs.clear();
            terminus.clear();
            for (int u = 0; u < to_use; ++u) {
                auto cu = c + u;
                vptrs.push_back(values[cu].data());
                const auto& idx = indices[cu];
                iptrs.push_back(idx.data());
                terminus.push_back(idx.data() + idx.size());
            }

            for (int r = 0; r < nr; r += block_size) {
                int rlimit = r + std::min(static_cast<int>(block_size), nr - r);
                for (int x = 0; x < to_use; ++x) {
                    auto& curval = vptrs[x];
                    auto& curidx = iptrs[x];
                    auto curterm = terminus[x];
                    for (; curterm != curidx && *curidx < rlimit; ++curidx, ++curval) {
                        buffer[*curidx] += *curval;
                    }
                }
            }
        }

        double sum = std::accumulate(buffer.begin(), buffer.end(), static_cast<double>(0));
        if (std::abs(sum - expected) > 0.00001) {
            std::cerr << "WARNING: different result from naive summation (" << sum << " vs " << expected << ")" << std::endl;
        }
    });
}
