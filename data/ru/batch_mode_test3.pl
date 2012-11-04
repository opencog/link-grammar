#!/usr/bin/perl

use lib 'LP/blib/lib';
use lib 'LP/blib/arch';

use Lingua::LinkParser;
use TestLem;
use strict;

use locale;

my $input_string = " По невыясненным обстоятельствам , Мурманский филиал Почты России не выполнил условия данного договора , в связи с чем почтовая корреспонденция в адрес ООО КНТ в году не поступала . \n";
#my $input_string;

my $cntok = 0; my $cntnotok = 0; my $pnotok = 0;
my $width = 260; my $max_parse_time = 60;
print "Ширина экрана: $width\n";
print "Максимальное время разбора: $max_parse_time\n";

while (<>) {
	$input_string = $_;
	chomp $input_string;
	next if ($input_string =~ /^[\s\ ]+$/);
	next if ($input_string =~ /^$/);
	print "\n Обрабатываем:[$input_string]\n";

        my $snt = $input_string;
         $snt =~ s/\n/ /g; $snt =~ s/\r/ /g;
         $snt =~ s/\d+//g;
         $snt =~ s/\.\ *$/\ \./;
         $snt =~ s/\?\ *$/\ \?/;
         $snt =~ s/\!\ *$/\ \!/;
	 $snt =~ s/\:\ *$/\ \:/;
         $snt =~ s/\,\ /\ \,\ /g;
	 $snt =~ s/\)\ /\ \)\ /g;
	 $snt =~ s/\ \(/\ \(\ /g;
	 $snt =~ s/\:\ /\ \:\ /g;
	 $snt =~ s/\"\ /\ \"\ /g;
	 $snt =~ s/\ \"/\ \"\ /g;
        $input_string = $snt;
	print "Предобработка:[$input_string]\n";


	my $sublink = ""; my $part_f = ""; my $sublink2 = "";

	my $sent_ok = 1;

	next if ($input_string =~ /^\;/);
	if ($input_string =~ /^\*/ ) {
		$input_string =~ s/^\*//;
		$sent_ok = 0;
	}
	next if ($input_string =~ /^[\s\ ]+$/);
	$input_string =~ s/\?\ *$/ !/;
	next if ($input_string eq "");

	gen_dict($input_string, "rus.dict");
	my $parser = new Lingua::LinkParser(DictFile => 'rus.dict', KnowFile => '', ConstFile => '', AffixFile => '');

	$parser->opts('disjunct_cost' => 2, 'min_null_count' => 0, 'max_null_count' => 30, 'max_parse_time' => $max_parse_time, 'screen_width' => $width);
	my $sent = $parser->create_sentence($input_string);
#	print "$sent \n";
	my $num_linkages = $sent->num_linkages;
	if ($num_linkages>0) {
		my $linkage = $sent->linkage(1);
                # print $linkage->num_words, "\n";
                my @sublinkages = $linkage->sublinkages;
		$sublink2 = @sublinkages[0];
		$sublink2 =~ s/\ +/ /g;
		print $parser->get_diagram($linkage), "\n";
		print @sublinkages[0], "\n"; 
	} else {
		print "За $max_parse_time секунд разбора не обнаружено: [$input_string]\n";
	}

	#sleep 1;
}


sub gen_dict {
	my $input_string = shift;
	my $dict = shift;

	my $lexer = lexer($input_string);
	my $words = TestLem::TestLem($lexer);

	generate_dict($dict, $words);
}


sub generate_dict {
	my $dict = shift;
	my $words = shift;

unlink($dict);
open FILE, "base.dict";
open NEWFILE, ">$dict";
while (<FILE>) {
	next if (/^%\ +morph/);
        print NEWFILE;
}
print NEWFILE $words;
close NEWFILE;
close FILE;

}


sub lexer {
        my $snt = shift;
        $snt =~ s/\s+/ /g;
        my @s = split(/([ ,.:"?!])/, $snt);
        my $out = "";
	$s[0] = lc $s[0];
        foreach my $s (@s)
        {
                $s && $s ne " " && ($out .= $s . "\n");
        }
	return $out;
}

