#! /usr/bin/env python
#
# Link Grammar example usage 
#
# May need to set the PYTHONPATH to get this to work:
# PYTHONPATH=$PYTHONPATH:/usr/local/lib/python2.7/dist-packages/link-grammar
# or something similar ...
#
from linkgrammar import Parser, Linkage, ParseOptions, Link

po = ParseOptions()

p = Parser()
linkages = p.parse_sent("This is a test.")
print "Found ", len(linkages), "linkages"
for linkage in linkages :
    print linkage.diagram
