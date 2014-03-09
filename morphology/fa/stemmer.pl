#!/usr/bin/perl
#
# Written by Jon Dehdari 2004-2005
# Perl 5.8
# Stemmer and Morphological Parser for Persian
# The license for this stemmer only is the LGPLv2.1 (www.fsf.org)
#
# The format of the resolve.txt file is as follows:
# 1. Mokassar: 		'ktb	ktAb'    OR    'ktb	ktAb_+PL'
# 2. Preparsed (speed):	'krdn	kr_+dn'
# 3. Don't stem:	'bArAn	bArAn'
# 4. Stop word:		'u	'

use strict;
use Getopt::Long;
#use diagnostics;

my $version        = "0.4.5";
my $date           = "2005/06/05";
my $copyright      = "(c) 2004-2005  Jon Dehdari - GPL v2.";
my $title          = "Persian stemmer $version, $date - $copyright";
my $resolve_file   = "resolve.txt";
my $recall         = 0;
my $show_links     = 0;
my $show_only_root = 0;
my $tokenize       = 0;
my $unvowel        = 0;
my @line;
my %resolve;
my @resolve;
my $resolve;
my $ar_chars    = "EqHSTDZLVU";
#my $longvowel    = "Aui]";

my $usage       = <<"END_OF_USAGE";
${title}

Syntax:   perl $0 [options] < input > output

Function: Stemmer and morphological analyzer for the Persian language.
          Inflexional morphemes are separated from their roots.

Options:
  -h, --help              Print usage
  -l, --links             Show morphological links
  -r, --root              Return only word roots
  -R, --recall            Increase recall by parsing ambiguous affixes
  -s, --stoplist <file>   Use stopword list <file>  (default: ./resolve.txt)
  -t, --tokenize          Tokenize punctuation
  -u, --unvowel           Remove short vowels
  -v, --version           Print version

END_OF_USAGE

GetOptions(
    'h|help|?'      => sub { print $usage; exit; },
    'l|links'       => \$show_links,
    'r|root'	    => \$show_only_root,
    'R|recall'	    => \$recall,
    's|stoplist:s'  => \$resolve_file,
    't|tokenize'    => \$tokenize,
    'u|unvowel'     => \$unvowel,
    'v|version'     => sub { print "$version\n"; exit; },
) or die $usage;


#open RESOLVE, "$resolve_file";
#while ($resolve = <RESOLVE>) {
#    chomp $resolve;
#    @resolve = split /\t/, $resolve;
#    %resolve = ( %resolve, "$resolve[0]" => "$resolve[1]" , );
#}
 
while ($_ = <>) {
chomp $_;

if ( $unvowel ) {
    $_ =~ s/\b([aeo])/|/g; # Inserts alef before words that begin with short vowel
    $_ =~ s/\bA/]/g;       # Changes long 'aa' at beginning of word to alef madda
    $_ =~ s/[aeo~]//g;     # Finally, removes all other short vowels and tashdids
}

#Inserts ZWNJ's where they should have been originally, but weren't
    $_ =~ s/(?<![a-zA-Z|])mi /mi-/g;    # 'mi-'
    $_ =~ s/(?<![a-zA-Z|])nmi /nmi-/g;  # 'mi-'
    $_ =~ s/ hA(?![a-zA-Z|])/-hA/g;     # '-hA'
    $_ =~ s/ hAi(?![a-zA-Z|])/-hAi/g;   # '-hA'
    $_ =~ s/h \|i(?![a-zA-Z|])/h-\|i/g; # '+h-|i'

#$_ =~ s/\b(?<!\])`exists($resolve[0])`\b/Eureka/g;
#print "Resolve-line: $resolve{??}\n";
#$_ =~ s/\b(?<!\])(??{exists($resolve{$_})})\b/eureka/g;
##@line = split /\b(?<!\])/, $_;
##print "LINE: @line\n";

#if ( exists($resolve{$_})) { print "$resolve{$_}\n";}
#else 
#{

## If these regular expressions are readable to you, you need to check in to a psychiatric ward!


##### Verb Section #####
# todo: 3Spast

