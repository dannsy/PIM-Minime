"""
Short script to run the DPU Minime program
"""

import os
import subprocess
from datetime import datetime

DEBUG = 0


def make(num_dpus=1, num_tasklets=1, log=False):
    # Calling make clean and make with the right number of DPUs and tasklets
    command = [
        f"make clean && make NR_DPUS={num_dpus} NR_TASKLETS={num_tasklets}",
    ]
    if DEBUG:
        print(f"Command being executed: {command}")

    subprocess.run(command, shell=True, capture_output=not log)


def execute(mode, total_mem="32M", bench_time=10):
    command = [f"./host -t {mode} -g {total_mem} -T {bench_time}"]
    subprocess.run(command, shell=True)


def benchmark(num_dpus, mode):
    # The number of tasklets to benchmark with
    tasklets_list = [1, 2, 4, 8, 12, 16, 24]

    stars = "*" * 28
    for num in tasklets_list:
        print(f"\n{stars} Benchmark with {num_dpus} DPUs and {num} tasklets {stars}\n")
        make(num_dpus, num)
        execute(mode, bench_time=0)


if __name__ == "__main__":
    # Changing current working directory to src (which contains the Makefile and C files)
    cwd = os.getcwd()
    file_dir = os.path.dirname(__file__)
    os.chdir(os.path.join(os.getcwd(), os.path.dirname(__file__)))
    os.chdir(os.path.join("..", "src"))

    # Getting user input
    num_dpus = int(input("Number of DPUs: "))
    mode = input("Choose mode (0: sequential read, 1: random read): ")
    # bench_time = int(input("Benchmark time: "))

    start_stars = "*" * 30
    end_stars = "*" * 32
    print(f"\n{start_stars} Starting benchmark for Minime {start_stars}\n")

    benchmark(num_dpus, mode)

    print(f"\n{end_stars} Done benchmark for Minime {end_stars}")
