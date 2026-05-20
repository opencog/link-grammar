# English Grammar Fixes

This file documents nontrivial grammar fixes in the English dictionary. It is
intended for maintainers who need more context than a source comment or commit
message can carry: the linguistic problem, the previous connector mechanism,
the source of overgeneration, the replacement strategy, and the validation
evidence.

The entries below describe only grammar work reflected by the accompanying
code and dictionary changes.

## Changes To The English-Language Link Types

This addendum summarizes English connector families and subscripted connector
forms added or retired by the grammar changes documented below. It is not a
replacement for the full Guide-to-Links; it records the link-type consequences
of these specific dictionary changes.

In the examples below, `h` and `d` prefixes are direction/dependency prefixes
on a connector occurrence and are not part of the uppercase connector family.

### Short Summary

Added uppercase connector families:

| Connector | Summary |
| --- | --- |
| `JW` | Connects a preposition to a wh noun-phrase object. This replaces the old subscripted `Jw` spelling so ordinary `J+` object branches cannot match wh objects. |
| `WJI` | Internal helper-token connector used by wh-preposition questions. `WJIa` ties the preposition to the helper token; `WJIb` ties the wh word to the same helper token. |
| `OFJ` | Certifies that an `of`-relative `Jr` path is tied to a corresponding postnominal `B#j` anchor. |
| `MJX` | Licensed conjoined postnominal-adjective helper family. `MJXl` and `MJXr` are used when one conjoined adjective supplies the complement license for the postnominal `Ma` or comparative `Mam` relation. |
| `MVSWH` | Connects a verb to subordinate temporal `as` in the `as.#while` path. This keeps temporal `as` distinct from ordinary broad `MVs` modifiers. |
| `CMPS` | Certifies a singular or mass comparative antecedent for a following singular comparative clause. |
| `CMPP` | Certifies a plural comparative antecedent for a following plural comparative clause. |
| `CMPX` | Certifies an agreement-neutral comparative head, such as bare `more` or a comparative preposition object. |
| `CMPC` | Certifies that a comparative modifier path licenses a following `Cc` / `CV` comparative clause. |
| `THBS` / `THBI` | Certify direct and inverted subject links for `THb` predicate that-clause complements. |
| `ITHB` / `PPTHB` / `PVTHB` | Carry the `THb` predicate license across modal, perfect, and passive auxiliary chains. |
| `INSERTL` / `INSERTR` | Tokenizer-only marker connector families. Paired `INSERTL<token>+` and `INSERTR<token>+` request an optional internal helper token named `<token>`. They are dictionary support markers, not ordinary grammar links. |

Changed or retired connector forms:

| Connector form | Change |
| --- | --- |
| `Jw` | Retired in favor of `JW`. The old spelling was a subscripted `J` form and could therefore be matched by broad ordinary `J+` branches. |
| `WJIa`, `WJIb` | Added as subtypes of `WJI` for the two sides of the `wjqprep` helper-token path. |
| `MJXl`, `MJXr` | Added as subtypes of `MJX` for left and right conjoined postnominal-adjective licensing; now also used by comparative `Mam` paths. |
| `O*n` in second-object positions | Replaced by the explicit non-pronoun set `On`, `Omn`, `Opn`, `Osn`, `Otn`, `Oun`, and `Oyn`. This preserves noun-like second objects while excluding pronoun `Ox` from those positions. |
| broad postposed `Ma` / `Mam` paths | Tightened so postposed adjective and comparative-adjective uses require a local complement license or a licensed `MJX` conjunction path. `Ma` and `Mam` remain ordinary connectors elsewhere. |
| subordinate temporal `as` with `MVs` | The `as.#while` temporal path now uses `MVSWH` instead of `MVs`. Other `MVs` uses remain ordinary modifiers. |
| comparative-clause `S**c` on `than.e` / `as.e-c` | Split into singular `Ss*c` and plural `Sp*c` branches that require `CMPS`, `CMPP`, or `CMPX` antecedent certificates. |
| comparative-clause `Cc` / `CV` on `than.e` / `as.e-c` | Tightened so clausal comparative continuations require a `CMPC` certificate from a local comparative modifier path. |
| `THb` predicate licensing | Split away from broad copular `be` paths. `THb` predicates now require `THBS`/`THBI` directly or an auxiliary-chain certificate through `ITHB`, `PPTHB`, or `PVTHB`. |
| naked `I*a+` on `to.r` | Removed from the affected `to.r` branch so infinitival `to` no longer has that unlicensed fallback path. The remaining rule-6 limitations are documented separately below. |
| `Jr` with `of` | No longer appears in the broad `of` object list. It is still available through the explicit `OFJ- & Jr+` path. |
| `U#t` | Stale PP-only selector from rule 55. The current English dictionary and link-type documentation do not define corresponding `U...t` connector forms, so this was not a retired dictionary connector. |

