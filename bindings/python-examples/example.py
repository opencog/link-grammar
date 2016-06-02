#! /usr/bin/env python
# -*- coding: utf8 -*-
#
# Link Grammar example usage
#
import locale

from linkgrammar import Sentence, ParseOptions, Dictionary
try:
    import linkgrammar._clinkgrammar as clg
except ImportError:
    import _clinkgrammar as clg

locale.setlocale(locale.LC_ALL, "en_US.UTF-8")

print "Version:", clg.linkgrammar_get_version()
po = ParseOptions()
#po = ParseOptions(short_length=16)

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
po = ParseOptions(islands_ok=True, max_null_count=1, display_morphology=True)
sent = Sentence("Senin ne istediğini bilmiyorum", Dictionary('tr'), po)
linkages = sent.parse()
linkage_stat(sent, 'Turkish')
for linkage in linkages:
    desc(linkage)
