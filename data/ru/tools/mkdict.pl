#!/usr/bin/perl
#
# Generate Russian dictionaries for Link Grammar. 
# Based on aot.ru opensource files
#
# Copyright (c) 2004, 2012, 2013 Sergey Protasov
# Copyright (c) 2012, 2013 Linas Vepstas

use strict;
use warnings;

use BerkeleyDB;
use Encode;
use utf8;
use vars qw( %rule %tail %root );

tie %rule, "BerkeleyDB::Hash", -Filename => "rule.db",  -Flags => DB_RDONLY  or die "run ./make-hash-db.pl first!";
tie %tail, "BerkeleyDB::Hash", -Filename => "tail.db",  -Flags => DB_RDONLY  or die "run ./make-hash-db.pl first!";
tie %root, "BerkeleyDB::Hash", -Filename => "root.db",  -Flags => DB_RDONLY  or die "run ./make-hash-db.pl first!";

sub lgtrim($) {
        my ($cur) = @_;
        $_ = $cur;
        chomp;
        s/имя,//;
        s/фам,//;
        s/Н,относ/Н/;
        s/Н,вопр>/Н/;
        return $_;
}

sub alnum($) {
    my ($i) = @_;

    my $a = int($i/26); my $va = $i % 26;
    my $b = int($a/26); my $vb = $a % 26;
    my $c = int($b/26); my $vc = $b % 26;

    my @lets = ('A'..'Z');

    my $la = $lets[$va];
    my $lb = $lets[$vb];
    my $lc = $lets[$vc];
    return "$lc$lb$la";
}

# These words are defined in the base.dict ( 4.0.dict ) 
my @nonwords1 = ("бы.p", "когда.i", "если.i", "и.i", "как.e",
	"не.p", "отныне.e", "отовсюду.e", "задом.e", "вполоборота.e",
	"боком.e", "велик.amss");
my %skipwords = (); $skipwords{$_}++ foreach @nonwords1;

my @nonwords2 = qw(не как где и если когда два две оба обе три четыре);
my $rHskipstemcon = ();
foreach my $nw1 ( @nonwords2 ) {
    for(my $i=0; $i <= length $nw1; $i++) {
        my ($left, $right) = (substr($nw1,0,$i), substr($nw1,$i));
        if(my $d = $root{ek($left)}) {
            my @num = split(/:/, $d);
            shift @num;
            foreach my $num(@num){
                if(my $c = dk($tail{ek("$num:$right")})) {
                    my $alcon = "LL".alnum($num)."+";
                    $rHskipstemcon->{$left}->{$num}++;
                    $rHskipstemcon->{"=".$right}->{$num}++;
                    #print eu("$i\t$nw1\t$left\t$right\t$num\t$alcon\t$c\n");
                }
            }
        }
    }
}

my %nums_for_empty_stem = ();
while (my ($stem, $val) = each(%root)){
   next unless $stem eq "";
   chomp $val; $val =~ s/^://;
   my @nums = split(/:/, $val);
   $nums_for_empty_stem{$_}++ foreach @nums;
}

print scalar(keys %rule) . "  n rules\n";

my $tcnt=0; my %uniqon = (); my %lefties = (); my %righties = (); my %suftags = ();
my %alltags = (); my $rHse = {};