### `JW`: Wh Preposition Objects

`JW` connects a preposition to a wh noun-phrase object in questions and
prepositional relatives:

```text
-->Wj--+-JW-+
|      |    |
with.p what ...
```

Focused example:

```text
    +------------------Xp-----------------+
    +------------->WV------------->+      |
    |                  +----I*d----+      |
    +-->Wj--+-JW-+     +-SIp-+     +-Osm+ |
    |       |    |     |     |     |    | |
LEFT-WALL with what did.v-d you open.v it ?
```

The old `Jw` spelling made this relation a subscripted form of ordinary
prepositional-object `J`. Since broad `J+` connectors can match subscripted
`J` forms, ordinary preposition-object paths could attach to a wh object
without selecting the continuation that licenses a wh-preposition construction.
The distinct `JW` family prevents that match.

### `WJI`: Wh-Question Helper Token

`WJI` is an internal helper-token connector family for wh-preposition
questions. It is used with the optional helper token `wjqprep`:

```text
                +--Qp--+
       +--WJIb--+      |
       |        |      |
what wjqprep did.v ...
       |        |
with.p +--WJIa--+
```

The preposition supplies `WJIa+`, the wh word supplies `WJIb+`, and the helper
token supplies:

```text
WJIb- & WJIa- & Qp+
```

The displayed/API linkage suppresses the helper word and the incident `WJI`
links after postprocessing. Thus `WJI` is a dictionary-internal certificate
used to express the wh-preposition question relation; it is not intended to be
visible in ordinary output.

### `OFJ`: `of`-Relative Certification

`OFJ` ties the special `of`-relative object path to the postnominal relative
anchor that licenses it. It is used in cases such as:

```text
The doctors, many of whom are surgeons, were angry.
```

Focused linkage fragment, in `link-parser` graph style:

```text
    |             +--------------------Spx--------------------+
    |             |         +-------------Xc------------+     |
    +----->Wd-----+---MXp---+-----Bpj----+              |     |
    |      +--Dmc-+     +-Xd+OFJ+-Jr+-RS-+---Opt--+     |     +---Pa--+
    |      |      |     |   |   |   |    |        |     |     |       |
LEFT-WALL the doctors.n , many of whom are.v surgeons.n , were.v-d angry.a .
```

Here `many` carries the postnominal relative anchor (`Bpj`) and also links to
`of` through `OFJ`. The preposition `of` then links to the relative object
`whom` through `Jr`. This keeps `Jr` out of the broad ordinary object list for
`of`, while preserving the intended relative construction.

### `MJX`: Licensed Conjoined Postnominal Adjectives

`MJX` marks conjoined postnominal adjective paths in which at least one
adjective has the complement relation needed to license the postnominal `Ma`
or comparative `Mam` use. It parallels the existing `MJl`/`MJr` ordering
convention:

```text
MJXl  left-side licensed postnominal-adjective conjunction path
MJXr  right-side licensed postnominal-adjective conjunction path
```

Focused linkage fragment, in `link-parser` graph style:

```text
    |               +-------------------Ma------------------+
    |               |          +<-----------MJla<-----------+
    +------>Wd------+          |       +-----Js----+        |
    |       +--Dmc--+          +--MVp--+    +-Ds**v+        +-->MJXr->+----TH---+
    |       |       |          |       |    |      |        |         |         |
LEFT-WALL many Democrats.n unhappy.a about the economy.n but.j-m doubtful.a that.j-c ...
```

In this example, `doubtful` has the `TH` complement that licenses the
postnominal adjective construction. The `MJXr` link lets the conjunction carry
the `Ma` anchor without allowing an unlicensed bare postnominal adjective
path. The same mechanism is used for conjoined comparative `Mam` paths such as
`taller and wider than Bill`, where the `MVp` license is supplied by the right
comparative adjective. The mirror `MJXl` form is used when the left adjective
supplies the license.

