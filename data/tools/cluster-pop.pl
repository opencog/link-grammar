#! /usr/bin/env perl
#
# cluster-pop.pl
#
# Populate the cluster table from Siva's clusters.
# Siva's clusters seems to contain just 13506 word forms ... 
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
	if (/^	(.*)$/)
	{
		$wrd = $1;
print "duude $wrd";
	}
}
