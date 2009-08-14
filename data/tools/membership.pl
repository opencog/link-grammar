#! /usr/bin/env perl
#
# membership.pl
#
# Print cluster membership contents for Siva's clusters.
# Siva's cluster seems to contain just 13506 word forms ... 
#

$| = 1;

$fileprev = "";
$cnt = 0;
while(<>)
{
	chop;

	if (/^cluster/)
	{
		print "$cnt\n";
		print "$_\n";
		$cnt = 0;
		next;
	}

	# XXXXXXXXXXXXXXXXXXXXXXXx Note: the grep here is buggy,
	# since it will return postive hits for e.g. parcel.n 
	# vs. "parceling" rather than the intended grep where the period is
	# escaped.  So use with caution!
	if (/^	(.*)$/)
	{
		$wrd = $1;
		$cmd = "grep \" " . $wrd . "\" ../blah/*";
		$out = qx/$cmd/;
		chop $out;
		if ($out =~/^\s*$/)
		{
			$cmd = "grep \"^" . $wrd . "\" ../blah/*";
			$out = qx/$cmd/;
			chop $out;
		}
		# print "$wrd in $out\n";
		($fn, $rest) = split /:/, $out;

		$cnt ++;
		if ($fileprev ne $fn)
		{
			print "\t$wrd -- $fn\n";
		}
		$fileprev = $fn;
	}
}