### `MVSWH`: Temporal `as` Verb Modifier

`MVSWH` connects a verb to subordinate temporal `as` in examples such as:

```text
I slipped on the ice as I ran home.
```

Focused linkage fragment:

```text
slipped.v-d --MVSWH-- as.#while --Cs-- I.p --Sp*i-- ran.v-d
                               \--CV----------------/
```

The old temporal `as.#while` path used ordinary `MVs`. That allowed adjective
disjuncts with broad `@MV+` modifier slots to connect to temporal `as` while
also carrying a comparative `EAy` link. The dedicated `MVSWH` family keeps the
temporal subordinate use available to verbs without allowing comparative
adjectives to satisfy it accidentally.

### `CMPS`, `CMPP`, And `CMPX`: Comparative Clause Antecedents

`CMPS`, `CMPP`, and `CMPX` connect a comparative antecedent to the comparative
clause marker `than.e` or `as.e-c`. They are dictionary-side certificates for
the agreement condition formerly checked by PP rule 58.

Focused singular/mass example:

```text
    +----------MVt----------+
    +------Ou------+        |
    |       +-Dmum-+--CMPS--+-Ss*c-+
    |       |      |        |      |
spent.v-d more money.n-u than.e was.v-d
```

Focused plural example:

```text
    +--------------MVt-------------+
    +---------Op--------+          |
    |          +--Dmcm--+---CMPP---+--Spxc-+
    |          |        |          |       |
interviewed.v more programmers.n than.e were.v-d
```

`CMPX` is used when the antecedent is agreement-neutral, for example bare
comparative `more` as an object:

```text
spent.v-d --Omm-- more --CMPX-- than.e --Ss*c-- was.v-d
```

The connector families are intentionally distinct uppercase names rather than
subscripted variants of one `CMP` family. Broad connector matching would allow
subscripted variants such as `CMPs` and `CMPx` to match each other, which would
lose the agreement distinction.

### `CMPC`: Comparative Clause Modifier Certificate

`CMPC` connects a comparative modifier witness to a later `than.e` or `as.e-c`
clausal comparative continuation. It replaces the PP rule 56 requirement that a
domain containing `Cc` also contain one of the comparative modifier links
`EEm`, `EEy`, `MVm`, `MVb`, or `MVy`.

Focused `MVy` example:

```text
    +-------MVz-------+
    +----MVy----+     +----CV-->+
    |           +-CMPC+-Cc-+-Ss-+
    |           |     |    |    |
tastes.v the same as.e-c it did.v-d
```

`EAy` does not provide `CMPC`, so a plain adjective comparative such as
`as intelligent as John does` cannot use the `as.e-c --Cc/CV--` clause path.

### `THBS`, `THBI`, `ITHB`, `PPTHB`, And `PVTHB`: `THb` Predicate Licenses

These connector families encode the subject or auxiliary evidence required for
predicate that-clause complements:

```text
THBS   direct subject license for a THb predicate
THBI   inverted subject license for a THb predicate
ITHB   modal/infinitive auxiliary license carried to be
PPTHB  perfect auxiliary license carried to been
PVTHB  passive auxiliary license carried to made
```

Focused direct example:

```text
    +------------------>CPa------------------+
    |                            +-----CV--->+
    |      +-Ds**c+--THBS-+--THb-+-Cet-+--Ss-+-Xp-+
    |      |      |       |      |     |     |    |
LEFT-WALL the problem.n is.v that.j-c he lied.q-d .
```

Focused modal example:

```text
    +--------------------->CPa---------------------+
    |                                  +-----CV--->+
    |      +-Ds**c+--THBS-+-ITHB+--THb-+-Cet-+--Ss-+-Xp-+
    |      |      |       |     |      |     |     |    |
LEFT-WALL the problem.n may.v be.v that.j-c he lied.q-d .
```

Focused passive example:

```text
    +------------------------------Xp-----------------------------+
    +--------------->WV-------------->+                           |
    +----->Wd------+                  |        +----CV--->+       |
    |      +-Ds**v-+---THBS--+--PVTHB-+---THb--+-Cet-+-Ss-+-Osm-+ |
    |      |       |         |        |        |     |    |     | |
LEFT-WALL an allegation.n was.v-d made.v-d that.j-c he did.v-d it .
```

