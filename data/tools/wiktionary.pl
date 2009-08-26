#! /usr/bin/env perl
#
# wiktionary.pl
#
# Verify parts of speech on wiktionary.
#
# Linas vepstas August 2009
#

$urlbase = "http://en.wiktionary.org/wiki/";
# http://www.thefreedictionary.com

$urlbase = "http://en.wiktionary.org/w/index.php?title=";
$urltail = "&action=edit";

$word = "lycée";
$word = "school";
$url = $urlbase. $word . $urltail;

$output = `w3mir -drr -s \"$url\"`;

# $output =~ /\{\{ni\n";en-noun\|(\w+)\}\}/;
if ($output =~ /\{\{en-noun\}\}/)
{
	print "Yes its a noun\n";
}
if ($output =~ /\{\{en-noun\|(\w+)\}\}/)
{
	$plu = $1;
	print "its  plural >>$plu<<\n";
}
print "finit\n";


