# English Grammar Fixes

This file documents nontrivial grammar fixes in the English dictionary. It is
intended for maintainers who need more context than a source comment or commit
message can carry: the linguistic problem, the previous connector mechanism,
the source of overgeneration, the replacement strategy, and the validation
evidence.

The entries below describe only grammar work reflected by the accompanying
code and dictionary changes.

## PP Migration From `4.0.knowledge` To `4.0.dict`

This section documents selected postprocessing (PP) rules being moved into
dictionary grammar. The immediate engineering motivation is metric-ordered
linkage extraction: if PP-bad raw linkages are generated cheaply, ordered
extraction may spend most of its early search on candidates that will be
rejected after extraction. The linguistic motivation is the same in ordinary
parsing terms: a connector path should encode the grammatical condition that
makes it valid whenever the dictionary can express that condition.

A PP-rule migration is treated as complete only when the dictionary replacement
is whole-rule or substantially complete on the agreed corpora and focused
challenge examples. Sentence-specific lexical tightenings are not considered
PP-rule replacements.

Entries use precise maintainer-facing technical prose. Claims are stated in
terms of observed linkages, dictionary expressions, PP rules, and test
evidence. Limitations and inferred analyses are identified as such.

## Deferred PP Migration Candidates

The following PP rules were tested for simple removal or identified as likely
future dictionary-migration work. They remain active because the current
dictionary does not yet encode the rejected condition narrowly enough.

| Rule(s) | Area | Current status |
| --- | --- | --- |
| 20-31, 37-39 | Expletive `it` complement licensing | Simple removal leaves `corpus-knowledge.batch` clean but raises `corpus-basic.batch` from 88 to 109 errors. The new accepts include bad ordinary-subject uses of `THi`/`TOi`-style complements, such as `Joe is likely that ...` and `It tried to have been ...`. A replacement needs a shared expletive-`it` subject/complement split across copular, adjectival, and verbal complement paths. |
| 32s, 32p, 32u, 34-36 | Existential `there` agreement | Simple removal leaves `corpus-knowledge.batch` clean but raises `corpus-basic.batch` from 88 to 92 errors. It accepts bad agreement such as `There is chasing dogs`, `There are a dog`, and coordinated singular complements with plural `are`. These need agreement-aware `there.r`/`be` object-path splits. |
| 42 | Predicate/question `BIq` | Simple removal raises the fast prefix from 1 to 3 errors and `corpus-basic.batch` from 88 to 90 errors. A prototype that moved `BI+` to a copula branch with `Ss*q-` still accepted a bad `big mind on everybody's question is who ...` parse, because a generic noun subject can match a subscripted verb-side connector and inherit the `Ss*q` link name. A replacement therefore needs a new connector family on the licensing nouns/clauses, not only a subscripted copula-side split. |
| 43, 44, 47, 48 | Comparative paths | Bulk removal leaves `corpus-knowledge.batch` clean and improves `corpus-fixes.batch` from 362 to 358 errors, but raises `corpus-basic.batch` from 88 to 90 errors. These rules contain overbroad positives mixed with real protections, so they need narrower comparative connector splits rather than deletion. |
| 56, 58, 59 | Comparative agreement and complement checks | Bulk removal leaves `corpus-knowledge.batch` clean but raises `corpus-basic.batch` from 88 to 91 errors and `corpus-fixes.batch` from 362 to 363 errors. The protected bad paths include `Ours is more elegant than yours works`, `She interviewed more programmers than was hired`, and `I am as intelligent as John does`. |
| 78 | `EAy` with `MVs` | Simple removal leaves `corpus-knowledge.batch` clean but raises `corpus-basic.batch` from 88 to 89 errors and `corpus-fixes.batch` from 362 to 363 errors. It protects cases where the second `as` is parsed as ordinary subordinate `as.#while` with `MVs` while an earlier comparative `EAy` is active, such as `I am as intelligent as John does` and `The coffee tastes as it did last year`. A replacement likely needs a dictionary-level comparative `as` pairing. |

## Library-Assisted Dictionary Helper Tokens

**Status:** implemented as dictionary support; used by the preposition
continuation changes for rules 12, 13, 14, and 10/11 when the `Wj` companion
path is needed.

### Rule / Area

The helper-token mechanism supports dictionary definitions that need an extra
internal syntactic anchor. The English dictionary currently uses it for the
`wjqprep` helper in wh-preposition questions and relatives. It was introduced
for rule 12 and is also present in the rule 13 and rules 10/11 preposition
continuation macros through the shared `Wj` companion path.

### Background

Ordinary Link Grammar parsing selects one disjunct for each word in a linkage.
Some grammatical relations need one surface word to participate in two
relations that are easier to express if there is a second local anchor. In the
rule 12 work, a fronted preposition must be tied both to its wh object and to
the question-verb path. Encoding this only on the visible preposition and wh
word is awkward because the useful witness relation would otherwise require
additional connector geometry on the same surface construction.

The helper-token mechanism does not change the core one-disjunct-per-word
model. Instead, the dictionary can request an optional hidden token in the word
graph, and the normal parser can then satisfy ordinary connectors through that
token.

### Implementation

Dictionary entries can include paired tokenizer-only markers:

```text
INSERTL<token>+
INSERTR<token>+
```

When a word with `INSERTL<token>+` is followed by a word with
`INSERTR<token>+`, tokenization adds an optional helper token named `<token>`
after the second word. Both marker connectors point right, so they carry
tokenizer instructions without being intended as real grammar links.

For the current English wh-preposition path:

```text
<marker-wjqprep-left>:  INSERTLwjqprep+
<marker-wjqprep-right>: INSERTRwjqprep+
wjqprep: WJIb- & WJIa- & Qp+
```

The helper token allows the preposition to keep its `Wj` relation to the wh
object while the helper carries the `Qp` relation to the question verb. The wh
word supplies `JW- & WJIb+`, and the preposition supplies `Wj- & WJIa+`.
Rules 13, 14, and 10/11 do not require every valid linkage to use the helper:
their replacements also use direct `MVp`, `Mj`, `MX#j`, `JW`, and `JQ`
continuations. The helper is nevertheless part of their replacement grammar
because their shared preposition continuations include the `Wj` companion
alternative.

