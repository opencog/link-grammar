#! /usr/bin/env perl
#
# insert.pl
#
# Dictionary maintenance tool - insert words into dictionary.
# Given a list of words and a dictionary file, this script
# will insert the words into the file, in alphabetical order,
# with a minimum of disruption of the file itself.
#
# It is assumed that the word-list holds one word per line,
# and that the word-list is already in alphabetical order.
#
# Example usage:
# ./insert.pl wordlist_file en/words/words.adj.1
#
# Copyright (C) 2009 Linas Vepstas <linasvepstas@gmail.com>
#

if ($#ARGV < 1)
{
	die "Usage: insert.pl wordlist-file dict-file\n";
}

# Get the word-list, and the dictionary, from the command line
my $wordlist_file = @ARGV[0];
my $dict_file = @ARGV[1];

open(WORDS, $wordlist_file);
my @words = ();
while (<WORDS>)
{
	# Get rid of newline at end of word.
	chop;
	push @words, $_;
}
close(WORDS);

my $word = shift @words;

open (DICT, $dict_file);
while (<DICT>)
{
	chop;
	my @entries = split;

	# Loop over the entries
	my $linelen = 0;
	foreach (@entries)
	{
		my $wd = $_;
		$linelen += length $_;
		while (($_ gt $word) && ($word ne ""))
		{
			print "$word ";
			$linelen += length $word;
			$word = shift @words;

			# Insert a newline if the resulting line is too long.
			if ($linelen > 62)
			{
				print "\n";
				$linelen = 0;
			}
		}
		print "$_ ";
	}
	print "\n";
}
close (DICT);


