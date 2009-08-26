#! /usr/bin/env perl
#
# wiktionary.pl
#
# Verify parts of speech on wiktionary.
#
# Linas vepstas August 2009
#

$|=1;

$urlbase = "http://en.wiktionary.org/wiki/";
# http://www.thefreedictionary.com

$urlbase = "http://en.wiktionary.org/w/index.php?title=";
$urltail = "&action=edit";

$word = "lycée";
$word = "school";

while (<>)
{
	chop;
	$word = $_;

	$url = $urlbase. $word . $urltail;

	$output = `w3mir -drr -s \"$url\"`;

	# $output =~ /\{\{ni\n";en-noun\|(\w+)\}\}/;
	if ($output =~ /\<TEXTAREA.*\{\{en-noun\}\}.*\<\/TEXTAREA\>/s)
	{
		print "NOUN- $word\n";
	}
	elsif ($output =~ /\<TEXTAREA.*\{\{en-noun\|(\w+)\}\}.*\<\/TEXTAREA\>/s)
	{
		$plu = $1;
		print "PLURAL- $plu\n";
	}
	elsif ($output =~ /\<TEXTAREA.*\{\{en-noun\|\-\}\}.*\<\/TEXTAREA\>/s)
	{
		print "MASS- $word\n";
	}
	elsif ($output =~ /\<TEXTAREA.*\{\{en-noun.*\<\/TEXTAREA\>/s)
	{
		print "NOUN-XXX- $word\n";
	}

	if ($output =~ /\<TEXTAREA.*\{\{en-verb.*\<\/TEXTAREA\>/s)
	{
		print "VERB- $word\n";
	}
	if ($output =~ /\<TEXTAREA.*\{\{en-adj.*\<\/TEXTAREA\>/s)
	{
		print "ADJ- $word\n";
	}
	if ($output =~ /\<TEXTAREA.*adjective\}\}.*\<\/TEXTAREA\>/s)
	{
		print "ADJ- $word\n";
	}
	if ($output =~ /\<TEXTAREA.*\{\{en-adv.*\<\/TEXTAREA\>/s)
	{
		print "ADV- $word\n";
	}
	else
	{
		print "XXX- $word\n";
	}
	`sleep 1`;
}


