#! /usr/bin/env perl
#
# dict-splitter.pl
#
# Split dictionary into distinct files.
#

$fileno = 0;
$doprt = 1;
$didprt = 0;
while(<>)
{
	chop;
	if (/^%/) { next; }
	if (/^ %/) { next; }

	if (/:/)
	{
		
		($wds, $rules) = split /:/;

		if (($wds =~ /^<.*>$/)  ||
		    ($wds =~ /^\//))
		{} else
		{
			print "$wds\n";
			$didprt = 1;
		}

		$doprt = 0;
		if ($rules =~ /;/)
		{
			$doprt = 1;
			if ($didprt)
			{
				$fileno ++;
				print "--------------------- $fileno\n";
			}
			$didprt = 0;
		}
		next;
	}

	if ($doprt)
	{
		if (/^\s*$/) { next; }   # if all whitespace, skip
		if (/^\//) { next; }
		print "$_\n";
		$didprt = 1;
		next;
	}
	
	if (/;/)
	{
		$doprt = 1;

		if ($didprt)
		{
			$fileno ++;
			print "--------------------- $fileno\n";
		}
		$didprt = 0;
	}

}
