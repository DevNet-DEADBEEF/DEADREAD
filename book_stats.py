import json
import csv

def calc_stats(cachefile):
    with open(cachefile, 'r') as f:
        data = json.load(f)

    for book in data:
        bookdata = data[book]

        print(f"{bookdata['title']} \\ {bookdata['words'][0]} {bookdata['words'][1]} {bookdata['words'][2]} {bookdata['words'][3]} {bookdata['words'][4]} \\ {bookdata['avgSenLen']}")


def calc_stats_blacklist(cachefile, blacklist):
    with open(cachefile, 'r') as f:
        data = json.load(f)
    
    with open(blacklist, newline='') as f:
        reader = csv.reader(f)
        wordblacklist = list(reader)

    for book in data:
        bookdata = data[book]

        topwords = []
        wordIndex = 0

        while len(topwords) < 5:
            allowWord = True
            word = bookdata['words'][wordIndex]
            for blist in wordblacklist:
                if word in blist:
                    allowWord = False
                    break
            if allowWord:
                topwords.append(word)
            wordIndex += 1

        print(f"{bookdata['title']} \\ {topwords[0]} {topwords[1]} {topwords[2]} {topwords[3]} {topwords[4]} \\ {bookdata['avgSenLen']}")
