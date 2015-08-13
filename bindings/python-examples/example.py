#! /usr/bin/env python
# -*- coding: utf8 -*-
#
# Link Grammar example usage
#
import locale

from linkgrammar import Sentence, ParseOptions, Dictionary
# from linkgrammar import _clinkgrammar as clg

locale.setlocale(locale.LC_ALL, "en_US.UTF-8")

po = ParseOptions()
#po = ParseOptions(linkage_limit=3)

def desc(lkg):
    print lkg.diagram()
    print 'Postscript:'
    print lkg.postscript()
    print '---'

def s(q):
    return '' if q == 1 else 's'

def linkage_stat(psent, lang):
    """
    This function mimics the linkage status report style of link-parser
    """
    random = ' of {0} random linkages'. \
             format(psent.num_linkages_post_processed()) \
             if psent.num_valid_linkages() < psent.num_linkages_found() else ''

    print '{0}: Found {1} linkage{2} ({3}{4} had no P.P. violations)'. \
          format(lang, psent.num_linkages_found(),
                 s(psent.num_linkages_found()),
                 psent.num_valid_linkages(), random)


# English is the default language
sent = Sentence("This is a test.", Dictionary(), po)
linkages = sent.parse()
linkage_stat(sent, 'English')
for linkage in linkages:
    desc(linkage)

# Russian
sent = Sentence("это большой тест.", Dictionary('ru'), po)
linkages = sent.parse()
linkage_stat(sent, 'Russian')
for linkage in linkages:
    desc(linkage)

# Turkish
sent = Sentence("çok şişman adam geldi", Dictionary('tr'), po)
linkages = sent.parse()
linkage_stat(sent, 'Turkish')
for linkage in linkages:
    desc(linkage)
