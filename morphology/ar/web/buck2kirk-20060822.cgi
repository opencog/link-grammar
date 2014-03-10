#!/usr/bin/perl 
# By Jon Dehdari, 2006.  Originally for ArabicLG, adapted for Shereen's thesis
# This script started out as a small program that converts aramorph.pl output to 
# something that Arabic teachers will use.  It has morphed into something 
# very different.  That's my excuse that it's not pretty code, and I'm 
# sticking to it!

use strict;
use CGI qw(:standard);
$CGI::POST_MAX=400000;
my $cgi = new CGI;
my %input;
my $counter = 0;
my ($newword, $romanized_word, $gloss, $nonhtml, $out);

my $appended_file   = $cgi->param ("appended_file");
my $preparsed_file   = $cgi->param ("preparsed_file");
for my $key ( $cgi->param() ) {
    $input{$key} = $cgi->param($key);
}

if ($input{'save2file'} || $input{'append2file'} || $input{'save2xls'}) { $nonhtml = 1; }

unless ($nonhtml) {
 print qq{Content-type: text/html; charset=windows-1256\n
 <!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//AR">
 <html lang="ar">
 <head><title>Arabic Text Analysis</title>
 <meta http-equiv="Content-Type" content="text/html; charset=windows-1256">
 </head>\n<body style="background: url(arabesque1.png) repeat-x"><br/>\n
 <center><big><big>Arabic Text Analysis</big></big></center><br/><br/>\n
 <form action="buck2kirk.cgi" method="post" enctype="multipart/form-data">
 };
 $| = 1; # flushes out buffer
 #<style>
 #   body \{ text-align\:right \}
 #   textarea \{ text-align\:right \}
 #<\/style>
}

### prints all the cgi input, for development
#for my $key ( keys %input ) {
# print $key, ' = ', $input{$key}, "<br>\n";
#}

$input{text_from} =~ s/\b(rm|mv|ln|passwd|shadow)\b//g;  # scrub text input for security

### I wanted to only call on aramorph_fast.pl once, since it's really slow to startup.  As a result, I give all the user input lines to the morpho engine just once, then split it, then foreach it.

