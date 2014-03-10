#!/usr/bin/perl
# Written by Jon Dehdari 2004-2006
# Perl 5.8
# Arabic Morphological Analyzer CGI
# The license is the GPL v. 2 (See http://www.fsf.org for details)

use strict;
use utf8;
#use diagnostics;
#binmode(STDOUT, ":utf8");
#use LWP::Simple qw(!head);
use CGI qw(:standard); #must use this full line
$CGI::POST_MAX=50000;
my $query = new CGI;

my $input_type      = param ("input_type");
my $output_type     = param ("output_type");
my $use_web_page    = param ("use_web_page");
my $web_page        = param ("web_page");
my $use_file        = param ("use_file");
my $uploaded_file   = param ("uploaded_file");
my $text_from       = param ("text_from");
#my $input_type     = "1";
#my $use_file       = "false";
#my $uploaded_file  = "false";
#my $use_web_page   = "false";
#my $web_page       = "false";
my $text_from_new;
my @charx;
my $charx;
my $input_rtl;

unless ($input_type eq 'roman') { $input_rtl = "true"; }


if ($input_type eq "utf8" || $output_type eq "utf8") { print $query->header( -charset => 'UTF-8'); }
else {  print $query->header( -charset => 'windows-1256'); }


print( 
	'<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//AR">',
	"\n",
	'<html lang="ar">',
	"\n",
	"\n",
#	"<style>\n",
#	"    body \{ text-align\:right \}\n",
#	"<\/style>\n",
	"<head>\n",
);
if ($input_type eq "utf8" || $output_type eq "utf8") { print '<meta http-equiv="Content-Type" content="text/html; charset=utf-8">'; }
if ($input_type eq "cp1256") { print '<meta http-equiv="Content-Type" content="text/html; charset=windows-1256">'; }
print(
        "\n",
	"<title>Buckwalter Arabic Morphological Analyzer<\/title>\n",
	"<\/head>\n",
	"<body>\n",
);

# For getting web page stuff
if ($use_web_page eq "--noroman") {
$text_from = "";  # Clears out residue from the web form
#$text_from = get "$web_page" or warn $!;
#print("$text_from");
}


elsif ($use_file eq "true") {
$text_from = "";  # Clears out residue from the web form
while (my $line = <$uploaded_file> ) {
$text_from = $text_from . $line ;
}
#if (length $text_from > 40000 ) {
#  die "Your uploaded file is too big.<br/></td></tr></table></html>\n";
#}
}

if ($use_web_page ne "--noroman" && $use_file ne "true") {
my $formated_text_from = "$text_from";
$formated_text_from =~ s/\n/<br\/>\n/g; #changes newline to <br>for from text
#print("$formated_text_from");
}

$text_from =~ s/(?<!\&#\w{4})[;]/ _$1_ /g; #Preserves punctuation for semicolon except for unicode decimal &#....;
$text_from =~ s/([.,?!])/ _$1_ /g; #Preserves punctuation


chomp $text_from;
my $word = $text_from;
#print "text_from: $text_from<br>";

print "\n";
#print '<div style="font-family: Monospace">';
print "\n<pre>\n";
$word =~ s/</&lt;/g;  # even <pre> doesn't like "<"
print "$word<br/>\n";
$word =~ s/&lt;/</g;  # change "&lt;" back to "<"
#$word =~ s/\n/<br\/>\n/g;
$word =~ s/"/\\"/g;
$word =~ s/\brm\b|\bmv\b|passwd|shadow|etc|var|cat//g;  # some security measures

my $out = `echo -e "$word" | ./aramorph_fast.pl -i $input_type 2>/dev/null  `;

$out =~ s/</&lt;/g;
print $out;
print "\n</pre>\n<br/>\n";
#print '</div>';


print <<EOF;
<hr align="left" width="620" /><br/><br/>
<form action="aramorph_fast.cgi" method="post" enctype="multipart/form-data">
<acronym title="Note: Copying and Pasting often changes the encoding type.">Encoding</acronym>:  
<select name="input_type">
    <option value="dt">Dil's Transliteration</option>
    <option value="roman">Buckwalter Transliteration</option>
    <option value="utf8" selected="selected">UTF-8</option>
<!--	<option value="unidec">Unicode Decimal</option> -->
    <option value="cp1256">Windows 1256</option>
<!--	<option value="uniname">Unicode Name</option> -->
<!--	<option value="iso88596">ISO 8859-6</option> -->
<!--	<option value="arabtex">ArabTeX</option> -->
</select> 

<!-- <input name="use_web_page" type="checkbox" value="true"> Use Web Page: <input name="web_page" type="text" maxlength="511" size="41" value="http://"> --> <br/><br/>
 <input name="use_file" type="checkbox" value="true"> Upload Text File: <input name="uploaded_file" type="file" size="30"> 
<br/><br/>
Text:<br/>
<textarea name="text_from" maxlength="800"  wrap="virtual" rows="05" cols="60"></textarea>
<br/><br/>
<input type="submit" value="Submit"><br/><br/>
</form>
EOF
;


#} # ends foreach (@word)

print( "\n<br\/>\n<\/body>\n<\/html>\n");
