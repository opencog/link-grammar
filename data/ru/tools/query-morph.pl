#!/usr/bin/perl -w

use strict;
use warnings;
use Encode;
use utf8;
use BerkeleyDB;
use vars qw( %rule %tail %root );

tie %rule, "BerkeleyDB::Hash", -Filename => "rule.db", -Flags => DB_RDONLY or die "cant";
tie %tail, "BerkeleyDB::Hash", -Filename => "tail.db", -Flags => DB_RDONLY or die "cant";
tie %root, "BerkeleyDB::Hash", -Filename => "root.db", -Flags => DB_RDONLY or die "cant";

my $cmda = eu("./query-morph.pl тест");

my $s = du($ARGV[0]) or die("need argument, word surface:\n$cmda\n");
my @res = ();
for(my $i=0; $i <= length($s); $i++) {
    my ($left, $right) = (substr($s,0,$i), substr($s,$i));
    if ( my $d = dk($root{ek($left)}) ) {
            my $rightspace = join("", map { " " } split(//,$right));
            print eu("$i \"$left.$rightspace\"\t$d\n");
            my @paradigm_list = split /:/, $d;
            shift @paradigm_list;
            foreach my $num (@paradigm_list) {
                if ( my $c = dk($tail{ek("$num:$right")}) ) {
                    print eu("\t\"$num:$right\"\t$c\n");
                    my @grammems_list = unpack "A2" x (length($c)/2), $c;
                    foreach my $l ( @grammems_list ) {
                        print eu("\t$l\t");
                        my $rule = dk($rule{ek($l)});
                        print eu("\t$rule\n");
                        if ( $rule =~ /мр-жр/ ) {
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
untie %rule; untie %tail; untie %root;

print "\nres:\n\t\t", eu(join("\n\t\t", @res)), "\n\n";

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