After PP has seen the full internal linkage, helper words and their incident
links are suppressed from the displayed/API linkage arrays. Thus the helper is
a dictionary-internal certificate, not a surface word in the presented parse.

### Implications

This mechanism is library-assisted dictionary grammar. It is appropriate when
the dictionary can express the intended grammar relation with an internal
anchor, but ordinary visible-word connector geometry is insufficient or would
overgenerate. It should not be used to hide arbitrary bad parses; the helper
must encode a grammatical witness relation.

### Verification

The helper-token support was validated with:

```sh
link-parser < ./data/en/corpus-knowledge.batch
link-parser < ./data/en/corpus-basic.batch
link-parser < ./data/en/corpus-fixes.batch
link-parser < ./data/en/corpus-fix-long.batch
```

Expected results for the current documented state are:

```text
corpus-knowledge.batch: 0 errors
corpus-basic.batch: 88 errors
corpus-fixes.batch: 362 errors
corpus-fix-long.batch: 9 errors
```

## Rule 12: `Wj` Requires `Jw` Or `JQ`

**Status:** implemented; the PP rule has been removed from `4.0.knowledge`.

### Rule / Area

The PP rule is:

```text
Wj , Jw JQ , "Misuse of preposition12"
```

The grammatical area is fronted prepositions in wh questions and preposition
object relatives.

### Problem

A `Wj` link marks a fronted preposition path. Such a preposition must be
licensed by the corresponding wh object or by the question-preposition path
(`JQ`). The removed PP rule used the historical wh-object name `Jw`; the
dictionary replacement uses `JW` for that relation. Without this witness, the
dictionary can generate raw linkages in which a fronted preposition is
syntactically detached from the wh construction that makes it grammatical.

### Old Mechanism

The older dictionary path allowed:

```text
Wj- & Qp+
```

on the preposition continuation. This directly connected the preposition to
the question verb but did not force the same local construction to include the
wh-object witness.

### Overgeneration Cause

The connector path encoded question-fronting but not the full wh-preposition
relation. The missing condition was checked later by PP as a link-existence
condition: a domain containing `Wj` also needed `Jw` or `JQ`.

### Implementation

The dictionary now has two relevant paths:

```text
JQ+ & Wj- & Qp+
Wj- & WJIa+
```

The first path keeps direct question-preposition cases where `JQ` is present.
The second path uses the `wjqprep` helper token.  The wh-object connector is
spelled `JW` in the dictionary, rather than subscripted `Jw`, so ordinary
preposition `J+` object branches cannot accidentally match it:

```text
preposition --WJIa-- wjqprep --WJIb-- wh-word
wjqprep --Qp-- question-verb
preposition --JW-- wh-word
```

This makes the wh-object witness part of the dictionary construction instead
of relying only on a later PP rejection.

The PP rule has been removed from `4.0.knowledge`; the dictionary construction
is now responsible for enforcing this condition before PP.

### Examples

Good examples covered by `corpus-knowledge.batch` include:

```text
With whom did you play tennis?
For whom were you mistaken?
To what do you owe your success?
Of which person were you speaking?
By what means will you arrive?

I know to what you owe your success.
Sophy wondered up to what number she should count.
Do you know in which room the procedure was performed?

The man with whom I play tennis is here.
The friend for whom Joe works is kind.
The subject about which Mary spoke was difficult.
The book of which I spoke was old.
```

Diagnostic bad examples include:

```text
*With did you play tennis?
*To do you owe your success?
*Of person were you speaking?
*The man with whom I play tennis with is here.
```

### Verification

The rule 12 removal was validated with ordinary parser runs:

```sh
link-parser < ./data/en/corpus-knowledge.batch
link-parser < ./data/en/corpus-basic.batch
link-parser < ./data/en/corpus-fixes.batch
link-parser < ./data/en/corpus-fix-long.batch
```

is expected to report:

```text
0 errors
```

## Rule 6: Infinitival `to` Requires A Filler/Gap Witness

**Status:** implemented; the PP rule has been removed from `4.0.knowledge`.

### Rule / Area

The removed PP rule was:

```text
I#a , B#m B#w , "incorrect use of 'to'6"
```

The grammatical area is infinitival `to` in filler/gap constructions.

### Background

The `to.r` entry has long noted that `I+ & MVi-` admits useful cases such as:

```text
What is there to do?
```

but may also admit bad infinitival-gap paths such as:

```text
*He is going to do.
```

The old PP rule tried to reject a completed linkage containing `I#a` unless
the linkage also contained `B#m` or `B#w`.

### Problem

The PP rule was downstream of extraction and global in shape. It did not prove
that the `B` witness belonged to the same infinitival construction as the
`I#a` link; it only required a matching `B` link somewhere in the linkage.

### Old Mechanism

The direct source of the bad path was the broad fallback branch in `to.r`:

```text
or I*a+
```

Because generic connectors can match subscripted variants, that fallback could
create `I#a` without a local proof of the intended filler/gap relation.

### Implementation

The dictionary removes the naked `I*a+` fallback from `to.r`. No replacement
connector is added in this step. Existing licensed infinitival constructions
continue to use their narrower dictionary paths.

This is stricter and more local than the old PP rule: the dictionary should not
generate an infinitival path whose correctness depends on finding an unrelated
`B#m` or `B#w` elsewhere in the sentence.

If future valid examples require the removed path, do not restore naked
`I*a+`. Add a local simulated-cross-link style connector family with a new
uppercase connector name, so generic `I-` connectors cannot accidentally match
it.

### Examples

Focused examples include:

```text
Tell me what to do.
Tell me which book to read.
The book to read is here.
What are you going to do?
What is there to do?
```

The broader bad examples:

```text
*He is going to do.
*He is going to have.
```

are not fully solved by this narrow rule-6 migration; they involve other
infinitival-gap paths and should be handled as separate grammar work.

### Verification

The rule 6 removal was validated with ordinary parser runs:

```sh
link-parser < ./data/en/corpus-knowledge.batch
link-parser < ./data/en/corpus-basic.batch
link-parser < ./data/en/corpus-fixes.batch
link-parser < ./data/en/corpus-fix-long.batch
```

## Rule 13: `JQ` Requires A Preposition Companion

