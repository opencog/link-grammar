#!/usr/bin/env python
"""
Demo: Find unlinked or unknown words.
These demo is extremely simplified.
It can only work with link-grammar library version >= 5.3.10.
Input: English sentences, one per line.
Output: If there are any []-marked words in the linkage results,
the output contains unique combinations of the input sentence with
these works marked.  No attempt is done to handle the walls.
Spell guesses are not handled in this demo.

Example:
This is a the test of bfgiuing and xxxvfrg
Output:
Sentence has 1 unlinked word:
1: LEFT-WALL this.p is.v [a] the test.n of bfgiuing[!].g and.j-n xxxvfrg[?].n RIGHT-WALL
2: LEFT-WALL this.p is.v a [the] test.n of bfgiuing[!].g and.j-n xxxvfrg[?].n RIGHT-WALL
3: LEFT-WALL this.p is.v [a] the test.n of bfgiuing[!].g and.j-n xxxvfrg[?].a RIGHT-WALL
4: LEFT-WALL this.p is.v a [the] test.n of bfgiuing[!].g and.j-n xxxvfrg[?].a RIGHT-WALL
"""
from __future__ import print_function
import sys
import re
import itertools

from linkgrammar import (Sentence, ParseOptions, Dictionary,
                         LG_TimerExhausted, Clinkgrammar as clg)
print("Version:", clg.linkgrammar_get_version())

def nsuffix(q):
    return '' if q == 1 else 's'

#-----------------------------------------------------------------------------#

DISPLAY_GUESSES = True   # Display regex and POS guesses

po = ParseOptions(verbosity=1)
lgdict = Dictionary('en')

po.verbosity = 0         # 1=more verbose; 2=trace; >5=debug
po.max_null_count = 999  # > allowed maximum number of words
po.max_parse_time = 10   # actual parse timeout may be about twice bigger
po.spell = 0             # spell guesses are not handled in this demo

# iter(): avoid python2 input buffering
for sentence_text in iter(sys.stdin.readline, ''):
    if sentence_text.strip() == '':
        continue
    sent = Sentence(sentence_text, lgdict, po)
    try:
        linkages = sent.parse()
    except LG_TimerExhausted:
        print('Sentence too complex for parsing in ~{} second{}.'.format(
            po.max_parse_time,nsuffix(po.max_parse_time)))
        continue
    if not linkages:
        print('Error occurred - sentence ignored.')
        continue
    if len(linkages) <= 0:
        print('Cannot parse the input sentence')
        continue
    null_count = sent.null_count()
    if null_count == 0:
        print("Sentence parsed OK")

    guess_found = False
    if DISPLAY_GUESSES:
        linkages, check_first = itertools.tee(linkages)
        # Check the first linkage for regexed/unknown words
        linkage = next(check_first)
        for word in list(linkage.words()):
            # search for something[x]
            if re.search(r'\S+\[[^]]+]', word):
                guess_found = True
                break

    # Show results with unlinked words or guesses
    if not guess_found and null_count == 0:
        continue

    print('Sentence has {} unlinked word{}:'.format(
        null_count, nsuffix(null_count)))
    result_no = 0
    uniqe_parse = {}
    for linkage in linkages:
        words = list(linkage.words())
        if str(words) in uniqe_parse:
            continue
        result_no += 1
        uniqe_parse[str(words)] = True
        print("{}: {}".format(result_no, ' '.join(words)))