while (my ($numsuf, $modestr) = each(%tail)){
    $numsuf = dk($numsuf); 

    chomp $numsuf;
    my ($lcon, $suf) = split(/:/, $numsuf);
    $suf = "=".$suf;
    $uniqon{$lcon} += 1; # 439

    my @modeli = unpack "A2" x (length($modestr)/2), dk($modestr);
    my @res = ();
    foreach my $mode ( @modeli ) { # 
       my $pos = defined $rule{ek($mode)} ? dk($rule{ek($mode)}) : "U";
       chomp $pos; 
       print eu($mode) . " U\n" if ( $pos eq "U" );
       #print eu($pos)."\n"; # Г,св,пе,прч,прш,дст,ед,мр,тв
       # copied straight from TestLem.pm
       if ( $pos =~ /мр-жр/ ) {
           $pos =~ s/мр-жр/мр/; push @res, $pos;
           $pos =~ s/мр/жр/; push @res, $pos;
       } else {
           push @res, $pos;
       }
       #print eu($pos) . "\n";
    }

    # copied straight from TestLem.pm
    # проверка винительного одушевленного
    for (my $i=0; $i <= $#res; $i++) {
        if($res[$i] =~ /П(.*мр.*)вн.*/ || $res[$i] =~ /П(.*мн.*)вн.*/) {
            my $rr = $1;
            my $l = 0; my $n = 0;
            foreach (@res) {
                my $rrr = $rr."рд";
                my $rri = $rr."им";
                $l = 1 if /П$rrr.*/; # correct
                $n = 1 if /П$rri.*/;
                #$l = 1 if /П$rrрд.*/; # incorrect, but match with TestLem.pm
                #$n = 1 if /П$rrим.*/;
            }
            $res[$i] .= ",од" if $l;
            $res[$i] .= ",но" if $n;
        }
        #$rHse->{$lcon}->{$res[$i]}++ if ($suf eq "="); # 439
        #print "$lcon ".eu($res[$i])."\n" if ($suf eq "=");
    }

    # copied straight from TestLem.pm
    # Get rid of duplicates
    my %dres = ();
    foreach my $pos(@res) {
       my $tpos = lgtrim($pos);
       $rHse->{$lcon}->{$tpos}++ if ($suf eq "=");
       #print "$lcon ".eu($tpos)."\n" if ($suf eq "=");
       $dres{$tpos} = 1;
    }
    my @uni = keys %dres;

    foreach my $pos(@uni) {
       $alltags{$pos}++;
       my $suftag = "$suf.".parse($pos); 

       if ( not defined $nums_for_empty_stem{$lcon} ) {
           my $alcon = alnum($lcon);
           my $lcontor = "LL$alcon-";
           if ( not defined $rHskipstemcon->{$suf}->{$lcon} ) {
               $lefties{$suftag} .= $lcontor . " ";
           }
       } else {
           $suftag =~ s/^=//;
       }

       my $rcontor = "<morph-" . $pos . ">";
       $righties{$suftag} .= $rcontor . " ";
       #print eu($suftag)." .=\t$rcontor\n";

       $suftags{$suftag} = 1;
    }
    $tcnt++;
}

my $scnt = scalar(keys %suftags);

print "gen suffix\n";
open(SUF,">suffix.dict") or die("cant");
print SUF "%\n";
print SUF "% Tagged Suffix Table\n";
print SUF "%\n";
print SUF "% Total number of suffixes = $tcnt\n";
print SUF "% Total number of tagged suffixes = $scnt\n";
print "% Total number of suffixes = $tcnt\n";
print "% Total number of tagged suffixes = $scnt\n";

my $un = scalar(keys %uniqon);
print SUF "% Number of unique stem-to-suffix connectors = $un\n";
print SUF "%\n";

my @suftagli = keys %suftags;
my @ssuftagli = sort @suftagli;

