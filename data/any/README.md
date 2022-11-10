
Definition of the "ANY" language.
--------------------------------
The dictionary define here will parse "any" language, exploring all
combinatoric possibilities.  It will generate a uniform distribution
of all possible random parse trees. This is useful in some machine
learning tasks.

Some important design notes to be kept in mind.

* The code implicitly assumes that white-space is a word-separator.
  This works great for most modern languages, but can present
  difficulties for certain languages, as well as ancient and academic
  texts.

  In particular, Chinese texts are NOT written with white-space
  separated words, and word segmentation is outside the bounds
  of what can be supported here.

* Punctuation: the `4.0.affix` file defines a set of leading and
  trailing punctuation that is automatically stripped from the
  beginnings and endings of words. The list of punctuation in manually
  assembled, and is appropriate for common English and European text
  corpora found on the net.  This is a stop-gap measure for proper
  text segmentation; its a bit ad hoc and can be problematic.

  A proper approach would preface this stage by a text segmentation
  stage. Ideally, the segmentation should be learned. Such a system
  is not currently available.

* Root words: The current dictionary allows the identification of
  multiple "root" words, e.g. of words that could be interpreted as
  root-verbs and root-nouns (subjects).  This may come as a surprise
  to some linguists, as there is only one root in the Chomskian
  tradition, and many dependency grammars share this tradition and
  only link to a single root, usually the root verb.

  This is accomplished by having LEFT-WALL attach with `@ANY+`.
