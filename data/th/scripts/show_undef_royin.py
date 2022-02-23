#!/usr/bin/env python
#-*- coding: utf-8 -*-

import os, sys

############################################################

def read_royin(lexdict, fname):
    fhdl = open(fname)
    for line in fhdl:
        line = line.strip()
        if len(line) == 0: continue

        tokens = line.split(',')
        if tokens[0].startswith('"'): tokens[0] = tokens[0][1:-1].replace(' ', '_')
        if tokens[1].startswith('"'): tokens[1] = tokens[1][1:-1]
        word, pos = tokens

        if word not in lexdict:
            lexdict[word] = set()
        lexdict[word].add(pos)

    fhdl.close()

############################################################

def read_content(words, fname):
    fhdl = open(fname)
    for line in fhdl:
        line = line.strip()
        if len(line) == 0: continue
        tokens = tuple(line.rsplit('.', 1))
        if tokens[0] not in words:
            words[tokens[0]] = set()
        words[tokens[0]].add(tokens)
    fhdl.close()

############################################################

def main():
    royin_fname = sys.argv[1]
    lexdict = {}
    words = {}

    tagdict = {
        'น.': 'n',
        'ก.': 'v',
        'อ.': 'ij',
        'ว.': 'ว.',
        'บ.': 'p',
        'ส.': 'pr',
        'สัน.': 'c',
        'นิ.': 'นิ.',
    }
    tagnames = [('n', 'Nouns'), ('v', 'Verbs'), ('ij', 'Interjections'), ('p', 'Prepositions'), ('pr', 'Pronouns'), ('c', 'Conjunctions'), ('ว.', 'Adjectives/Adverbs'), ('นิ.', 'Nipáda')]

    undefdict = {}
    incordict = {}

    read_royin(lexdict, royin_fname)
    for fname in sys.argv[2:]:
        read_content(words, fname)

    for word in lexdict:
        if word not in words:
            if word.endswith('-'): continue
            for pos in lexdict[word]:
                if pos == 'null': continue
                tag = tagdict[pos]
                if tag not in undefdict:
                    undefdict[tag] = set()
                undefdict[tag].add(word)

    for word in words:
        if word in lexdict:

            pars_tags = set()
            for elem in words[word]:
                if len(elem) == 2:
                    pars_tags.add(elem[1])
                else:
                    pars_tags.add('n')
            
            royin_tags = set()
            for p in lexdict[word]:
                if p == 'null': continue
                if p != 'ว.':
                    royin_tags.add(tagdict[p])
                else:
                    royin_tags.add('r')
                    royin_tags.add('j')
            
            diff = pars_tags - royin_tags
            # input(f'{word} : {pars_tags} - {royin_tags}')
            if len(diff) > 0:
                for tag in diff:
                    if tag not in incordict:
                        incordict[tag] = set()
                    incordict[tag].add(word)

    for tag, name in tagnames:
        # if tag != 'ว.': continue

        # print(f'**** UNDEF {name} ****')
        # for word in sorted(undefdict[tag]):
        #     print(f'{word}.{tag}')
        # print()

        if tag in incordict:
            print(f'# INCORRECT: {name}')
            for word in sorted(incordict[tag]):
                print(f'{word}.{tag}')
            print()

############################################################

if __name__ == '__main__':
    main()