my %common=();
my $nsf = 0;
foreach my $suftag ( @ssuftagli ) {
    my $left  = defined $lefties{$suftag}  ? $lefties{$suftag}  : "";
    my $right = defined $righties{$suftag} ? $righties{$suftag} : "";

    chomp $left; chomp $right;
    #print eu("$suftag\t[$left][$right]\n");
    
    my @rightli = split / /, $right;
    my %runiq = map { $_ => 1 } @rightli;
    my @uright = keys %runiq;
    if (1 < $#uright) {
        print "% Heyyyyy multi!\n";
        die "oh no Mr. Billl!";
    }
    my @leftli = split / /, $left;
    my @sleftli = sort @leftli;

    next if ( $#sleftli eq -1 and $suftag =~ m/^\=/ );
    next if ( $suftag =~ m/^\=\./ );

    # Compute the connection string....
    # suftag:=а.cmv constr:(LLDTX- or LLDTY-)
    my $constr = "(".join(" or ", @sleftli).")\n";

    $constr .= "    & " . $uright[0];

    $common{$constr} .= "\n" . $suftag;

    $nsf++;
}

print SUF "%\n% Total number of suftags = $nsf\n%\n";
print "% Total number of suftags = $nsf\n";

$nsf = scalar(keys %common);
print SUF "%\n% Total number of entries = $nsf\n%\n";
print "% Total number of entries = $nsf\n";


# reverse the thin, so that we can alphabetize it.
my %rev = ();
while ((my $constr, my $sufs) = each(%common)) {
	$rev{$sufs} = $constr;
	# print "$sufs :\n  $constr;\n";
}

# place into alphabetical order
foreach my $sufs( sort keys %rev ) {
	my $constr = $rev{$sufs};
	print SUF eu($sufs)." :\n  ".eu($constr).";\n";
}
close(SUF);

# mkmorph

my %cat2 = (); my %uni = (); my %ord = (); my %cmnt = ();

# load the morphology info.
my $comment = "";
my $cnt = 0;
open FILE, "4.0.morph";
while (<FILE>) {
   next unless /^\%/;

   # Save the inline comments for later re-printing.
   if (!/morph/) { $comment .= du($_); }

   next unless /morph/;
   my $str = du($_);
   chomp $str;

   #% morph{"ПРЕДК,нст"} ="<глагол-пе-пр-sub> &  (<глагол-pr> or <макро-глагол-pr>)";
   (my $nr, my $rl) = split(/\=/, $str);
   $nr =~ s/^\%\ +morph\{\"//; $nr =~ s/\"\}\ *$//;
   $rl =~ s/^\ ?\"//; $rl =~ s/\"\;\ ?$//;

   $cat2{$nr} = $rl;

   # Keep them in the same order.
   $cnt++;
   $uni{$nr} = $cnt;
   $ord{$cnt} = $nr;
   $cmnt{$cnt} = $comment;
   $comment = "";
}
close FILE;

print "gen morph\n";
open(MRF,">morph.dict") or die("cant");
print MRF <<EOF;
%%
%% Link Grammar for Russian -- Morphology rules.
%%
%% based on aot.ru opensource files
%%
%% Copyright (c) 2004, 2012 Sergey Protasov
%% http://sz.ru/parser/
%% svp dot zuzino.net.ru, svp dot tj.ru
%%
%% Below uses the utf8 encoding
EOF

my $mcnt = scalar( keys %cat2 );
print MRF "%\n% Total number of rules = $mcnt\n%\n";
print "% Total number of rules = $mcnt\n";

my $rcnt=1; my %posex = ();
foreach my $cnt ( sort { $a <=> $b } values %uni ) {
   my $pos = $ord{$cnt};
   $posex{$pos}++;
   my $rule = $cat2{$pos};
   if ($rcnt != $cnt) { 
      # print " Elided: $cnt\n"; $rcnt++;
   }
   $rcnt++;

   # Print the in-line comments, if any.
   if ($cnt > 1) { print MRF eu($cmnt{$cnt}); }
   print MRF "<morph-".eu($pos)."> :\n  ".eu($rule).";\n\n";
}

print MRF "%\n% Undefined ..... TODO XXX FIXME\n%\n";
foreach my $tag ( sort keys %alltags ) {
    next if ( defined  $posex{$tag} );
    print MRF "<morph-".eu($tag).">\n";
}
print MRF "   : XXX+;\n";
close(MRF);

system("mkdir ./words") if ( ! -d "./words");

# mkroot mkstem

$rcnt = 0; my %uniq = (); %rev = ();

while (my ($stem, $val) = each(%root)){
   chomp $val;
   $stem = dk($stem);
   $val = dk($val);
   $val =~ s/^://;

   my @nums = split(/:/, $val);
   foreach my $num (@nums){
       $uniq{$num} = 1;
   }
   $rcnt++;
   $rev{$val} .= " " . $stem;
   #print eu("duuude $stem is $val\n");
}
# There are 135775 stems according to this...
# and something like 2997 connectors! WoW! why so many connectors ???
# Worse, the stems are divided into 6955 classes! Ouch

print "gen stem\n";
$un = scalar( keys %uniq );
my $revcnt = scalar( keys %rev );

open(RTS,">stem.dict") or die("cant");
print RTS "%\n% Stems (roots)\n%\n% Total number of stems = $rcnt\n";
print RTS "% Number of stem connectors = $un\n";
print RTS "% Number of stem classes = $revcnt\n";
print RTS "%\n";
print "% Total number of stems = $rcnt\n";
print "% Number of stem connectors = $un\n";
print "% Number of stem classes = $revcnt\n";

# put back in an order where we can sort alphabetically
my %fwd= ();
while (my ($nums, $stems) = each(%rev)){
    #print eu("$stems\t\t$nums\n");
    chomp $stems; $stems =~ s/^\s+|\s+$//g;
    # вист грипп блеф кейф кикс флирт 16:1383
    $fwd{$stems} = $nums;
}

# The actual alphabetical sort

$revcnt = 0; my $filecnt = 1;

foreach my $stems ( sort keys %fwd ) {
    $stems =~ s/^\s+|\s+$//g;
    next if ( $stems eq "" ); # skip nums_for_empty_stem
    next unless defined $fwd{$stems};
    my @words = sort split(/\s+/, $stems);
    my @paradigms = split /:/, $fwd{$stems};
    next unless $#paradigms > -1;
    #print eu("$stems\n\t\t$fwd{$stems}\n");

    my %emptysuflinks = ();
    foreach my $snum ( @paradigms ) {
        if ( defined $rHse->{$snum} ) {
            foreach my $key ( keys %{$rHse->{$snum}} ) {
                next unless defined $posex{$key};
                $emptysuflinks{$key}++;
            }
        }
    }

    # write inline if less than 40 words, else dump to a file
    if ( $#words < 40 ) {
        my $cnt = 0;
        foreach my $w ( @words ) {
             print RTS eu($w).".= ";
             $cnt++;
             if (5 < $cnt) {
                 print RTS "\n";
                 $cnt = 0;
             }
        }
        print RTS ":\n";
    } else {
        print RTS "/ru/words/words.$filecnt:\n";
        open WFILE, ">words/words.$filecnt";
        foreach my $w ( @words ) {
             print WFILE eu($w).".=\n";
        }
        close WFILE;
        $filecnt++;
    }

    my @links = map { "LL".alnum($_)."+" }  @paradigms;
    print RTS "  ".eu(join(" or ", @links)).";\n\n";
    $revcnt ++;

    next unless scalar( keys %emptysuflinks ) > 0;

    # write inline if less than 40 words, else dump to a file
    foreach my $key (keys %emptysuflinks ) {
        my $cnt = 0;
        my $tot = 0;
        foreach my $w ( @words ) {
             my $wkey = $w.".".parse($key);
             next if ( defined $skipwords{$wkey} );
             next if ( defined $skipwords{$w} );
             print RTS eu($wkey)." ";
             $cnt++;
             $tot++;
             if (5 < $cnt) {
                 print RTS "\n";
                 $cnt = 0;
             }
        }
        print RTS ":  ".eu("<morph-$key>").";\n\n" if $tot > 0;
    }
}
# print "% duude revers=$revcnt\n";
close(RTS);

if ( -d "../words" ) {
    system("cp *.dict ../");
    system("rm -f ../words/words.*");
    system("cp words/words.* ../words/");
}


sub parse
{
        my $s = shift;
        #return $s;
        my $res = "";
        $res .= "n" if $s =~ /^С(,|$)/; # существительное noun
        $res .= "a" if $s =~ /^П(,|$)/; # прилагательное adjective
        $res .= "v" if $s =~ /^Г(,|$)/; # глагол verb
        $res .= "e" if $s =~ /^Н(,|$)/; # наречие
        $res .= "p" if $s =~ /^ЧАСТ(,|$)/; # частица
        $res .= "c" if $s =~ /^ЧИСЛ(,|$)/; # числительное
        $res .= "k" if $s =~ /^ЧИСЛ-П(,|$)/;
        $res .= "i" if $s =~ /^СОЮЗ(,|$)/;
        $res .= "j" if $s =~ /^ПРЕДЛ(,|$)/; # предлог
        $res .= "o" if $s =~ /^ПОСЛ(,|$)/;
        $res .= "q" if $s =~ /^МЕЖД(,|$)/; # междометие
        $res .= "x" if $s =~ /^ПРЕДК(,|$)/; # предикатив
        $res .= "y" if $s =~ /^ВВОДН(,|$)/;
        $res .= "f" if $s =~ /^ФРАЗ(,|$)/; # фразеологизм
        $res .= "m" if $s =~ /^МС(,|$)/; # местоимение
        $res .= "w" if $s =~ /^МС-П(,|$)/; # местоимение-предлог
        $res .= "u" if $s =~ /^МС-ПРЕДК(,|$)/; # местоимение-предикатив
        $res .= "n" if $s =~ /,нс(,|$)/; # несовершенное imperfective
        $res .= "s" if $s =~ /,св(,|$)/; # совершенное   perfective
        $res .= "n" if $s =~ /,нп(,|$)/; # непереходный
        $res .= "p" if $s =~ /,пе(,|$)/; # переходный
        $res .= "p" if $s =~ /,прч(,|$)/; # причастие
        $res .= "d" if $s =~ /,дст(,|$)/; # действительный залог active voice
        $res .= "s" if $s =~ /,стр(,|$)/; # страдательный залог  passive voice 
        $res .= "d" if $s =~ /,дпр(,|$)/; # деепричастие
        $res .= "i" if $s =~ /,инф(,|$)/; # инфинитив 
        $res .= "n" if $s =~ /,нст(,|$)/; # настоящее  present
        $res .= "f" if $s =~ /,буд(,|$)/; # будущее    future
        $res .= "p" if $s =~ /,прш(,|$)/; # прошлое    past
        $res .= "v" if $s =~ /,пвл(,|$)/; # повелительное наклонение 
        $res .= "1" if $s =~ /,1л(,|$)/; # 1-ое лицо
        $res .= "2" if $s =~ /,2л(,|$)/; # 2-ое лицо
        $res .= "3" if $s =~ /,3л(,|$)/; # 3-е лицо

        $res .= "a" if $s =~ /,аббр(,|$)/; # аббревиатура
        $res .= "d" if $s =~ /,но(,|$)/; # неодушевленное
        $res .= "l" if $s =~ /,од(,|$)/; # одушевленное 
        $res .= "m" if $s =~ /,мр(,|$)/; # мужской род   masculine gender
        $res .= "f" if $s =~ /,жр(,|$)/; # женский род   feminine
        $res .= "n" if $s =~ /,ср(,|$)/; # средний род   neuter gender
        $res .= "s" if $s =~ /,ед(,|$)/; # единственное  singular
        $res .= "p" if $s =~ /,мн(,|$)/; # множественное plural
        $res .= "i" if $s =~ /,им(,|$)/; # именительный  common case
        $res .= "g" if $s =~ /,рд(,|$)/; # родительный   genitive case
        $res .= "d" if $s =~ /,дт(,|$)/; # дательный     dative case
        $res .= "t" if $s =~ /,тв(,|$)/; # творительный  instrumental case (Ablative case)
        $res .= "v" if $s =~ /,вн(,|$)/; # винительный   objective case (or Accusative case)
        $res .= "p" if $s =~ /,пр(,|$)/; # предложный    prepositional case
        $res .= "s" if $s =~ /,кр(,|$)/; # краткая форма
        $res .= "s" if $s =~ /,сравн(,|$)/; # сравнительная степень
        $res .= "f" if $s =~ /,фам(,|$)/; # фамилия
        $res .= "n" if $s =~ /,имя(,|$)/; # имя 
        return $res;
}

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

