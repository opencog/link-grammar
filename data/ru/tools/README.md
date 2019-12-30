
Regenerating the Russian Dictionaries
-------------------------------------
     May 2014


The raw data is located in the file morphs20030410.mrd.  This is the
master morphology file from 2003. The corresponding grammatical
markup is in rgramtab20030306.tab.  The current Russian dicts are built
from these two files.

It is to be converted to Berkley DB files by running:
```
  time ./make-hash-db.pl
```
This will create the files `rule.db`, `root.db` and `tail.db`.  It should
take about 45 seconds or less to run.  These are then used to create
the link-grammar dictionaries:
```
  time ./mkdict.pl
```
This should take about 25 seconds or less.  After this, the Russian
dictionaries are ready to be used!

The `query-morph.pl` tool will examine the morphology candidates for a
given word. For example:
```
  ./query-morph.pl тест
```

Morphology sources
------------------
The morphology is nominally a part of http://www.aot.ru  It comes from
the "seman" project:
```
  http://seman.sourceforge.net/
```
Source data for the morphology is available under the LGPL license at:
```
  http://sourceforge.net/p/seman/svn/HEAD/tree/trunk/
```
A read-only version can be obtained as:
```
  svn checkout svn://svn.code.sf.net/p/seman/svn/trunk seman-svn
```
So, for example, `rgramtab.tab` is in `trunk/Dicts/Morph/` and the
`morphs.mrd` is in `trunk/Dicts/SrcMorph/RusSrc/morphs.mrd`.  These files
are encoded in KOI8-R.  The iconv command can be used to convert
KOI8-R to UTF, as follows:
```
   iconv -f KOI8-R -t UTF-8  <infile>  >   <outfile>
```
So:
```
   iconv -f KOI8-R -t UTF-8 morphs.mrd > morphs-utf8.mrd
   iconv -f KOI8-R -t UTF-8 rgramtab.tab > rgramtab-utf8.tab
```

The End.
--------
