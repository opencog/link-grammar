#! /usr/bin/perl
# Written by Jon Dehdari 2005
# Perl 5.8
# Converts Persian and Arabic UTF-8 text to Romanized Arabic/Farsi text (my customized Romanization scheme)
# The license is the GNU General Public License v. 2 or newer
# To Do:	

use strict;
#use diagnostics;
use encoding "utf8";
use charnames ":full";


my $in;
my $line;
my $charx;
my @charx;
my $newchar;
my %utf8_2_roman;


%utf8_2_roman = ( 	
		"\N{EXTENDED ARABIC-INDIC DIGIT ZERO}" => '0', # EXTENDED ARABIC-INDIC DIGIT ZERO # Farsi specific
		"\N{EXTENDED ARABIC-INDIC DIGIT ONE}" => '1;', # EXTENDED ARABIC-INDIC DIGIT ONE # Farsi specific
		"\N{EXTENDED ARABIC-INDIC DIGIT TWO}" => '2', # EXTENDED ARABIC-INDIC DIGIT TWO # Farsi specific
		"\N{EXTENDED ARABIC-INDIC DIGIT THREE}" => '3', # EXTENDED ARABIC-INDIC DIGIT THREE # Farsi specific
		"\N{EXTENDED ARABIC-INDIC DIGIT FOUR}" => '4', # EXTENDED ARABIC-INDIC DIGIT FOUR # Farsi specific
		"\N{EXTENDED ARABIC-INDIC DIGIT FIVE}" => '5', # EXTENDED ARABIC-INDIC DIGIT FIVE # Farsi specific
		"\N{EXTENDED ARABIC-INDIC DIGIT SIX}" => '6', # EXTENDED ARABIC-INDIC DIGIT SIX # Farsi specific
		"\N{EXTENDED ARABIC-INDIC DIGIT SEVEN}" => '7', # EXTENDED ARABIC-INDIC DIGIT SEVEN # Farsi specific
		"\N{EXTENDED ARABIC-INDIC DIGIT EIGHT}" => '8', # EXTENDED ARABIC-INDIC DIGIT EIGHT # Farsi specific
		"\N{EXTENDED ARABIC-INDIC DIGIT NINE}" => '9', # EXTENDED ARABIC-INDIC DIGIT NINE # Farsi specific
		
		"\N{ARABIC LETTER ALEF}" => 'A', # ARABIC LETTER ALEF
		"\N{MERCURY}" => '|', # ARABIC LETTER ALEF #initial position; fake hex value for now
		"\N{ARABIC LETTER BEH}" => 'b', # ARABIC LETTER BEH
		"\N{ARABIC LETTER TEH MARBUTA}" => 'a', # ARABIC LETTER TEH MARBUTA # temporary, for arabic
		"\N{ARABIC LETTER PEH}" => 'p', # ARABIC LETTER PEH
		"\N{ARABIC LETTER TEH}" => 't', # ARABIC LETTER TEH
		"\N{ARABIC LETTER THEH}" => 'V', # ARABIC LETTER THEH
		"\N{ARABIC LETTER JEEM}" => 'j', # ARABIC LETTER JEEM
		"\N{ARABIC LETTER TCHEH}" => 'c', # ARABIC LETTER TCHEH
		"\N{ARABIC LETTER HAH}" => 'H', # ARABIC LETTER HAH
		"\N{ARABIC LETTER KHAH}" => 'x', # ARABIC LETTER KHAH
		"\N{ARABIC LETTER DAL}" => 'd', # ARABIC LETTER DAL
		"\N{ARABIC LETTER THAL}" => 'L', # ARABIC LETTER THAL
		"\N{ARABIC LETTER REH}" => 'r', # ARABIC LETTER REH
		"\N{ARABIC LETTER ZAIN}" => 'z', # ARABIC LETTER ZAIN
		"\N{ARABIC LETTER JEH}" => 'J', # ARABIC LETTER JEH
		"\N{ARABIC LETTER SEEN}" => 's', # ARABIC LETTER SEEN
		"\N{ARABIC LETTER SHEEN}" => 'C', # ARABIC LETTER SHEEN
		"\N{ARABIC LETTER SAD}" => 'S', # ARABIC LETTER SAD
		"\N{ARABIC LETTER DAD}" => 'D', # ARABIC LETTER DAD
		"\N{ARABIC LETTER TAH}" => 'T', # ARABIC LETTER TAH
		"\N{ARABIC LETTER ZAH}" => 'Z', # ARABIC LETTER ZAH
		"\N{ARABIC LETTER AIN}" => 'E', # ARABIC LETTER AIN
		"\N{ARABIC LETTER GHAIN}" => 'G', # ARABIC LETTER GHAIN
		"\N{ARABIC LETTER FEH}" => 'f', # ARABIC LETTER FEH
		"\N{ARABIC LETTER QAF}" => 'q', # ARABIC LETTER QAF
		"\N{ARABIC LETTER KAF}" => 'k', # ARABIC LETTER KAF
		"\N{ARABIC LETTER KEHEH}" => 'k', # ARABIC LETTER KEHEH
		"\N{ARABIC LETTER GAF}" => 'g', # ARABIC LETTER GAF
		"\N{ARABIC LETTER LAM}" => 'l', # ARABIC LETTER LAM
		"\N{ARABIC LETTER MEEM}" => 'm', # ARABIC LETTER MEEM
		"\N{ARABIC LETTER NOON}" => 'n', # ARABIC LETTER NOON
		"\N{ARABIC LETTER WAW}" => 'u', # ARABIC LETTER WAW
		"\N{ARABIC LETTER HEH}" => 'h', # ARABIC LETTER HEH
		"\N{ARABIC LETTER YEH}" => 'i', # ARABIC LETTER YEH
		"\N{ARABIC LETTER FARSI YEH}" => 'i', # ARABIC LETTER FARSI YEH
		"\N{ARABIC LETTER ALEF MAKSURA}" => 'A', # ARABIC LETTER ALEF MAKSURA # temp
		"\N{ARABIC FATHA}" => 'a', # ARABIC FATHA #note the short vowels "a", "o" and "e" do not normally appear in Farsi orthography.
		"\N{ARABIC DAMMA}" => 'o', # ARABIC DAMMA
		"\N{ARABIC KASRA}" => 'e', # ARABIC KASRA
		
		"\N{ARABIC LETTER ALEF WITH MADDA ABOVE}" => ']', # ARABIC LETTER ALEF WITH MADDA ABOVE
		"\N{ARABIC LETTER HAMZA}" => 'M', # ARABIC LETTER HAMZA
		"\N{ARABIC FATHATAN}" => 'N', # ARABIC FATHATAN
		"\N{ARABIC LETTER ALEF WITH HAMZA ABOVE}" => '|', # ARABIC LETTER ALEF WITH HAMZA ABOVE # temp
		"\N{ARABIC LETTER WAW WITH HAMZA ABOVE}" => 'U', # ARABIC LETTER WAW WITH HAMZA ABOVE
		"\N{ARABIC LETTER ALEF WITH HAMZA BELOW}" => '|', # ARABIC LETTER ALEF WITH HAMZA BELOW # temp
		"\N{ARABIC LETTER YEH WITH HAMZA ABOVE}" => 'I', # ARABIC LETTER YEH WITH HAMZA ABOVE
		"\N{ARABIC LETTER HEH WITH YEH ABOVE}" => 'X', # ARABIC LETTER HEH WITH YEH ABOVE
		
		"\N{ARABIC PERCENT SIGN}" => '%', # ARABIC PERCENT SIGN	
		"\N{ARABIC COMMA}" => ',', # ARABIC COMMA
		"\N{ARABIC SEMICOLON}" => ';', # ARABIC SEMICOLON
		"\N{ARABIC QUESTION MARK}" => '?', # ARABIC QUESTION MARK
		"\N{ZERO WIDTH NON-JOINER}" => "-", # ZERO WIDTH NON-JOINER
		
		"\xa0" => ' ', # NO-BREAK SPACE
		"\x20" => ' ', # SPACE
		"\x2e" => '.', # FULL STOP
		"\x5b" => '{', # LEFT SQUARE BRACKET	
		"\x5d" => '}', # RIGHT SQUARE BRACKET	
		"\xab" => '{', # LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
		"\xbb" => '}', # RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
		#"\x{}" => '(', # LEFT PARENTHESIS
		#"\x{}" => ')', # RIGHT PARENTHESIS
		#"\x{}" => '-', # HYPHEN-MINUS
		#"\x{}" => ':', # COLON
		"\x0a" => "\n", # LINE FEED
		
#		"0" => '0', # DIGIT ZERO
#		"1" => '1', # DIGIT ONE
#		"2" => '2', # DIGIT TWO
#		"3" => '3', # DIGIT THREE
#		"4" => '4', # DIGIT FOUR
#		"5" => '5', # DIGIT FIVE
#		"6" => '6', # DIGIT SIX
#		"7" => '7', # DIGIT SEVEN
#		"8" => '8', # DIGIT EIGHT
#		"9" => '9', # DIGIT NINE

	);


	
open STDIN, "<:encoding(UTF-8)" ;


while ($line = <STDIN>) {
    
	$line =~ s/([\x01-\xa0])\N{ARABIC LETTER ALEF}/$1\N{MERCURY}/g; # Converts a regular alef into a basic placeholder alef, to be used with the authors romanization scheme eg. Au -> |u .
	
	@charx = split //, $line;
	foreach $charx (@charx)
	   {
		$newchar = $utf8_2_roman{$charx}; 
		print("$newchar");
	   }
}

