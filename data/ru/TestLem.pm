#!/usr/bin/perl -w
package TestLem;
use strict;
use BerkeleyDB;
use strict;
use locale;
use vars qw( %rule %tail %root %add %cat2 );

BEGIN {
        use Exporter   ();
        our ($VERSION, @ISA, @EXPORT, @EXPORT_OK, %EXPORT_TAGS);
        $VERSION     = 1.00;
        @ISA         = qw(Exporter);
        @EXPORT      = qw(&rules);
        %EXPORT_TAGS = ( );     # eg: TAG => [ qw!name1 name2! ],
        @EXPORT_OK   = qw();
}
our @EXPORT_OK;
END { }
1;



sub get_data
{
	my ($s) = @_;
	my @res = ();
	for(my $i=0; $i <= length $s; $i++)
	{
		my ($left, $right) = (substr($s,0,$i), substr($s,$i));
		if(my $d = $root{$left}) {
			my @num = split /:/, $d;
			shift @num;
			foreach my $num(@num){
				if(my $c = $tail{"$num:$right"})
				{ 
					my @list = 
					unpack "A2" x (length($c)/2), $c;
					foreach my $l(@list) {
						my $rule = $rule{$l};
						if(my $t = $add{"$left:$num"})
						{
							$rule .= ",$t";
						}
						if($rule =~ /мр-жр/)
						{
							$rule =~ s/мр-жр/мр/;
							push @res, $rule;
							$rule =~ s/мр/жр/;
							push @res, $rule;
						} else {
							push @res, $rule;
						}
					}
				}
			}
		}
	}
# проверка винительного одушевленного
	for(my $i=0; $i <= $#res; $i++) {
		if($res[$i] =~ /П(.*мр.*)вн.*/ || $res[$i] =~ /П(.*мн.*)вн.*/) {
			my $rr = $1;
			my $l = 0;
			my $n = 0;
			foreach (@res) {
				$l = 1 if /П$rrрд.*/;
				$n = 1 if /П$rrим.*/;
			}
			$res[$i] .= ",од" if $l;
			$res[$i] .= ",но" if $n;
		}
	}
	my %res = ();
	foreach (@res) {$res{$_} = 1;}
	return keys %res;
}

sub parse
{
	my ($s) = @_;
	my $res = "";
	$res .= "n" if $s =~ /^С(,|$)/;
	$res .= "a" if $s =~ /^П(,|$)/;
	$res .= "v" if $s =~ /^Г(,|$)/;
	$res .= "e" if $s =~ /^Н(,|$)/;
	$res .= "p" if $s =~ /^ЧАСТ(,|$)/;
	$res .= "c" if $s =~ /^ЧИСЛ(,|$)/;
	$res .= "k" if $s =~ /^ЧИСЛ-П(,|$)/;
	$res .= "i" if $s =~ /^СОЮЗ(,|$)/;
	$res .= "j" if $s =~ /^ПРЕДЛ(,|$)/;
	$res .= "o" if $s =~ /^ПОСЛ(,|$)/;
	$res .= "q" if $s =~ /^МЕЖД(,|$)/;
	$res .= "x" if $s =~ /^ПРЕДК(,|$)/;
	$res .= "y" if $s =~ /^ВВОДН(,|$)/;
	$res .= "f" if $s =~ /^ФРАЗ(,|$)/;
	$res .= "m" if $s =~ /^МС(,|$)/;
	$res .= "w" if $s =~ /^МС-П(,|$)/;
	$res .= "u" if $s =~ /^МС-ПРЕДК(,|$)/;
	$res .= "n" if $s =~ /,нс(,|$)/;
	$res .= "s" if $s =~ /,св(,|$)/;
	$res .= "n" if $s =~ /,нп(,|$)/;
	$res .= "p" if $s =~ /,пе(,|$)/;
	$res .= "p" if $s =~ /,прч(,|$)/;
	$res .= "d" if $s =~ /,дст(,|$)/;
	$res .= "s" if $s =~ /,стр(,|$)/;
	$res .= "d" if $s =~ /,дпр(,|$)/;
	$res .= "i" if $s =~ /,инф(,|$)/;
	$res .= "n" if $s =~ /,нст(,|$)/;
	$res .= "f" if $s =~ /,буд(,|$)/;
	$res .= "p" if $s =~ /,прш(,|$)/;
	$res .= "v" if $s =~ /,пвл(,|$)/;
	$res .= "1" if $s =~ /,1л(,|$)/;
	$res .= "2" if $s =~ /,2л(,|$)/;
	$res .= "3" if $s =~ /,3л(,|$)/;
	
	$res .= "a" if $s =~ /,аббр(,|$)/;
	$res .= "d" if $s =~ /,но(,|$)/;
	$res .= "l" if $s =~ /,од(,|$)/;
	$res .= "m" if $s =~ /,мр(,|$)/;
	$res .= "f" if $s =~ /,жр(,|$)/;
	$res .= "n" if $s =~ /,ср(,|$)/;
	$res .= "s" if $s =~ /,ед(,|$)/;
	$res .= "p" if $s =~ /,мн(,|$)/;
	$res .= "i" if $s =~ /,им(,|$)/;
	$res .= "g" if $s =~ /,рд(,|$)/;
	$res .= "d" if $s =~ /,дт(,|$)/;
	$res .= "t" if $s =~ /,тв(,|$)/;
	$res .= "v" if $s =~ /,вн(,|$)/;
	$res .= "p" if $s =~ /,пр(,|$)/;
	$res .= "s" if $s =~ /,кр(,|$)/;
	$res .= "s" if $s =~ /,сравн(,|$)/;
	$res .= "f" if $s =~ /,фам(,|$)/;
	$res .= "n" if $s =~ /,имя(,|$)/;
	return $res;
}


