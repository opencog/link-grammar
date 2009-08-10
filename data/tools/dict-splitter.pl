#! /usr/bin/env perl
#
# dict-splitter.pl
#
# Split dictionary into distinct files.
#
# Usage: cat 4.0.dict | ./dict-splitter.pl
#
# Each word cluster will be written to a distinct file.
#

$fileno = 0;
$doprt = 1;
$didprt = 0;

$file_prefix = "../blah/blah-";

$filename = "> " . $file_prefix . $fileno;
open FOUT, $filename;

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
			print FOUT "$wds\n";
			$didprt = 1;
		}

		$doprt = 0;
		if ($rules =~ /;/)
		{
			$doprt = 1;
			if ($didprt)
			{
				$fileno ++;
				
				# print "--------------------- $fileno\n";
				$filename = "> " . $file_prefix . $fileno;
				open FOUT, $filename;
			}
			$didprt = 0;
		}
		next;
	}

	if ($doprt)
	{
		if (/^\s*$/) { next; }   # if all whitespace, skip
		if (/^\//) { next; }
		print FOUT "$_\n";
		$didprt = 1;
		next;
	}
	
	if (/;/)
	{
		$doprt = 1;

		if ($didprt)
		{
			$fileno ++;
			# print "--------------------- $fileno\n";
			$filename = "> " . $file_prefix . $fileno;
			open FOUT, $filename;
		}
		$didprt = 0;
	}

}
