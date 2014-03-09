#! /usr/bin/perl
# Created by Jon Dehdari 2002
# Perl 5.6
# Converts Farsi Isiri 3342 text to Romanized Farsi text (my customized Romanization scheme)
# To Do:	chars to address:  a1 3a (':' colon) 78 a2 e6 e7 e9 27 2e 5b ('[' opening bracket) 5d (']' closing bracket)

%isiri2roman = ( 	
	#	"\xb0" => '&#1776;', # EXTENDED ARABIC-INDIC DIGIT ZERO # Farsi specific
	#	"\xb1" => '&#1777;', # EXTENDED ARABIC-INDIC DIGIT ONE # Farsi specific
	#	"\xb2" => '&#1778;', # EXTENDED ARABIC-INDIC DIGIT TWO # Farsi specific
	#	"\xb3" => '&#1779;', # EXTENDED ARABIC-INDIC DIGIT THREE # Farsi specific
	#	"\xb4" => '&#1780;', # EXTENDED ARABIC-INDIC DIGIT FOUR # Farsi specific
	#	"\xb5" => '&#1781;', # EXTENDED ARABIC-INDIC DIGIT FIVE # Farsi specific
	#	"\xb6" => '&#1782;', # EXTENDED ARABIC-INDIC DIGIT SIX # Farsi specific
	#	"\xb7" => '&#1783;', # EXTENDED ARABIC-INDIC DIGIT SEVEN # Farsi specific
	#	"\xb8" => '&#1784;', # EXTENDED ARABIC-INDIC DIGIT EIGHT # Farsi specific
	#	"\xb9" => '&#1785;', # EXTENDED ARABIC-INDIC DIGIT NINE # Farsi specific
		
		"\xc1" => 'A', # ARABIC LETTER ALEF
		"\xc3" => 'b', # ARABIC LETTER BEH
		"\xc4" => 'p', # ARABIC LETTER PEH # Farsi specific
		"\xc5" => 't', # ARABIC LETTER TEH
		"\xc6" => 'V', # ARABIC LETTER THEH
		"\xc7" => 'j', # ARABIC LETTER JEEM
		"\xc8" => 'c', # ARABIC LETTER TCHEH # Farsi specific
		"\xc9" => 'H', # ARABIC LETTER HAH
		"\xca" => 'x', # ARABIC LETTER KHAH
		"\xcb" => 'd', # ARABIC LETTER DAL
		"\xcc" => 'L', # ARABIC LETTER THAL
		"\xcd" => 'r', # ARABIC LETTER REH
		"\xce" => 'z', # ARABIC LETTER ZAIN
		"\xcf" => 'J', # ARABIC LETTER JEH # Farsi specific
		"\xd0" => 's', # ARABIC LETTER SEEN
		"\xd1" => 'C', # ARABIC LETTER SHEEN
		"\xd2" => 'S', # ARABIC LETTER SAD
		"\xd3" => 'D', # ARABIC LETTER DAD
		"\xd4" => 'T', # ARABIC LETTER TAH
		"\xd5" => 'Z', # ARABIC LETTER ZAH
		"\xd6" => 'E', # ARABIC LETTER AIN
		"\xd7" => 'G', # ARABIC LETTER GHAIN
		"\xd8" => 'f', # ARABIC LETTER FEH
		"\xd9" => 'q', # ARABIC LETTER QAF
		"\xda" => 'k', # ARABIC LETTER KEHEH # Farsi specific
		"\xdb" => 'g', # ARABIC LETTER GAF # Farsi specific
		"\xdc" => 'l', # ARABIC LETTER LAM
		"\xdd" => 'm', # ARABIC LETTER MEEM
		"\xde" => 'n', # ARABIC LETTER NOON
		"\xdf" => 'u', # ARABIC LETTER WAW #note "u", "v", and "w" are all "vav"s in Farsi orthography.
		"\xe0" => 'h', # ARABIC LETTER HEH
		"\xe1" => 'i', # ARABIC LETTER YEH #note "i" and  "y" are both "ya"s in Farsi orthography.
		"\xf0" => 'a', # ARABIC FATHA #note the short vowels "a", "o" and "e" do not normally appear in Farsi orthography.
		"\xf2" => 'o', # ARABIC DAMMA
		"\xf1" => 'e', # ARABIC KASRA
		
		"\xc0" => '1', # ARABIC LETTER ALEF WITH MADDA ABOVE
		"\xc2" => 'M', # ARABIC LETTER HAMZA
		"\xf8" => '?', # ARABIC LETTER ALEF WITH HAMZA ABOVE
		"\xfa" => 'U', # ARABIC LETTER WAW WITH HAMZA ABOVE
		"\xf9" => '?', # ARABIC LETTER ALEF WITH HAMZA BELOW
		"\xfb" => 'I', # ARABIC LETTER YEH WITH HAMZA ABOVE
		#"\xd2" => 'X', # ARABIC LETTER HEH WITH YEH ABOVE     I don't know the isiri hex value for this!!
		
		"\xa5" => '%', # ARABIC PERCENT SIGN	
		"\xac" => ',', # ARABIC COMMA
		"\xbb" => ';', # ARABIC SEMICOLON
		"\xbf" => '?', # ARABIC QUESTION MARK
		
		"\xa0" => ' ', # SPACE (really &#0032;)
		"\xa8" => '(', # LEFT PARENTHESIS
		"\xa9" => ')', # RIGHT PARENTHESIS
		"\xad" => '-', # HYPHEN-MINUS
		"\xa6" => '.', # FULL STOP (really &#0046;)
		"\xba" => ':', # COLON
		"\x0a" => "\n", # CARRIAGE RETURN (really &#0010;)

	);

