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
linkages = Sentence("This is a test.", Dictionary(), po).parse()
print "English: found ", len(linkages), "linkages"
for linkage in linkages:
    desc(linkage)

# Russian
linkages = Sentence("это большой тест.", Dictionary('ru'), po).parse()
print "Russian: found ", len(linkages), "linkages"
for linkage in linkages:
    desc(linkage)

# Turkish
linkages = Sentence("çok şişman adam geldi", Dictionary('tr'), po).parse()
print "Turkish: found ", len(linkages), "linkages"
for linkage in linkages:
    desc(linkage)