The dedicated families are uppercase connector families rather than
subscripted `S`, `I`, `PP`, or `Pv` forms. This is intentional: broad connector
matching would let an ordinary connector match a subscripted certificate and
make an unlicensed predicate appear licensed.

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
| `Pa##j` predicative adjective | Object licensing for predicative-adjective complements | A simple verb-side object macro for the PP-allowed objects is unsafe. Connector matching lets a subscripted positive connector such as `Os*e+` match an ordinary `Os-` noun and produce an allowed-looking `Os*e` link, so the macro admits the same bad singular-object paths that the PP rule was intended to reject. A replacement needs a true object-class certificate, a new exact connector family on the noun/pronoun side, or equivalent library-assisted dictionary support. |

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

The old concern in rule 6 was narrower than all possible bad `to.r` parses:
it concerned paths where `to.r` created an `I#a` link without a local
filler/gap witness. Other bad `to.r` or `going to` analyses can exist through
different `I` subscripts and are separate grammar issues.

For example, sentences such as:

```text
*He is going to do.
*He is going to have.
```

are not evidence that the rule-6 migration is incomplete unless their accepted
linkage uses `I#a`. In current focused checks, these sentences parse through
`I*d` or `If` paths, not through the removed `I#a` fallback.

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

## Rule 19: License `Qe` How-Adverb Questions In The Dictionary

**Status:** implemented; the PP rule has been removed from `4.0.knowledge`.

### Rule / Area

The removed PP rule was:

```text
Qe , EEh EAh , "Incorrect use of adverb19"
```

The grammatical area is `how`-licensed adjective and adverb questions, such as
`How quickly did Joe run?` and `How tall is he?`.

### Problem

The historical rule required any PP domain containing `Qe` to also contain an
`EEh` or `EAh` link. That is the right shape for direct `how` questions:

```text
how --EEh-- quickly --Qe-- did
how --EAh-- tall    --Qe-- is
```

Some current dictionary paths already carried that witness. However, the broad
ordinary-adverb expression also allowed a raw `Qe+` continuation from an
ordinary `EE-`/`EF-` modifier disjunct. After rule 19 was removed by itself,
that branch admitted lower-ranked parses such as:

```text
how --AF-- is
tall.e --Qe-- is
```

Those parses have a `Qe` question link but no `EEh` or `EAh` witness in the
same PP domain.

### Old Mechanism

PP checked the completed domain after extraction and rejected a `Qe` link if
no `EEh` or `EAh` link appeared in that domain. This made the rule a late
global safety check rather than part of the dictionary path that created the
question relation.

### Overgeneration Cause

The ordinary adverb macro mixed two uses in one expression. The direct
`EEh- & Qe+` branch correctly represents `how`-licensed adverb questions, but
the generic `EE-/EF-` branch also contained `Qe+`. That generic branch could
attach to a `Qe-` question verb while the apparent `how` word was connected
elsewhere, leaving PP to reject the completed linkage.

Some surface inversions that look related use `Qd`, not `Qe`; they are outside
the scope of this rule.

### Implementation

Rule 19 is removed from `4.0.knowledge`. The ordinary-adverb macro keeps its
existing direct `EEh- & {Qe+}` branch and removes the loose `Qe+` alternative
from the generic `EE-/EF-` branch. Thus an ordinary adverb can still form a
`Qe` question path when the same disjunct carries the `EEh` witness, but it no
longer creates a `Qe` path from an unrelated modifier relation.

This is not a general cleanup of `how` grammar. For example, a sentence such
as `*How slickly did you say it was?` already contains the `EEh` witness, so
rule 19 is not the mechanism that distinguishes it from accepted `how`
questions.

### Examples

Focused accepted examples include:

```text
How quickly did Joe run
How tall is he?
How tall did you say he was?
I know how quickly you ran
How much more quickly did you run
How often does it happen?
```

Focused rejected examples include:

```text
*How quickly Joe ran
*I know how quickly did you run
*Quickly did Joe run
*Very quickly did Joe run
*I know quickly did John run
```

### Verification

