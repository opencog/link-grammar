#! /usr/bin/env perl
#
# cluster-pop.pl
#
# Populate the cluster table from Siva's clusters.
# Siva's clusters seems to contain just 13506 word forms ... 
#
# Usage: cat clusters_weka-trimmed.txt | ./cluster-pop.pl
#

use DBI;

$| = 1;

my $dbh = DBI->connect('DBI:SQLite:dbname=clusters.db', 'linas', 'asdf')
   or die "Couldn't connect to database: " . DBI->errstr;

my $insert = $dbh->prepare(
	'INSERT INTO ClusterMembers (cluster_name, inflected_word) VALUES (?,?)');

my $cluster_name = "";
my $inflected_word = "";

my $clust_cnt = 0;
my $word_cnt = 0;

while(<>)
{
	chop;

	if (/^cluster/)
	{
		$cluster_name = $_;
		print "$cluster_name\n";
		$clust_cnt ++;
		next;
	}
	if (/^	(.*)$/)
	{
		$inflected_word = $1;
		print "\t$inflected_word\n";
		$insert->execute($cluster_name, $inflected_word)
			or die "Couldn't execute statement: " . $insert->errstr . 
			"\nword was " . $inflected_word;

		$word_cnt ++;
	}
}

print "Added $word_cnt words to $clust_cnt clusters\n";