######## Verb Prefixes ########
    $_ =~ s/\b(?<!\])n(\S{2,}?(?:im|id|nd|m|(?!A|u)i|d|(?:r|u|i|A|n|m|z)dn|(?:f|C|x|s)tn)(?:mAn|tAn|CAn|C)?)\b/n+_$1/g; # neg. verb prefix 'n+'
    $_ =~ s/(\bn\+_|\b)mi-(.{2,}?(?:im|id|nd|m|(?!hA)i|d)(?:mAn|tAn|CAn|C)?)\b/$1mi-+_$2/g;    # Durative verb prefix 'mi+'
    $_ =~ s/(\bn\+_|\b)mi(?!-)(.{2,}?(?:im|id|nd|m|(?!hA)i|d)(?:mAn|tAn|CAn|C)?)\b/$1mi+_$2/g; # Durative verb prefix 'mi+'
    $_ =~ s/\bb(?!ud)([^ ]{2,}?(?:im|id|nd|m|(?!hA)i|d)(?:mAn|tAn|CAn|C)?)\b/b+_$1/g;          # Subjunctive verb prefix 'be+'

######## Verb Suffixes ########
    $_ =~ s/(.*?(?:..d|..(?:s|f|C|x)t|\bn\+_.{2,}?|mi\+_.{2,}?|b\+_.{2,}?)(?:im|id|nd|m|(?!A|u)i|d))(mAn|tAn|CAn|C)\b/$1_+$2/g;   # Verbal Object verb suffix
    $_ =~ s/\b(n\+_.{2,}?|\S?mi\+_.{2,}?|b\+_\S{2,}?)([uAi])([iI])(im|id|i)(_\+.*)?\b/$1$2_+0$4$5/g;    # Removes epenthesized 'i/I' before Verbal Person suffixes 'im/id/i'
    $_ =~ s/\b(n\+_.{2,}?|.?mi\+_.{2,}?|b\+_\S{2,}?)([uA])i(nd|d|m)(_\+.*?)?\b/$1$2_+0$3$4/g;    # Removes epenthesized 'i' before Verbal Person suffixes 'm/d/nd'
    $_ =~ s/(.*?(?:\S{2}d|\S(?:s|f|C|x)t|\bn\+_\S{2,}?|mi-?\+_\S{2,}?|\bb\+_\S{2,}?))(nd|id|im|d|(?!A|u)i|m)(_\+.*?)?\b/$1_+$2$3/g;    # Verbal Person verb suffix
    $_ =~ s/(\S{2,}?)d_\+(nd|id|im|d|i|m)(_\+.*?)?\b/$1_+d_+$2$3/g;	# Verbal tense suffix 'd'
    $_ =~ s/(\S+?)(s|f|C|x)t_\+(nd|id|im|d|i|m)(_\+.*?)?\b/$1$2_+t_+$3$4/g;	# Verbal tense suffix 't'

    $_ =~ s/\b(\S{2,}?)(r|u|i|A|n|m)dn\b/$1$2_+dn/g;               # Verbal Infinitive '+dan'
    $_ =~ s/\b(\S{2,}?)(f|C|x|s)tn\b/$1$2_+tn/g;                   # Verbal Infinitive '+tan'
    $_ =~ s/\b(\S{2,}?)(i|n|A|u|z|r|b|h|s|k|C|f)ndh\b/$1$2_+ndh/g; # Verbal present participle '+andeh'
    $_ =~ s/\b(\S{2,}?)(C|r|n|A|u|i|m|z)dh\b/$1$2_+d_+h/g;         # Verbal past participle '+deh'
    $_ =~ s/\b(\S{2,}?)(C|f|s|x)th\b/$1$2_+th/g;                   # Verbal past participle '+teh'

    $_ =~ s/\b(C|z|kr|bu|dA|\]ur|di|br|\]m|mr|kn|ci)(dn|dh)\b/$1_+$2/g;  # 'shodan/zadan' Infinitive or Verbal past participle
    $_ =~ s/\b(C|z|kr|bu|dA|\]ur|di|br|\]m|mr|kn|ci)d(nd|i|id|m|im)?\b/$1_+d_+$2/g;  # 'shodan/zadan...' simple past - temp. until resolve file works
    $_ =~ s/\b(xuAh|dAr)(d|nd|i|id|m|im)\b/$1_+$2/g;  # future/have - temp. until resolve file works
    $_ =~ s/_\+d_\+\B/_+d/g;                          # temp. until resolve file works

