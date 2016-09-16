#! /usr/bin/env python
# -*- coding: utf8 -*-
"""Link Grammar example usage"""

from __future__ import print_function, division  # We require Python 2.6 or later

from linkgrammar import Sentence, ParseOptions, Dictionary, Clinkgrammar as clg

print ("Version:", clg.linkgrammar_get_version())
po = ParseOptions(verbosity=1)

def desc(lkg):
    print (lkg.diagram())
    print ('Postscript:')
    print (lkg.postscript())
    print ('---')

def s(q):
    return '' if q == 1 else 's'

def linkage_stat(psent, lang, sent_po):
    """
    This function mimics the linkage status report style of link-parser
    """
    random = ' of {0} random linkages'. \
             format(psent.num_linkages_post_processed()) \
             if psent.num_linkages_found() > sent_po.linkage_limit else ''

    print ('{0}: Found {1} linkage{2} ({3}{4} had no P.P. violations)'. \
          format(lang, psent.num_linkages_found(),
                 s(psent.num_linkages_found()),
                 psent.num_valid_linkages(), random))


en_lines = [
    'This is a test.',
    'I feel is the exciter than other things', # from issue #303 (10 linkages)
]

po = ParseOptions(min_null_count=0, max_null_count=999)
#po.linkage_limit = 3

# English is the default language
en_dir = Dictionary() # open the dictionary only once
for text in en_lines:
    sent = Sentence(text, en_dir, po)
    linkages = sent.parse()
    linkage_stat(sent, 'English', po)
    for linkage in linkages:
        desc(linkage)

# Russian
sent = Sentence("Целью курса является обучение магистрантов основам построения и функционирования программного обеспечения сетей ЭВМ.", Dictionary('ru'), po)
linkages = sent.parse()
linkage_stat(sent, 'Russian', po)
for linkage in linkages:
    desc(linkage)

# Turkish
po = ParseOptions(islands_ok=True, max_null_count=1, display_morphology=True, verbosity=1)
sent = Sentence("Senin ne istediğini bilmiyorum", Dictionary('tr'), po)
linkages = sent.parse()
linkage_stat(sent, 'Turkish', po)
for linkage in linkages:
    desc(linkage)

# Prevent interleaving "Dictionary close" messages
po = ParseOptions(verbosity=0)
