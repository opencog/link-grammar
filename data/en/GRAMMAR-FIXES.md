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

## Library-Assisted Dictionary Helper Tokens

**Status:** implemented as dictionary support; used by the preposition
continuation changes for rules 12, 13, and 10/11 when the `Wj` companion path
is needed.

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
word supplies `Jw- & WJIb+`, and the preposition supplies `Wj- & WJIa+`.
Rules 13 and 10/11 do not require every valid linkage to use the helper:
their replacements also use direct `MVp`, `Mj`, `MX#j`, `Jw`, and `JQ`
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

At the time of the relevant commits, expected results were:

```text
corpus-knowledge.batch: 0 errors
corpus-basic.batch: 88 errors
corpus-fixes.batch: 367 errors
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
licensed by the corresponding wh object (`Jw`) or by the question-preposition
path (`JQ`). Without this witness, the dictionary can generate raw linkages in
which a fronted preposition is syntactically detached from the wh construction
that makes it grammatical.

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
The second path uses the `wjqprep` helper token:

```text
preposition --WJIa-- wjqprep --WJIb-- wh-word
wjqprep --Qp-- question-verb
preposition --Wj-- wh-word
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

Focused coverage is in `data/en/corpus-knowledge.batch`. Before removing the
PP rule, the replacement was checked by suppressing rule 12 while disabling
metric extraction:

```sh
link-parser -test=noPP:12,no-metric-extraction < ./data/en/corpus-knowledge.batch
link-parser -test=noPP:12,no-metric-extraction < ./data/en/corpus-basic.batch
link-parser -test=noPP:12,no-metric-extraction < ./data/en/corpus-fixes.batch
link-parser -test=noPP:12,no-metric-extraction < ./data/en/corpus-fix-long.batch
```

After removal, the focused corpus should pass with ordinary parsing:

```sh
link-parser < ./data/en/corpus-knowledge.batch
```

is expected to report:

```text
0 errors
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
corpus-fixes.batch: 367 errors
corpus-fix-long.batch: 9 errors
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
(`Jw`) or by the JQ question/relative path.

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
  or (Wj- & WJIa+)
  or <marker-wjqprep-left>
  or <fronted>;
```

Preposition entries now use `<prep-main-no-rel>` for ordinary continuations
and add an explicit `Jw+ & <prep-main-rel>` alternative when the relative
continuation is allowed. JQ-bearing alternatives use `<prep-main-jq-a>`, which
also includes `<prep-main-rel>` as a companion path.

The `of` entry is deliberately narrower: it continues to exclude broad `Jw+`
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
corpus-fixes.batch: 367 errors
corpus-fix-long.batch: 9 errors
```