The rule 19 removal was validated with:

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
corpus-fixes.batch: 361 errors
corpus-fix-long.batch: 8 errors
```

Accepted-linkage comparison against the pre-migration baseline used
`-test=auto-next-linkage:3` with `!links`, `!limit=10000`, `!short=254`, and
`!null=0` to display the first three accepted linkages. The first three
accepted displayed linkages for `How tall is he?` and `How often does it
happen?` match the baseline public link rows exactly.

For `How quickly did Joe run?`, the first two accepted displayed linkages
match the baseline public link rows exactly. The third displayed linkage now
uses `quickly --Em-- run` instead of the baseline's duplicate low-cost
`quickly --Qe-- did` analysis. This is the expected consequence of removing
the loose ordinary-adverb `Qe+` path while preserving the direct
`EEh- & Qe+` how-question path.

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

Rules 71, 72, and 73 remain active because they were not part of this
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

## Rules 58 And 59: Comparative Clauses Require A Compatible Antecedent

**Status:** implemented; PP rules 58 and 59 have been removed from
`4.0.knowledge`.

### Rule / Area

The removed PP rules were:

```text
Ss#c , Dmum Dmuy Om Oy Jm Jy Ds*y MX#m , "Bad comparative58"
S##c , Dm#m D##y Om Oy Jm Jy MX#m , "Bad comparative59"
```

The grammatical area is comparative clauses introduced by `than.e` and
`as.e-c`. A comparative-clause subject link such as `Ss*c` or `Sp*c` should be
licensed by a compatible comparative antecedent in the same construction. A
singular comparative-clause subject should be licensed by a singular, mass, or
agreement-neutral antecedent; a plural comparative-clause subject should be
licensed by a plural or agreement-neutral antecedent.

### Problem

The old dictionary let `than.e` and `as.e-c` use a broad `S**c+` connector for
comparative clause subjects. That connector could match both singular and
plural verb-side subject connectors without any dictionary-side proof that the
antecedent had compatible number or mass behavior.

For example, the raw linkage for the rejected sentence below looked locally
well-formed:

```text
She interviewed more programmers than was hired.
```

The antecedent side used plural `Dmcm`/`Op`, while the comparative clause used
singular `Ss*c`. PP rule 58 rejected the completed linkage after extraction.
Rule 59 was the corresponding general check: any `S##c` comparative-clause
subject needed evidence of a comparative antecedent or related comparative
path in the same domain.

### Overgeneration Cause

The comparative marker was not connected to the antecedent evidence. The
`Dmum`/`Dmuy` and plural `Dmcm`/`Dmcy` links were already present on the
antecedent noun phrase, and object/preposition paths such as `Om` and `Jm`
were already visible elsewhere in the linkage, but the `S**c+` branch on
`than.e`/`as.e-c` did not require a local dictionary-side certificate for that
evidence.

### Implementation

The dictionary now adds explicit comparative antecedent certificates:

```text
CMPS  singular or mass antecedent
CMPP  plural antecedent
CMPX  agreement-neutral comparative head
```

Singular and mass noun-main paths can optionally carry `CMPS+`; plural
noun-main paths can optionally carry `CMPP+`. Bare comparative object and
preposition-object paths such as `more` with `Omm-` or `Jm-` can carry
`CMPX+`, because those paths were agreement-neutral under the PP rule.

The `S**c+` comparative-clause branch is split in both `than.e` and `as.e-c`:

```text
CMPS- ... Ss*c+
CMPP- ... Sp*c+
CMPX- ... Ss*c+ / Sp*c+
```

Both left-connector orders are present. In object comparatives, the antecedent
noun is closer to `than.e`/`as.e-c` than the modified verb; in subject
comparatives, the antecedent noun is farther away than the modified verb.

The clausal `than.e` continuation using `Cc+ & CV+` is also tied to the same
certificate family:

```text
CMPS-/CMPP-/CMPX- ... MVt- ... Cc+ ... CV+
MVt- ... CMPS-/CMPP-/CMPX- ... Cc+ ... CV+
```

This blocks raw paths such as `more elegant than yours works`, where the
comparative adjective had `MVt` but the `than.e` clause lacked a compatible
comparative antecedent certificate.

### Implications

This is a dictionary replacement for rules 58 and 59. The new
`CMPS`/`CMPP`/`CMPX` links are visible internal certificate links, so affected
top linkages differ from older master-style linkages by one additional
comparative certificate. The public grammar result is unchanged on the
ordinary English corpora tested.

### Examples

Focused accepted examples include:

