# Validate a large set of ghost replays in parallel. A producer thread walks the ghost directories
# and feeds ghost paths into a bounded queue. Each consumer thread owns a persistent Kinoko process
# running in replay worker mode (`kinoko replay --worker`), hands it one ghost at a time over stdin,
# and collects the result line it prints. Workers that crash or hang are restarted, and the ghost
# that caused it is recorded as CRASH or TIMEOUT.

import argparse
import collections
import ctypes
import os
import queue
import subprocess
import sys
import threading
import time
from typing import Iterable, Iterator, Optional

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEFAULT_KINOKO = os.path.join(REPO_ROOT, 'out', 'kinoko.exe' if sys.platform == 'win32' else 'kinoko')
DEFAULT_GHOST_ROOT = 'E:\\AllSyncedGhosts'

RESULT_PREFIX = 'KINOKO_RESULT\t'
STATUSES = ['SYNC', 'DESYNC', 'NOFINISH', 'ERROR', 'CRASH', 'TIMEOUT']

_DONE = object()


def suppress_windows_error_dialogs():
    # A crashing worker must not block on a Windows Error Reporting dialog. The error mode is
    # inherited by child processes.
    if sys.platform != 'win32':
        return

    SEM_FAILCRITICALERRORS = 0x0001
    SEM_NOGPFAULTERRORBOX = 0x0002
    SEM_NOOPENFILEERRORBOX = 0x8000
    ctypes.windll.kernel32.SetErrorMode(
        SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX)


def walk_ghosts(path: str) -> Iterator[str]:
    if os.path.isfile(path):
        yield path
        return

    stack = [path]
    while stack:
        current = stack.pop()
        try:
            with os.scandir(current) as it:
                subdirs = []
                for entry in it:
                    if entry.is_dir(follow_symlinks=False):
                        subdirs.append(entry.path)
                    elif entry.name.lower().endswith('.rkg'):
                        yield entry.path
                # Visit subdirectories in sorted order for a deterministic traversal
                stack.extend(sorted(subdirs, reverse=True))
        except OSError as e:
            print(f'Unable to scan {current}: {e}', file=sys.stderr)