**Status:** implemented; the PP rule has been removed from `4.0.knowledge`.

### Rule / Area

The removed PP rule required a `JQ` domain to contain at least one companion
from this family:

```text
Mj Wj MX#j MVp
```

The grammatical area is wh-preposition questions and relatives. A `JQ` link
should not be an isolated artifact; it must be part of a construction that
also supplies the corresponding preposition or modifier relation.

### Problem

The dictionary previously allowed optional `JQ+` on broad preposition object
branches. That made it possible to generate raw linkages where `JQ` was present
without a local companion proving that the preposition was actually used in a
question or relative construction.

### Old Mechanism

Many preposition entries used forms equivalent to:

```text
{JQ+} & J+ & <prep-main-a>
```

or the same pattern with related object connectors. The optional `JQ+` could
be added to a broad continuation whose other alternatives were valid for
ordinary non-question uses.

### Overgeneration Cause

The dictionary treated `JQ` as an optional annotation on a broad preposition
branch. The grammatical companion relation was not encoded in the same branch,
so PP had to reject complete linkages whose `JQ` had no suitable witness.

### Implementation

The dictionary now uses JQ-specific companion continuations:

```text
<prep-main-jq-b>:
  MVp- or [Mp- & MVp-]-0.61;

<prep-main-jq-a>:
  <prep-main-jq-b>
  or <prep-main-rel>
  or (Wj- & WJIa+)
  or <marker-wjqprep-left>;
```

Preposition entries split ordinary and JQ-bearing alternatives. Non-JQ parses
keep their ordinary continuations. JQ parses use `<prep-main-jq-a>` or
`<prep-main-jq-b>`, which restricts them to continuations that provide one of
the rule-13 companions.

### Implications

This is a complete dictionary replacement for rule 13. The change narrows only
JQ-bearing alternatives; ordinary preposition behavior is kept in the non-JQ
branches.

### Examples

Focused examples include:

```text
By what means will you arrive?
Of which person were you speaking?
Sophy wondered up to what number she should count.
Do you know in which room the procedure was performed?
Exactly what mark did it reach?
Up to what mark did it reach?
```

### Verification

The rule 13 migration was validated with:

```sh
link-parser < ./data/en/corpus-knowledge.batch
link-parser < ./data/en/corpus-basic.batch
link-parser < ./data/en/corpus-fixes.batch
link-parser < ./data/en/corpus-fix-long.batch
```

Expected results:

```text
corpus-knowledge.batch: 0 errors
corpus-basic.batch: 88 errors
corpus-fixes.batch: 362 errors
corpus-fix-long.batch: 9 errors
```

## Rule 14: Wh Preposition Objects Require A Companion

**Status:** implemented; the PP rule has been removed from `4.0.knowledge`.

### Rule / Area

The removed PP rule was:

```text
Jw , Mj Wj MX#j , "Misuse of preposition14"
```

The grammatical area is wh objects of prepositions. A wh-object preposition
link must be part of a relative, question, or conjoined relative construction
that also supplies `Mj`, `Wj`, or `MX#j`.

### Problem

Generic preposition object branches used `J+`. Since `J+` can match a
subscripted `Jw-` connector, ordinary preposition-object paths could attach to
a wh word without selecting the relative or question continuation that makes
the construction grammatical. PP rule 14 rejected those completed raw linkages
after extraction.

### Old Mechanism

The broad shape was:

```text
preposition: (J+ & ordinary-continuation) or (Jw+ & relative-continuation)
wh-word:     Jw-
```

The explicit `Jw+` branch was valid, but the ordinary `J+` branch could also
match the wh word because `Jw` was encoded as a subscripted `J` connector.

### Overgeneration Cause

The dictionary used connector subscripting to distinguish wh preposition
objects, but connector matching treats a bare connector such as `J+` as broad
enough to match subscripted variants. That made the wh-object distinction
visible to PP but not strong enough to block an ordinary dictionary path.

### Implementation

The wh preposition object connector is now a distinct uppercase connector
family in the dictionary:

```text
preposition: (J+ & ordinary-continuation)
           or (JW+ & (relative-continuation or wh-question-continuation))
wh-word:     JW-
```

Because `JW` is not a subscripted form of `J`, ordinary `J+` object branches no
longer match wh preposition objects. The explicit `JW+` branch is then the only
way to form the wh-object link, and that branch requires either:

```text
<prep-main-rel>
```

for relative uses, or:

```text
<prep-main-whq>:
  (Wj- & WJIa+)
  or <marker-wjqprep-left>;
```

for wh-question uses through the `wjqprep` helper-token path.

This rejected the first attempted implementation, which replaced ordinary
`J+` with a macro containing subscripted positive variants such as `Jj+`,
`Js+`, and `Jv+`. Those variants also matched bare `J-` objects and multiplied
ordinary preposition linkages, for example changing `I spoke with him.` from
10 linkages to 60. The `JW` split preserves ordinary `J` behavior and changes
only the wh-object connector family.

### Examples

Focused good examples include:

```text
With whom did you play tennis?
For whom were you mistaken?
The friend for whom Joe works is kind.
That is the man for whom and with whom Joe works.
He replied with a yes.
He greeted me with a loud hallo!
He wanted to look at and listen to everything.
```

Diagnostic bad examples include:

```text
*That is the man for whom and with Janet Joe works.
*The man with whom I play tennis with is here.
```

### Verification

The rule 14 migration was validated with:

```sh
link-parser < ./data/en/corpus-knowledge.batch
link-parser < ./data/en/corpus-basic.batch
link-parser < ./data/en/corpus-fixes.batch
link-parser < ./data/en/corpus-fix-long.batch
```

Expected results:

```text
corpus-knowledge.batch: 0 errors
corpus-basic.batch: 88 errors
corpus-fixes.batch: 362 errors
corpus-fix-long.batch: 9 errors
```

## Redundant `CONTAINS_NONE` PP Checks

**Status:** implemented for rules 69, 70, 74, 75, 76, 77, and 79.

### Rule / Area

The removed PP rules were:

```text
S    , Spxi        , "Bad n-v agreement69"
SI   , SIpxi       , "Bad n-v agreement70"
OX   , I* PP* TO* Pa* Pam Pg* Pv* LE* AFd* MVta,
                  "Bad use of 'filler' subject74"
MXsr , Sp#w        , "Bad n-v agreement75"
MXpr , Ss#w S#iw   , "Bad n-v agreement76"
Mr   , B#*         , "Bad use of 'whose'77"
VCz  , EAy         , "Bad comparative79"
```

The grammatical areas are older agreement, filler-subject, possessive-relative,
and comparative safety checks in `CONTAINS_NONE_RULES`.

### Problem

These checks no longer appear to carry observable behavior in the current
English dictionary. Keeping redundant PP checks is undesirable because a future
dictionary change can expose an old rule as an extraction blocker even when it
does not protect the current regression behavior.

### Old Mechanism

Each rule rejected a completed linkage if a selector link and a prohibited
criterion link appeared in the same PP domain. Unlike the dictionary
migrations above, these rules are not replaced by new connector structure.
They are removed because the current dictionary does not appear to require
them for the tracked behavior.

### Implementation

The seven rules are removed from `4.0.knowledge`.

Rules 71, 72, 73, and 78 remain active because they were not part of this
redundancy removal. The ID-less `Bad subject inversion` rule also remains
active.

### Implications

This removes seven complete PP checks from the blocker set without changing
the ordinary corpus counts. If future focused examples show that one of these
old conditions still represents real bad grammar, the preferred fix should be
a dictionary or parser-level rule that rejects the bad path before PP.

### Verification

The removal should be validated with ordinary parser runs:

```sh
link-parser < ./data/en/corpus-knowledge.batch
link-parser < ./data/en/corpus-basic.batch
link-parser < ./data/en/corpus-fixes.batch
link-parser < ./data/en/corpus-fix-long.batch
```

Expected results:

```text
corpus-knowledge.batch: 0 errors
corpus-basic.batch: 88 errors
corpus-fixes.batch: 362 errors
corpus-fix-long.batch: 9 errors
```

## Rules 49 And 50: Remove Overbroad `Pafc` Comparative Checks

**Status:** implemented; both PP rules have been removed from
`4.0.knowledge`.

### Rule / Area

The removed PP rules were:

```text
Pafc , EB#m EB#y , "Bad comparative49"
Pafc , Pa* Paf*  , "Bad comparative50"
```

The grammatical area is predicative adjectival complements in comparative
constructions headed by `than`.

### Problem

The rules rejected valid idiomatic comparative predications in the current
dictionary. In the tracked corpus they blocked examples such as:

```text
He is nothing less than inspired!
He is more than capable!
```

These sentences are grammatical: `less than` and `more than` function as
degree modifiers of the following predicative adjective rather than as ordinary
comparative clauses needing the older PP rejection.

### Old Mechanism

The older PP checks treated `Pafc` as incompatible with nearby comparative
degree evidence (`EB#m`, `EB#y`) and with predicative-adjective links
(`Pa*`, `Paf*`) in the same relevant domain. This was intended as a safety
check for malformed comparative structures, but it also rejected legitimate
degree-comparative predications.

### Overgeneration Cause

The rules were negative domain checks over link names rather than a positive
description of the grammatical comparative construction. They could not
distinguish a bad comparative-clause parse from an idiomatic
`more/less than ADJ` degree construction that uses the same surface
preposition and adjective-complement links.

### Implementation

Rules 49 and 50 are removed from `4.0.knowledge`. No replacement connector
structure is added in this change because the current dictionary already
produces valid PP-clean linkages for the recovered examples once the overbroad
negative checks are gone.

### Implications

This is a whole-rule removal, not a sentence-specific lexical tightening. It
reduces the PP blocker set and also recovers two positive examples in
`corpus-fixes.batch`. If future examples expose a genuine malformed
comparative that these checks used to reject, the preferred fix should encode
the narrower grammatical distinction in dictionary connectors rather than
reinstating broad `Pafc` domain exclusions.

### Verification

The removal should be validated with ordinary parser runs:

```sh
link-parser < ./data/en/corpus-knowledge.batch
link-parser < ./data/en/corpus-basic.batch
link-parser < ./data/en/corpus-fixes.batch
link-parser < ./data/en/corpus-fix-long.batch
```

Expected results:

```text
corpus-knowledge.batch: 0 errors
corpus-basic.batch: 88 errors
corpus-fixes.batch: 362 errors
corpus-fix-long.batch: 9 errors
```

## Rules 51-54: Remove Overbroad `MVat` / `MVpt` Comparative Checks

**Status:** implemented; all four PP rules have been removed from
`4.0.knowledge`.

### Rule / Area

The removed PP rules were:

```text
MVat , MVm     , "Bad comparative51"
MVpt , MVm     , "Bad comparative52"
MVat , MVa MVp , "Bad comparative53"
MVpt , MVa MVp , "Bad comparative54"
```

The grammatical area is `than`-headed comparative adjuncts, especially
comparative clauses or reduced comparative adjuncts following adjectival or
nominal material.

### Problem

The rules rejected a valid reduced comparative adjunct in the current
dictionary:

```text
they report less robust earnings than previously
```

Here `than previously` is a grammatical reduced comparative adjunct. It does
not need a full verbal comparative clause, and rejecting it because it lacks
the older companion links is too broad.

### Old Mechanism

The older checks required `MVat` and `MVpt` domains to contain one of several
other comparative modifier links (`MVm`, `MVa`, or `MVp`). Those companion
links are useful in many ordinary comparative clauses, but they are not a
necessary property of every valid `than` adjunct.

### Overgeneration Cause

The checks encoded a broad domain-level co-occurrence expectation rather than
the grammar of a specific malformed construction. Reduced comparative
adjuncts can be locally complete without the companion links named by the PP
rules, so the PP check rejected legitimate parses.

### Implementation

Rules 51, 52, 53, and 54 are removed from `4.0.knowledge`. No new connector
structure is added in this change: the existing dictionary already provides
PP-clean linkages for the recovered reduced-comparative examples once the
overbroad negative checks are removed.

### Implications

This removes another complete comparative PP rule family and recovers one
positive `corpus-fixes.batch` example. If a future malformed `than` adjunct
requires rejection, the preferred replacement is a narrower dictionary
analysis of that construction rather than a global prohibition on `MVat` or
`MVpt` without particular companion links.

