#! /usr/bin/perl
# Written by Jon Dehdari 2004
# Perl 5.6
# Converts Persian and Arabic HTML Unicode text to Romanized Persian/Arabic text
# Syntax: ./htmlunicode2roman_1-4.pl < input.html > output.txt

%unicode2roman = ( 	
		 '&#1776;' => '0', # EXTENDED ARABIC-INDIC DIGIT ZERO # Farsi specific
		 '&#1777;' => '1', # EXTENDED ARABIC-INDIC DIGIT ONE # Farsi specific
		 '&#1778;' => '2', # EXTENDED ARABIC-INDIC DIGIT TWO # Farsi specific
		 '&#1779;' => '3', # EXTENDED ARABIC-INDIC DIGIT THREE # Farsi specific
		 '&#1780;' => '4', # EXTENDED ARABIC-INDIC DIGIT FOUR # Farsi specific
		 '&#1781;' => '5', # EXTENDED ARABIC-INDIC DIGIT FIVE # Farsi specific
		 '&#1782;' => '6', # EXTENDED ARABIC-INDIC DIGIT SIX # Farsi specific
		 '&#1783;' => '7', # EXTENDED ARABIC-INDIC DIGIT SEVEN # Farsi specific
		 '&#1784;' => '8', # EXTENDED ARABIC-INDIC DIGIT EIGHT # Farsi specific
		 '&#1785;' => '9', # EXTENDED ARABIC-INDIC DIGIT NINE # Farsi specific

		 '&#1575;' => 'A', # ARABIC LETTER ALEF
		 '&#9791;' => '|', # ARABIC LETTER ALEF #initial position; fake hex value for now
		 "&#1576;" => 'b', # ARABIC LETTER BEH
		 '&#1577;' => 'a', # ARABIC LETTER TEH MARBUTA
		 '&#1662;' => 'p', # ARABIC LETTER PEH
		 '&#1578;' => 't', # ARABIC LETTER TEH
		 '&#1579;' => 'V', # ARABIC LETTER THEH
		 '&#1580;' => 'j', # ARABIC LETTER JEEM
		 '&#1670;' => 'c', # ARABIC LETTER TCHEH
		 '&#1581;' => 'H', # ARABIC LETTER HAH
		 '&#1582;' => 'x', # ARABIC LETTER KHAH
		 '&#1583;' => 'd', # ARABIC LETTER DAL
		 '&#1584;' => 'L', # ARABIC LETTER THAL
		 '&#1585;' => 'r', # ARABIC LETTER REH
		 '&#1586;' => 'z', # ARABIC LETTER ZAIN
		 '&#1688;' => 'J', # ARABIC LETTER JEH
		 '&#1587;' => 's', # ARABIC LETTER SEEN
		 '&#1588;' => 'C', # ARABIC LETTER SHEEN
		 '&#1589;' => 'S', # ARABIC LETTER SAD
		 '&#1590;' => 'D', # ARABIC LETTER DAD
		 '&#1591;' => 'T', # ARABIC LETTER TAH
		 '&#1592;' => 'Z', # ARABIC LETTER ZAH
		 '&#1593;' => 'E', # ARABIC LETTER AIN
		 '&#1594;' => 'G', # ARABIC LETTER GHAIN
		 '&#1601;' => 'f', # ARABIC LETTER FEH
		 '&#1602;' => 'q', # ARABIC LETTER QAF
		 '&#1603;' => 'k', # ARABIC LETTER KAF
		 '&#1705;' => 'k', # ARABIC LETTER KEHEH
		 '&#1711;' => 'g', # ARABIC LETTER GAF
		 '&#1604;' => 'l', # ARABIC LETTER LAM
		 '&#1605;' => 'm', # ARABIC LETTER MEEM
		 '&#1606;' => 'n', # ARABIC LETTER NOON
		 '&#1608;' => 'u', # ARABIC LETTER WAW
		 '&#1607;' => 'h', # ARABIC LETTER HEH
		 '&#1610;' => 'i', # ARABIC LETTER YEH
		 '&#1740;' => 'i', # ARABIC LETTER FARSI YEH
		 '&#1609;' => 'A', # ARABIC LETTER ALEF MAKSURA
		 '&#1614;' => 'a', # ARABIC FATHA
		 '&#1615;' => 'o', # ARABIC DAMMA
		 '&#1616;' => 'e', # ARABIC KASRA
		 '&#1617;' => '~', # ARABIC SHADDA
		
		 '&#1570;' => ']', # ARABIC LETTER ALEF WITH MADDA ABOVE
		 '&#1569;' => 'M', # ARABIC LETTER HAMZA
		 '&#1611;' => 'N', # ARABIC FATHATAN
		 '&#1571;' => '|', # ARABIC LETTER ALEF WITH HAMZA ABOVE # temp
		 '&#1572;' => 'U', # ARABIC LETTER WAW WITH HAMZA ABOVE
		 '&#1573;' => '|', # ARABIC LETTER ALEF WITH HAMZA BELOW # temp
		 '&#1574;' => 'I', # ARABIC LETTER YEH WITH HAMZA ABOVE
		 '&#1728;' => 'X', # ARABIC LETTER HEH WITH YEH ABOVE
		
		 '&#1642;' => '%', # ARABIC PERCENT SIGN	
		 '&#1548;' => ',', # ARABIC COMMA
		 '&#1563;' => ';', # ARABIC SEMICOLON
		 '&#1567;' => '?', # ARABIC QUESTION MARK
		 '&#8204;' => "-", # ZERO WIDTH NON-JOINER
		
		 ' ' => ' ', # SPACE
		 '(' => '(', # LEFT PARENTHESIS
		 ')' => ')', # RIGHT PARENTHESIS
		 '.' => '.', # FULL STOP
		 ':' => ':', # COLON
		 "\n" => "\n", # LINE FEED

	);

#$in = shift || die "Provide a valid input file argument\n";
#$out= shift || die "Provide an output file argument\n";
#$in ne $out || die "Input and output files cannot be the same\n";
#open(IN,$in);
#open(OUT,">$out");

while ($line = <STDIN>) {
	$line =~ s/ &#1575;/ &#9791;/g; # Converts a regular alef into a basic placeholder alef, to be used with the authors romanization scheme eg. Au -> |u .
	$line =~ s/^&#1575;/&#9791;/g; # Same, at beginning of a line
	$line =~ s/ &#1570;/ &#1575;/g; # Same, at beginning of a line
	$line =~ s/^&#1570;/&#1575;/g; # Same, at beginning of a line
	#$line =~ s/<br>/\n/g;
	$line =~ s/<p>/\n/g;
	$line =~ s/<\/td>/\n/g;
	$line =~ s/<\/div>/\n/g;
	$line =~ s/<.*?>//g; # Deletes all HTML tags on 1 line
	$line =~ s/<.*?//g;  # Deleses 1st part of line-spanning HTML tags
	$line =~ s/.*?>//g;  # Deletes 2nd part of line-spanning HTML tags
	$line =~ s/body { text-align:right }//g; # Removes stylesheet stuff

	@charx = split(/(?=\&\#)|(?=\s)|(?=\n)/, $line);
	foreach $charx (@charx)
	   {
	   	#print "$charx ";
		$newchar = $unicode2roman{$charx}; 
		print STDOUT ($newchar);
		#print "\n";

	   }
	  #prints a newline after every line
	  # print STDOUT ("<br>\n");
	}
	#prints the bottom part of the html page
	#print STDOUT ( "\n<\/body>\n<\/html>\n");


#close (IN);
#close (OUT);
