#! /usr/bin/env perl
#
# delete.pl
#
# Dictionary maintenance tool - delete words from dictionary.
# Given a list of words and a dictionary file, this script
# will delete the words from the file.
#
# It is assumed that the word-list holds one word per line,
# and that the word-list is already in alphabetical order.
#
# Example usage:
# ./delete.pl wordlist_file en/words/words.adj.1
#
# Copyright (C) 2009 Linas Vepstas <linasvepstas@gmail.com>
#

if ($#ARGV < 1)
{
	die "Usage: delete.pl wordlist-file dict-file\n";
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
	$gotone = 0;
	foreach (@entries)
	{
		my $entry = $_;
		while (($entry gt $word) && ($word ne ""))
		{
			$word = shift @words;
		}
		if ($entry ne $word)
		{
			# print "$entry ";
			print "$entry";
			$gotone = 1;
		}
		else
		{
			$word = shift @words;
		}
	}
	if ($gotone)
	{
		print "\n";
	}
}
close (DICT);


