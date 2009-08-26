#! /usr/bin/env perl
#
# wiktionary.pl
#
# Verify parts of speach on wiktionary.
#
# Linas vepstas August 2009
#

$urlbase = "http://en.wiktionary.org/wiki/";
# http://www.thefreedictionary.com

$urlbase = "http://en.wiktionary.org/w/index.php?title=";
$urltail = "&action=edit";

$word = "lycée";
$url = $urlbase. $word . $urltail;

print "its $url\n";

$output = `w3mir -drr -s \"$url\"`;
print "its $output\n";


