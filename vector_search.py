time = __import__("time")

parser = __import__("argparse").ArgumentParser(
    description="Analyze a Project Gutenberg ebook for average sentence length and top 5 words."
);parser.add_argument(
    "search", help="Search the vector database.",
    nargs=2,
);parser.add_argument(
    "--debug", action="store_true", help="Enable debug output."
);parser.add_argument(
    "-f", "--cachefile", type=str, default="cache.pk.gz", help="Path to the cache file."
);args = parser.parse_args();imp_start = time.perf_counter()
sentence_transformers = __import__("sentence_transformers")
pickle = __import__("pickle")
np = __import__("numpy")
torch = __import__("torch")

print(f"Imports finished in {time.perf_counter() - imp_start:.3f} seconds")
basics_start = time.perf_counter()

sem_search = lambda query, embeds: sentence_transformers.util.semantic_search(model.encode_query(query), embeds, top_k=5)[0]

save_search = lambda cache_d, path: print(f"Saving cache to {path}...") or open(path, "wb").write(pickle.dumps(cache_d))
# def save_cache(cache, path):
#     print(f"Saving cache to {path}...")
#     with open(path, "wb") as fgz:
#         pickle.dump(cache, fgz)


print(f"Basics loaded in {time.perf_counter() - basics_start:.3f} seconds")
ebook_timer = time.perf_counter()

print(f"Loading cache {args.cachefile}...")
cache = dict()
with open(args.cachefile, "rb") as f:
    try:
        cache = pickle.load(f)
    except Exception as e:
        print(f"Error loading cache: {e}")

model = sentence_transformers.SentenceTransformer("tomaarsen/static-retrieval-mrl-en-v1", truncate_dim=256)
print(f"Loading cached embeddings")
data = cache["books"]
embeds = cache["vec"]

embeds = np.array(embeds)
embeds = torch.Tensor(embeds)
print(f"Embeddings loaded in {time.perf_counter() - ebook_timer:.3f} seconds")

query = args.search[1].strip()

query_timer = time.perf_counter_ns()
sims = sem_search(query, embeds)
print(f"Top {len(sims)} results ({(time.perf_counter_ns() - query_timer) // 1_000_000}ms):")
for sim in sims:
    simi = sim['corpus_id']
    score = sim['score']
    match = data.loc[(data['low_index'] <= simi) & (data['high_index'] >= simi)]['title'].values[0]
    print(f"{match} ({simi} {score:.3f})")