$in = shift || die "Provide a valid input file argument\n";
$out= shift || die "Provide an output file argument\n";
$in ne $out || die "Input and output files cannot be the same\n";
open(IN,$in);
open(OUT,">$out");

while ($line = <IN>) {
	@charx = split //, $line;
	foreach $charx (@charx)
	   {
		$newchar = $isiri2roman{$charx}; 
		
		$newchar =~ s/(\W)A/$1\|/g; # Converts a regular alef into a basic placeholder alef, to be used with the authors romanization scheme eg. Au -> |u
		print OUT ($newchar);

	   }
	  #prints a carrage return after every line
	  # print OUT ("<br>");
	}
	#prints the bottom part of the html page
	#print OUT ( "\n<\/body>\n<\/html>\n");



    #while ($line = <IN>) 
    #{
	#@readcontents = <IN>;
	#foreach $tempcontents (@readcontents) {
            #print $tempcontents, "\n";		

		## inserts initial alef before short vowels "a", "e", and "o".
		#$tempcontents =~ s/ &#1614;/ &#1571;&#1614;/g;	# "a"	
		#$tempcontents =~ s/ &#1615;/ &#1571;&#1615;/g;    # "o"
		#$tempcontents =~ s/ &#1616;/ &#1573;&#1616;/g;    # "e"
		#$tempcontents =~ s/ &#1573;&#1616; / &#1616; /g;    # "e" corrects for ezafe constructs
		
		## changes initial long "A" to alef with madda above.
		#$tempcontents =~ s/ &#1575;/ &#1570;/g; # ARABIC LETTER ALEF WITH MADDA ABOVE
		
		## changes final arabic "k" to final farsi "k".
		#$tempcontents =~ s/&#1603; /&#1705; /g;
		
		## changes final arabic "y" to final farsi "y".
		#$tempcontents =~ s/&#1610; /&#1740; /g;  # ararbic alef maksura is &#1609;
		
		
		## reverses punctuation marks.
		#$tempcontents =~ s/?/&#1567;/g;		
		#$tempcontents =~ s/,/&#1548;/g;
		#$tempcontents =~ s/;/&#1563;/g; #must be last!

		## maintains carriage returns for web pages
		#$tempcontents =~ s/\n/<br>/g;  # currently this function barfs at me.
	#}

#while (@readcontents)
#{
	#while (@readcontents) {
		#print OUT (@readcontents);
		#@readcontents = <IN>;
	#}
	
	### Writes to Output File
	#while (@readcontents) {		
		### Print data to file
		
		#print "@readcontents" || warn ("didn't print the new words in $newfile \n$!");
		#@readcontents = <OUT>;
	#}
#}

close (IN);
close (OUT);
