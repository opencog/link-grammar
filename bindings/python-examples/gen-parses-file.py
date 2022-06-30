#!/usr/bin/env python3
"""
Generate linkage diagrams in the order and format used in the
parses-xx.txt files of tests.py.
Note: For the correct order, linkage_limit here and in test.py
should be enough to produce all the sentence linkages.

TODO: Add constituents.
"""

import sys
import re
import argparse

from linkgrammar import (Sentence, ParseOptions, Dictionary,
                         LG_Error, LG_TimerExhausted, Clinkgrammar as clg)

import lg_testutils  # Found in the same directory of this test script

def nsuffix(q):
    return '' if q == 1 else 's'

class Formatter(argparse.HelpFormatter):
    """ Display the "lang" argument as a first one, as in link-parser. """
    def _format_usage(self, usage, actions, groups, prefix):
        usage_message = super(Formatter, self)._format_usage(usage, actions, groups, prefix)
        return re.sub(r'(usage: \S+) (.*) \[lang]', r'\1 [lang] \2', str(usage_message))

#-----------------------------------------------------------------------------#


print("Version:", clg.linkgrammar_get_version())

# Decorate Sentence.parse with eqcost_sorted_parse.
lg_testutils.add_eqcost_linkage_order(Sentence)

is_stdin_atty = sys.stdin.isatty()

PROMPT = "sentence: " if is_stdin_atty else ""

args = argparse.ArgumentParser(formatter_class=Formatter)
args.add_argument('lang', nargs='?', default='en',
                  help="language or dictionary location")
args.add_argument("-v", "--verbosity", type=int, default=0,
                  choices=range(0, 199), metavar='[0-199]',
                  help="1: Basic verbosity; 2-4: Trace; >5: Debug")
args.add_argument("-d", "--debug", type=ascii, default="",
                  help="comma-separated list of files and/or functions to debug")
args.add_argument("-p", "--position", action="store_true",
                  help="show word sentence position")
args.add_argument("-s", "--spell_guess", action="store_true",
                  help="set spell guessing")
args.add_argument("-nm", "--no-morphology", dest='morphology', action='store_false',
                  help="do not display morphology")
args.add_argument("-i", "--interactive", action="store_true",
                  help="interactive mode after each result")

arg = args.parse_args()

try:
    lgdict = Dictionary(arg.lang)
except LG_Error:
    # The default error handler will print the error message
    args.print_usage()
    sys.exit(2)

po = ParseOptions(verbosity=arg.verbosity)

MAX_DISPLAY_LINKAGES = 6
po.max_null_count = 999    # > allowed maximum number of words
po.linkage_limit = 100000  # maximum number of linkages to generate
                           # (should be >= num_linkages_found)
po.max_parse_time = 10     # actual parse timeout may be about twice bigger
po.spell_guess = arg.spell_guess
po.display_morphology = arg.morphology
po.debug = arg.debug.replace("'", "")

while True:
    try:
        sentence_text = input(PROMPT)
    except EOFError:
        print("EOF")
        exit(0)

    if not is_stdin_atty and sentence_text:
        if sentence_text[0] == '%':
            continue
        if sentence_text[0] == '!':  # ignore user-settings for now
            continue
    if sentence_text.strip() == '':
        continue
    if not is_stdin_atty:
        print("\n" + sentence_text)

    sent = Sentence(str(sentence_text), lgdict, po)
    try:
        linkages = sent.parse()
    except LG_TimerExhausted:
        print('Sentence too complex for parsing in ~{} second{}.'.format(
            po.max_parse_time, nsuffix(po.max_parse_time)))
        continue
    if not linkages:
        print('Error occurred - sentence ignored.')
        continue
    if len(linkages) <= 0:
        print('Cannot parse the input sentence')
        continue
    null_count = sent.null_count()

    """
    if arg.position:
        print(' ' * len(PROMPT), end='')
        for p in range(0, len(sentence_text)):
            print(p % 10, end="")
        print()
    """

    linkages = list(linkages)
    if clg.sentence_num_linkages_found(sent._obj) > po.linkage_limit:
        print('Warning: linkage_limit too low (should be at least',
              str(clg.sentence_num_linkages_found(sent._obj)) + ')')

    # Show results with unlinked words or guesses

    result_no = 0
    uniqe_parse = {}
    for linkage in linkages:
        words = list(linkage.words())
        result_no += 1

        if arg.position:
            words_char = []
            words_byte = []
            for wi, w in enumerate(words):
                words_char.append(w + str((linkage.word_char_start(wi), linkage.word_char_end(wi))))
                words_byte.append(w + str((linkage.word_byte_start(wi), linkage.word_byte_end(wi))))

                print(u"{}{}".format("P", ' '.join(words_char)))
                print(u"{}{}".format("P", ' '.join(words_byte)))
        else:
            if result_no == 1:
                print(u"{}{}".format("I", sentence_text))
            else:
                print("N")
            diagram = linkage.diagram()
            for line in iter(diagram.splitlines()):
                print(u"{}{}".format("O", line))

        if result_no == MAX_DISPLAY_LINKAGES:
            break

    if arg.interactive:
        print("Interactive session (^D to end):")
        import code
        code.interact(local=locals())
