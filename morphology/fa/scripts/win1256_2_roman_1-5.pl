#! /usr/bin/perl
# Created by Jon Dehdari 2002
# Perl 5.6
# Converts Arabic Windows 1256 text to Romanized Arabic/Farsi text (my customized Romanization scheme)
# To Do:


%win1256_2roman = ( 	
		"\xb0" => '0', # EXTENDED ARABIC-INDIC DIGIT ZERO # Farsi specific
		"\xb1" => '1', # EXTENDED ARABIC-INDIC DIGIT ONE # Farsi specific
		"\xb2" => '2', # EXTENDED ARABIC-INDIC DIGIT TWO # Farsi specific
		"\xb3" => '3', # EXTENDED ARABIC-INDIC DIGIT THREE # Farsi specific
		"\xb4" => '4', # EXTENDED ARABIC-INDIC DIGIT FOUR # Farsi specific
		"\xb5" => '5', # EXTENDED ARABIC-INDIC DIGIT FIVE # Farsi specific
		"\xb6" => '6', # EXTENDED ARABIC-INDIC DIGIT SIX # Farsi specific
		"\xb7" => '7', # EXTENDED ARABIC-INDIC DIGIT SEVEN # Farsi specific
		"\xb8" => '8', # EXTENDED ARABIC-INDIC DIGIT EIGHT # Farsi specific
		"\xb9" => '9', # EXTENDED ARABIC-INDIC DIGIT NINE # Farsi specific
		
		"\xc7" => 'A', # ARABIC LETTER ALEF
		"\xff" => '|', # ARABIC LETTER ALEF #initial position; fake hex value for now
		"\xc8" => 'b', # ARABIC LETTER BEH
		"\xc9" => 'a', # ARABIC LETTER TEH MARBUTA # temporary, for arabic
		"\x81" => 'p', # ARABIC LETTER PEH # Farsi specific
		"\xca" => 't', # ARABIC LETTER TEH
		"\xcb" => 'V', # ARABIC LETTER THEH
		"\xcc" => 'j', # ARABIC LETTER JEEM
		"\x8d" => 'c', # ARABIC LETTER TCHEH # Farsi specific
		"\xcd" => 'H', # ARABIC LETTER HAH
		"\xce" => 'x', # ARABIC LETTER KHAH
		"\xcf" => 'd', # ARABIC LETTER DAL
		"\xd0" => 'L', # ARABIC LETTER THAL
		"\xd1" => 'r', # ARABIC LETTER REH
		"\xd2" => 'z', # ARABIC LETTER ZAIN
		"\x8e" => 'J', # ARABIC LETTER JEH # Farsi specific
		"\xd3" => 's', # ARABIC LETTER SEEN
		"\xd4" => 'C', # ARABIC LETTER SHEEN
		"\xd5" => 'S', # ARABIC LETTER SAD
		"\xd6" => 'D', # ARABIC LETTER DAD
		"\xd8" => 'T', # ARABIC LETTER TAH
		"\xd9" => 'Z', # ARABIC LETTER ZAH
		"\xda" => 'E', # ARABIC LETTER AIN
		"\xdb" => 'G', # ARABIC LETTER GHAIN
		"\xdd" => 'f', # ARABIC LETTER FEH
		"\xde" => 'q', # ARABIC LETTER QAF
		"\xdf" => 'k', # ARABIC LETTER KAF
		"\x98" => 'k', # ARABIC LETTER KEHEH # Farsi specific
		"\x90" => 'g', # ARABIC LETTER GAF # Farsi specific
		"\xe1" => 'l', # ARABIC LETTER LAM
		"\xe3" => 'm', # ARABIC LETTER MEEM
		"\xe4" => 'n', # ARABIC LETTER NOON
		"\xe6" => 'u', # ARABIC LETTER WAW #note "u", "v", and "w" are all "vav"s in Farsi orthography.
		"\xe5" => 'h', # ARABIC LETTER HEH
		"\xed" => 'i', # ARABIC LETTER YEH #note "i" and  "y" are both "ya"s in Farsi orthography.
		"\xec" => 'A', # ARABIC LETTER ALEF MAKSURA # temp
		"\xf3" => 'a', # ARABIC FATHA #note the short vowels "a", "o" and "e" do not normally appear in Farsi orthography.
		"\xf5" => 'o', # ARABIC DAMMA
		"\xf6" => 'e', # ARABIC KASRA
		
		"\xc2" => ']', # ARABIC LETTER ALEF WITH MADDA ABOVE
		"\xc1" => 'M', # ARABIC LETTER HAMZA
		"\xf0" => 'N', # ARABIC FATHATAN (TANVIN)
		"\xc3" => '|', # ARABIC LETTER ALEF WITH HAMZA ABOVE # temp
		"\xc4" => 'U', # ARABIC LETTER WAW WITH HAMZA ABOVE
		"\xc5" => '|', # ARABIC LETTER ALEF WITH HAMZA BELOW # temp
		"\xc6" => 'I', # ARABIC LETTER YEH WITH HAMZA ABOVE
		"\xc0" => 'X', # ARABIC LETTER HEH WITH YEH ABOVE
		"\xa5" => '%', # ARABIC PERCENT SIGN	
		"\xa1" => ',', # ARABIC COMMA
		"\xba" => ';', # ARABIC SEMICOLON
		"\xbf" => '?', # ARABIC QUESTION MARK
		
		"\xa0" => ' ', # NO-BREAK SPACE
		"\x20" => ' ', # SPACE
		"\xa8" => '(', # LEFT PARENTHESIS
		"\xa9" => ')', # RIGHT PARENTHESIS
		#"\xad" => '-', # HYPHEN-MINUS
		#"\xa6" => '.', # FULL STOP
		"\x5b" => '{', # LEFT SQUARE BRACKET	
		"\x5d" => '}', # RIGHT SQUARE BRACKET	
		"\xab" => '{', # LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
		"\xbb" => '}', # RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK  
		"\x2e" => '.', # FULL STOP
		"\xba" => ':', # COLON
		"\x0a" => "\n", # LINE FEED
		"\x9d" => '-', # ZERO WIDTH NON-JOINER

		"0" => '0', # DIGIT ZERO
		"1" => '1', # DIGIT ONE
		"2" => '2', # DIGIT TWO
		"3" => '3', # DIGIT THREE
		"4" => '4', # DIGIT FOUR
		"5" => '5', # DIGIT FIVE
		"6" => '6', # DIGIT SIX
		"7" => '7', # DIGIT SEVEN
		"8" => '8', # DIGIT EIGHT
		"9" => '9', # DIGIT NINE


	);

my $in = shift || die "Provide a valid input file argument\n";
#$out= shift || die "Provide a valid output file argument\n";
$in ne $out || die "Input and output files cannot be the same\n";
open(IN,$in);
#open(OUT,">$out");


### Temporary, for testing purposes
$in =~ s/\./_/g;
print STDOUT "\#__$in\n";

while (my $line = <IN>) {
	$line =~ s/([^\x81-\xff])\xc7/$1\xff/g; # Converts a regular alef into a basic placeholder alef, to be used with the authors romanization scheme eg. Au -> |u .

	$line =~ s/<br>/\n/g;
	$line =~ s/<p>/\n/g;
	$line =~ s/<\/td>/\n/g;
	$line =~ s/<\/div>/\n/g;
	$line =~ s/<.*?>//g; # Deletes all HTML tags on 1 line
	$line =~ s/<.*?//g;  # Deleses 1st part of line-spanning HTML tags
	$line =~ s/.*?>//g;  # Deletes 2nd part of line-spanning HTML tags
	
	my @charx = split //, $line;
	foreach my $charx (@charx)
	   { my $newchar = $win1256_2roman{$charx}; 
		print STDOUT ($newchar);
		#print OUT ($newchar);

	   }
	}

close (IN);
#close (OUT);
