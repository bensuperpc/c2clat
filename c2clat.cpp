// © 2020 Erik Rigtorp <erik@rigtorp.se>
// SPDX-License-Identifier: MIT
// Forked by Bensuperpc <bensuperpc@gmail.com>
// Measure inter-core one-way data latency
//
// Build:
// g++ -O3 -DNDEBUG -march=native -flto -flto-partition=none c2clat.cpp -o c2clat -pthread
//
// Plot results using gnuplot:
// $ c2clat -p | gnuplot -p

#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <map>
#include <thread>
#include <vector>

void pinThread(int cpu) {
  cpu_set_t set;
  CPU_ZERO(&set);
  CPU_SET(cpu, &set);
  if (sched_setaffinity(0, sizeof(set), &set) == -1) {
    perror("sched_setaffinity");
    exit(1);
  }
}

int main(int argc, char *argv[]) {

  const auto nsamples = 4000;
  const auto samplessize = 100;

  bool plot = false;

  int opt;
  while ((opt = getopt(argc, argv, "ps:")) != -1) {
    switch (opt) {
    case 'p':
      plot = true;
      break;
//    case 's':
//      nsamples = std::stoi(optarg);
//      break;
    default:
      goto usage;
    }
  }

  if (optind != argc) {
  usage:
    std::cerr << "c2clat 1.0.0 © 2020 Erik Rigtorp <erik@rigtorp.se>\n"
                 "c2clat-forked 1.1.0 - 2021 Bensuperpc <bensuperpc@gmail.com>\n"
                 "usage: c2clat [-p]\n"
                 "\nPlot results using gnuplot:\n"
                 "c2clat -p | gnuplot -p\n";
    exit(1);
  }

  cpu_set_t set;
  CPU_ZERO(&set);
  if (sched_getaffinity(0, sizeof(set), &set) == -1) {
    perror("sched_getaffinity");
    exit(1);
  }

  // enumerate available CPUs
  std::vector<int> cpus;
  for (auto i = 0; i < CPU_SETSIZE; ++i) {
    if (CPU_ISSET(i, &set)) {
      cpus.push_back(i);
    }
  }

  std::map<std::pair<int, int>, std::chrono::nanoseconds> data;

  for (size_t i = 0; i < cpus.size(); ++i) {
    for (size_t j = i + 1; j < cpus.size(); ++j) {

      alignas(64) std::atomic<int> seq1 = {-1};
      alignas(64) std::atomic<int> seq2 = {-1};

      auto t = std::thread([&] {
        pinThread(cpus[i]);
        for (auto m = 0; m < nsamples; ++m) {
          for (auto n = 0; n < samplessize; ++n) {
            while (seq1.load(std::memory_order_acquire) != n)
              ;
            seq2.store(n, std::memory_order_release);
          }
        }
      });

      std::chrono::nanoseconds rtt = std::chrono::nanoseconds::max();

      pinThread(cpus[j]);
      for (int m = 0; m < nsamples; ++m) {
        seq1 = seq2 = -1;
        auto ts1 = std::chrono::steady_clock::now();
        for (auto n = 0; n < samplessize; ++n) {
          seq1.store(n, std::memory_order_release);
          while (seq2.load(std::memory_order_acquire) != n)
            ;
        }
        auto ts2 = std::chrono::steady_clock::now();
        rtt = std::min(rtt, ts2 - ts1);
      }

      t.join();

      data[{i, j}] = rtt / 4 / samplessize;
      data[{j, i}] = rtt / 4 / samplessize;
    }
  }

  if (plot) {
    std::cout
        << "set title \"Inter-core one-way data latency between CPU cores\"\n"
        << "set palette rgb 33,13,10\n"
        << "set xlabel \"CPU\"\n"
        << "set ylabel \"CPU\"\n"
        << "set cblabel \"Latency (ns)\"\n"
        << "unset key\n"
        << "$data << EOD\n";
  }

  std::cout << std::setw(4) << "CPU";
  for (size_t i = 0; i < cpus.size(); ++i) {
    std::cout << " " << std::setw(4) << cpus[i];
  }
  std::cout << std::endl;
  for (size_t i = 0; i < cpus.size(); ++i) {
    std::cout << std::setw(4) << cpus[i];
    for (size_t j = 0; j < cpus.size(); ++j) {
      std::cout << " " << std::setw(4) << data[{i, j}].count();
    }
    std::cout << std::endl;
  }

  if (plot) {
    std::cout << "EOD\n"
              << "plot '$data' matrix rowheaders columnheaders using 2:1:3 with image,"
              << "'$data' matrix rowheaders columnheaders using 2:1:(sprintf('%g',$3)) with labels\n";
  }

  return 0;
}
