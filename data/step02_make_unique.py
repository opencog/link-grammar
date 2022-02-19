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
        content.add(line)
    fhdl.close()

############################################################

def create_node(filetree):
    fname = filetree.fname
    result = {'fname': fname, 'words': set(), 'dtrs': []}
    if fname is not None:
        if os.path.exists(fname):
            read_content(result['words'], fname)
    for dtr in filetree.dtrs:
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
    if common is None:
        common = set()
    if len(common) != 0:
        for dtr in node['dtrs']:
            dtr['words'] = dtr['words'] - common

############################################################

def remove_specific_from_parent(node):
    for dtr in node['dtrs']:
        dtr['words'] = dtr['words'] - node['words']
        remove_specific_from_parent(dtr)

############################################################

def write_hier(opath, node):
    if node['fname'] is not None:
        head, tail = os.path.split(node['fname'])
        ofname = os.path.join(opath, tail)
    else:
        ofname = os.path.join(opath, 'words.any')
    ofhdl = open(ofname, 'w')
    for word in sorted(node['words']):
        ofhdl.write(word + '\n')
    ofhdl.close()

    for dtr in node['dtrs']:
        write_hier(opath, dtr)

############################################################

def main():
    # content = set()
    ifname = sys.argv[1]
    opath = sys.argv[2]

    hier = load_filetree(os.path.join(ifname))
    hier.print()

    pos_hier = create_node(hier)
    move_common_to_parent(pos_hier)
    remove_specific_from_parent(pos_hier)
    print(pos_hier)
    write_hier(opath, pos_hier)

############################################################

if __name__ == '__main__':
    main()
