import numpy as np
import torch
from sentence_transformers import SentenceTransformer
from sentence_transformers.util import semantic_search

model = SentenceTransformer("tomaarsen/static-retrieval-mrl-en-v1", truncate_dim=256)

def sem_search(query: str, embeds, books, top_k=5, unique_books=False):
    query_embed = model.encode_query(query)

    top_k *= len(books)

    sims = semantic_search(query_embed, torch.Tensor(np.array(embeds)), top_k=top_k)[0]
    np_books = np.array(books)
    results = []
    seen_books = set()
    
    for sim in sims:
        simi = sim['corpus_id']
        score = sim['score']
        match = np_books[np_books[:, 3].astype(int) <= simi]
        match = match[match[:, 4].astype(int) >= simi]
        if len(match) > 0:
            match = match[0]
            book_id = match[0]
            
            if unique_books and book_id in seen_books:
                continue
            seen_books.add(book_id)
            
        else:
            match = ["Unknown", "Unknown", "Unknown", -1, -1]
        
        results.append((match, simi, score))
        
        if unique_books and len(results) >= top_k:
            break
    
    return results

def encode_sentences(sentences):
    return model.encode(sentences).tolist()
