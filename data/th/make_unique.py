#!/usr/bin/env python
#-*- coding: utf-8 -*-

import os, sys

############################################################

class Node:

    def __init__(self, fname, spc, dtrs=None, parent=None):
        if dtrs is None: dtrs = []
        self.fname = fname
        self.spc = spc
        self.dtrs = dtrs
        self.parent = parent

    def add_daughter(self, node):
        self.dtrs.append(node)
        node.parent = self

    def repr(self):
        result = '('
        if self.fname is not None:
            result += self.fname + f'/{self.spc}'
        else:
            result += 'TOP'
        for dtr in self.dtrs:
            result += ' ' + dtr.repr()
        result += ')'
        return result

    def __repr__(self):
        return self.repr()

    def __str__(self):
        return self.repr()

    def print(self, spc=0):
        print(spc * ' ', end='')
        if self.fname is not None:
            print(self.fname)
        else:
            print('TOP')
        for dtr in self.dtrs:
            dtr.print(spc + 4)

############################################################

def load_filetree(fname):

    def count_leading_space(line):
        i = 0
        for i in range(len(line)):
            if line[i].isspace():
                i += 1
            else:
                break
        return i

    def make_hier_node(fname, spc):
        return Node(fname, spc)

    def read_hier(node, lines, idx):
        # print(f'>>> idx = {idx} : {lines[idx]}')
        # print(f'... filetree = {filetree}')
        # print(f'... node = {node}')
        if idx >= len(lines) - 1:
            return node
        
        if len(lines[idx].strip()) == 0:
            # print(f'....... skip\n')
            return read_hier(node, lines, idx + 1)

        cur_spc = count_leading_space(lines[idx])
        # print(f'... cur_spc = {cur_spc}')
        if cur_spc == node.spc:
            # print(f'....... case 1 (sibling): cur_spc == prev_spc\n')
            sibling = make_hier_node(lines[idx].strip(), cur_spc)
            node.parent.add_daughter(sibling)
            return read_hier(sibling, lines, idx + 1)
        elif cur_spc > node.spc:
            # print(f'....... case 2 (daughter): cur_spc > prev_spc\n')
            dtr = make_hier_node(lines[idx].strip(), cur_spc)
            node.add_daughter(dtr)
            return read_hier(dtr, lines, idx + 1)
        else:
            # print(f'....... case 3 (end): cur_spc < prev_spc\n')
            return read_hier(node.parent, lines, idx)

    fhdl = open(fname)
    lines = [line.rstrip() for line in fhdl]
    fhdl.close()
    
    filetree = make_hier_node(None, -1)
    read_hier(filetree, lines, 0)
    return filetree

############################################################

def read_content(content, fname):
    fhdl = open(fname)
    for line in fhdl:
        line = line.strip()
        if len(line) == 0: continue
        if line.startswith('#'): continue
        content.add(line)
    fhdl.close()

############################################################

def create_node(filetree, wordsettbl, excls=None):
    if excls is None: excls = set()

    fname = filetree.fname
    if fname not in wordsettbl:
        wordsettbl[fname] = set()
    result = {'fname': fname, 'dtrs': [], 'orgsize': 0}
    print(f'>>> Loading {fname} ...', end='')
    if fname is not None:
        if os.path.exists(fname):
            read_content(wordsettbl[fname], fname)
            # print(f'wordset = {wordsettbl[fname]}')
            # input(f'excls = {excls}')
            wordsettbl[fname] = wordsettbl[fname] - excls
        if len(wordsettbl[fname]) == 0:
            print('***ZERO members***')
        else:
            result['orgsize'] = len(wordsettbl[fname])
            print(f'{len(wordsettbl[fname])} members')
    for dtr in filetree.dtrs:
        tree = create_node(dtr, wordsettbl, excls)
        result['dtrs'].append(tree)
    return result

############################################################

def move_common_to_parent(node, wordsettbl):
    for dtr in node['dtrs']:
        move_common_to_parent(dtr, wordsettbl)

    if len(node['dtrs']) < 2:
        return

    common = None
    for dtr in node['dtrs']:
        if common is None:
            common = wordsettbl[dtr['fname']]
        else:
            common = common & wordsettbl[dtr['fname']]

    if common is not None and len(common) != 0:
        for dtr in node['dtrs']:
            diff = wordsettbl[dtr['fname']] - common
            if len(diff) > 0:
                wordsettbl[dtr['fname']] = diff

############################################################

def remove_specific_from_parent(node, wordsettbl):
    for dtr in node['dtrs']:
        wordsettbl[dtr['fname']] = wordsettbl[dtr['fname']] - wordsettbl[node['fname']]

    for dtr in node['dtrs']:
        remove_specific_from_parent(dtr, wordsettbl)

############################################################

def write_hier(opath, node, wordsettbl, spc=0):
    if node['fname'] is not None:
        head, tail = os.path.split(node['fname'])
        ofname = os.path.join(opath, tail)
    else:
        ofname = os.path.join(opath, 'words.any')
    print(spc * ' ' + f'>>> Writing out: {ofname} ... {node["orgsize"]} --> {len(wordsettbl[node["fname"]])}')
    ofhdl = open(ofname, 'w')
    for word in sorted(wordsettbl[node['fname']]):
        ofhdl.write(word + '\n')
    ofhdl.close()

    for dtr in node['dtrs']:
        write_hier(opath, dtr, wordsettbl, spc+4)

def write_stats(ofhdl, opath, node, wordsettbl, spc=0):
    if node['fname'] is not None:
        head, tail = os.path.split(node['fname'])
        ofname = os.path.join(opath, tail)
    else:
        ofname = os.path.join(opath, 'words.any')
    ofhdl.write(spc * ' ' + f'{ofname}\t{node["orgsize"]}\t{len(wordsettbl[node["fname"]])}\n')

    for dtr in node['dtrs']:
        write_stats(ofhdl, opath, dtr, wordsettbl, spc+4)

############################################################

def main():
    # content = set()
    ifname = sys.argv[1]
    opath = sys.argv[2]
    exclfname = sys.argv[3]

    hier = load_filetree(ifname)
    hier.print()

    excls = set()
    read_content(excls, exclfname)

    wordsettbl = {}
    pos_hier = create_node(hier, wordsettbl, excls)
    move_common_to_parent(pos_hier, wordsettbl)
    # remove_specific_from_parent(pos_hier, wordsettbl)
    # print(pos_hier)
    write_hier(opath, pos_hier, wordsettbl)

    ofstats = os.path.join(opath, 'stats.tsv')
    ofhdl = open(ofstats, 'w')
    ofhdl.write('File\tOriginal size\tPost-processed size\n')
    write_stats(ofhdl, opath, pos_hier, wordsettbl)
    ofhdl.close()

############################################################

if __name__ == '__main__':
    main()
