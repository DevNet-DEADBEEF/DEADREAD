from datetime import datetime
from http.server import HTTPServer, BaseHTTPRequestHandler
import os
import time
import subprocess
import sys
import threading
import random


mark_cache = None
mark_cache_time = 0
git_eval = None

def setup():
    global git_eval

    subprocess.call("git pull", shell=True)
    subprocess.call("make speedtest", shell=True)
    git_eval = subprocess.check_output("git log HEAD -n 1", shell=True).decode('utf-8').strip()

setup()

def mark():
    times = []

    root_dir = sys.argv[1]
    files = os.listdir(root_dir)
    random.shuffle(files)
    # files = files[:(len(files) // 5)]  # Use only half of the files for benchmarking
    for file in files:
        if file.endswith('.txt'):
            print(f"Benchmarking {file} ({files.index(file) + 1}/{len(files)})...")
            file_path = os.path.join(root_dir, file)
            for i in range(15):  # Run each file x times
                print(f"Run {i+1}/15: ???.???ms", end="          \r")
                start_time = time.perf_counter_ns()
                subprocess.Popen(f"./speedtest {file_path} > cpp.dump.debugify.help.pls.fix", shell=True, stdout=None, stderr=None).wait()
                end_time = time.perf_counter_ns()
                elapsed_time = (end_time - start_time) / 1_000_000  # Convert to milliseconds
                times.append(elapsed_time)
                print(f"Run {i+1}/15: {elapsed_time:.3f}ms", end="          \r")

    avg = sum(times) / len(times) if times else 0
    min_t = min(times) if times else 0
    max_t = max(times) if times else 0

    print(f"Benchmark Results over {len(times)} files:")
    print(f"Average time: {avg:.3f}ms")
    print(f"Minimum time: {min_t:.3f}ms")
    print(f"Maximum time: {max_t:.3f}ms")
    return f"Average: {avg:.3f}ms, Min: {min_t:.3f}ms, Max: {max_t:.3f}ms, Totals: {len(times)} books"


def thread_mark():
    while True:
        global mark_cache, mark_cache_time
        mark_cache = mark()
        mark_cache_time = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        time.sleep(10)  # Sleep for 5 seconds


class SimpleHTTPRequestHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)
        self.end_headers()
        if mark_cache is None:
            self.wfile.write(b"Benchmarking in progress, please wait...")
        else:
            response = f"Last Benchmark at {mark_cache_time}\nOn {git_eval}\n{mark_cache}"
            self.wfile.write(response.encode('utf-8'))

thread = threading.Thread(target=thread_mark, daemon=True)
thread.start()
print("Started background benchmarking thread.")

httpd = HTTPServer(('localhost', 42789), SimpleHTTPRequestHandler)
print("Starting server on http://localhost:42789")
httpd.serve_forever()
