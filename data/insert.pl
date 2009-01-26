#! /usr/bin/env perl
#
# insert.pl
# Dictionary maintenance tool - insert words into dictionary.
# Given a list of words and a dictionary file, this script
# will insert the words into the file, in alphabetical order,
# with a minimum of disruption of the file itself.
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
@words = <WORDS>;
close(WORDS);

open (DICT);
while (<DICT>)
{

}
close (DICT);


