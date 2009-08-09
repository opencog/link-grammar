#! /usr/bin/env perl
#
# dict-splitter.pl
#
# Split dictionary into distinct files.
#

$fileno = 0;
$doprt = 1;
while(<>)
{
	chop;
	if (/^%/) { next; }
	if (/^ %/) { next; }

	if (/:/)
	{
		
		($wds, $rules) = split /:/;
		print "duude $wds\n";

		$doprt = 0;
		if ($rules =~ /;/)
		{
			$doprt = 1;
			$fileno ++;

			print "--------------------- $fileno\n";
		}
		next;
	}

	if ($doprt)
	{
		if (/^\s*$/) { next; }
		print "whole line $_\n";
		next;
	}
	
	if (/;/)
	{
		$doprt = 1;
		$fileno ++;

		print "--------------------- $fileno\n";
	}

}
