import numpy as np
import torch
from sentence_transformers import SentenceTransformer
from sentence_transformers.util import semantic_search

model = SentenceTransformer("tomaarsen/static-retrieval-mrl-en-v1", truncate_dim=256)

def sem_search(query: str, embeds, books, top_k=5):
    query_embed = model.encode_query(query)
    sims = semantic_search(query_embed, torch.Tensor(np.array(embeds)), top_k=top_k)[0]
    np_books = np.array(books)
    results = []
    for sim in sims:
        simi = sim['corpus_id']
        score = sim['score']
        match = np_books[np_books[:, 3].astype(int) <= simi]
        match = match[match[:, 4].astype(int) >= simi]
        if len(match) > 0:
            match = match[0]
        else:
            match = ["Unknown", "Unknown", "Unknown", -1, -1]
        results.append((match, simi, score))
    return results

def encode_sentences(sentences):
    return model.encode(sentences).tolist()
