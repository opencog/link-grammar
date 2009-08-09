#! /usr/bin/env perl
#
# fix-len.pl
# Dictionary maintenance tool - make dictionary line lengths uniform.
# Reads dictionary on stdin, prints it, with a uniform number
# of words per line, to stdout.
#
# Example usage:
# cat en/words/words.adj.2 | ./fix-len.pl
#
# Copyright (C) 2009 Linas Vepstas <linasvepstas@gmail.com>
#

my $linelen = 0;
while (<>)
{
	chop;
	my @entries = split;

	# Loop over the entries
	foreach (@entries)
	{
		my $wd = $_;
		$linelen += 1 + length $_;
		print "$_ ";

		# Insert a newline if the resulting line is too long.
		if ($linelen > 60)
		{
			print "\n";
			$linelen = 0;
		}
	}
}

print "\n";

