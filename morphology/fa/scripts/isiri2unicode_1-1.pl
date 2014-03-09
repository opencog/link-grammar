# Created by Jon Dehdari 2002
# Perl 5.6
# Converts Isiri 3342  Farsi text to Unicode HTML Page
# To Do: auto-insert alef before short vowels, change final yeh to farsi style yeh

%isiri2unicode = ( 	
		"\xb0" => '&#1776;', # EXTENDED ARABIC-INDIC DIGIT ZERO # Farsi specific
		"\xb1" => '&#1777;', # EXTENDED ARABIC-INDIC DIGIT ONE # Farsi specific
		"\xb2" => '&#1778;', # EXTENDED ARABIC-INDIC DIGIT TWO # Farsi specific
		"\xb3" => '&#1779;', # EXTENDED ARABIC-INDIC DIGIT THREE # Farsi specific
		"\xb4" => '&#1780;', # EXTENDED ARABIC-INDIC DIGIT FOUR # Farsi specific
		"\xb5" => '&#1781;', # EXTENDED ARABIC-INDIC DIGIT FIVE # Farsi specific
		"\xb6" => '&#1782;', # EXTENDED ARABIC-INDIC DIGIT SIX # Farsi specific
		"\xb7" => '&#1783;', # EXTENDED ARABIC-INDIC DIGIT SEVEN # Farsi specific
		"\xb8" => '&#1784;', # EXTENDED ARABIC-INDIC DIGIT EIGHT # Farsi specific
		"\xb9" => '&#1785;', # EXTENDED ARABIC-INDIC DIGIT NINE # Farsi specific
		
		"\xc1" => '&#1575;', #etc. # ARABIC LETTER ALEF
		"\xc3" => '&#1576;', # ARABIC LETTER BEH
		"\xc4" => '&#1662;', # ARABIC LETTER PEH # Farsi specific
		"\xc5" => '&#1578;', # ARABIC LETTER TEH
		"\xc6" => '&#1579;', # ARABIC LETTER THEH
		"\xc7" => '&#1580;', # ARABIC LETTER JEEM
		"\xc8" => '&#1670;', # ARABIC LETTER TCHEH # Farsi specific
		"\xc9" => '&#1581;', # ARABIC LETTER HAH
		"\xca" => '&#1582;', # ARABIC LETTER KHAH
		"\xcb" => '&#1583;', # ARABIC LETTER DAL
		"\xcc" => '&#1584;', # ARABIC LETTER THAL
		"\xcd" => '&#1585;', # ARABIC LETTER REH
		"\xce" => '&#1586;', # ARABIC LETTER ZAIN
		"\xcf" => '&#1688;', # ARABIC LETTER JEH # Farsi specific
		"\xd0" => '&#1587;', # ARABIC LETTER SEEN
		"\xd1" => '&#1588;', # ARABIC LETTER SHEEN
		"\xd2" => '&#1589;', # ARABIC LETTER SAD
		"\xd3" => '&#1590;', # ARABIC LETTER DAD
		"\xd4" => '&#1591;', # ARABIC LETTER TAH
		"\xd5" => '&#1592;', # ARABIC LETTER ZAH
		"\xd6" => '&#1593;', # ARABIC LETTER AIN
		"\xd7" => '&#1594;', # ARABIC LETTER GHAIN
		"\xd8" => '&#1601;', # ARABIC LETTER FEH
		"\xd9" => '&#1602;', # ARABIC LETTER QAF
		"\xda" => '&#1705;', # ARABIC LETTER KEHEH # Farsi specific
		"\xdb" => '&#1711;', # ARABIC LETTER GAF # Farsi specific
		"\xdc" => '&#1604;', # ARABIC LETTER LAM
		"\xdd" => '&#1605;', # ARABIC LETTER MEEM
		"\xde" => '&#1606;', # ARABIC LETTER NOON
		"\xdf" => '&#1608;', #  ARABIC LETTER WAW #note "u", "v", and "w" are all "vav"s in Farsi orthography.
		#"\xdf" => '&#1608;', # ARABIC LETTER WAW
		#"\xdf" => '&#1608;', # ARABIC LETTER WAW
		"\xe0" => '&#1607;', # ARABIC LETTER HEH
		#"\xd2" => '&#1728;', # ARABIC LETTER HEH WITH YEH ABOVE
		"\xe1" => '&#1610;', #  ARABIC LETTER YEH #note "i" and  "y" are both "ya"s in Farsi orthography.
		#"\xe1" => '&#1610;', # ARABIC LETTER YEH
		"\xfb" => '&#1574;', # ARABIC LETTER YEH WITH HAMZA ABOVE #note "`" and "I" are the same in Farsi orthography.
		#"\xd2" => '&#1574;', # ARABIC LETTER YEH WITH HAMZA ABOVE
		"\xf0" => '&#1614;', #  ARABIC FATHA #note the short vowels "a", "o" and "e" do not normally appear in Farsi orthography.
		"\xf2" => '&#1615;', # ARABIC DAMMA
		"\xf1" => '&#1616;', # ARABIC KASRA
		
		"\xc0" => '&#1570;', # ARABIC LETTER ALEF WITH MADDA ABOVE
		"\xc2" => '&#1569;', # ARABIC LETTER HAMZA
		"\xf8" => '&#1571;', # ARABIC LETTER ALEF WITH HAMZA ABOVE
		"\xfa" => '&#1572;', # ARABIC LETTER WAW WITH HAMZA ABOVE
		"\xf9" => '&#1573;', # ARABIC LETTER ALEF WITH HAMZA BELOW
		"\xfb" => '&#1574;', # ARABIC LETTER YEH WITH HAMZA ABOVE
		
		"\xa5" => '&#1642;', # ARABIC PERCENT SIGN	
		"\xag" => '&#1548;', # ARABIC COMMA
		"\xbb" => '&#1563;', # ARABIC SEMICOLON
		"\xbf" => '&#1567;', # ARABIC QUESTION MARK
		
		"\xa0" => ' ', # SPACE (really &#0032;)
		"\xa8" => '&#0040;', # LEFT PARENTHESIS
		"\xa9" => '&#0041;', # RIGHT PARENTHESIS
		"\xad" => '&#0045;', # HYPHEN-MINUS
		"\xa6" => '.', # FULL STOP (really &#0046;)
		"\xba" => '&#0058;', # COLON

	);

$in = shift || die "Provide a valid input file argument\n";
$out= shift || die "Provide an output file argument\n";
$in ne $out || die "Input and output files cannot be the same\n";

open(IN,$in);
open(OUT,">$out");
	
#prints the top part of an html page
print OUT ( 
	'<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//FA">',
	"\n",
	'<html lang="fa">',
	"\n",
	"\n",
	"<style>\n",
	"    body \{ text-align\:right \}\n",
	"<\/style>\n",
	"<head>\n",
	"<title><\/title>\n",
	"<\/head>\n",
	"<body>\n"
);
#prints the main part of the page (the transliterated characters)
while ($line = <IN>) {
	@charx = split //, $line;
	foreach $charx (@charx)
	{
		$newchar = $isiri2unicode{$charx}; 
		print OUT ($newchar);
	}
	#prints a carrage return after every line
	print OUT ("<br>");
}
#prints the bottom part of the html page
print OUT ( "\n<\/body>\n<\/html>\n");

#while ($line = <IN>) {
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
}
close (OUT);
close (IN);
