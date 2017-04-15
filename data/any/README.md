
Defintion of the "ANY" language.
--------------------------------
The dictionary define here will parse "any" language, exploring all
combinatoric possibilities.  It is used for certain machine-learning
tasks, when one wants to iterate over all possible parse trees in
every possible way.

Some important design notes to be kept in mind.

* The code implicitly assumes that white-space is a word-separator.
  This works great for just about all modern langauges, but can
  present difficulties for ancient and academic texts.

  In particular, Chinese texts are NOT written with white-space
  separated words, and word segmentation is outside the bounds
  of what can be supported here.

* Punctuation: the `4.0.affix` file defines a set of leading and
  trailing punctuation that is automatically stripped from the
  beginings and endings of words. The list of punctuation in manually
  assembled, and is more-or-less complete and appropriate for most
  modern lanuages.  Again, this assumption can be problematic.

  In a truly purist approach to language-learning, the learning process
  should be able to discern punctuation on it's own, without needing
  to have it be pre-specified. However, this is currently impractical,
  and so the short-term hack is that punctuation is stripped, manually,
  here.
  

