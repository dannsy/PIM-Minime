"""
Short script to run the DPU Minime program
"""

import argparse
import os
import subprocess
from datetime import datetime

DEBUG = 0


def make(num_dpus=1, num_tasklets=1, block_size=2, log=False):
    # Calling make clean and make with the right number of DPUs and tasklets
    command = [
        f"make clean && make NR_DPUS={num_dpus} NR_TASKLETS={num_tasklets} BLOCK_SIZE={block_size}",
    ]
    if DEBUG:
        print(f"Command being executed: {command}")

    subprocess.run(command, shell=True, capture_output=not log)


def execute(mode, total_mem="32M", bench_time=10):
    cycle_str = "Average cycles per DPU: "
    throughput_str = "Overall throughput: "

    # command = [f"./host -t {mode} -g {total_mem} -T {bench_time}"]
    command = ["./host", "-t", f"{mode}", "-g", f"{total_mem}", "-T", f"{bench_time}"]
    completed_process = subprocess.run(command, capture_output=True)
    stdout_str = completed_process.stdout.decode("utf-8")

    start_index = stdout_str.find(cycle_str) + len(cycle_str)
    end_index = stdout_str[start_index:].find("\n")
    cycles = stdout_str[start_index : start_index + end_index]

    start_index = stdout_str.find(throughput_str) + len(throughput_str)
    end_index = stdout_str[start_index:].find("\n")
    throughput = stdout_str[start_index : start_index + end_index]
    # print(f"{cycles}, {throughput}")
    print(stdout_str)


def benchmark(mode, num_dpus, bench_time):
    # The number of tasklets to benchmark with
    # tasklets_list = [1, 2, 4, 8, 12, 16, 24]
    tasklets_list = [1]
    block_sizes = [2, 4, 8, 16, 32, 64, 128, 256, 512]

    stars = "*" * 28
    for num in tasklets_list:
        for block_size in block_sizes:
            print(
                f"\n{stars} Benchmark with {num_dpus} DPUs and {num} tasklets block size {block_size} {stars}\n"
            )
            make(num_dpus, num, block_size)
            execute(mode, bench_time=bench_time)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="something")
    parser.add_argument("mode", metavar="-m", type=int, help="The benchmark mode")
    parser.add_argument("num_dpus", metavar="-n", type=int, help="The number of DPUs")
    parser.add_argument(
        "bench_time", metavar="-t", type=int, help="The benchmark duration (in seconds)"
    )
    args = parser.parse_args()

    mode = args.mode
    num_dpus = args.num_dpus
    bench_time = args.bench_time

    # Changing current working directory to src (which contains the Makefile and C files)
    cwd = os.getcwd()
    file_dir = os.path.dirname(__file__)
    os.chdir(os.path.join(os.getcwd(), os.path.dirname(__file__)))
    os.chdir(os.path.join("..", "src"))

    start_stars = "*" * 30
    end_stars = "*" * 32
    print(f"\n{start_stars} Starting benchmark for Minime {start_stars}\n")

    benchmark(mode, num_dpus, bench_time)

    print(f"\n{end_stars} Done benchmark for Minime {end_stars}")
