#!/usr/bin/perl
# Written by Jon Dehdari 2004-2005
# Perl 5.8
# Arabic LG Syntax Parser
# The license is the GPL (www.fsf.org)

use strict;
use utf8;
#use diagnostics;
#binmode(STDOUT, ":utf8");
#use LWP::Simple qw(!head);
use CGI qw(:standard); #must use this full line, not just CGI
$CGI::POST_MAX=50000;
my $query = new CGI;

my $input_type      = param ("input_type");
my $output_type     = param ("output_type");
my $width           = param ("width");
my $root_only       = param ("root_only");
my $preserve_links  = param ("preserve_links");
my $use_web_page    = param ("use_web_page");
my $web_page        = param ("web_page");
my $use_file        = param ("use_file");
my $uploaded_file   = param ("uploaded_file");
my $text_from       = param ("text_from");
#my $input_type     = "1";
#my $text_from      = "\u{d986}\u{d8a7}\u{d986}\u{d987}\u{d8a7}";
#my $text_from      = "\u{0646}\u{0627}\u{0646}\u{0647}\u{0627}";
#my $text_from      = "&#1606;&#1575;&#1606;&#1607;&#1575;";
#my $text_from      = "قدومي بفارغ الصبر";
#my $preserve_links = 0;
#my $use_file       = "false";
#my $uploaded_file  = "false";
#my $use_web_page   = "false";
#my $web_page       = "false";
#my $remove_stops   = "false";
my $text_from_new;
my @charx;
my $charx;
my $input_rtl;

if ($input_type =~ /[^0]/) { $input_rtl = "true"; }


if ($input_type eq "utf8" || $output_type eq "utf8") { print $query->header( -charset => 'UTF-8'); }
else {  print $query->header( -charset => 'windows-1256'); }


print( 
	'<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//FA">',
	"\n",
	'<html lang="en">',
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
	"<title>Arabic LG Syntax Parser<\/title>\n",
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

#my $lgout =  `echo -e '!width=$width\n$word\n' | ./aramorph_fast.pl -i $input_type  | /usr/bin/lgparser /home/jon/public_html/arabiclg/4.0.dict 2>/dev/null `;
my $lgout = `echo -e "$word" | ./aramorph_fast.pl -i $input_type 2>/dev/null | ./buck2lg.sh | (echo -e '!width=$width\n' ; cat) | /usr/bin/link-grammar ../4.0.dict 2>/dev/null | egrep -v 'Opening|RETURN|I can|\+\+Time|width set to' `;

#$lgout =~ s/I can't interpret.*\n//g;
$lgout =~ s/linkparser\>//g;
$lgout =~ s/</&lt;/g;
print $lgout;
print "\n</pre>\n<br/>\n";
#print '</div>';


print <<EOF;
<hr align="left" width="620" /><br/><br/>
<form action="arabiclg.cgi" method="post" enctype="multipart/form-data">
<acronym title="Note: Copying and Pasting often changes the encoding type.">Encoding</acronym>:  
<select name="input_type">
	<option value="roman">Romanized Buckwalter</option>
	<option value="utf8" selected="selected">UTF-8</option>
<!--	<option value="0">Unicode Decimal</option> -->
	<option value="cp1256">Windows 1256</option>
<!--	<option value="7">ISO 8859-6</option> -->
<!--	<option value="8">ArabTeX</option> -->
</select> 
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Width: 
<input type="text" name="width" maxlength="3" value="150" >
<!-- <input name="use_web_page" type="checkbox" value="true"> Use Web Page: <input name="web_page" type="text" maxlength="511" size="41" value="http://"><br/><br/> -->
<!-- <input name="use_file" type="checkbox" value="true"> Upload Text File: <input name="uploaded_file" type="file" size="29"> -->
<br/><br/>
Text:<br/>
<textarea name="text_from" maxlength="200"  wrap="virtual" rows="03" cols="60"></textarea>
<br/><br/>
<input type="submit" value="Submit"><br/><br/>
</form>
EOF
;


#} # ends foreach (@word)

print( "\n<br\/>\n<\/body>\n<\/html>\n");
