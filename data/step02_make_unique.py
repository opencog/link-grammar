#!/usr/bin/env python
#-*- coding: utf-8 -*-

import os, sys

############################################################

def read_content(content, fname):
    fhdl = open(fname)
    for line in fhdl:
        line = line.strip()
        if len(line) == 0: continue
        content.add(line)
    fhdl.close()

############################################################

def create_node(filetree):
    fname = filetree[0]
    result = {'fname': fname, 'words': set(), 'dtrs': []}
    if fname is not None:
        read_content(result['words'], fname)
    for dtr in filetree[1]:
        tree = create_node(dtr)
        result['dtrs'].append(tree)
    return result

############################################################

def move_common_to_parent(node):
    common = None
    for dtr in node['dtrs']:
        move_common_to_parent(dtr)
        if common is None:
            common = dtr['words']
        else:
            common = common & dtr['words']
    if len(common) != 0:
        for dtr in node['dtrs']:
            dtr['words'] = dtr['words'] - common

############################################################

def remove_specific_from_parent(node):
    for dtr in node['dtrs']:
        dtr['words'] = dtr['words'] - node['words']
        remove_specific_from_parent(dtr)

############################################################

def main():
    content = set()
    opath = sys.argv[1]
    for fname in sys.argv[2:]:
        read_content(content, fname)
    for word in sorted(content):
        print(word)

############################################################

if __name__ == '__main__':
    main()
