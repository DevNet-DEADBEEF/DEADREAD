import os
import time

times = []

root_dir = os.path.dirname(os.path.abspath(__file__))

for file in os.listdir('./books'):
    if file.endswith('.txt'):
        file_path = os.path.join('books', file)
        start_time = time.perf_counter_ns()
        os.system(f"./main {file_path}")
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
