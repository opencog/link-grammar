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

def desc(linkage):
    print linkage.diagram()
    print 'Postscript:'
    print linkage.postscript()
    print '---'


# English is the default language
sent = Sentence("This is a test.", Dictionary(), po)
linkages = sent.parse()
print "English: found ", sent.num_valid_linkages(), "linkages"
for linkage in linkages:
    desc(linkage)

# Russian
sent = Sentence("это большой тест.", Dictionary('ru'), po)
linkages = sent.parse()
print "Russian: found ", sent.num_valid_linkages(), "linkages"
for linkage in linkages:
    desc(linkage)

# Turkish
sent = Sentence("çok şişman adam geldi", Dictionary('tr'), po)
linkages = sent.parse()
print "Turkish: found ", sent.num_valid_linkages(), "linkages"
for linkage in linkages:
    desc(linkage)
