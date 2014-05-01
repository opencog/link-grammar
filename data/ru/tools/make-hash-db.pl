#!/usr/bin/perl -w

use strict;
use warnings;
use BerkeleyDB;
use vars qw( %file_tail %file_rule %rule %file_root %file_para %para );
use Encode;

# mrd format docs
# https://code.google.com/p/opencorpora/wiki/MRDFileFormat
# http://aot.ru/docs/sokirko/Dialog2004.htm

system("rm -f rule.db tail.db root.db para.db");

tie %file_rule, 'BerkeleyDB::Hash', -Filename => "rule.db", -Flags => DB_CREATE or die "cant\n";

print "Preparing rules:\n";
open(F, "rgramtab20030306.tab") or die("cant");
while(<F>) {
    if(/^\s*$/)  { next; }
    if(/^\/\/.*$/) { next; }
    chomp; # Ancode, ,PartOfSpeech, Grammems
    my @a = split(/ /,du($_));
    my $s;
#    if($#a>3) {$s=":".$a[4];} else {$s="";}
    $file_rule{ek($a[0])} = ek($a[2].",".$a[3]);
    $rule{$a[0]} = $a[2].",".$a[3];
}
close F;
untie %file_rule;

$| = 1;

tie %file_para, 'BerkeleyDB::Hash', -Filename => "para.db", -Flags => DB_CREATE or die "cant\n";
tie %file_tail, 'BerkeleyDB::Hash', -Filename => "tail.db", -Flags => DB_CREATE or die "cant\n";
open(F, "morphs20030410.mrd") or die("cant");
my $nt = <F>; chomp $nt;
print "Preparing tails:\n";
for(my $i=0; $i<$nt; $i++) {
    my $s = <F>;
    chomp $s;
    $file_para{ek($i)} = ek(du($s)); 
    $para{$i} = du($s);
    my @a = split(/%/, du($s));
    shift @a;
    my @rulestr = ();
    my $state = 0;
    for my $a (@a) {
        my ($tail ,$rulestr) = split /\*/, $a;
        $tail = lc $tail;
        $file_tail{ek("$i:$tail")} = ek($rulestr);
    }
    print "." if ($i % 100 == 0);
}
untie %file_tail;
untie %file_para;

my %examples = (); my %para_freq = ();

my $nr = <F>; chomp $nr;
print "\nPreparing roots:\n";
tie %file_root, 'BerkeleyDB::Hash', -Filename => "root.db", -Flags => DB_CREATE or die "cant\n";
my $ni=0;
while(<F>) {
    chomp;
    my $s = lc(du($_));
    my ($str, $num) = split(/ /, $s);
    $str = "" if $str eq "#";
    $file_root{ek($str)} .= ek(":$num");
    $para_freq{$num}++;
    print "." if ( $ni++ % 1000 eq 0 );
    my $type = $para{$num};
    my @tpl = map { $str.$_ } split(/%/,$type); shift @tpl; 
    my @gra = @tpl;
    s/\*.+?$// foreach @tpl;
    s/^.+?\*// foreach @gra;
    foreach my $i ( 0 .. $#gra ) {
         my $c = $gra[$i];
         my @anlist = unpack "A2" x (length($c)/2), $c;
         foreach my $ancode ( @anlist ) {
             my @cases = defined $examples{$ancode} ? @{ $examples{$ancode} } : ();
             if ( $#cases < 5 ) {
                 push @{ $examples{$ancode} }, lc($tpl[$i]);
             }
         }
    }
}
untie %file_root;

open(EXM,"|sort -k 2 > morph-examp.txt") or die("cant");
foreach my $ancode ( keys %examples ) {
    my @cases =  @{ $examples{$ancode} };
    my $rl = $rule{$ancode};
    print EXM eu("$ancode\t$rl\t".join("\t",@cases))."\n";
}
close(EXM);

open(ANC,">ancode-not-defined.txt") or die("cant");
foreach my $ancode ( sort { $rule{$a} cmp $rule{$b} } keys %rule ) {
    if ( ! defined $examples{$ancode} ) {
        print ANC eu("$ancode\t$rule{$ancode}\n");
    }
}
close(ANC);

open(PAR,">paradigm-not-used.txt") or die("cant");
foreach my $i ( sort { $a <=> $b } keys %para ) {
    if ( ! defined $para_freq{$i} ) {
        print PAR eu("$i\t$para{$i}\n");
    }
}
close(PAR);

print "\ntails:$nt\troots:$nr\n";

sub eu {
    return encode("utf-8",shift);
}

sub du {
    return decode("utf-8",shift);
}

sub ek {
    return encode("koi8-r",shift);
}

sub dk {
    return decode("koi8-r",shift);
}