######## Contractions ########
    $_ =~ s/\bcist\b/ch |st/g;                       # 'cheh ast'  - semicheap hack
    $_ =~ s/\b([^+ ]+?)([uAi])st(?!\s)\b/$1$2 |st/g; # normal "[uAi] ast", can't be followed by space (eg. mAst vs ...mA |st.)
    $_ =~ s/\bmrA\b/mn rA/g;                         # 'man rA'    - semicheap hack
    $_ =~ s/\btrA\b/tu rA/g;                         # 'man rA'    - semicheap hack


##### Noun Section #####

    $_ =~ s/\b([^+ ]{2,}?)(u|A)i(CAn|C|tAn|mAn)(_\+.*?)?\b/$1$2_+0_+$3$4/g;  # Removes epenthesized 'i' before genitive pronominal enclitics
    $_ =~ s/\b([^+ ]+?)([^uAi+])(CAn|C|tAn|mAn)(_\+.*?)?\b/$1$2_+$3$4/g;     # Genitive pronominal enclitics
#   $_ =~ s/\b([^+ ]{2,}?)(A|u)\b//g;             # Removes epenthesized 'i' before accusative enclitics
    $_ =~ s/\b([^+ ]{2,}?)(?!A)gAn\b/$1h_+An/g;   # Nominal plural suffix from stem ending in 'eh'
    $_ =~ s/\b([^+ ]+?)(A|u)i\b/$1$2_+e/g;        # Ezafe preceded by long vowel
    $_ =~ s/\b([^+ ]{2,}?)(hA|-hA)\b/$1_+$2/g;           # Nominal plural suffix
    $_ =~ s/\b([^+ ]{2,}?)(hA|-hA)(_\+.*?)\b/$1_+$2$3/g; # Nominal plural suffix
    $_ =~ s/\b(\S*?[$ar_chars]\S*?)At\b/$1_+h/g;         # Arabic plural: +At
    $_ =~ s/\b((?:m|\|)\S*?)At\b/$1_+h/g;                # Arabic plural: +At
    $_ =~ s/\b(m[^+ ]{3,}?)t\b/$1_+t/g;                  # Arabic fem: +at
    $_ =~ s/\b([^+ ]*?[$ar_chars][^+ ]*?)t\b/$1_+t/g;    # Arabic fem: +at

##### Adjective Section #####

    $_ =~ s/\b([^+ ]+?)trin\b/$1_+trin/g;  # Adjectival Superlative suffix
    $_ =~ s/\b([^+ ]+?)tr\b/$1_+tr/g;      # Adjectival Comparative suffix
    $_ =~ s/\b([^+ ]+?)(?!A)gi\b/$1h_+i/g; # Adjectival suffix from stem ending in 'eh'
    $_ =~ s/\b([^+ ]+?)(i|I)i\b/$1_+i/g;   # '+i' suffix preceded by 'i' (various meanings)


##### End #####

### Increase recall, but lower precision
if ( $recall ) {
    $_ =~ s/\b([^+ ]{2,}?)(An)\b/$1_+$2/g; # Plural suffix '+An'
    $_ =~ s/\b([^+ ]+?)i\b/$1_+i/g;        # Indef. '+i' suffix
}

### Deletes everything but the root
if ( $show_only_root ) {
    $_ =~ s/\b[^ ]+\+_([^ ]+?)\b/$1/g;  # Removes prefixes
    $_ =~ s/\b([^ ]+?)_\+[^ ]+\b/$1/g;  # Removes suffixes
}

### Deletes word boundaries ' ' from morpheme links '_+'/'+_'
unless ( $show_links ) {
    $_ =~ s/_\+0/ /g;  # Removes epenthesized letters
    $_ =~ s/_\+-/ /g;  # Removes suffix links w/ ZWNJs
    $_ =~ s/_\+/ /g;   # Removes all suffix links
    $_ =~ s/-\+_/ /g;  # Removes prefix links w/ ZWNJs
    $_ =~ s/\+_/ /g;   # Removes all prefix links
}

### Tokenizes punctuation
if ( $tokenize ) {
    $_ =~ s/([ ,.;:!?(){}#1-9\/])/ $1 /g;  # Pads punctuation w/ spaces
    $_ =~ s/(\s){2,}/$1/g;                 # Removes multiple spaces
}

print "$_\n";

#} # ends else
} # ends while (<>)