### Verification

The removal should be validated with ordinary parser runs:

```sh
link-parser < ./data/en/corpus-knowledge.batch
link-parser < ./data/en/corpus-basic.batch
link-parser < ./data/en/corpus-fixes.batch
link-parser < ./data/en/corpus-fix-long.batch
```

Expected results:

```text
corpus-knowledge.batch: 0 errors
corpus-basic.batch: 88 errors
corpus-fixes.batch: 362 errors
corpus-fix-long.batch: 9 errors
```

## Rules 60 And 61: Remove Redundant `THc` / `TOc` Comparative Checks

**Status:** implemented; both PP rules have been removed from
`4.0.knowledge`.

### Rule / Area

The removed PP rules were:

```text
THc , TH             , "Bad comparative60"
TOc , TO** TOf* TOi* , "Bad comparative61"
```

The grammatical area is comparative complement licensing for `than`/`as`
constructions that take finite or infinitival clause material.

### Problem

The current dictionary no longer appears to need these two PP checks for the
tracked behavior. Removing them does not change the ordinary corpus counts or
the focused comparative examples tested with this change.

### Old Mechanism

Each rule rejected a completed linkage if a comparative selector link and a
specified clause-complement link appeared in the same PP domain. This was a
negative safety check over link names rather than a positive dictionary
description of the bad construction.

### Overgeneration Cause

The rules are broader than a current observable malformed construction. The
dictionary already constrains the tested comparative complement paths enough
that these PP checks do not protect the agreed corpora.

### Implementation

Rules 60 and 61 are removed from `4.0.knowledge`. No replacement connector
structure is added.

### Implications

This removes two complete comparative PP checks from the blocker set. If a
future focused test shows a real malformed `THc` or `TOc` construction, the
preferred repair is a narrower dictionary rule for that construction rather
than reinstating the broad domain-level prohibition.

### Verification

The removal should be validated with ordinary parser runs:

```sh
link-parser < ./data/en/corpus-knowledge.batch
link-parser < ./data/en/corpus-basic.batch
link-parser < ./data/en/corpus-fixes.batch
link-parser < ./data/en/corpus-fix-long.batch
```

Expected results:

```text
corpus-knowledge.batch: 0 errors
corpus-basic.batch: 88 errors
corpus-fixes.batch: 362 errors
corpus-fix-long.batch: 9 errors
```

## Rules 45 And 46: Remove Redundant `MV#a` / `MV#i` Comparative Checks

**Status:** implemented; both PP rules have been removed from
`4.0.knowledge`.

### Rule / Area

The removed PP rules were:

```text
MV#a , Pam Pafm EAm Ds*m EAy AFm Mam Am , "Bad comparative45"
MV#i , Pam Pafm EAm Ds*m EAy AFm Mam Am , "Bad comparative46"
```

The grammatical area is comparative modifier licensing for adjective and
adverb comparative paths.

### Problem

The current dictionary no longer appears to need these two PP checks for the
tracked behavior. Removing them does not change the agreed corpus counts or
the focused comparative examples tested with this change.

### Old Mechanism

Each rule rejected a completed linkage if `MV#a` or `MV#i` occurred with one
of the listed comparative, predicative, or adjective-modifier links in the
same PP domain. This was a broad negative co-occurrence check.

### Overgeneration Cause

The rules are not tied to an observable malformed construction in the current
tests. The dictionary already constrains the tested adjective and adverb
comparative paths sufficiently for the agreed corpora.

### Implementation

Rules 45 and 46 are removed from `4.0.knowledge`. No replacement connector
structure is added.

### Implications

This removes two complete comparative PP checks from the blocker set without
changing ordinary corpus behavior. If a future malformed construction is
found, it should be handled by a narrower dictionary analysis instead of
restoring the broad `MV#a`/`MV#i` domain exclusions.

### Verification

The removal should be validated with ordinary parser runs:

```sh
link-parser < ./data/en/corpus-knowledge.batch
link-parser < ./data/en/corpus-basic.batch
link-parser < ./data/en/corpus-fixes.batch
link-parser < ./data/en/corpus-fix-long.batch
```

Expected results:

```text
corpus-knowledge.batch: 0 errors
corpus-basic.batch: 88 errors
corpus-fixes.batch: 362 errors
corpus-fix-long.batch: 9 errors
```

## Rules 55 And 57: Remove Redundant Comparative Checks

**Status:** implemented for rules 55 and 57. Neighboring rules 56, 58, and
59 remain active.

### Rule / Area

The removed PP rules were:

```text
U#t  , D##m D##y Om Oy Jm Jy Am MX#m , "Bad comparative55"
Sp#c , Dmcm Dmcy Om Oy Jm Jy MX#m    , "Bad comparative57"
```

The grammatical area is comparative agreement and complement licensing in
question and clause-comparative paths.

### Problem

The current dictionary no longer appears to need these two PP checks for the
tracked behavior. Rule 55 is structurally unreachable under the current
dictionary: the selector `U#t` can only match a three-character `U` link ending
in `t`, such as `Upt` or `Ust`, but the English dictionary does not currently
define any `U...t` connectors. Removing rules 55 and 57 does not change the
agreed corpus counts or the focused comparative examples tested with this
change.

### Old Mechanism

Both rules rejected completed linkages by checking for broad link-name
co-occurrence inside PP domains. Rule 55 used `U#t` as the selector; rule 57
used `Sp#c`. The criterion sets mixed determiner, object, comparative, and
modifier links.

### Overgeneration Cause

No current agreed test requires these broad negative checks. For rule 55, the
old selector cannot be produced by the current dictionary. Comparative `U`
links observed in focused examples use names such as `Us`, `Up`, or `Upc`,
which do not match `U#t`. For rule 57, the dictionary already constrains the
tested comparative paths sufficiently.

### Implementation

Rules 55 and 57 are removed from `4.0.knowledge`. No replacement connector
structure is added.

Rules 56, 58, and 59 were tested as possible neighboring removals and are not
part of this change. Each produced a `corpus-basic.batch` regression and
therefore remains active until a narrower dictionary replacement is designed.

### Implications

This removes two complete comparative PP checks from the blocker set without
changing ordinary corpus behavior. It also narrows the remaining comparative
PP work: rules 56, 58, and 59 are not simple redundancy removals.

