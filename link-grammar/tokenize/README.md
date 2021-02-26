Version 5.5.0 - Official wordgraph display support
==================================================

As of version 5.5.0, the default configuration includes the word-graph
display. A new API function
`bool sentence_display_wordgraph(Sentence sent, const char *modestr);`
has been added, and `link-parser` can use it (controlled by the
`!wordgraph` user variable).

Version 5.3.0 - Introduction of a tokenizer word-split graph - AKA "wordgraph"
==============================================================================

In this version the tokenizer code has been heavily modified once again.
Instead of tokenizing all the words in one pass directly into the
word-array of the parser, it now tokenizes them using a graph which its
nodes are "subwords".  Initially the original sentence words are tokenized
to subwords only by whitespace. After that step, each subword is handled in
its turn, and gets split to further subwords if needed. A special care is
taken if one of the alternatives of a subword is the subword itself
(alone, which is usual, or with a dict-cap token - a new experimental
mechanism which is described below).

The new way of tokenizing is much more flexible that the old one, and it
serves as an infrastructure on which new tokenizing and display features
can be implemented. One of them, that is implemented in this release, is
spelling for languages other then English. This is now possible because the
spell result can now pass further tokenization if needed. This also
enhances the spelling for English in case the spell result is a
contraction. In addition, the multi-level of tokenization, inherent to the
word-graph, allows multiple run-on and spell-correction fixes on the same
word at once.

The tokenizing code is still based much on the old code and further
work is needed to clean it up (or to replace it, e.g. by a
regex-tokenizer). It still doesn't use the full power of the word-graph,
and there are constructs that need to be tokenized but they are not (they
are also not in the sentence test batches). E.g. `--` between words without
whitespace.

There is still no API to get information from the word-graph. In particular,
it is not possible to find out the sentence words after punctuation
tokenization, as in previous releases.

Since the parser cannot currently use the word-graph directly, there is a
need to convert it to the 2D-word-array that it uses. This is implemented
by the function `flatten_wordgraph()`, which uses a complex algorithm: It
scans all the word-graph paths in parallel, trying to advance to the next
words and to issue them into the 2D-word-array.

It advances to the next word of a given word in rounds, using two passes, one
word per word-graph path on each round:
Pass 1. Next words which are in the same alternative of the given word.
Pass 2. Next words which are in a different alternative (of the common
ancestor word) of words that has already been advanced to in pass 1
and this pass.

The words that got advanced to are issued into the 2D-word-array.  It is
possible that the second pass above cannot get advance in particular
word-graph path, because the next word is in the same alternative of one of
the next words in other paths. In that case an empty word is issued. This
constraint arises because all the next words in each word-graph advance
round, are issued into the same 2D-word-array "slot", which cannot hold
words from the same alternative.

As in the previous versions, due to the way alternatives are implemented,
morphemes from different word-tokenization alternatives can be mixed in a
linkage. Such linkages are of course useless and need to be detected and
discarded. This is done by the `sane_linkage_morphism()` function. In the
previous versions it validated that the chosen words (the words in the
linkage) that are subwords (e.g. morphemes) of a sentence word, all belong
to a single tokenization alternative of that word.
It now works in another way - it validates that the chosen words create a
path in the word-graph. In case of "null-words" - words with no linkage -
the first path which is encountered is used. It means that a word in the
word-graph path corresponding to a null-word, may be only one of the potential
possibilities.

Word-graph display
------------------
Another feature that has been implemented, mainly for debug (but it can
also be useful for inspiration and fun), is displaying a graphical
representation of the word graph. The graphs can be displayed in several
ways, controlled by one-letter flags. The command `!wordgraph=1` enables the
display of a graphs which includes no "prev" links for clarity, but
includes "unsplit word" links.  Other graphical representation modes can be
enabled by `!wordgraph=2` (display unsplit words as subgraphs) and
`!wordgraph=3` together with `!test=wg:FLAGS` (when FLAGS are lowercase letters
as defined in wordgraph.h) to use display modes according to FLAGS.
For example, the `p` flag (i.e. `!test=wg:p`) adds "prev" links.

The graphical display code is compiled by default; if it is not desired
(e.g. because it involves an invocation of an external program (`dot`)
and in addition, temporary files are created), use
`--disable-wordgraph-display` to disable this feature.

On Windows this feature is enabled when compiled with `USE_WORDGRAPH_DISPLAY`.
See "[msvc/RDADME.md](/msvc/README.md)" for further details.

Quote handling
--------------
Quotes now are not discarded, but are considered to be regular dict tokens.
In this version they have no significant linkage and always attach to the word
before them (or to the LEFT-WALL). In order to compare detailed batch runs with
previous versions of the library, a `!test=removeZZZ` can be used to remove the
quote display.

Handling capitalized words
--------------------------
Not as in previous releases, capital letters which got downcased are not
restored for display if the affected words have a linkage.

A new experimental handling of capital words using the dictionary has been
introduced. It inserts the token `<1stCAP>` before the uc version, and
`<nonCAP>` before the lc one, as discussed in:
https://groups.google.com/forum/?hl=en#!topic/link-grammar/hmK5gjXYWbk
It is enabled by `!test=dictcap`. The special "dictcap" tokens are not yet
discarded, so in order to compare results to previous library versions, the
following can be used: `!test=dictcap,removeZZZ`.