```text
She interviewed more programmers than were hired.
She spent more money than was budgeted.
She spent less money than was budgeted.
She spent more than was budgeted.
She spoke to more than was expected.
She interviewed as many programmers as were hired.
More people came to the party than were expected.
More people came to the party than was expected.
Our program was better than was expected.
```

Focused rejected examples include:

```text
*She interviewed more programmers than was hired.
*She interviewed as many programmers as was hired.
*Our program was better than were expected.
```

### Verification

The rule 58 and 59 migration was validated with:

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
corpus-fixes.batch: 361 errors
corpus-fix-long.batch: 8 errors
```

## Rule 56: Comparative `Cc` Clauses Require A Modifier License

**Status:** implemented; PP rule 56 has been removed from `4.0.knowledge`.

### Rule / Area

The removed PP rule was:

```text
Cc , EEm EEy MVm MVb MVy , "Bad comparative56"
```

The grammatical area is clausal comparative continuations introduced by
`as.e-c` and `than.e`, where `Cc` links the comparative marker to the subject
of the following clause and `CV` links it to the clause verb.

### Problem

The old dictionary allowed `as.e-c` to take a raw `MVz- & Cc+ & CV+` branch
without proving that the preceding phrase had a comparative modifier license.
For example:

```text
*I am as intelligent as John does.
```

could use `as.e-y --EAy-- intelligent.a` and
`intelligent.a --MVz-- as.e-c --Cc/CV-- John does`. The linkage was locally
well-formed, but `EAy` was not one of the links that licensed a `Cc` comparative
clause under rule 56.

### Implementation

The dictionary now uses the internal certificate connector `CMPC`. The
dictionary-side counterparts of the old PP criteria can optionally supply
`CMPC+`:

```text
EEm
EEy
MVm
MVb
MVy
```

The `as.e-c` `Cc+ & CV+` branch requires `CMPC-` in either left-connector order
with `MVz-`. The `than.e` `Cc+ & CV+` branch also requires `CMPC-`, in addition
to the `CMPS` / `CMPP` / `CMPX` antecedent certificates introduced for rules 58
and 59.

### Implications

This is a dictionary replacement for rule 56. Valid clausal comparatives such
as `the same as it did` acquire an internal `CMPC` link. Plain adjective
comparatives with `EAy`, such as `as intelligent as John does`, do not acquire
that certificate and therefore cannot use the `as.e-c --Cc/CV--` path.

### Examples

Focused accepted examples include:

```text
The coffee tastes the same as it did last year.
He runs as quickly as John does.
I earn as much money in a month as John earns in a year.
Ours works more elegantly than yours does.
You are as authoritative as he is.
```

Focused rejected examples include:

```text
*I am as intelligent as John does.
```

### Verification

The rule 56 migration was validated with:

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
corpus-fixes.batch: 361 errors
corpus-fix-long.batch: 8 errors
```

## Rule 78: Separate Temporal `as` From Comparative `EAy`

**Status:** implemented; the PP rule has been removed from `4.0.knowledge`.

### Rule / Area

The removed PP rule was:

```text
EAy , MVs , "Bad comparative78"
```

The grammatical area is comparative `as ... as` constructions. The relevant
valid pattern is:

```text
I am as intelligent as John.
```

with `as.e-y --EAy-- intelligent.a` and the adjective linking to the second
`as` through an ordinary comparative continuation such as `MVp`.

### Problem

The broad temporal `as.#while` path also used `MVs`. Since many adjective
disjuncts expose broad `@MV+` modifier slots, a comparative adjective could
accidentally connect to temporal subordinate `as` while also carrying the
first comparative `EAy` link:

```text
*I am as intelligent as John does.
```

The bad raw path has this shape:

```text
as.e-y --EAy-- intelligent.a --MVs-- as.#while --Cs/CV-- John does
```

### Old Mechanism

The old PP rule rejected any completed domain containing both `EAy` and
`MVs`. This worked as a backstop but left the parser and ordered extractor to
generate the bad comparative/temporal combination first.

### Overgeneration Cause

The dictionary reused ordinary `MVs` for the temporal `as.#while` branch:

```text
MVs- & Cs+ & CV+
```

That made temporal `as` look like an ordinary broad modifier target. The
problem is not that temporal `as` is ungrammatical; it is that this temporal
branch must be available to verbs but not to comparative adjectives that are
being licensed by `EAy`.

