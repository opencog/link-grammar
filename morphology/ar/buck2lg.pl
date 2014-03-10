#!/usr/bin/perl 
# converts aramorph.pl output to line of morphemes separated by spaces
# replaces non-portable buck2lg.sh
# usage: cat file | ./buck2lg.pl | more

use strict;

while (<>) {
  next unless m/[a-zA-Z]/;
  if    (m/SOLUTION 1:/) { s/.*?\] (.*)/$1/g; s/\//./g; s/\+/ /g; }
  elsif (m/NEWLINE/)     {} 
  elsif (m/NOT FOUND$/)  {s/Comment:\s+(\S+?)\s+NOT FOUND/a${1}/g} # There might be a bug in Perl 5.8: the "a" must be present in second term of s///
  else                   { s/.*//g }


  s/\(null\)//g; 
  tr/{/</; 
  tr/\n/ /;
  s/\..*?( |\n)/$1/g;		# removes .POS
  s/\s+/ /g;			# removes excessive spaces
  s/[aiuo~]//g;			# devocalizes
  if (m/[a-zA-Z]/) {		# some newline trickery
      if (m/NEWLINE/) {
	  print "\n";
      }
      else {print};
  }
}
