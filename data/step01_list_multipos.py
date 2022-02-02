#!/usr/bin/env python3
#-*- coding: utf-8 -*-

import os, sys

############################################################

def load_dict(wordtbl, fname):
    fhdl = open(fname, 'r')
    for line in fhdl:
        line = line.strip()
        if len(line) == 0: continue
        if line.startswith('#') or line.startswith('%'): continue
        
        word, postag = line.rsplit('.', maxsplit=1)
        if word not in wordtbl:
            wordtbl[word] = set()
        wordtbl[word].add((postag, fname))
    fhdl.close()

############################################################

def cluster_postags(wordtbl):
    clstbl = {}
    for word in wordtbl:
        cls = frozenset(wordtbl[word])
        if cls not in clstbl:
            clstbl[cls] = set()
        clstbl[cls].add(word)
    return clstbl

############################################################

def print_wordlist(wordlist, colno=10):
    wordlist = sorted(wordlist)
    for i in range(len(wordlist)):
        if i % colno == 0:
            print('\n', end='')
        print(wordlist[i], end='  ')
    print()

############################################################

def print_clusters(clstbl, wordtbl):

    def repr_cls(cls):
        return '\n\t'.join([f'{x}/{y}' for (x, y) in cls])

    print(f'Total number of distinct words = {len(wordtbl)}\n')

    clsidx = 0
    clslist = [x for (i, x) in sorted([(len(cls), cls) for cls in clstbl], reverse=True)]
    for idx, cls in enumerate(clslist):
        print(40 * '-')
        print(f'Class {idx + 1}:\n\t{repr_cls(cls)}', end='')
        print_wordlist(clstbl[cls])
        print()

############################################################

def main():
    wordtbl = {}
    for fname in sys.argv[1:]:
        sys.stderr.write(f'Reading: {fname} ...'); sys.stderr.flush()
        load_dict(wordtbl, fname)
        sys.stderr.write(f' done\n'); sys.stderr.flush()
    clstbl = cluster_postags(wordtbl)
    print_clusters(clstbl, wordtbl)

############################################################

if __name__ == '__main__':
    main()
