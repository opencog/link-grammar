#!/bin/sh
### Usage: ./arabiclg_one_sentence.sh 'input_sentence'

#export ARABICLG_HOME=/home/tcsh/public_html/arabiclg
export ARABICLG_HOME=../

time echo -e "$*"			 			|	# time this script and echo STDIN
${ARABICLG_HOME}/bin/aramorph_fast.pl -i roman 2>/dev/null	|	# morphology; assumed romanized input
${ARABICLG_HOME}/bin/buck2lg.sh					|	# some ugly postprocessing to glean morphemes
(echo -e '!width=118\n!max-length=300\n'; cat; echo -e "\n\n\n\n\n")|	# prepends link-grammar width info
link-grammar ${ARABICLG_HOME}/4.0.dict 2>/dev/null		|	# link-grammar parsing
egrep -v 'Opening|width set to|\+Time|RETURN'			|	# eliminates useless lines
egrep -C 70 --color "UNUSED=[0-9]+|Found [0-9]+ linkages"		# highlight number of unused links