If a future dictionary change introduces a `U...t` connector, rule 55's old
grammar intent should be re-evaluated before the new connector is accepted.

### Verification

Rule 55's unreachable-selector condition can be checked directly:

```sh
rg "\bU[a-zA-Z*#]*t[+-]" data/en/4.0.dict data/en/4.0.dict.m4
```

Expected result: no matches.

The removal should also be validated with ordinary parser runs:

```sh
link-parser < ./data/en/corpus-knowledge.batch
link-parser < ./data/en/corpus-basic.batch
link-parser < ./data/en/corpus-fixes.batch
link-parser < ./data/en/corpus-fix-long.batch
```

Expected results:

```text
corpus-knowledge.batch: 0 errors
corpus-basic.batch: 88 errors
corpus-fixes.batch: 362 errors
corpus-fix-long.batch: 9 errors
```

## Rule 62: Remove Redundant `TOtc` Comparative Check

**Status:** implemented; the PP rule has been removed from
`4.0.knowledge`.

### Rule / Area

The removed PP rule was:

```text
TOtc , TOt , "Bad comparative62"
```

The grammatical area is infinitival comparative complements.

### Problem

The current dictionary no longer appears to need this PP check for the tracked
behavior. Removing it does not change the agreed corpus counts or the focused
infinitival-comparative examples tested with this change.

### Old Mechanism

The rule rejected a completed linkage when `TOtc` and `TOt` occurred in the
same PP domain. This was a broad negative check over infinitival-comparative
link names.

### Overgeneration Cause

The current dictionary already constrains the tested infinitival comparative
paths sufficiently. No current agreed test requires this PP check.

### Implementation

Rule 62 is removed from `4.0.knowledge`. No replacement connector structure is
added.

### Implications

This removes one complete comparative PP check from the blocker set without
changing ordinary corpus behavior. If a future malformed infinitival
comparative requires rejection, it should be handled with a narrower
dictionary rule.

### Verification

The removal should be validated with ordinary parser runs:

```sh
link-parser < ./data/en/corpus-knowledge.batch
link-parser < ./data/en/corpus-basic.batch
link-parser < ./data/en/corpus-fixes.batch
link-parser < ./data/en/corpus-fix-long.batch
```

Expected results:

```text
corpus-knowledge.batch: 0 errors
corpus-basic.batch: 88 errors
corpus-fixes.batch: 362 errors
corpus-fix-long.batch: 9 errors
```

## Rules 15 And 16: Tie `Jr` To Postnominal `B#j`

**Status:** implemented; both PP rules have been removed from
`4.0.knowledge`.

### Rule / Area

The removed PP rules were:

```text
B#j , Jr  , "Incorrect relative15"
Jr  , B#j , "Incorrect relative16"
```

The grammatical area is relative `of whom` and related postnominal
preposition-object relative constructions.

### Problem

The dictionary allowed `of` to use `Jr+` in a broad object branch. Ordinary
nouns and noun-like words can expose generic `J-` object connectors, and
ordinary heads can expose generic `M+` modifiers. This allowed raw linkages
such as:

```text
end.n --M-- of --Jr-- half
```

in phrases like:

```text
at the end of half an hour
```

That is not a postnominal relative construction and cannot legitimately supply
the required `B#j` relation.

### Old Mechanism

The old dictionary relied on PP to reject accidental `Jr` or `B#j` links after
extraction. The valid construction to preserve is:

```text
The doctors, many of whom are surgeons, were angry.
```

with the useful relation:

```text
many --Bpj-- are
many --OFJ-- of --Jr-- whom
```

### Overgeneration Cause

The old `of` branch treated relative `Jr+` as another broad object option
beside `Js+`, `Jp+`, and `Ju+`. That let ordinary noun and modifier connectors
satisfy a relative-clause connector pattern accidentally. Conversely,
postnominal macros could expose `B#j+` without forcing the corresponding
relative `of` path.

### Implementation

The dictionary now uses a dedicated `OFJ` connector for the relative
`of whom` path:

```text
many --OFJ-- of --Jr-- whom
```

The postnominal head supplies both `OFJ+` and `B#j+`. The `of` entry takes
`OFJ- & Jr+` for the relative path, and `Jr+` is removed from the broad object
list.

The postnominal macros expose optional `B#j+` links only together with `OFJ+`:

```text
[OFJ+ & B*j+]
[OFJ+ & Bsj+]
[OFJ+ & Bpj+]
[OFJ+ & Buj+]
```

The no-punctuation `noun-main2-s` postnominal branch also no longer exposes a
bare `Bsj+` path. This makes the dictionary enforce both directions:

```text
Jr  -> OFJ -> B#j
B#j -> OFJ -> Jr
```

### Implications

This is a complete dictionary replacement for rules 15 and 16. Ordinary `of`
object paths still use `Js+`, `Jp+`, `Ju+`, `Mgp+`, or the `QI`/`CV` path;
they cannot accidentally invent `of --Jr-- noun` paths.

Other `of which` cases often use the `QI`/`CV` path and are not forced through
`Jr`.

### Examples

Focused examples include:

```text
The doctors, many of whom are surgeons, were angry.
The box contained many books, some of which were badly damaged.
The male of which bears a tail ran.
```

### Verification

The rules 15 and 16 removal was validated with ordinary parser runs:

```sh
link-parser < ./data/en/corpus-knowledge.batch
link-parser < ./data/en/corpus-basic.batch
link-parser < ./data/en/corpus-fixes.batch
link-parser < ./data/en/corpus-fix-long.batch
```

## Rule 41: Remove Redundant `BIh` Predicate Check

**Status:** implemented; the PP rule has been removed from `4.0.knowledge`.

### Rule / Area

The removed PP rule was:

```text
BIh , Ss#b SIs#b SFsi SFIsi , "Bad use of predicate41"
```

The grammatical area is embedded predicate/clause licensing for `BIh`.

### Problem

The related `THb` and `BIq` predicate rules are not removed here; they require
separate focused validation. The `BIh` rule is different in the current
grammar: after removing it, the agreed corpus results remain unchanged, and
the tested `BIh` paths are already constrained by the dictionary branches that
expose them.

### Old Mechanism