### Implementation

Temporal `as.#while` now uses the dedicated verb-side connector family
`MVSWH`:

```text
verb --MVSWH-- as.#while --Cs/CV-- subordinate clause
```

The common verb modifier path exposes optional `MVSWH+`, while adjective
`@MV+` slots do not match this distinct uppercase family. Valid temporal
sentences such as:

```text
I slipped on the ice as I ran home.
```

therefore keep a zero-null parse, while comparative adjectives can no longer
satisfy temporal `as.#while` by accident.

This migration does not claim to solve every bad sentence involving `as`. For
example, a bad sentence that lacks `EAy` is outside the scope of rule 78 and
needs separate grammar analysis.

### Implications

This removes one complete comparative PP check from the blocker set and makes
the relevant condition visible to dictionary parsing instead of postprocessing.
It also makes the surface link for temporal `as.#while` more specific:
`MVSWH` replaces `MVs` on that path.

### Verification

Focused examples:

```text
I am as intelligent as John.
You are as sweet as sugar.
Any program as good as ours should be useful.
The coffee tastes the same as it did last year.
I slipped on the ice as I ran home.
*I am as intelligent as John does.
```

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
corpus-fixes.batch: 361 errors
corpus-fix-long.batch: 8 errors
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

## Rule 40: License `THb` Predicate Complements In The Dictionary

**Status:** implemented; PP rule 40 has been removed from `4.0.knowledge`.

### Rule / Area

The removed PP rule was:

```text
THb , S##t SI##t SFsi SFIsi , "Bad use of predicate40"
```

The grammatical area is predicate that-clause complements, as in:

```text
The problem is that he lied.
The problem may be that he lied.
An allegation was made that he did it.
```

A `THb` predicate complement should be licensed by the subject or inverted
subject construction that carries the relevant that-clause predicate relation,
or by the corresponding auxiliary chain.

### Problem

The old dictionary exposed `THb+` through broad copular and passive paths. PP
then checked the completed domain for a subject or filler-subject link whose
name matched `S##t`, `SI##t`, `SFsi`, or `SFIsi`. This allowed many locally
well-formed raw linkages to be constructed before PP could reject them.

Simple PP-rule removal accepted bad predicate uses such as:

```text
*How likely is John that he will come.
*How tired are you that John is coming.
*It is more likely that Joe died than John is that Fred died.
```

An initial dictionary attempt used an optional companion certificate link on
the subject disjunct. That was unsafe: a filler `it` in one clause could send
the companion link to an unrelated later copula, thereby licensing a bad
comparative-clause `THb` path. The final replacement makes the certificate the
actual subject or auxiliary link, not a separate optional companion.

### Implementation

That-clause nouns, `what`, `all`, and filler `it` now expose dedicated
predicate-license subject connectors:

```text
THBS+   direct licensed subject
THBI-   licensed inverted subject
```

The corresponding copular and auxiliary branches consume:

```text
THBS-   direct licensed subject on the predicate verb
THBI+   licensed inverted subject on the predicate verb
```

The broad `THb+` alternatives were removed from ordinary `be` and passive
branches and moved into `vc-be-thb` variants that require these licensed
subject paths. Auxiliary chains use dedicated uppercase certificate links:

```text
subject --THBS-- modal --ITHB-- be --THb-- that-clause
subject --THBS-- have --PPTHB-- been --THb-- that-clause
subject --THBS-- was --PVTHB-- made --THb-- that-clause
```

These are distinct uppercase connector families rather than subscripted
variants such as `S*t`, `I*t`, or `PP*t`. Link Grammar connector matching would
allow broad ordinary connectors to match subscripted variants, so subscripted
certificates cannot safely encode this condition.

### Implications

This is a dictionary replacement for rule 40. The accepted linkages are not
byte-for-byte identical to older master-style output: valid `THb` predicates
now show `THBS`, `THBI`, `ITHB`, `PPTHB`, or `PVTHB` certificate links where
master used ordinary `Ss*t`, `I`, `PP`, or `Pvf` links. The change is
intentional because those ordinary families could not safely enforce the
predicate license before postprocessing.

### Verification

Focused accepted-linkage comparison against `master` inspected the first three
displayed accepted linkages for:

```text
The problem is that he lied.
The problem may be that he lied.
An allegation was made that he did it.
```

