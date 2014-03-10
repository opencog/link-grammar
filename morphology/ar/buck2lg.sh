# converts aramorph.pl output to line of morphemes separated by spaces
# usage: cat file | ./buck2lg.sh | more
 perl -p -e '
    if    (m/SOLUTION 1:/) { s/.*?\] (.*)/$1/g; s/\//./g; s/\+/ /g; }
    elsif (m/NEWLINE/)     {} 
    elsif (m/NOT FOUND$/)  {s/Comment:\s+(\S+?)\s+NOT FOUND/a${1}/g} # There might be a bug in Perl 5.8: the "a" must be present in second term of s///
    else                   { s/.*//g }
    s/\(null\)//g; 
    tr/{/</; '					| # enigmatic junk
 egrep '[a-zA-Z]'				| # eliminates empty lines
 perl -p -e 's/\n/ /g; s/NEWLINE/\n/g'		| # converts 'NEWLINE' to \n
 egrep '[a-zA-Z]'				| # eliminates empty lines
 perl -p -e 's/\..*?( |\n)/$1/g; s/[aiuo~]//g;'	  # removes .POS and devocalizes