PP rejected `BIh` domains that lacked one of the listed subject or
filler-subject links. That was a safety check for broader historical
predicate/clause paths.

### Implementation

The `predicate41` PP rule is removed from `4.0.knowledge`. No dictionary
connector change is needed for this rule in the current grammar.

### Implications

This removal applies only to `BIh`. It does not imply that the neighboring
`THb` or `BIq` predicate rules are redundant.

### Examples

Focused examples include:

```text
It was as if he knew.
*It was as if knew.
I left because I was tired.
*I left because was tired.
I know when he left.
```

The existing grammar still accepts some unrelated adverbial readings, such as
`left.e` in `*I know when left.`; that is not caused by this rule removal.

### Verification

The rule 41 removal should be validated with ordinary parser runs:

```sh
link-parser < ./data/en/corpus-knowledge.batch
link-parser < ./data/en/corpus-basic.batch
link-parser < ./data/en/corpus-fixes.batch
link-parser < ./data/en/corpus-fix-long.batch
```

## Rule 63: License Postposed `Ma` Adjectives In The Dictionary

**Status:** implemented; the PP rule has been removed from `4.0.knowledge`.

### Rule / Area

The removed PP rule was:

```text
Ma** , TO TOf TH MVp TOt QI OF MVt MVz MVh Ytm Ya ,
       "Bad use of adjective63"
```

The grammatical area is complement-licensed postposed adjective use. A
postposed `Ma` adjective is grammatical when it has a local complement or
modifier relation that licenses this position, such as `MVp`, `OF`, `TH`,
`TO`, `QI`, `Ya`, or related links.

### Problem

Several adjective classes exposed `Ma-` in the same connector group as ordinary
predicative or adjective-conjunction paths. This allowed a raw linkage to use a
bare adjective as a postposed modifier before the dictionary had proved that
the postposed use was licensed.

For example, the dictionary could build local fragments such as:

```text
gift --Ma-- inexpensive
child --Ma-- little
```

Those fragments need the later PP rule to decide whether a suitable licensing
relation also exists.

### Old Mechanism

The old dictionary allowed the `Ma` connector shape broadly and relied on PP to
reject the extracted linkage unless the same domain contained one of the
licensing links:

```text
voters --Ma-- angry --MVp-- about
man --Ma-- proud --OF-- of
topic --Ma-- difficult --TOt-- to
```

This was grammatically precise enough after extraction, but it left the parse
set with unlicensed postposed-adjective paths.

### Overgeneration Cause

The overgeneration was structural, not lexical. Good postposed-adjective uses
exist, but the earlier connector group did not require the licensing
complement at the point where the `Ma` link was created.

Conjoined postposed adjectives require one additional distinction: the `Ma`
anchor can be on the conjunction while the actual license is on one of the
conjoined adjectives.

### Implementation

The dictionary now separates bare predicative/conjoined adjective paths from
postposed-adjective paths that carry a local license.

The helper macro:

```text
POST_ADJ_LIC(base, license)
```

keeps the old predicative and right-conjunction alternatives while making the
`Ma-` and `dMJXr-` postposed-adjective variants require the specified
complement expression. Common postposed PP complements such as `angry about
the economy`, `loyal to Hussein`, and `heavy with sadness` are exposed through
explicit `Ma-` / `MJX` helper paths.

For conjoined postposed adjectives, conjunction entries use `MJXl` and `MJXr`
connector variants when they provide the `Ma-` anchor. These variants are
exposed only by adjective paths that carry a complement license, so the old PP
condition is now represented by dictionary connector structure.

The neighboring comparative `Mam` rule 64 remains active.

### Implications

This is a structural replacement for rule 63. Ordinary adjectives remain in
their word classes, but common bare `Ma` postposed-adjective fragments are no
longer generated unless the postposed use has a local complement license.

The change does not try to solve every semantic question about adjective
plausibility; it encodes the syntactic licensing condition that the old PP rule
checked.

### Examples

Focused accepted examples include:

```text
I need something useful.
The apartment available is small.
A man proud of his work arrived.
Voters angry about the economy will probably vote for Clinton.
Many Democrats unhappy about the economy but doubtful that Clinton can be elected probably won't vote at all.
We need a programmer knowledgeable about Lisp.
It is believed that even the troops loyal to Hussein will soon be forced to surrender.
He cried, his heart heavy with sadness.
```

Focused rejected examples include:

```text
*A gift inexpensive arrived.
*A child little arrived.
*A man proud arrived.
*Many Democrats unhappy but doubtful probably won't vote at all.
```

### Verification

The rule 63 migration should be validated with ordinary parser runs:

```sh
link-parser < ./data/en/corpus-knowledge.batch
link-parser < ./data/en/corpus-basic.batch
link-parser < ./data/en/corpus-fixes.batch
link-parser < ./data/en/corpus-fix-long.batch
```

## Rule 65: Remove Overbroad Postnominal `MX#a` Check

**Status:** implemented; the PP rule has been removed from `4.0.knowledge`.

### Rule / Area

The removed PP rule was:

```text
MX#a , TO TOf TH MVp TOt QI OF MVt MVz MVh Ytm Ya MJ E EA ,
       "Bad use of adjective65"
```

The grammatical area is postnominal and parenthetical adjective modification.

### Problem

The old rule rejected any PP domain containing `MX#a` unless the same domain
also contained one of the listed complement or modifier links. The source
comment already noted that this blocked good cases such as:

```text
The dog, unsatisfied, barked loudly.
```

The fixes corpus has the same concern for:

```text
The lady, unpleased, spoke sharply.
The lady, displeased, spoke sharply.
```

### Old Mechanism

The dictionary built the `MX#a` relation and PP later required a separate
licensing link in the same domain. That was too strict for comma-delimited
parenthetical adjective uses, where the adjective itself is the intended
modifier.

### Implementation

The `adjective65` PP rule is removed from `4.0.knowledge`. No dictionary
connector change is applied for this rule.

This differs from a connector-level migration: the rule was removed because it
was already documented as overbroad and the tracked fixes corpus contains good
sentences that should not be rejected by it.

### Implications

This removal does not prove that every possible `MX#a` analysis is correct. It
removes a PP check that rejected documented good grammar. Any remaining bad
postnominal-adjective analyses should be handled by more precise dictionary
work, not by restoring this broad rule.

