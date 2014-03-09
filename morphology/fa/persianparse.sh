#!/bin/sh
### Syntax: persianparse.sh 'input_sentence'
echo -e "!width=160\n$*\n\n" | stemmer.pl -u | ~/installs/lgparser-4.1/link-4.1/lgparser ~/20050424/ling/persianlg/data/4.0.dict