sub TestLem {
	(my $lexer) = @_;


#my %cat2 = ();

open FILE, "4.0.morph";
while (<FILE>) {
	next unless /^\%/;
	next unless /morph/;
	my $str = $_;
	chomp $str;
#% morph{"ПРЕДК,нст"} ="<глагол-пе-пр-sub> &  (<глагол-pr> or <макро-глагол-pr>)";
	(my $nr, my $rl) = split(/\=/, $str);
	$nr =~ s/^\%\ +morph\{\"//; $nr =~ s/\"\}\ *$//;
	$rl =~ s/^\ ?\"//; $rl =~ s/\"\;\ ?$//;
	$cat2{$nr} = $rl;
}
close FILE;



tie %rule, "BerkeleyDB::Hash",
	-Filename => "rule.db" or die "Error blin ";
tie %tail, "BerkeleyDB::Hash",
	-Filename => "tail.db" or die "Error blin ";
tie %root, "BerkeleyDB::Hash",
	-Filename => "root.db" or die "Error blin ";
tie %add, "BerkeleyDB::Hash",
	-Filename => "add.db" or die "Error blin ";


my %words = ();

my @rows = split(/\n/, $lexer);

foreach my $s1 (@rows) {
#	my $s = lc $s1;
	my $s = $s1;
	chomp $s;
	$words{$s} = 1;
}

my @nonwords = qw(не как где и если когда два две оба обе три четыре);

foreach (@nonwords) {
	delete $words{$_};
}

my $out = "";

foreach my $s(sort keys %words) {
#	my @r = get_data(lc $s);
	my @r = get_data($s);
	foreach my $i(@r) {
		if (rules($i)) {
			$out.="$s." . parse($i) . ": " . rules($i) . "; % $i\n";
		} else {
			system(" echo \"rules $i not defined $s\" >> /tmp/testlem.log");
		}
	}
}

untie %rule;
untie %tail;
untie %root;
untie %add;

return $out;

}


sub get_tokens {
	my $str = shift;
	my %hash = ();
	        $str =~ s/>([^<]*?)</></g;
                $str =~ s/>([^>]*)$//;
                $str =~ s/^(.*?)<//;
	my @tokens = split(/></,$str);
	foreach (@tokens) {
		$hash{$_}++;
	}
	my @tokens2 = keys %hash;
	return @tokens2;
}

sub rules($) {
        my ($cur) = @_;
        $_ = $cur;
        chomp;
	s/имя,//;
	s/фам,//;
        return $cat2{$_};
}