my $scriptout;
if ($input{text_from} || $preparsed_file) { # User has already input text into box or uploaded preparsed file
  if ($preparsed_file) { 
      while (my $line = <$preparsed_file> ) {
	  $scriptout .= $line ;
#	  print "preparsed_file: " . <$preparsed_file> . " <br/>\n";
      }
  }
  else {
    $scriptout = `echo -e '$input{text_from}' | nice ./aramorph_fast.pl -i $input{input_type} 2>/dev/null  `;
  }


my @scriptout = split /\n/, $scriptout;

foreach $_ (@scriptout) {
  my ($romanized_lemma, $romanized_full, $pos_tag, $ar_word, $unknown_loop);

  if    (m/SOLUTION \d+:/) { 
    if  (m/\* SOLUTION \d+:/) { # if starred word, erase checked status of first solution and cause checking of current solution by faking new word status
      $out =~ s/ checked //g;
      $newword = 1;
    }
      s/.*\[(.*?)\] (.*)/$1/g; 
      $romanized_lemma = $1; 
      $pos_tag = $2; 
      $romanized_full = $2;
      $romanized_full =~ s/([^+ \/]+)\/[^+ ]+/$1/g; # removes part-of-speech info
      $romanized_full =~ tr/o//d;	# remove sukuns
  }
  elsif (m/NEWLINE/)     {}  # Deal with this stuff later
  elsif (s/^ *\(GLOSS\):[+ ]+(.+?)[+ ]*$/$1/) {
      $gloss = $1;
      $gloss =~ s/(.*)<verb>$/$1/g;  # removes final <verb>
      $out =~ s/(.*):: "/$1::${gloss}"/;
      $out .= " ${gloss} ";

#      print $out;
#      ($out, $gloss) = undef;
#      $| = 1; # flushes out buffer
  }
  elsif (m/Comment:\s+(\S+?)\s+NOT FOUND/)  {
      my $templine = $1;
      $romanized_lemma = undef; 
      $romanized_full = undef;
      $counter++;

       if ($input{output_type} eq "dt" ) { # DT
        if ($templine =~ m/\d+/) { # Is a number
         $out .= qq"<br/>\n
         <input type=\"radio\" name=\"${counter}_${romanized_word}\" value=\"${romanized_word} :: ${romanized_word}/NUMBER :: \" checked >" . buck2dt($romanized_word) . " - Number <br/>
         <input type=\"radio\" name=\"${counter}_${romanized_word}\" value=\"${romanized_word} :: ${romanized_word}/UNKNOWN :: \" >" . buck2dt($romanized_word) . " - Other <br/>
	 ";
	}
	else { # Is not a number
         $out .= qq"<br/>\n
         <input type=\"radio\" name=\"${counter}_${romanized_word}\" value=\"${romanized_word} :: ${romanized_word}/NOUN_PROP :: \" checked >" . buck2dt($romanized_word) . " - Proper Noun <br/>
         <input type=\"radio\" name=\"${counter}_${romanized_word}\" value=\"${romanized_word} :: ${romanized_word}/NOUN :: \" >" . buck2dt($romanized_word) . " - Noun <br/>
         <input type=\"radio\" name=\"${counter}_${romanized_word}\" value=\"${romanized_word} :: ${romanized_word}/ADJ :: \" >" . buck2dt($romanized_word) . " - Adjective <br/>
         <input type=\"radio\" name=\"${counter}_${romanized_word}\" value=\"${romanized_word} :: ${romanized_word}/VERB_PERFECT :: \" >" . buck2dt($romanized_word) . " - Perfect Verb <br/>
         <input type=\"radio\" name=\"${counter}_${romanized_word}\" value=\"${romanized_word} :: ${romanized_word}/VERB_IMPERFECT :: \" >" . buck2dt($romanized_word) . " - Imperfect Verb <br/>
         <input type=\"radio\" name=\"${counter}_${romanized_word}\" value=\"${romanized_word} :: ${romanized_word}/UNKNOWN :: \" >" . buck2dt($romanized_word) . " - Other <br/>
         ";
        }
       }
       else { # Buckwalter transliteration
        if ($templine =~ m/\d+/) { # Is a number
         $out .= qq{<br/>\n
         <input type=\"radio\" name=\"${counter}_${romanized_word}\" value=\"${romanized_word} :: ${romanized_word}/NUMBER :: \" checked >$romanized_word - Number <br/>
         <input type=\"radio\" name=\"${counter}_${romanized_word}\" value=\"${romanized_word} :: ${romanized_word}/UNKNOWN :: \" >$romanized_word - Other <br/>
	 };
	}
	else { # Is not a number
         $out .= qq{<br/>\n
         <input type=\"radio\" name=\"${counter}_${romanized_word}\" value=\"${romanized_word} :: ${romanized_word}/NOUN_PROP :: \" checked >$romanized_word - Proper Noun <br/>
         <input type=\"radio\" name=\"${counter}_${romanized_word}\" value=\"${romanized_word} :: ${romanized_word}/NOUN :: \" >$romanized_word - Noun <br/>
         <input type=\"radio\" name=\"${counter}_${romanized_word}\" value=\"${romanized_word} :: ${romanized_word}/ADJ :: \" >$romanized_word - Adjective <br/>
         <input type=\"radio\" name=\"${counter}_${romanized_word}\" value=\"${romanized_word} :: ${romanized_word}/VERB_PERFECT :: \" >$romanized_word - Perfect Verb <br/>
         <input type=\"radio\" name=\"${counter}_${romanized_word}\" value=\"${romanized_word} :: ${romanized_word}/VERB_IMPERFECT :: \" >$romanized_word - Imperfect Verb <br/>
         <input type=\"radio\" name=\"${counter}_${romanized_word}\" value=\"${romanized_word} :: ${romanized_word}/UNKNOWN :: \" >$romanized_word - Other <br/>
         };
        }
       }

  }   
  elsif (s/^INPUT STRING: (.*)/<br\/>\n$1 &nbsp; /) { 
      print $out;
      ($out, $gloss) = undef;
      $| = 1; # flushes out buffer

      $ar_word = $1;
      $out .= "<br/><br/>\n$ar_word";
      $newword = 1;
  }
  elsif (s/^LOOK-UP WORD: (\S*)//) { $romanized_word = $1; }
  else                   { s/.*//g }

  my $romanized_lemma_html = $romanized_lemma;	# protects "<" in html
  $romanized_lemma_html =~  s/_(\d+)//g;	# eg. word_1 --> word

  s/\(null\)//g; 
  tr/{/</; 
  tr/\n/ /;
  s/\s+/ /g;			# removes excessive spaces
  #s/[aiuo~]//g;		# devocalize
  if (m/[a-zA-Z]/) {		# some newline trickery
      if (m/NEWLINE/) {
	  $out .= "<hr/><br/>\n";
      }
      elsif ($romanized_lemma) {
          y/o//d;	# remove sukuns
	  if ($newword) { # auto select first radio button per word
	      $counter++;
	      ### I'm holding off on compacting the following code until the dust settles a little bit
	      if ($input{output_type} eq "dt" ) {
		  $out .= "<br/>\n  <input type=\"radio\" name=\"${counter}_${romanized_word}\" value=\"${romanized_lemma} :: ${pos_tag} :: \" checked >" . buck2dt($romanized_lemma) . " &nbsp;&nbsp; " . buck2dt($romanized_full) . " &nbsp;&nbsp;&nbsp;";
	      }
	      else {
		$romanized_lemma_html =~ s/<(?!br)/&lt;/g;	# protects "<" in html
		$romanized_full =~ s/<(?!br)/&lt;/g;	# protects "<" in html
		$out .= "<br/>\n  <input type=\"radio\" name=\"${counter}_${romanized_word}\" value=\"${romanized_lemma} :: ${pos_tag} :: \" checked >$romanized_lemma_html &nbsp;&nbsp; $romanized_full &nbsp;&nbsp;&nbsp;";
	      }
	      $newword = 0;
	  }
	  else { # non-first parse
	      if ($input{output_type} eq "dt" ) {
		    $out .= "<br/>\n  <input type=\"radio\" name=\"${counter}_${romanized_word}\" value=\"${romanized_lemma} :: ${pos_tag} :: \">" . buck2dt($romanized_lemma) . " &nbsp;&nbsp; " . buck2dt($romanized_full) . " &nbsp;&nbsp;&nbsp;";
	      }
	      else {
                    $romanized_lemma_html =~ s/<(?!br)/&lt;/g;	# protects "<" in html
		    $romanized_full =~ s/<(?!br)/&lt;/g;	# protects "<" in html
		    $out .= "<br/>\n  <input type=\"radio\" name=\"${counter}_${romanized_word}\" value=\"${romanized_lemma} :: ${pos_tag} :: \">$romanized_lemma_html &nbsp;&nbsp; $romanized_full &nbsp;&nbsp;&nbsp;";
              }
	  }
      }
  }
} # foreach @out
print <<EOF
<hr/><br/>
 <input type="hidden" name="words_checked" value="true">
<br/>
 <input name="save2file" type="checkbox" value="true"> Save to New <b>Text</b> File <br/><br/>
 <input name="save2xls" type="checkbox" value="true"> Save to New <b>Excel</b> File <br/><br/>
 <input name="append2file" type="checkbox" value="true"> <b>Append</b> Existing Text File: <input name="appended_file" type="file" size="58"><br/><br/>
EOF
} # if $input{text_from}

elsif ($input{words_checked}) { # User has already checked radio buttons of word senses
 undef $input{words_checked};
 ### prints all the cgi input, for development
 #print "<pre>";
 if ($nonhtml ) {
  my $outfile;
 if ($input{append2file}) {
#  open (APPENDFILE, ">>$input{appended_file}") || die "cannot open: $!";
#  while (my $line = <$input{appended_file}> or die "cannot open file") {
#  while (my $line = <$appended_file> or die "cannot open file") {
  while (my $line = <APPENDEDFILE>) {
#   print "line:$line\n";
   $outfile .= $line;
  }
 }
  for my $key ( sort { $a <=> $b } keys %input ) {
#   if ($input{$key}) { print APPENDFILE "$key\t$input{$key}\n"; }
   if ($input{$key} && $key =~ /^\d+_/) {
    $outfile .=  "${key} :: ${input{$key}}\n";
   }
  }
#  print "\nSaved<br/>\n";
  if ($input{'save2xls'}) {print "Content-type: application/vnd.ms-excel;\n\n";
   undef $input{'save2xls'};
   $outfile =~ s/\b(\d+)_/$1\t/g;
   $outfile =~ s/::/\t/g;
   print $outfile;
  }
  else {print "Content-type: text/plain;\n\n";
   print <$appended_file> if $input{'append2file'};#temp
   print "\n$outfile\n" ; 
  }
#  open APPENDEDFILE, "/tmp/$appended_file";
#  close APPENDEDFILE;
#  close APPENDFILE;

}
 else {
  for my $key ( sort { $a <=> $b } keys %input ) {
   if ($input{$key}) { print $key, ' = ', $input{$key}, "<br>\n"; }
  }
 }
 #print "</pre>";
}


else { # No input from user, yet
 print <<EOF;
Input Format: &nbsp;
<select name="input_type">
	<option value="dt">Dil Transliteration</option>
	<option value="roman">Buckwalter Transliteration</option>
	<option value="utf8">UTF-8</option>
<!--	<option value="0">Unicode Decimal</option> -->
	<option value="cp1256" selected="selected">Windows 1256</option>
<!--	<option value="7">ISO 8859-6</option> -->
<!--	<option value="8">ArabTeX</option> -->
</select><br/><br/>
Output Format: &nbsp;
<select name="output_type">
	<option value="dt" selected="selected">Dil Transliteration</option>
	<option value="roman">Buckwalter Transliteration</option>
<!--	<option value="utf8">UTF-8</option> -->
<!--	<option value="0">Unicode Decimal</option> -->
<!--	<option value="cp1256">Windows 1256</option> -->
<!--	<option value="7">ISO 8859-6</option> -->
<!--	<option value="8">ArabTeX</option> -->
</select><br/><br/>
Upload preparsed file: &nbsp; <input name="preparsed_file" type="file" size="29">
<br/><br/>
<textarea style="text-align:right" name="text_from" rows="9" cols="70"></textarea>
<br/><br/>
EOF
}

unless ($nonhtml) {
print '<input type="submit" value="Submit" />'."\n";
print "</body>\n</html>\n";
}

# ==============================================================

sub buck2dt {
   $_ = shift;

   tr/\'\|\>\&\<\}AbptvjHxd\*rzs\$SDTZEg_fqklmnhwYyFNKaui\~o\`\{/CMLWEYAbQtVjHxdvrzspSDTZcg_fqklmnhweyNUIaui\~o\`A/; # Buckwalter translit to DT

   return $_;

}

# ==============================================================

sub unknown_guess {
   $_ = shift;

#   tr/\'\|\>\&\<\}AbptvjHxd\*rzs\$SDTZEg_fqklmnhwYyFNKaui\~o\`\{/CMLWEYAbQtVjHxdvrzspSDTZcg_fqklmnhweyNUIaui\~o\`A/; # Buckwalter translit to DT

   return $_;

}

# ==============================================================
