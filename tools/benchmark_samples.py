#!/usr/bin/env python3

"""Benchmarks Kinoko by replaying every ghost in the samples folder using replay worker mode.

Each run starts one `kinoko replay --worker` process, feeds it every sample ghost over stdin, and
measures the wall time until the process exits. Passing multiple executables benchmarks them in an
interleaved order, so that background noise affects each build equally.

Usage (from the repository root):
    python tools/benchmark_samples.py                                 # benchmark out/kinoko.exe
    python tools/benchmark_samples.py baseline.exe out/kinoko.exe     # compare two builds
    python tools/benchmark_samples.py --runs 10 --warmup 2 a.exe b.exe
"""

import ctypes
import collections
import os
import statistics
import subprocess
import sys
import time
from argparse import ArgumentParser

ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT_DIR = os.path.join(ROOT_DIR, 'out')
DEFAULT_KINOKO = os.path.join(OUT_DIR, 'kinoko.exe' if sys.platform == 'win32' else 'kinoko')
DEFAULT_SAMPLES = os.path.join(ROOT_DIR, 'samples')

RESULT_PREFIX = 'KINOKO_RESULT\t'


def suppress_windows_error_dialogs():
    # A crashing Kinoko must not block on a Windows Error Reporting dialog
    if sys.platform == 'win32':
        SEM_FAILCRITICALERRORS = 0x0001
        SEM_NOGPFAULTERRORBOX = 0x0002
        SEM_NOOPENFILEERRORBOX = 0x8000
        ctypes.windll.kernel32.SetErrorMode(
            SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX)


def find_ghosts(samples_dir):
    ghosts = []
    for dirpath, _, filenames in os.walk(samples_dir):
        for filename in filenames:
            if filename.lower().endswith('.rkg'):
                ghosts.append(os.path.abspath(os.path.join(dirpath, filename)))
    return sorted(ghosts)


def run_once(kinoko, ghosts, cwd):
    """Replays all ghosts in a single worker process.

    Returns the elapsed wall time in seconds and a dict mapping ghost path to status.
    """
    stdin = ''.join(f'{ghost}\n' for ghost in ghosts)

    start = time.perf_counter()
    proc = subprocess.Popen([kinoko, 'replay', '--worker'], cwd=cwd, stdin=subprocess.PIPE,
                            stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True,
                            encoding='utf-8', errors='replace')
    stdout, _ = proc.communicate(stdin)
    elapsed = time.perf_counter() - start

    results = {}
    for line in stdout.splitlines():
        if line.startswith(RESULT_PREFIX):
            fields = line[len(RESULT_PREFIX):].split('\t')
            if len(fields) >= 2:
                results[fields[1]] = fields[0]

    missing = len(ghosts) - len(results)
    if missing > 0:
        sys.exit(f'{kinoko} exited with code {proc.returncode} before reporting {missing} of '
                 f'{len(ghosts)} ghosts. Does this build support `replay --worker`?')

    return elapsed, results


def format_stats(times, num_ghosts):
    median = statistics.median(times)
    stdev = statistics.stdev(times) if len(times) > 1 else 0.0
    return (f'min {min(times):8.3f}s  median {median:8.3f}s  mean {statistics.mean(times):8.3f}s  '
            f'stdev {stdev:6.3f}s  ({median / num_ghosts * 1000:.1f} ms/ghost)')


def main():
    parser = ArgumentParser(description='Benchmark Kinoko replay worker mode on sample ghosts')
    parser.add_argument('kinoko', nargs='*', default=[DEFAULT_KINOKO],
                        help='Kinoko executables to benchmark; the first is the baseline '
                             '(default: out/kinoko)')
    parser.add_argument('--samples', default=DEFAULT_SAMPLES,
                        help='directory searched recursively for .rkg ghosts (default: samples)')
    parser.add_argument('--runs', type=int, default=100,
                        help='timed runs per executable (default: 100)')
    parser.add_argument('--warmup', type=int, default=1,
                        help='untimed runs per executable before timing (default: 1)')
    parser.add_argument('--cwd', default=OUT_DIR,
                        help='working directory containing the extracted Race folder '
                             '(default: out)')
    args = parser.parse_args()

    if args.runs < 1:
        parser.error('--runs must be at least 1')

    executables = [os.path.abspath(exe) for exe in args.kinoko]
    for exe in executables:
        if not os.path.isfile(exe):
            sys.exit(f'Executable not found: {exe}')

    ghosts = find_ghosts(args.samples)
    if not ghosts:
        sys.exit(f'No .rkg files found in {args.samples}')

    suppress_windows_error_dialogs()

    print(f'{len(ghosts)} ghosts, {len(executables)} executable(s), '
          f'{args.warmup} warmup + {args.runs} timed run(s) each\n')

    for i in range(args.warmup):
        for exe in executables:
            print(f'Warmup {i + 1}/{args.warmup}: {exe}', flush=True)
            run_once(exe, ghosts, args.cwd)

    times = [[] for _ in executables]
    statuses = [None] * len(executables)
    for i in range(args.runs):
        # Rotate the order each round so no build is consistently run first
        order = list(range(len(executables)))
        offset = i % len(executables)
        for idx in order[offset:] + order[:offset]:
            exe = executables[idx]
            elapsed, results = run_once(exe, ghosts, args.cwd)
            times[idx].append(elapsed)
            if statuses[idx] is None:
                statuses[idx] = results
            print(f'Run {i + 1}/{args.runs}: {elapsed:8.3f}s  [{idx}] {exe}', flush=True)

    print('\nResults:')
    baseline_median = statistics.median(times[0])
    for idx, exe in enumerate(executables):
        print(f'  [{idx}] {exe}{" (baseline)" if idx == 0 and len(executables) > 1 else ""}')
        print(f'    {format_stats(times[idx], len(ghosts))}')

        counts = collections.Counter(statuses[idx].values())
        print('    ' + ', '.join(f'{status}: {count}' for status, count in sorted(counts.items())))

        if idx != 0:
            delta = (statistics.median(times[idx]) - baseline_median) / baseline_median * 100
            print(f'    median vs baseline: {delta:+.2f}% '
                  f'({"faster" if delta < 0 else "slower"})')

            # Branch hints should never change replay results
            for ghost in ghosts:
                if statuses[idx][ghost] != statuses[0][ghost]:
                    print(f'    WARNING: {ghost}: {statuses[0][ghost]} -> '
                          f'{statuses[idx][ghost]}')


if __name__ == '__main__':
    main()
