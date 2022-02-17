#!/usr/bin/env python
#-*- coding: utf-8 -*-

import os, sys

############################################################

def read_content(content, fname):
    fhdl = open(fname)
    for line in fhdl:
        line = line.strip()
        content.add(line)
    fhdl.close()

############################################################

def main():
    content = set()
    for fname in sys.argv[1:]:
        read_content(content, fname)
    for word in sorted(content):
        print(word)

############################################################

if __name__ == '__main__':
    main()