The same `THb` constructions remain accepted. The expected differences are the
new explicit certificate links:

```text
Ss*t  -> THBS
I     -> ITHB
PP    -> PPTHB
Pvf   -> PVTHB
```

The focused bad examples above still have no accepted zero-null linkage in the
examined set; their remaining raw parses are rejected by other active PP
checks such as the S-V inversion and cycle rules.

The rule 40 migration was validated with ordinary parser runs:

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
corpus-fixes.batch: 361 errors
corpus-fix-long.batch: 8 errors
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

The related `BIq` predicate rule is not removed here; it requires separate
focused validation. The `BIh` rule is different in the current grammar: after
removing it, the agreed corpus results remain unchanged, and the tested `BIh`
paths are already constrained by the dictionary branches that expose them.

### Old Mechanism

PP rejected `BIh` domains that lacked one of the listed subject or
filler-subject links. That was a safety check for broader historical
predicate/clause paths.

### Implementation

The `predicate41` PP rule is removed from `4.0.knowledge`. No dictionary
connector change is needed for this rule in the current grammar.

### Implications

This removal applies only to `BIh`. It does not imply that the neighboring
`BIq` predicate rule is redundant.

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

## Rule 64: Comparative Postposed `Mam` Adjectives Require A License

**Status:** implemented; the PP rule has been removed from `4.0.knowledge`.

### Rule / Area

The removed PP rule was:

```text
Mam , TO TOf TH MVp TOt QI OF MVt MVz MVh Ytm Ya ,
      "Bad use of adjective64"
```

The grammatical area is postnominal comparative adjective modification, as in
`a man taller than John` or `a box bigger than this`.

### Problem

The dictionary exposed `Mam-` in the same broad comparative-adjective groups
as ordinary comparative predicative and conjunction paths. This allowed raw
postnominal comparative fragments such as:

```text
man --Mam-- taller
box --Mam-- bigger
```

without requiring the comparative adjective to supply the relation that makes
the postnominal use grammatical.

### Old Mechanism

PP accepted the completed linkage only if the same domain contained one of the
license links listed in rule 64. Common good fragments include:

```text
man --Mam-- taller --MVp-- than
box --Mam-- bigger --MVp-- than
```

For conjoined comparatives, the `Mam` anchor can be on the conjunction while
the license is on one of the conjoined adjectives:

```text
man --Mam-- and --MJXr-- wider --MVp-- than
```

### Overgeneration Cause

The overgeneration was the comparative counterpart of rule 63. A valid
postnominal comparative adjective needs a local comparative or complement
license, but the earlier `Mam` connector path did not require that license
when the `Mam` link was formed.

Conjoined comparative adjectives require the same additional witness path as
conjoined ordinary postposed adjectives: the conjunction may carry the `Mam`
anchor while the actual license is supplied by the left or right adjective.

### Implementation

The dictionary now requires `Mam-` paths to carry a local rule-64 license
directly. The helper expression:

```text
<comp-post-adj-license>
```

enumerates the dictionary-side counterparts of the former PP criteria:
comparative modifier links such as `MVp`, `MVt`, `MVz`, and `MVh`;
infinitival, clausal, interrogative, and `of` complements; and measurement
links such as `Ytm` and `Ya`.

The comparative helper macros:

```text
COMP_ADJ_POST(base)
COMP_ADJ_POST_LIC(base, license)
```

preserve the ordinary comparative adjective paths while making the `Mam-`
variant require the relevant license. The same macros expose `MJXl` and
`MJXr` variants for conjoined comparative adjectives, so a conjunction can
carry `Mam-` only when one of the conjoined adjectives supplies the license.

### Implications

This is a structural replacement for rule 64. The dictionary no longer builds
unlicensed postnominal comparative `Mam` fragments that PP would later reject.
The change reuses the existing `MJX` licensed-conjunction family rather than
adding a new public connector type.

### Examples

Focused accepted examples include:

```text
I saw a man taller than John.
The man taller than John arrived.
I need a box bigger than this.
A box bigger than this arrived.
A man taller and wider than Bill arrived.
```

Focused rejected examples include:

```text
*I saw a man taller.
*The man taller arrived.
*A man bigger quickly arrived.
*A man taller and wider arrived.
```

### Verification

The rule 64 migration should be validated with ordinary parser runs:

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
