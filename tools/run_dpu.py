"""
Short script to run the DPU Minime program
"""

import os
import subprocess
from datetime import datetime

ITERATIONS = 10
DEBUG = 1


def make(num_dpus=1, num_tasklets=1):
    command = [
        f"make clean && make NR_DPUS={num_dpus} NR_TASKLETS={num_tasklets}",
    ]
    if DEBUG:
        print(f"Command being executed: {command}")

    print()
    subprocess.run(command, shell=True)


def benchmark(mode, total_mem="32M", bench_time=10):
    command = [f"./host -d -t {mode} -g {total_mem} -T {bench_time}"]
    subprocess.run(command, shell=True)


if __name__ == "__main__":
    # Changing current working directory to src (which contains the Makefile and C files)
    cwd = os.getcwd()
    file_dir = os.path.dirname(__file__)
    os.chdir(os.path.join(os.getcwd(), os.path.dirname(__file__)))
    os.chdir(os.path.join("..", "src"))

    # Getting user input
    num_dpus = int(input("Number of DPUs: "))
    num_tasklets = int(input("Number of tasklets: "))
    mode = input("Choose mode (0: sequential read, 1: random read): ")
    bench_time = int(input("Benchmark time: "))

    # Calling make clean and make with the right number of DPUs and tasklets
    make(num_dpus, num_tasklets)

    start_stars = "*" * 30
    end_stars = "*" * 32
    print(f"\n{start_stars} Starting benchmark for Minime {start_stars}\n")

    benchmark(mode, bench_time=bench_time)

    print(f"\n{end_stars} Done benchmark for Minime {end_stars}")