def read_ghost_list(list_path: str) -> Iterator[str]:
    with open(list_path, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if line:
                yield line


class Stats:
    def __init__(self):
        self.lock = threading.Lock()
        self.counts = collections.Counter()
        self.processed = 0
        self.discovered = 0
        self.discovery_done = False

    def record(self, status: str):
        with self.lock:
            self.counts[status] += 1
            self.processed += 1


class ResultWriter:
    def __init__(self, path: str, include_synced: bool):
        self.lock = threading.Lock()
        self.include_synced = include_synced
        self.file = open(path, 'w', encoding='utf-8', newline='\n')
        self.file.write('status\tpath\tmessage\n')

    def write(self, status: str, path: str, message: str):
        if status == 'SYNC' and not self.include_synced:
            return

        message = message.replace('\t', ' ').replace('\r', ' ').replace('\n', ' | ')
        with self.lock:
            self.file.write(f'{status}\t{path}\t{message}\n')

    def close(self):
        with self.lock:
            self.file.close()


class KinokoWorker:
    """A persistent Kinoko process in replay worker mode."""

    def __init__(self, kinoko: str, cwd: str, stop: threading.Event):
        self.kinoko = kinoko
        self.cwd = cwd
        self.stop = stop
        self.lock = threading.Lock()
        self.proc: Optional[subprocess.Popen] = None
        self.lines: queue.Queue = queue.Queue()
        self.start()

    def start(self):
        with self.lock:
            if self.stop.is_set():
                return

            self.lines = queue.Queue()
            self.proc = subprocess.Popen(
                [self.kinoko, 'replay', '--worker'],
                cwd=self.cwd,
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                encoding='utf-8',
                errors='replace',
                bufsize=1,
            )
            # Drain stdout on a dedicated thread so the consumer can wait with a timeout
            threading.Thread(target=self._read_output, args=(self.proc, self.lines),
                             daemon=True).start()

    @staticmethod
    def _read_output(proc: subprocess.Popen, lines: queue.Queue):
        for line in proc.stdout:
            lines.put(line.rstrip('\r\n'))
        lines.put(None)

    def replay(self, ghost: str, timeout: float) -> Optional[tuple[str, str]]:
        """Replays a ghost, returning (status, message), or None if the run is being stopped.
        Restarts the process on crash/timeout."""
        log = collections.deque(maxlen=8)

        try:
            self.proc.stdin.write(ghost + '\n')
            self.proc.stdin.flush()
        except OSError:
            # The process died before receiving the ghost; collect whatever it printed
            pass

        deadline = time.monotonic() + timeout
        while True:
            remaining = deadline - time.monotonic()
            try:
                line = self.lines.get(timeout=max(remaining, 0))
            except queue.Empty:
                self.kill()
                if self.stop.is_set():
                    return None
                self.start()
                return 'TIMEOUT', f'No result after {timeout:g}s'

            if line is None:
                code = self.proc.wait()
                if self.stop.is_set():
                    return None
                self.start()
                detail = ' | '.join(log) if log else 'no output'
                return 'CRASH', f'Exit code {code}: {detail}'

            if line.startswith(RESULT_PREFIX):
                parts = line.split('\t', 3)
                status = parts[1] if len(parts) > 1 else 'ERROR'
                message = parts[3] if len(parts) > 3 else ''
                if log and status != 'SYNC':
                    message = f'{message} | {" | ".join(log)}' if message else ' | '.join(log)
                return status, message

            if line:
                log.append(line)

    def kill(self):
        with self.lock:
            if self.proc and self.proc.poll() is None:
                self.proc.kill()
                self.proc.wait()

    def close(self):
        if not self.proc:
            return

        try:
            self.proc.stdin.close()
        except OSError:
            pass

        try:
            self.proc.wait(timeout=10)
        except subprocess.TimeoutExpired:
            self.kill()


def producer(sources: Iterable[Iterator[str]], work: queue.Queue, stats: Stats,
             stop: threading.Event, num_consumers: int):
    try:
        for source in sources:
            for ghost in source:
                while not stop.is_set():
                    try:
                        work.put(ghost, timeout=0.5)
                        break
                    except queue.Full:
                        continue

                if stop.is_set():
                    return

                with stats.lock:
                    stats.discovered += 1
    finally:
        with stats.lock:
            stats.discovery_done = True

        for _ in range(num_consumers):
            work.put(_DONE)


def consumer(kinoko: str, cwd: str, timeout: float, work: queue.Queue, stats: Stats,
             writer: ResultWriter, stop: threading.Event, verbose: bool,
             workers: list, workers_lock: threading.Lock):
    worker = KinokoWorker(kinoko, cwd, stop)
    with workers_lock:
        workers.append(worker)

    try:
        while not stop.is_set():
            ghost = work.get()
            if ghost is _DONE:
                break

            result = worker.replay(ghost, timeout)
            if result is None:
                break

            status, message = result
            stats.record(status)
            writer.write(status, ghost, message)

            if verbose and status != 'SYNC':
                print(f'{status}: {ghost} {message}', flush=True)
    finally:
        if stop.is_set():
            worker.kill()
        else:
            worker.close()


def format_duration(seconds: float) -> str:
    seconds = int(seconds)
    return f'{seconds // 3600}:{seconds // 60 % 60:02}:{seconds % 60:02}'


def print_progress(stats: Stats, start_time: float):
    with stats.lock:
        processed = stats.processed
        discovered = stats.discovered
        done = stats.discovery_done
        counts = dict(stats.counts)

    elapsed = time.perf_counter() - start_time
    rate = processed / elapsed if elapsed > 0 else 0.0
    failed = processed - counts.get('SYNC', 0)
    error_rate = failed / processed * 100 if processed else 0.0

    total = f'{discovered}' if done else f'{discovered}+'
    eta = ''
    if done and rate > 0:
        eta = f', ETA {format_duration((discovered - processed) / rate)}'

    breakdown = ', '.join(f'{s} {counts[s]}' for s in STATUSES if counts.get(s))
    print(f'[{format_duration(elapsed)}] {processed}/{total} ghosts ({rate:.1f}/s{eta}) - '
          f'{error_rate:.3f}% failed ({breakdown})', flush=True)


def main():
    parser = argparse.ArgumentParser(
        description='Validate ghost replays in parallel using a pool of Kinoko worker processes.')
    parser.add_argument('paths', nargs='*',
                        help=f'Ghost files or directories (searched recursively). Defaults to '
                             f'{DEFAULT_GHOST_ROOT} if neither paths nor --list are provided.')
    parser.add_argument('--list', dest='list_file',
                        help='File containing one ghost path per line')
    parser.add_argument('-j', '--jobs', type=int, default=os.cpu_count() or 1,
                        help='Number of worker processes (default: logical CPU count)')
    parser.add_argument('--kinoko', default=DEFAULT_KINOKO, help='Path to the Kinoko executable')
    parser.add_argument('--cwd', default=None,
                        help='Working directory for Kinoko, which must contain the game assets '
                             '(default: directory of the Kinoko executable)')
    parser.add_argument('-o', '--output', default='parallel_results.tsv',
                        help='Output TSV of results (default: parallel_results.tsv)')
    parser.add_argument('--include-synced', action='store_true',
                        help='Also write synced ghosts to the output file')
    parser.add_argument('--timeout', type=float, default=60.0,
                        help='Seconds before a single replay is considered hung (default: 60)')
    parser.add_argument('--progress', type=float, default=10.0,
                        help='Seconds between progress reports (default: 10)')
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='Print every non-synced ghost as it completes')
    args = parser.parse_args()

    kinoko = os.path.abspath(args.kinoko)
    if not os.path.isfile(kinoko):
        parser.error(f'Kinoko executable not found: {kinoko}')

    cwd = os.path.abspath(args.cwd) if args.cwd else os.path.dirname(kinoko)
    jobs = max(1, args.jobs)

    sources = [walk_ghosts(p) for p in args.paths]
    if args.list_file:
        sources.append(read_ghost_list(args.list_file))
    if not sources:
        sources.append(walk_ghosts(DEFAULT_GHOST_ROOT))

    suppress_windows_error_dialogs()

    stats = Stats()
    writer = ResultWriter(args.output, args.include_synced)
    stop = threading.Event()
    work: queue.Queue = queue.Queue(maxsize=jobs * 64)
    workers: list = []
    workers_lock = threading.Lock()

    print(f'Running {jobs} Kinoko workers ({kinoko})', flush=True)
    start_time = time.perf_counter()

    producer_thread = threading.Thread(target=producer, args=(sources, work, stats, stop, jobs),
                                       daemon=True)
    consumer_threads = [
        threading.Thread(target=consumer,
                         args=(kinoko, cwd, args.timeout, work, stats, writer, stop, args.verbose,
                               workers, workers_lock),
                         daemon=True)
        for _ in range(jobs)
    ]

    producer_thread.start()
    for t in consumer_threads:
        t.start()

    interrupted = False
    try:
        next_report = time.perf_counter() + args.progress
        while any(t.is_alive() for t in consumer_threads):
            time.sleep(0.25)
            if time.perf_counter() >= next_report:
                print_progress(stats, start_time)
                next_report += args.progress
    except KeyboardInterrupt:
        interrupted = True
        print('Interrupted, stopping workers...', flush=True)
        stop.set()
        with workers_lock:
            for worker in workers:
                worker.kill()
        # Unblock consumers waiting on an empty queue
        for _ in range(jobs):
            try:
                work.put_nowait(_DONE)
            except queue.Full:
                break
        for t in consumer_threads:
            t.join(timeout=15)
    finally:
        writer.close()

    total_time = time.perf_counter() - start_time

    print_progress(stats, start_time)
    print(f'Total evaluation time: {format_duration(total_time)} ({total_time:.2f} seconds)')
    print(f'Results written to {os.path.abspath(args.output)}')

    if interrupted:
        sys.exit(130)

    failed = stats.processed - stats.counts.get('SYNC', 0)
    sys.exit(1 if failed else 0)


if __name__ == '__main__':
    main()
