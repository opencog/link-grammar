#!/usr/bin/perl
# Written by Jon Dehdari 2002-2004
# Perl 5.6 and newer
# Converts Romanized Persian text to web page with HTML Unicode characters
# Syntax: ./roman2unicode_1-4.pl < input.txt > output.html

%roman2unicode = ( 	
		'0' => '&#1776;', # EXTENDED ARABIC-INDIC DIGIT ZERO # Persian specific
		'1' => '&#1777;', # EXTENDED ARABIC-INDIC DIGIT ONE # Persian specific
		'2' => '&#1778;', # EXTENDED ARABIC-INDIC DIGIT TWO # Persian specific
		'3' => '&#1779;', # EXTENDED ARABIC-INDIC DIGIT THREE # Persian specific
		'4' => '&#1780;', # EXTENDED ARABIC-INDIC DIGIT FOUR # Persian specific
		'5' => '&#1781;', # EXTENDED ARABIC-INDIC DIGIT FIVE # Persian specific
		'6' => '&#1782;', # EXTENDED ARABIC-INDIC DIGIT SIX # Persian specific
		'7' => '&#1783;', # EXTENDED ARABIC-INDIC DIGIT SEVEN # Persian specific
		'8' => '&#1784;', # EXTENDED ARABIC-INDIC DIGIT EIGHT # Persian specific
		'9' => '&#1785;', # EXTENDED ARABIC-INDIC DIGIT NINE # Persian specific

		'A' => '&#1575;', # ARABIC LETTER ALEF
		'|' => '&#1575;', # ARABIC LETTER ALEF
		'b' => '&#1576;', # ARABIC LETTER BEH
		'p' => '&#1662;', # ARABIC LETTER PEH 
		't' => '&#1578;', # ARABIC LETTER TEH
		'V' => '&#1579;', # ARABIC LETTER THEH
		'j' => '&#1580;', # ARABIC LETTER JEEM
		'c' => '&#1670;', # ARABIC LETTER TCHEH
		'H' => '&#1581;', # ARABIC LETTER HAH
		'x' => '&#1582;', # ARABIC LETTER KHAH
		'd' => '&#1583;', # ARABIC LETTER DAL
		'L' => '&#1584;', # ARABIC LETTER THAL
		'r' => '&#1585;', # ARABIC LETTER REH
		'z' => '&#1586;', # ARABIC LETTER ZAIN
		'J' => '&#1688;', # ARABIC LETTER JEH 
		's' => '&#1587;', # ARABIC LETTER SEEN
		'C' => '&#1588;', # ARABIC LETTER SHEEN
		'S' => '&#1589;', # ARABIC LETTER SAD
		'D' => '&#1590;', # ARABIC LETTER DAD
		'T' => '&#1591;', # ARABIC LETTER TAH
		'Z' => '&#1592;', # ARABIC LETTER ZAH
		'E' => '&#1593;', # ARABIC LETTER AIN
		'G' => '&#1594;', # ARABIC LETTER GHAIN
		'f' => '&#1601;', # ARABIC LETTER FEH
		'q' => '&#1602;', # ARABIC LETTER QAF
		'K' => '&#1705;', # ARABIC LETTER KEHEH
		'k' => '&#1603;', # ARABIC LETTER KAF
		'g' => '&#1711;', # ARABIC LETTER GAF
		'l' => '&#1604;', # ARABIC LETTER LAM
		'm' => '&#1605;', # ARABIC LETTER MEEM
		'n' => '&#1606;', # ARABIC LETTER NOON
		'u' => '&#1608;', # ARABIC LETTER WAW
		'v' => '&#1608;', # ARABIC LETTER WAW
		'w' => '&#1608;', # ARABIC LETTER WAW
		'h' => '&#1607;', # ARABIC LETTER HEH
		'X' => '&#1728;', # ARABIC LETTER HEH WITH YEH ABOVE
		#'i' => '&#1610;', # ARABIC LETTER YEH
		'i' => '&#1740;', # ARABIC LETTER FARSI YEH # For Arabic, comment out this line and uncomment previous line
		#'y' => '&#1740;', # ARABIC LETTER FARSI YEH # This should not be used, except as a temporary crutch
		'I' => '&#1574;', # ARABIC LETTER YEH WITH HAMZA ABOVE
		'a' => '&#1614;', # ARABIC FATHA
		'o' => '&#1615;', # ARABIC DAMMA
		'e' => '&#1616;', # ARABIC KASRA
		'~' => '&#1617;', # ARABIC SHADDA
		
		',' => '&#1548;', # ARABIC COMMA
		';' => '&#1563;', # ARABIC SEMICOLON
		'?' => '&#1567;', # ARABIC QUESTION MARK
		' ' => ' ', # space
		'.' => '.', # period

		']' => '&#1570;', # ARABIC LETTER ALEF WITH MADDA ABOVE
		'M' => '&#1569;', # ARABIC LETTER HAMZA
		'N' => '&#1611;', # ARABIC FATHATAN
		'U' => '&#1572;', # ARABIC LETTER WAW WITH HAMZA ABOVE
		'-' => '&#8204;', # ZERO WIDTH NON-JOINER
		# add more characters later!

	);
	
#$in = shift || die "Provide a valid input file argument\n";
#$out= shift || die "Provide an output file argument\n";
#$in ne $out || die "Input and output files cannot be the same\n";

#open(IN,$in);
#open(OUT,">$out");

#prints the top part of an html page
print STDOUT ( 
	'<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">',
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

while ($line = <STDIN>) {

	$line =~ s/^([aeo])/|$1/g; # Inserts alef before words that begin with short vowel
	$line =~ s/ ([aeo])/ |$1/g; # Inserts alef before words that begin with short vowel
	$line =~ s/^A/]/g; # Changes long 'a' at beginning of word to alef madda
	$line =~ s/ A/ ]/g; # Changes long 'a' at beginning of word to alef madda
	$line =~ s/_A/_]/g; # Changes long 'a' at beginning of word to alef madda
	$line =~ s/k/K/g; # Changes KAF's to KEHEH's.  Comment this out for Arabic text.
	$line =~ s/_e_/e_/g; # Changes logical ezafe representation to surface form
	$line =~ s/_([aeo])/_|$1/g; # Inserts alef before words that begin with short vowel
	$line =~ s/_/ /g; # Changes word boundaries '_' to spaces
	$line =~ s/\t/   /g; # Changes tabs into spaces
	#$line =~ s/[aeo~]//g; # Uncomment this line if you want unvoweled text

	#Inserts ZWNJ's where they should have been originally, but weren't
	$line =~ s/(?<![a-zA-Z|])mi /mi-/g; # 'mi-'
	$line =~ s/(?<![a-zA-Z|])nmi /nmi-/g; # 'mi-'
	$line =~ s/ hA(?![a-zA-Z|])/-hA/g; # '-hA'
	$line =~ s/ hAi(?![a-zA-Z|])/-hAi/g; # '-hA'
	$line =~ s/h \|i(?![a-zA-Z|])/h-\|i/g; # '+h-|i'

	# This is the heart and soul of the page
	@charx = split //, $line;
	foreach $charx (@charx)
	{
		$newchar = $roman2unicode{$charx}; 
		print STDOUT ($newchar);
	}

	print STDOUT ("<br>\n"); # Prints a <br> tag and newline character after every line
}

print STDOUT ( "\n<\/body>\n<\/html>\n"); # Prints the bottom part of web page

#close (OUT);
#close (IN);
