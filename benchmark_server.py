from datetime import datetime
from http.server import HTTPServer, BaseHTTPRequestHandler
import os
import time
import subprocess
import sys
import threading


mark_cache = None
mark_cache_time = 0


def mark():
    times = []

    subprocess.call("git pull", shell=True)

    root_dir = sys.argv[1]
    for file in os.listdir(root_dir):
        if file.endswith('.txt'):
            file_path = os.path.join(root_dir, file)
            start_time = time.perf_counter_ns()
            subprocess.Popen(f"./main {file_path}", shell=True, stdout=None, stderr=None).wait()
            end_time = time.perf_counter_ns()
            elapsed_time = (end_time - start_time) / 1_000_000  # Convert to milliseconds
            times.append(elapsed_time)

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
        time.sleep(60)  # Sleep for 1 min


class SimpleHTTPRequestHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)
        self.end_headers()
        if mark_cache is None:
            self.wfile.write(b"Benchmarking in progress, please wait...")
        else:
            response = f"Last Benchmark at {mark_cache_time}\n{mark_cache}"
            self.wfile.write(response.encode('utf-8'))

thread = threading.Thread(target=thread_mark, daemon=True)
thread.start()
print("Started background benchmarking thread.")

httpd = HTTPServer(('localhost', 8000), SimpleHTTPRequestHandler)
print("Starting server on http://localhost:8000")
httpd.serve_forever()
