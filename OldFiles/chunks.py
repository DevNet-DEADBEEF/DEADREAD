import os
import sys

if "merge" in sys.argv:
    with open("chunks/cache.7z", "wb") as f:
        for chunk in os.listdir("./chunks"):
            if chunk.startswith("chunk") and chunk.endswith(".bin"):
                print(f"Merging {chunk}...")
                with open(f"chunks/{chunk}", "rb") as fc:
                    f.write(fc.read())
    print("Done.")
else:
    file = "cache.7z"
    file_bytes = os.path.getsize(file)
    chunk_size = 1024 * 1_000_000  # 1 MB
    print(f"File size: {file_bytes} bytes -> {file_bytes / chunk_size:.2f} chunks")

    # split the file into chunks of 950 MB
    i = 0
    with open (file, "rb") as f:
        while not f.closed:
            chunk = f.read(chunk_size)
            if not chunk:
                break
            with open(f"chunks/chunk{i}.bin", "wb") as fc:
                print(f"Writing chunk {i}...")
                fc.write(chunk)
            i += 1
    print("Done.")
