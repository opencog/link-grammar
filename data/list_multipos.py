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
        
        if '.' in line:
            word, postag = line.rsplit('.', maxsplit=1)
        else:
            word, postag = line, 'any'
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

def print_postbl(clstbl, wordtbl, fields, fieldmap):
    clsidx = 0
    clslist = [x for (i, x) in sorted([(len(cls), cls) for cls in clstbl], reverse=True)]
    print('\t' + '\t'.join(fields))
    emptyrow = '\t'.join([f if f == '|' else '' for f in fields])
    print('\t' + emptyrow)

    for cls in clslist:
        tempcls = [fieldmap[x[1]] for x in cls]
        # input(f'tempcls = {tempcls}')
        for word in clstbl[cls]:
            print(word, end='')
            for field in fields:
                if field == '|':
                    print('\t|', end='')
                elif field in tempcls:
                    print('\t1', end='')
                else:
                    print('\t', end='')
            print()
        print('\t' + emptyrow)

############################################################

def main():
    wordtbl = {}
    for fname in sys.argv[1:]:
        sys.stderr.write(f'Reading: {fname} ...'); sys.stderr.flush()
        load_dict(wordtbl, fname)
        sys.stderr.write(f' done\n'); sys.stderr.flush()
    clstbl = cluster_postags(wordtbl)
    # print_clusters(clstbl, wordtbl)

    fields = [
        '|',
        'V', 'VA', '|',
        'N', 'NA', 'NP', '|',
        'PR', 'RL', 'SM', '|',
        'J', 'JL', '|',
        'PAA', 'PAN', 'PAV', 'PNA', 'PNN', 'PNV', 'PVA', 'PVN', 'PVV', '|',
        'R', 'RA', 'RC', 'RQ', 'X', 'NGL', 'NGR', 'PS', 'PA', 'IJ', '|',
        'NUM', 'NUMV', 'NUMO', 'ORD', '|',
        'CLN', 'CLV', 'QFL', 'QFR', '|',
        'CD', 'CN', 'CV', '|',
        'ANY', '|'
    ]
    fieldmap = {
        'words.adj.common': 'J',
        'words.adj.lhs': 'JL',
        'words.adv.attrmod': 'RA',
        'words.adv.cohesive': 'RC',
        'words.adv.common': 'R',
        'words.adv.qf': 'RQ',
        'words.aux.common': 'X',
        'words.cl.noun': 'CLN',
        'words.cl.verb': 'CLV',
        'words.conj.discont': 'CD',
        'words.conj.noun': 'CN',
        'words.conj.verb': 'CV',
        'words.intj.common': 'IJ',
        'words.n.attribute': 'NA',
        'words.n.common': 'N',
        'words.n.prefix': 'NP',
        'words.neg.lhs': 'NGL',
        'words.neg.rhs': 'NGR',
        'words.num.common': 'NUM',
        'words.num.movable': 'NUMV',
        'words.num.ordinal': 'NUMO',
        'words.ordmark': 'ORD',
        'words.part.common': 'PA',
        'words.passmark': 'PS',
        'words.prep-n.n': 'PNN',
        'words.prep-n.v': 'PNV',
        'words.prep-v.n': 'PVN',
        'words.prep-v.nv': 'PVA',
        'words.prep-v.v': 'PVV',
        'words.prep.lw': 'PLW',
        'words.prep.n': 'PAN',
        'words.prep.nv': 'PAA',
        'words.prep.v': 'PAV',
        'words.pro.common': 'PR',
        'words.qf.lhs': 'QFL',
        'words.qf.rhs': 'QFR',
        'words.relpro.common': 'RL',
        'words.sclmark': 'SM',
        'words.v.attribute': 'VA',
        'words.v.common': 'V',

        'words.n.title': 'ANY',
        'words.n.designator': 'ANY',
        'words.n.person': 'ANY',
        'words.n.legalperson': 'ANY',
        'words.n.location': 'ANY',
        'words.n.organization': 'ANY',
        'words.n.brand': 'ANY',
        'words.n.term': 'ANY',
    }
    print_postbl(clstbl, wordtbl, fields, fieldmap)

############################################################

if __name__ == '__main__':
    main()