### Examples

Focused examples include:

```text
The lady, unpleased, spoke sharply.
The lady, displeased, spoke sharply.
The dog, unsatisfied, barked loudly.
```

### Verification

The rule 65 removal should be validated with ordinary parser runs:

```sh
link-parser < ./data/en/corpus-knowledge.batch
link-parser < ./data/en/corpus-basic.batch
link-parser < ./data/en/corpus-fixes.batch
link-parser < ./data/en/corpus-fix-long.batch
```

## Rule 66: Split Non-Pronoun Second Objects From `Oxn`

**Status:** implemented; the PP rule has been removed from `4.0.knowledge`.

### Rule / Area

The removed PP rule was:

```text
Oxn , JUNK , "Bad use of pronoun66"
```

The grammatical area is ditransitive-like verb frames and related object
complement paths. A second object in these frames may be a noun, proper name,
time expression, or similar non-pronominal object, but it should not be an
`Ox-` pronoun object.

### Problem

The dictionary used the broad `O*n+` connector family in many second-object
positions. That connector family also matched `Ox-`, so raw linkages could
place a pronoun in a second-object position that English word order does not
license.

### Old Mechanism

The old PP rule used a deliberately impossible criterion:

```text
Oxn , JUNK , "Bad use of pronoun66"
```

There was no matching `JUNK` connector in the dictionary, so every occurrence
of an `Oxn` link was rejected after extraction. This acted as a negative
connector constraint, but it left the parser free to build and extract the bad
raw linkage first.

### Overgeneration Cause

The overgeneration was caused by using one broad connector family for two
different object roles. The same `O*n+` spelling was convenient for noun-like
second objects but too broad for pronouns, because `Ox-` belongs to the same
matched family at the link-name level.

### Implementation

The dictionary now defines a narrow helper macro:

```text
<obj2-non-pronoun>: On+ or Omn+ or Opn+ or Osn+ or Otn+ or Oun+ or Oyn+;
```

Second-object positions that previously used standalone `O*n+` now use
`<obj2-non-pronoun>`. The unrelated `dCO*n+` family is unchanged. This keeps
the intended noun/name/time/mass second-object behavior while excluding
pronoun `Ox-` from those positions.

### Implications

This is a direct dictionary replacement for rule 66. It is intentionally
conservative: the helper macro preserves the object families that were needed
for the existing second-object uses and removes only the pronoun path that the
old PP rule rejected.

### Examples

Focused accepted examples include:

```text
I gave him Mary.
I gave him a rose.
I gave him for his birthday a very expensive present.
Please paint it all white.
John made himself familiar with the drawings.
You should hear him sing.
```

Focused rejected examples include:

```text
*I gave Mary him.
*I gave my brother it.
*I gave him for his birthday it.
```

### Verification

The rule 66 migration should be validated with ordinary parser runs:

```sh
link-parser < ./data/en/corpus-knowledge.batch
link-parser < ./data/en/corpus-basic.batch
link-parser < ./data/en/corpus-fixes.batch
link-parser < ./data/en/corpus-fix-long.batch
```

## Rules 10 And 11: `Mj` / `MX#j` Require `Jw` Or `JQ`

**Status:** implemented; both PP rules have been removed from
`4.0.knowledge`.

### Rule / Area

The removed PP rules were:

```text
Mj   , Jw JQ , "Incorrect relative10"
MX#j , Jw JQ , "Incorrect relative11"
```

The grammatical area is preposition-object relatives. A relative preposition
continuation using `Mj` or `MX#j` should be licensed by a wh-object relation
or by the JQ question/relative path. The removed PP rules used the historical
subscripted name `Jw`; the dictionary replacement uses the distinct uppercase
connector family `JW` for the wh-object relation.

### Problem

The old generic preposition continuation included the relative branch directly
inside `<prep-main-a>`. As a result, ordinary preposition object branches could
select a relative continuation without locally requiring the wh-object witness.

### Old Mechanism

The relevant old structure was:

```text
<prep-main-a>:
  <prep-main-b>
  or (<subcl-verb> & (Mj- or (Xd- & Xc+ & MX*j-)))
  or ...
```

Many prepositions then used broad object alternatives containing
`<prep-main-a>`. PP rules 10 and 11 rejected raw linkages where `Mj` or
`MX#j` appeared without `Jw` or `JQ`.

### Overgeneration Cause

The dictionary mixed ordinary preposition continuations and relative
preposition continuations in one macro. The relative branch could therefore be
selected by a preposition object path that did not also prove the presence of
the wh-object relation.

### Implementation

The generic continuation was split:

```text
<prep-main-rel>:
  <subcl-verb> & (Mj- or (Xd- & Xc+ & MX*j-));

<prep-main-no-rel>:
  <prep-main-b>
  or (JQ+ & Wj- & Qp+)
  or <fronted>;
```

Preposition entries now use `<prep-main-no-rel>` for ordinary continuations
and add an explicit `JW+ & <prep-main-rel>` alternative when the relative
continuation is allowed. JQ-bearing alternatives use `<prep-main-jq-a>`, which
also includes `<prep-main-rel>` as a companion path.

The `of` entry is deliberately narrower: it continues to exclude broad `JW+`
relative use and keeps only its existing JQ relative continuation.

### Implications

This is a complete dictionary replacement for rules 10 and 11. The change is
local to preposition continuation structure and preserves the final PP check as
authoritative for remaining rules.

### Examples

Focused examples include:

```text
The friend for whom Joe works is kind.
The subject about which Mary spoke was difficult.
The subject, about which Mary spoke, was difficult.
That is the man for whom and with whom Joe works.
The book of which I spoke was old.
```

### Verification

The rules 10 and 11 migration was validated with:

```sh
link-parser < ./data/en/corpus-knowledge.batch
link-parser < ./data/en/corpus-basic.batch
link-parser < ./data/en/corpus-fixes.batch
link-parser < ./data/en/corpus-fix-long.batch
```

Expected results:

```text
corpus-knowledge.batch: 0 errors
corpus-basic.batch: 88 errors
corpus-fixes.batch: 362 errors
corpus-fix-long.batch: 9 errors
```
