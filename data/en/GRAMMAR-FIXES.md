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

Several added families below are certificate or carrier links. They are
public-visible grammatical links, but their primary role is to encode a
licensing or agreement state in the dictionary so that a completed-linkage PP
scan is no longer needed. They should therefore be read as grammar-engineering
support for a real syntactic constraint, not as ordinary relation names like
`S`, `O`, or `MV`.

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
| `CMPO` | Certifies that a comparative object or measure path licenses a following `MV#o` object-clause comparative continuation. |
| `WTHAN` | Connects `way` / `ways` nouns to a following `than to ...` infinitival comparative. |
| `THBS` / `THBI` | Certify direct and inverted subject links for `THb` predicate that-clause complements. |
| `ITHB` / `PPTHB` / `PVTHB` | Carry the `THb` predicate license across modal, perfect, and passive auxiliary chains. |
| `TSIC` | Certifies that a `TSi` subjunctive that-clause predicate is licensed by filler/expletive `it`. |
| `TOIC` | Certifies that a `TOi` infinitive-complement predicate is licensed by filler/expletive `it`. |
| `THIC` | Certifies that a `THi` that-clause predicate is licensed by direct filler/expletive `it` evidence. |
| `TTHI` | Carries object-raising `THi` evidence from an object-raising predicate to infinitival `to`. |
| `ITHI` | Carries `THi` evidence across infinitival and inverted auxiliary paths. |
| `PPTHI` | Carries `THi` evidence across perfect auxiliary paths. |
| `PTHI` | Connects an auxiliary carrier to a lower predicate that owns the `THi` complement. |
| `TOCL` | Carries object-raising cleft-object evidence from an object-raising predicate to infinitival `to`. |
| `IOCL` | Carries cleft-object evidence across infinitival `to` and inverted auxiliary paths. |
| `PPOCL` | Carries cleft-object evidence across perfect auxiliary paths. |
| `ROCL` | Certifies the special cleft-object path where the licensing filler `it` appears inside the following clause. |
| `QIIC` | Certifies that a `QIi` question-clause predicate is licensed by filler/expletive `it` on direct subject, inverted subject, and object-complement paths. |
| `TQII` | Carries object-raising `QIi` evidence from an object-raising predicate to infinitival `to`. |
| `IQII` | Carries the same object-raising `QIi` evidence from infinitival `to` to the lower predicate. |
| `PQII` | Connects the lower predicate to a `QIi` adjective in object-raising `it to be ...` paths. |
| `CIIC` | Certifies that a `Ci` finite-clause predicate is licensed by filler/expletive `it` on direct subject, inverted subject, and object-complement paths. |
| `TCII` | Carries object-raising `Ci` evidence from an object-raising predicate to infinitival `to`. |
| `ICII` | Carries the same object-raising `Ci` evidence from infinitival `to` to the lower predicate. |
| `PCII` | Connects the lower predicate to a `Ci` adjective in object-raising `it to be ...` paths. |
| `BIQS` / `BIQI` | Certify direct and inverted subject links for `BIq` predicate wh-complements. |
| `IBIQ` / `PPBIQ` | Carry the `BIq` predicate license across modal and perfect auxiliary chains. |
| `EEXK` | Certifies degree-modified adverbial result-clause targets, such as `so quickly that ...` and `so much that ...`. |
| `EAXK` | Certifies degree-modified adjective result-clause targets, such as `so big that ...`. |
| `DTHAT` | Certifies `such` / `such a` / `such an` noun phrases that license a following result-clause `that`. |
| `RTHAT` | Carries a result-clause certificate from an adverbial or noun head to `that.j-c` while `MVh` keeps the clause attached to the modified predicate. |
| `MVH` | Connects an adjective result-clause head directly to `that.j-c`, replacing the old `MVh` path for certified adjective result clauses. |
| `SJI` | Connects the logical subject of a controlled bare infinitive to the infinitival verb, paired with `I*j`. |
| `SGP` | Connects the logical subject of a controlled present participle to the participial verb, paired with `Pg`. |
| `NDH` | Connects an `H`-licensed quantity word to a unit noun in wh/degree `B#m` extraction, preventing ordinary `ND` quantity paths from licensing the extraction. |
| `DWH` | Wh/degree determiner-certificate family. `DWHs`, `DWHp`, and `DWHu` carry singular, plural, and uncountable `D##w` / `H` evidence to `B#m` extraction nouns. |
| `ECWH` | Carries an `H` wh-quantity certificate through `more` to a following `DWH` or `BWH` extraction target. |
| `BWH` | Carries an `HA` wh/degree certificate from a degree word or comparative `more` to a following `B#m` extraction noun. |
| `HWS` | Certifies `how` quantity phrases that start a `Ws` subject question and therefore must not use extracted `B#m` or non-inverted `Ca` continuations. |
| `DWS` | Determiner certificate used by `HWS` uncountable subject questions, such as `How much sugar is needed?`; currently represented by `DWSu`. |
| `EEHWS` | Certifies the narrow `how long before ...` temporal-fragment path as a `Ws` question without allowing the broader `EEh + Ca` overgeneration. |
| `MVZP` | Adjective-only certificate for predicative/participial parenthetical `as` clauses such as `unclear as worded`. It keeps the `as.e-c` `Pa` branch away from ordinary verb-side `MVzp` paths. |
| `RWB` | Certifies that a bare-`what` `Wb` opener is tied to an inverted verb question path. |
| `OAJ` | Connects a verb to a non-expletive object that is allowed to license a `Pa**j` predicative-adjective complement. |
| `THR{S,P,U}` | Certifies singular, plural, and uncountable existential-`there` agreement. The brace notation summarizes the concrete link types `THRS`, `THRP`, and `THRU`. |
| `TTHR{S,P,U}` | Carries the same existential-`there` agreement state from a raising predicate to the following infinitival `to`. |
| `ITHR{S,P,U}` | Carries the same agreement state from infinitival `to`, modal, or auxiliary paths to the next predicate. |
| `PGTHR{S,P,U}` | Carries the same agreement state through `going to be` paths. |
| `PPTHR{S,P,U}` | Carries the same agreement state through perfect `have been` paths. |
| `PATHR{S,P,U}` | Carries the same agreement state through predicative adjective paths such as `likely to be`. |
| `IFI` | Certifies filler-`it` inverted auxiliary paths to a lower raising predicate. This separates expletive/filler inversion from ordinary `I` infinitival continuation. |
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
| comparative object-clause `MV#o` on `than.e` / `as.e-c` | Tightened so object-control comparative continuations require a `CMPO` certificate from a local comparative object or measure path. |
| `than to ...` infinitival comparative after `way` / `ways` | Added `WTHAN` so valid `way ... than to ...` comparatives do not depend on the retired naked `to.r I*a+` fallback. |
| `THb` predicate licensing | Split away from broad copular `be` paths. `THb` predicates now require `THBS`/`THBI` directly or an auxiliary-chain certificate through `ITHB`, `PPTHB`, or `PVTHB`. |
| `TSi` subjunctive that-clause predicate licensing | `TSi+` predicate branches now require a `TSIC` certificate from filler/expletive `it` when they select a subjunctive that-clause complement. Ordinary `THi` and `TOi` paths remain separate. |
| `TOi` infinitive-complement predicate licensing | `TOi+` predicate branches now require a `TOIC` certificate from filler/expletive `it`. Tough-subject infinitives continue to use separate paths such as `TOt`. |
| `THi` that-clause predicate licensing | `THi+` predicate branches now require direct `THIC` evidence from filler/expletive `it`, direct same-copula `SFsi`/`SFIsi` evidence for clefts, or a carrier path through `TTHI`/`ITHI`/`PPTHI` and `PTHI`. `THi` certificates deliberately exclude comparative `AF` predicate paths so an outer `it` cannot license a distant comparative clause. |
| cleft-object `O#i` paths | `Osi` / `Opi` cleft-object branches on `be` are no longer exposed through generic copular complement paths. They require direct `SFsi` / `SFIsi` evidence, an object-raising or auxiliary carrier through `TOCL`, `IOCL`, or `PPOCL`, or an `ROCL` path to a filler `it` inside the following clause. |
| `QIi` question-clause predicate licensing | `QIi+` predicate branches now require direct `QIIC` evidence from filler/expletive `it`, or an object-raising carrier path through `TQII`, `IQII`, and `PQII`. |
| `Ci` finite-clause predicate licensing | `Ci+` predicate branches now require direct `CIIC` evidence from filler/expletive `it`, or an object-raising carrier path through `TCII`, `ICII`, and `PCII`. The `Ci` certificate intentionally excludes comparative `AF` predicate paths so an outer `it` cannot license a distant comparative clause. |
| `BIq` predicate licensing | Split away from broad copular `be` paths. `BIq` predicates now require `BIQS`/`BIQI` directly or an auxiliary-chain certificate through `IBIQ` or `PPBIQ`. |
| result-clause `that.j-c` with `MVh` | Bare `MVh-` on `that.j-c` was replaced by certified paths. Adverbial and noun result clauses use `RTHAT- & MVh-`; adjective result clauses use `MVH-`. |
| result-clause `EExk` / `EAxk` / `D##k` witnesses | Replaced for `that` result clauses by uppercase certificate families `EEXK`, `EAXK`, and `DTHAT`. The older `EExk`, `EAxk`, and `D...k` forms remain available for ordinary non-result-clause degree and determiner uses. |
| controlled-subject `Sj` / `Sg` | Replaced by uppercase `SJI` and `SGP` in controlled bare-infinitive and present-participle constructions. Ordinary finite `S` connectors must not match these controlled-subject witnesses. |
| unit wh-extraction `ND` | The `how many <unit> ...` `B#m` extraction branch now uses `NDH` instead of ordinary `ND`. Ordinary quantity uses of `ND` remain unchanged. |
| wh/degree `D##w` paths for `B#m` extraction | Some extracted wh/degree noun phrases now use `DWHs`, `DWHp`, or `DWHu` instead of ordinary `D**w`, `Dmc`, or `Dmu` on the certified branch. Ordinary determiner paths remain available outside the extraction branch. |
| `more` in wh/degree `B#m` extraction | `ECWH` carries the wh-quantity certificate through `more`; the following noun phrase must expose either a `DWH` determiner certificate or a `BWH` degree certificate. Ordinary comparative `EC` / `ECa` paths remain available outside this extraction branch. |
| `Ws` quantity and temporal `how` paths | Subject-question quantity paths now use `HWS`, with `DWSu` on uncountable subject nouns. The focused temporal fragment `how long before ...` uses `EEHWS`. Ordinary `H` and `EEh` remain available for non-`Ws` questions and embedded clauses. |
| `as.e-c` `MVzp` predicative branch | Replaced by adjective-only `MVZP`. This preserves adjectival parentheticals such as `unclear as worded` while preventing ordinary verbs from using the `as.e-c --Pa--` branch in bad comparative paths. |
| bare-`what` `Wb` openers | `Wb` paths through `what` now require `RWB`, which is exposed only by the inverted verb question path. Ordinary `R` remains available for non-`Wb` wh/opening paths. |
| predicative-adjective object `O` forms | `Pa**j` complement paths now use `OAJ` instead of ordinary `Osm`, `Op`, `Ox`, or `Os*e` object links. `OXi` remains available for complement-bearing `it` cases that are still governed by the expletive-`it` PP rules. Ordinary object links remain available for non-`Pa**j` constructions. |
| existential `there` with `SFst`, `SFp`, `SFut`, `SFIst`, `SFIp` | Replaced for existential `there.r` and the related deictic `here` path by agreement-specific `THR*` connectors. The old broad `SF*` forms remain available to unrelated grammar paths. |
| `there.r OXt-` | Removed. Locative `there` uses ordinary modifier paths such as `MVp`; existential and presentational `there` use agreement-specific `THR*` links. |
| filler-`it` inverted auxiliary paths | `SFI` question auxiliaries now use `IFI` to reach lower raising predicates instead of broad ordinary `I` continuations. Ordinary-subject question auxiliaries keep the existing `I` paths. |
| naked `I*a+` on `to.r` | Removed from the affected `to.r` branch so infinitival `to` no longer has that unlicensed fallback path. The remaining rule-6 limitations are documented separately below. |
| `Jr` with `of` | No longer appears in the broad `of` object list. It is still available through the explicit `OFJ- & Jr+` path. |
| `U#t` | Stale PP-only selector from rule 55. The current English dictionary and link-type documentation do not define corresponding `U...t` connector forms, so this was not a retired dictionary connector. |
| `AFdi` | Retired from `than.e` comparative paths. Valid expletive-`it` comparative clauses use other certified complement or comparative analyses; the old `AFdi` arm allowed a lower ordinary subject clause to satisfy the local shape before PP rejected it. |
| `to.r` with `SFsx` | The infinitival `to.r` branch no longer exposes a direct `SFsx+ & <S-CLAUSE>` subject path. Valid infinitival nominal subjects use local `TOn` / `IV` evidence instead. |
| `than.e` with `AFd` and `THc` | The finite that-clause comparative arm no longer combines `AFd+` with `THc+`. `AFd+` remains available on infinitival comparative continuations such as `TOic` and `TOfc`. |

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

### `NDH`: H-Licensed Unit Quantities For `B#m` Extraction

`NDH` connects an `H`-licensed quantity word such as `many` to a unit noun in
wh/degree extraction:

```text
   +--H--+-NDH-+---Bpm---+
   |     |     |         |
how many miles.i ... bike.v
```

The unit noun can then expose the `Rw+ & Bpm+` extraction branch. Ordinary
quantity paths such as `many people` and postposed `you biked how many miles`
continue to use `ND`; only the extracted unit branch requires `NDH`.

### `DWH`: Wh/Degree Determiner Certificates For `B#m` Extraction

`DWH` is a certificate determiner family used when the wh/degree witness must
be carried by the noun phrase that supplies the `B#m` extraction link. Its
subtypes are:

```text
DWHs  singular or singular-like extraction noun
DWHp  plural extraction noun
DWHu  uncountable or mass extraction noun
```

Focused examples:

```text
    +->Wq--+--H-+-DWHp+---Rw--+---Bpm---+
    |      |    |     |       |         |
LEFT-WALL how many times.n did.v-d ... do.v

    +->Wq--+--H-+-DWHu+---Rw--+---Bsm---+
    |      |    |      |       |         |
LEFT-WALL how much effort.n did.v-d ... give.v
```

Ordinary determiner links such as `D**w`, `Dmc`, and `Dmu` remain available
for non-extracted noun phrases. The certified extraction branch uses `DWH`
only where the old PP rule would have required a `D##w` or `H` witness in the
same domain as `B#m`.

### `ECWH`: H-Certificate Propagation Through `more`

`ECWH` connects an `H`-licensed quantity word to `more` when the following noun
phrase will supply a `B#m` extraction link:

```text
    +--H--+ECWH+-DWHp+---Bpm---+
    |     |    |     |         |
how many more times.n ... do.v
```

For adjective-modified extraction nouns, `more` can instead pass the
certificate with `BWH`:

```text
    +--H--+ECWH+--EAm--+---A---+
    |     |    |       |       |
how many more stupid.a times.n
              +--BWH---+
```

The adjective branch requires the `BWH` link. This prevents the
sentence-initial degree-question path from licensing forms such as
`*How much more efficient programs are available.`

### `BWH`: HA-Certificate Propagation To `B#m` Extraction Nouns

`BWH` carries an `HA` wh/degree witness to the noun that exposes the `B#m`
extraction branch:

```text
    +--EAh--+---HA---+
    |       |        |
how efficient.a a program.n ... is.v
    |                 |
    +------BWH--------+
```

The `HA` relation itself remains ordinary and does not require `BWH`; examples
such as `It was so big a dog that it filled the cage` still use normal `HA`.
`BWH` is added only on the extraction-certified branch where the old PP rule
would have accepted `B#m` because the same domain also contained `HA`.

### `HWS`, `DWS`, And `EEHWS`: Safe `Ws` How-Question Certificates

`HWS` connects sentence-opening `how` to a quantity word when the question is
a `Ws` subject question. The certified branch intentionally exposes only
non-extraction continuations:

```text
    +->Ws--+-HWS+-Dmc-+---Sp---+
    |      |    |     |        |
LEFT-WALL how many dogs.n ran.v-d
```

For uncountable subject nouns, `DWSu` keeps the `Ws` quantity branch distinct
from wh-extraction `DWHu` and from ordinary `Dmu`, which can also match
subscripted wh determiner forms:

```text
    +->Ws--+-HWS+-DWSu-+---Ss--+
    |      |    |      |       |
LEFT-WALL how much sugar.n-u is.v ...
```

`EEHWS` is a narrow certificate for temporal fragments headed by `how long
before ...`:

```text
    +->Ws--+EEHWS+--Yt--+
    |      |     |      |
LEFT-WALL how long.e before ...
```

Ordinary `H` and `EEh` remain available for non-`Ws` questions and embedded
clauses, such as `How much money did you earn?` and `I wonder how many times
you did it.` This split replaces the old rule-71 check without making broad
`Ws + B#m` or `Ws + Ca` paths accepted.

### `RWB`: Bare-`what` Topic-Question Certification

`RWB` connects bare `what` to an inverted verb in `Wb` topic questions:

```text
    +->Wb--+--RWB--+--SI--+
    |      |       |      |
LEFT-WALL what did.v-d you ...
```

The same `what` disjunct also carries `BW` or `Bsw` to the extracted
predicate. `RWB` is not used by ordinary `Wq` or `Ws` questions; those paths
still use the older `R` relation and remain governed by their own grammar
rules.

### `OAJ`: Predicative-Adjective Object Licensing

`OAJ` connects a verb to a non-expletive object class that may license a
`Pa**j` predicative-adjective complement:

```text
    +->Wd--+-Sp*i+-OAJ+--Pa**j--+
    |      |     |    |         |
LEFT-WALL I.p want.v it green.a .
```

The link replaces the older use of ordinary object links such as `Osm`, `Op`,
`Ox`, and `Os*e` in `Pa**j` frames. Keeping a distinct uppercase family
prevents broad `O+` branches from matching singular common-noun object links
and accidentally licensing examples such as `*I want a gift inexpensive`.

The special `OXi` link remains available in `Pa**j` frames for `it` when the
adjective also takes a complement, as in `I made it clear that I was angry`.
That preserves the still-active expletive-`it` checks until those rules are
migrated separately.

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

### `CMPO`: Comparative Object-Clause Certificate

`CMPO` connects a comparative object or measure witness to a following
`than.e` or `as.e-c` object-clause continuation. It replaces the PP rule 47
requirement that a domain containing `MV#o` also contain one of the local
comparative witnesses such as `D##m`, `D##y`, `Om`, `Oy`, `Jm`, `Jy`, `Am`,
or `MX#m`.

Focused `Oy` example:

```text
    +-------MVzo------+
    +----Oy----+      +--Bc--+
    |          +-CMPO-+-Ct+  |
I did.v as.e-y much as.e-c he did.v
```

The certificate is carried by the comparative object or measure expression,
not by a plain adjective. Thus `as much as he did` and `the same as it did`
can still use `MVzo`, while `as intelligent as John does` cannot use that
object-clause branch merely because the adjective has broad modifier
connectors.

### `WTHAN`: `way ... than to ...` Infinitival Comparatives

`WTHAN` connects a `way` or `ways` noun to a following `than.e` when the
comparative continuation is an infinitival `to` clause:

```text
way.n --WTHAN-- than.e --TO-- to.r
than.e --IV--> leave.v
```

This is a narrow replacement for one valid use that previously depended on the
retired naked `to.r I*a+` fallback. It is deliberately tied to `way` nouns
rather than to a broad `MVp` modifier path, because broad `MVp` licensing would
accept malformed adjective comparatives such as `*He is more likely than to
stay`.

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

### `TSIC`: Expletive-`It` License For `TSi` That-Clauses

`TSIC` connects filler/expletive `it` to a predicate that selects a
subjunctive `TSi` that-clause complement:

```text
TSIC  filler/expletive-it certificate for TSi predicates
```

Focused linkage fragment, in schematic edge-list notation:

```text
it --SFsi-- is --Paf-- important --TSi-- that
it ----------------TSIC------------------ important
```

The direct subject path, inverted subject path, and object-complement filler
path for `it` can supply `TSIC`. Predicate entries that still expose `TSi+`
through the migrated branch consume `TSIC-` in the same predicate disjunct.
This keeps grammatical examples such as `It is important that women be ready`
available while preventing an ordinary subject from licensing `TSi`, as in
`*Joe is important that women be ready`.

### `TOIC`: Expletive-`It` License For `TOi` Infinitives

`TOIC` connects filler/expletive `it` to a predicate that selects a `TOi`
infinitive complement:

```text
TOIC  filler/expletive-it certificate for TOi predicates
```

Focused linkage fragment, in schematic edge-list notation:

```text
it --SFsi-- is --Paf-- easy --TOi-- to --I-- use
it ---------------TOIC--------------- easy
easy ----------------IV------------------ use
```

The same direct subject, inverted subject, and object-complement filler paths
for `it` that can license `TSIC` can also supply `TOIC`. Predicate entries
that expose `TOi+ & IV+` through the migrated branch consume `TOIC-` in the
same predicate disjunct.

This family is deliberately separate from tough-subject infinitives. A
sentence such as `Our program is easy to use` uses `TOt`, not `TOi`, because
the matrix subject is interpreted as the object of the infinitive. By
contrast, `It is easy to use the program` uses filler/expletive `it` and can
therefore supply the `TOIC` certificate for `TOi`.

### `THIC`, `TTHI`, `ITHI`, `PPTHI`, And `PTHI`: Expletive-`It` License For `THi` Clauses

These connector families encode the filler/expletive-`it` evidence required
by `THi` that-clause predicates:

```text
THIC   direct filler/expletive-it certificate for THi predicates
TTHI   object-raising carrier from the higher predicate to infinitival to
ITHI   carrier from infinitival to or an inverted auxiliary to the lower predicate
PPTHI  carrier from a perfect auxiliary to the lower predicate
PTHI   lower-predicate link to the THi adjective or copula
```

Direct predicate-adjective cases use `THIC`:

```text
it --SFsi-- is --Paf-- likely --THi-- that
it ----------------THIC---------------- likely
```

Object-raising and auxiliary paths cannot always use a direct `THIC` link
from `it` to the lower predicate, because the link would cross the infinitival
or inverted auxiliary chain. They therefore use carrier paths:

```text
want --OXi-- it
want --TTHI-- to --ITHI-- be --PTHI-- likely --THi-- that

does --SFIsi-- it
does --ITHI-- seem --PTHI-- likely --THi-- that

has --SFIsi-- it
has --PPTHI-- seemed --PTHI-- likely --THi-- that
```

Cleft clauses such as `It was in Paris that ...` are a same-copula case:
the copula owns both the filler-`it` subject link and the `THi` link. These
use a direct `SFsi`/`SFIsi` copular branch instead of a separate `THIC` link.

The `THi` certificate intentionally excludes comparative `AF` predicate paths.
Otherwise an outer filler `it` could license a distant comparative clause, as
in the rejected path for `*It is more likely that Joe died than John is that
Fred died`.

### `TOCL`, `IOCL`, `PPOCL`, And `ROCL`: Filler-`It` License For Cleft Objects

These connector families encode the filler/expletive-`it` evidence required
by cleft-like `Osi` / `Opi` object paths on `be`:

```text
TOCL   object-raising carrier from the higher predicate to infinitival to
IOCL   carrier from infinitival to or an inverted auxiliary to lower be
PPOCL  carrier from a perfect auxiliary to lower been
ROCL   direct link from the cleft-object predicate to a following filler it
```

Direct finite clefts use the same copula to carry both the filler-`it` subject
and the `Osi` / `Opi` branch:

```text
it --SFsi-- was --Osi-- John
              \--R/B-- who stole ...
```

Auxiliary and object-raising paths use carrier links:

```text
it --SFsi-- might --IOCL-- be --Osi-- John
want --OXi-- it --TOCL-- to --IOCL-- be --Osi-- John
it --SFsi-- has --PPOCL-- been --Osi-- John
```

Some accepted cleft-object paths have the filler `it` inside the following
clause rather than as the subject of the same `be`. These use `ROCL`:

```text
is --Osi-- that
is --ROCL-- it --SFsi-- was
is --Bs--- was
```

The `ROCL` path is deliberately narrower than the old ordinary `R` path: the
target must be the `it` disjunct that also carries the filler-`it` subject
relation into the following clause.

### `QIIC`, `TQII`, `IQII`, And `PQII`: Expletive-`It` License For `QIi` Questions

These connector families encode the filler/expletive-`it` evidence required
by `QIi` question-clause predicates:

```text
QIIC  direct filler/expletive-it certificate for QIi predicates
TQII  object-raising carrier from the higher predicate to infinitival to
IQII  object-raising carrier from infinitival to to the lower predicate
PQII  lower-predicate link to the QIi adjective
```

Direct subject and direct object-complement cases use `QIIC`:

```text
it --SFsi-- is --Paf-- unknown --QIi-- whether
it ----------------QIIC----------------- unknown

made --OXi-- it
made --Paf*j-- clear --QIi-- how
it ----------QIIC---------- clear
```

Object-raising `it to be ...` paths cannot use the direct `QIIC` link from
`it` to the adjective, because it would cross the infinitival predicate path.
They therefore use a carrier chain modeled on the existing existential
`there` carrier families:

```text
want --OXi-- it
want --TQII-- to --IQII-- be --PQII-- obvious --QIi-- how
```

These are technical certificate links, but they encode the same grammatical
condition as the old PP rule: a `QIi` predicate in these branches is licensed
only when the selected linkage also contains the appropriate filler/expletive
`it` evidence.

### `CIIC`, `TCII`, `ICII`, And `PCII`: Expletive-`It` License For `Ci` Clauses

These connector families encode the filler/expletive-`it` evidence required
by `Ci` finite-clause predicates:

```text
CIIC  direct filler/expletive-it certificate for Ci predicates
TCII  object-raising carrier from the higher predicate to infinitival to
ICII  object-raising carrier from infinitival to to the lower predicate
PCII  lower-predicate link to the Ci adjective
```

Direct subject and direct object-complement cases use `CIIC`:

```text
it --SFsi-- is --Paf-- likely --Ci-- he --CV-- came
it ----------------CIIC---------------- likely

considered --OXi-- it
considered --Paf-- likely --Ci-- he --CV-- came
it ----------CIIC---------- likely
```

Object-raising `it to be ...` paths use a carrier chain:

```text
want --OXi-- it
want --TCII-- to --ICII-- be --PCII-- likely --Ci-- he
```

These are technical certificate links. They encode in the dictionary the same
condition previously checked after extraction: a `Ci` predicate in these
branches is licensed only by the appropriate filler/expletive `it` relation.

### `BIQS`, `BIQI`, `IBIQ`, And `PPBIQ`: `BIq` Predicate Licenses

These connector families encode the subject or auxiliary evidence required for
predicate wh-complements:

```text
BIQS   direct subject license for a BIq predicate
BIQI   inverted subject license for a BIq predicate
IBIQ   modal/infinitive auxiliary license carried to be
PPBIQ  perfect auxiliary license carried to been
```

Focused direct example:

```text
    +-------------------------Xp------------------------+
    +--------->WV--------->+                            |
    +----->Wd------+       |    +--------B*w-------+    |
    |      +---Ds--+--BIQS-+BIqd+-Rn+--Sp-+----I---+    |
    |      |       |       |    |   |     |        |    |
LEFT-WALL the question.n is.v  who we should.v invite.v .
```

Focused modal example:

```text
    +----------------------------Xp---------------------------+
    +------------>WV------------>+                            |
    +----->Wd------+             |    +--------B*w-------+    |
    |      +---Ds--+--BIQS-+-IBIQ+BIqd+-Rn+--Sp-+----I---+    |
    |      |       |       |     |    |   |     |        |    |
LEFT-WALL the question.n may.v be.v  who we should.v invite.v .
```

Focused perfect example:

```text
    +----------------------------Xp----------------------------+
    +----------->WV----------->+                               |
    +---->Wd-----+             |      +---------CV------->+    |
    |      +--Ds-+-BIQS-+-PPBIQ+--BIq-+--Cs-+--Sp-+---I---+    |
    |      |     |      |      |      |     |     |       |    |
LEFT-WALL the issue.n has.v been.v whether we should.v leave.v .
```

As with the `THb` certificate families, these are dedicated uppercase
connectors rather than subscripted `S`, `I`, or `PP` forms. The earlier
prototype using only `Ss*q`/`SIs*q` was unsafe because an ordinary subject path
could still match a subscripted verb-side connector and inherit an apparently
licensed `S##q` link name.

### Result-Clause Certificates: `EEXK`, `EAXK`, `DTHAT`, `RTHAT`, And `MVH`

These connector families encode the degree or determiner witness needed for
result-clause `that` constructions:

```text
EEXK   degree link from so/sufficiently to an adverbial result target
EAXK   degree link from so/sufficiently to an adjective result target
DTHAT  determiner link from such/such a/such an to a noun result target
RTHAT  result-clause certificate from an adverbial or noun target to that.j-c
MVH    adjective-headed result-clause link to that.j-c
```

Focused adverb example:

```text
    |           +---------MVh---------+                |
    +---->WV--->+-----MVa----+        +-----CV--->+    |
    +->Wd--+-Ss-+      +-EEXK+--RTHAT-+-Cet-+--Ss-+    |
    |      |    |      |     |        |     |     |    |
LEFT-WALL he ran.v-d so.e quickly that.j-c he fell.v-d .
```

Focused adjective example:

```text
    +--------->WV-------->+                                  |
    +----->Wd-----+       +----Pa---+       +-----CV--->+    |
    |      +-Ds**c+--Ss*s-+    +EAXK+--MVH--+-Cet-+--Ss-+    |
    |      |      |       |    |    |       |     |     |    |
LEFT-WALL the shuttle.n is.v so.e big.a that.j-c it fell.v-d .
```

Focused noun example:

```text
    |               +----------------------MVh---------------------+      |
    |               +---------MVp--------+                         |      |
    +------>WV----->+------Os------+     +------Jp-----+           |      |
    +->Wd--+---Ss---+        +Ds**c+--Mp-+    +--DTHAT-+---RTHAT---+      |
    |      |        |        |     |     |    |        |           |      |
LEFT-WALL she presented.v-d her case.n with such eloquence.n-u that.j-c ...
```

The older lowercase witness forms `EExk`, `EAxk`, and `D...k` remain available
for ordinary degree and determiner uses, for example `so quickly`, `so big`,
and `such eloquence` without a result clause. The uppercase families are used
only when the target also carries the local `that` certificate.

### `IFI`: Filler-It Inverted Auxiliary Continuation

`IFI` connects an inverted question auxiliary with a lower raising predicate
when the subject relation is filler `it`:

```text
    +----Qd----+         |
    |    +SFIsi+--IFI--+ |
    |    |     |       | |
LEFT-WALL does it seem likely ...
```

The relation is narrow: it is used for filler-`it` auxiliary paths such as
`Does it seem likely that Joe came?`, while ordinary-subject raising questions
keep the existing `I` relation:

```text
does --IFI-- seem
does --I*d-- seem
```

The uppercase family is required because a subscripted spelling such as `Ifi`
would still match broad ordinary `I` connectors. Rule 73 needs the filler
inversion path to reach only lower predicates that explicitly accept this
expletive/filler relation, not every verb with an ordinary infinitival
continuation.

### `SJI` And `SGP`: Controlled Subjects

`SJI` and `SGP` encode controlled-subject relations in small-clause-like
complements:

```text
SJI  logical subject of a bare infinitive, paired with I*j
SGP  logical subject of a present participle, paired with Pg
```

Focused bare-infinitive example:

```text
    +-----------------Xp-----------------+
    +--------->WV-------->+----I*j---+   |
    +->Wd--+--Sp--+---I---+-Ox-+-SJI-+   |
    |      |      |       |    |     |   |
LEFT-WALL you should.v hear.v him sing.v .
```

Focused present-participle example:

```text
    +-------------------------Xp------------------------+
    +---->WV---->+-----Pg-----+---------Osn---------+   |
    +->Wd--+-Sp*i+-Ox-+--SGP--+----K----+     +Ds**c+   |
    |      |     |    |       |         |     |     |   |
LEFT-WALL I.p feel.v him breathing.v down.r my.p back.n .
```

These are dedicated uppercase families rather than subscripted `S` forms. A
broad finite-subject connector such as `S-` can match subscripted variants, so
the old `Sj`/`Sg` spellings allowed ordinary finite-subject paths to satisfy
the PP backstop accidentally. The uppercase families keep controlled subjects
separate from ordinary finite subjects.

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
| 1-2 | `SI#*` / `SI#x` embedded and fronted inversion | Simple removal raises `corpus-basic.batch` errors by accepting embedded and fronted inversion negatives. Rule 1 removal accepts eleven starred examples, including `*I know how quickly did you run` and `*I wonder how much money have you earned`; it also improves `corpus-fixes.batch` by one and `corpus-failures.batch` by three, so the rule is mixed rather than purely obsolete. Rule 2 removal accepts starred examples such as `*After the movie did he realize his mistake` and `*I wonder which dog did he say you chased`. A replacement needs to distinguish valid matrix question/comparative/fronted inversion paths from embedded complement paths that should not license these `SI` forms. |
| 5 | `Ws` wh-subject/opening link | Simple removal improves `corpus-fixes.batch` by four and `corpus-failures.batch` by four, but accepts one `corpus-knowledge.batch` negative and eight `corpus-basic.batch` negatives, including `*How big dogs run` and `*Who to invite to the party`. A replacement needs to preserve valid wh fragments and exclamatives while requiring the appropriate `D##w`, `S##w`, or `H` evidence for ordinary wh-subject/opening paths. |
| 7 | `Wq` question/opening link | Simple removal improves `corpus-fixes.batch` by six and `corpus-failures.batch` by four, but it accepts one `corpus-knowledge.batch` negative and twenty `corpus-basic.batch` negatives, including `*Which dog you chased` and `*How much money you earn`. A replacement needs to separate valid fragment/exclamative uses such as `How quickly?` and `What a great day was today!` from ordinary wh questions that still need inversion evidence. |
| 38 | Remaining inverted expletive `it` complement licensing | Rules 37 and 39 were removed as redundant after the dedicated predicate-certificate migrations. Rule 38 remains active: simple removal accepts `*I wonder how important is it to turn off the computer`. A replacement needs to separate valid inverted filler/expletive `it` paths from embedded ordinary inversion paths that should not license the lower complement. |
| 43, 48 | Comparative paths | Bulk removal leaves `corpus-knowledge.batch` clean and improves `corpus-fixes.batch`, but raises `corpus-basic.batch` by accepting comparative negatives. Rules 44 and 47 were migrated separately with the `MVZP` and `CMPO` splits. Rule 48 still accepts the knowledge/basic negative `*I am as intelligent as John does` when removed by itself, so it needs a narrower comparative connector split rather than deletion. |
| BOUNDED `s` | `s` domain boundedness | Simple removal is unsafe: it accepts the `corpus-knowledge.batch` negative `*How much of the book you read` and multiple `corpus-basic.batch` negatives, including `*He ran I know how quickly`. A replacement needs to preserve the grammatical distinction between valid fronted/inverted `s` domains and embedded or otherwise unbounded `s`-domain paths. |

`FORM_A_CYCLE_RULES` is intentionally not listed as a dictionary-migration
candidate. For metric-ordered extraction, this class is handled by
extractor-side cycle state, not by English dictionary connector replacement.

## Stale Expletive-It Companion Selectors

**Status:** implemented for rules 25-29.

### Rule / Area

The removed PP rules were:

```text
COqi , SFsi SFIsi OXi , "Complement requires 'it'25"
CPi  , SFsi SFIsi OXi , "Complement requires 'it'26"
Eqi  , SFsi SFIsi OXi , "Complement requires 'it'27"
LEi  , SFsi SFIsi OXi , "Complement requires 'it'28"
MVti , SFsi SFIsi OXi , "Complement requires 'it'29"
```

These rules were part of the broader expletive-`it` complement-licensing
family. In the current dictionary, however, these five selector links cannot
be formed as completed links: `CPi` and `MVti` occur only on negative
connectors, `Eqi` occurs only on a positive connector, `LEi` has no positive
counterpart, and `dCOqi` has no matching negative endpoint.

### Implementation

No new connector family is needed. The stale rules are removed from
`4.0.knowledge`; the remaining expletive-`it` rules stay active because they
still reject real overgeneration.

### Examples

There are no focused accepted or rejected sentences for these stale selectors,
because the relevant completed links are unreachable in the current
dictionary. The focused evidence is the connector endpoint audit above, plus
the unchanged corpus behavior after removing the rules as a family.

### Verification

The removal was validated with ordinary parser runs:

```sh
link-parser < ./data/en/corpus-knowledge.batch
link-parser < ./data/en/corpus-basic.batch
link-parser < ./data/en/corpus-fixes.batch
link-parser < ./data/en/corpus-fix-long.batch
link-parser < ./data/en/corpus-failures.batch
```

The observed counts were:

```text
corpus-knowledge.batch: 0 errors
corpus-basic.batch: 88 errors
corpus-fixes.batch: 359 errors
corpus-fix-long.batch: 8 errors
corpus-failures.batch: 1497 errors
```

## Rule 20: License `THi` Complements With Filler `It`

**Status:** implemented; PP rule 20 has been removed from `4.0.knowledge`.

### Rule / Area

The removed PP rule was:

```text
THi , SFsi SFIsi OXi , "Complement requires 'it'20"
```

The grammatical area is filler/expletive-`it` licensing of that-clause
predicate complements:

```text
It is likely that he came.
Does it seem likely that Joe came?
I made it clear that he came.
I want it to be likely that he came.
It was in Paris that Debussy first heard Balinese music.
```

These constructions differ from ordinary subject predicates. A raw linkage
such as `*Joe is likely that he came` has a locally plausible predicate and
that-clause relation, but it lacks the filler/expletive `it` relation that
licenses the `THi` complement.

### Problem

The old dictionary exposed `THi+` directly on adjectives, copular clefts,
passive verbs, and some raising paths. PP then checked whether the completed
`THi` domain also contained one of the filler/expletive-`it` relations
`SFsi`, `SFIsi`, or `OXi`.

Simple removal of the PP rule accepted wrong ordinary-subject and wrong-object
paths such as:

```text
*Joe is likely that he came.
*I made Anne clear that he came.
*Does it act likely that Joe came?
```

The local `Paf`/`THi` shape is not sufficient by itself; the predicate branch
needs evidence that the linkage contains the appropriate filler/expletive
`it` relation.

### Implementation

Direct predicate branches consume a `THIC` certificate:

```text
<thi-verb>: THIC- & THi+;
```

The filler/expletive `it` entry can supply `THIC+` on direct subject,
inverted subject, and object-complement paths. Object-raising and inverted
auxiliary cases use carrier links when a direct `THIC` link would cross the
intervening chain:

```text
TTHI  -> ITHI -> PTHI
SFI   -> ITHI -> PTHI
SFI   -> PPTHI -> PTHI
```

Cleft uses such as `It was in Paris that ...` are handled as a direct copular
case because the copula itself owns the `SFsi`/`SFIsi` and `THi` links.

The `THi` certificate excludes comparative `AF` predicate paths. This mirrors
the rule-24 `Ci` migration: an outer filler `it` must not license a distant
comparative predicate, as in `*It is more likely that Joe died than John is
that Fred died`.

### Verification

Focused accepted-linkage comparison inspected the first three displayed
accepted linkages for:

```text
It is likely that he came.
Does it seem likely that Joe came?
It was in Paris that Debussy first heard Balinese music.
```

The same grammatical constructions remain accepted. Direct predicate examples
show `THIC`; inverted auxiliary examples show `ITHI` and `PTHI`; cleft
examples keep the public `SFsi`/`THi`/`Pp` shape, with the `THi` link now
coming from the filler-`it` copular branch.

The rule 20 migration was validated with ordinary parser runs:

```sh
link-parser < ./data/en/corpus-knowledge.batch
link-parser < ./data/en/corpus-basic.batch
link-parser < ./data/en/corpus-fixes.batch
link-parser < ./data/en/corpus-fix-long.batch
link-parser < ./data/en/corpus-failures.batch
```

Observed results:

```text
corpus-knowledge.batch: 0 errors
corpus-basic.batch: 88 errors
corpus-fixes.batch: 359 errors
corpus-fix-long.batch: 8 errors
corpus-failures.batch: 1496 errors
```

## Rule 31: License Cleft-Object `O#i` Paths With Filler `It`

**Status:** implemented; PP rule 31 has been removed from `4.0.knowledge`.

### Rule / Area

The removed PP rule was:

```text
O#i , SFsi SFIsi OXi , "Complement requires 'it'31"
```

The grammatical area is cleft-like object complements of `be`, including
ordinary, inverted, auxiliary, perfect, and object-raising forms:

```text
It was John who stole the documents.
Was it John who stole the documents?
It might be John who stole the documents.
I want it to be John who stole the documents.
```

The same `Osi` / `Opi` connector shape was also used by accepted constructions
where the filler `it` appears inside the following clause:

```text
The shame of it is, is that it was a good idea.
```

### Problem

The old dictionary exposed `Osi+` and `Opi+` cleft-object branches through
generic copular `be` complements:

```text
Osi+ & R+ & Bs+
Opi+ & R+ & Bp+
```

After extraction, PP rejected a completed domain containing an `O#i` link
unless the same domain also contained a filler/expletive `it` relation:
`SFsi`, `SFIsi`, or `OXi`.

Simple removal of the PP rule accepted ordinary-subject raw linkages such as:

```text
*How likely is John that he will come?
*I believe Fred was John who stole the priceless documents.
*That is the man, in Joe's opinion, we should hire.
```

Those raw linkages used the local `Osi+ & R+ & Bs+` shape without the
filler/expletive `it` relation that makes the cleft-object construction
grammatical.

### Implementation

The generic `be` complement no longer exposes ordinary `Osi+` / `Opi+`
cleft-object branches. Direct finite clefts are instead exposed only on
branches that also own the same-copula filler-`it` subject relation:

```text
SFsi- & Osi+ & R+ & Bs+
SFIs+ & Osi+ & R+ & Bs+
```

The same direct branch is available on common contracted and negative copular
forms, such as `It's John who...`, `Isn't it John who...`, and `It wasn't
John who...`.

Auxiliary, perfect, and object-raising paths carry the filler-`it` evidence to
the lower `be`:

```text
TOCL  -> IOCL
SFsi  -> IOCL
SFIsi -> IOCL
SFsi  -> PPOCL
IOCL  -> PPOCL
```

Thus `It might be John who...` uses `it --SFsi-- might --IOCL-- be`, and
`I want it to be John who...` uses `want --OXi-- it --TOCL-- to --IOCL-- be`.
Perfect paths such as `It has been John who...` use `PPOCL`.

The accepted "shame of it" pattern cannot be handled by same-copula or
ordinary auxiliary carrying, because the licensing `it` appears inside the
following clause. For this case the old broad `R+` leg is replaced by an
`ROCL+` leg:

```text
is --Osi-- that
is --ROCL-- it --SFsi-- was
is --Bs--- was
```

This keeps the relevant filler-`it` evidence local to the dictionary path
while preventing ordinary `R` targets such as `who`, `he`, or `we` from
licensing the `O#i` branch.

### Verification

Focused accepted-linkage comparison inspected the first three displayed
accepted linkages for:

```text
It was John who stole the priceless documents.
It might be John who stole the priceless documents.
The shame of it is, is that it was a good idea.
```

The direct finite example keeps the same public `SFsi`, `Osi`, `R`, and `Bs`
shape as the reference parse. The modal example keeps the same public
cleft-object shape while replacing the ordinary modal `I` carrier with
`IOCL`. The "shame of it" example remains accepted; the migrated dictionary
uses the narrower `ROCL` link where the reference parse used the broad `R`
path.

The rule 31 migration was validated with ordinary parser runs:

```sh
link-parser < ./data/en/corpus-knowledge.batch
link-parser < ./data/en/corpus-basic.batch
link-parser < ./data/en/corpus-fixes.batch
link-parser < ./data/en/corpus-fix-long.batch
link-parser < ./data/en/corpus-failures.batch
```

Observed results:

```text
corpus-knowledge.batch: 0 errors
corpus-basic.batch: 88 errors
corpus-fixes.batch: 359 errors
corpus-fix-long.batch: 8 errors
corpus-failures.batch: 1496 errors
```

## Rules 37 And 39: Remove Redundant Non-Inverted Expletive-It Backstops

**Status:** implemented; PP rules 37 and 39 have been removed from
`4.0.knowledge`.

### Rule / Area

The removed PP rules were:

```text
SFsi* , TOi THi QIi TSi O#i Ci THb CPi CPu COqi CPi Eqi BIh,
        "Bad use of 'it'37"
OXi   , TOi THi QIi TSi O#i Ci THb CPi COqi CPi Eqi BIh,
        "Bad use of 'it'39"
```

The grammatical area is non-inverted filler/expletive `it` licensing for
predicate complements. Earlier migrations split the individual complement
families into direct certificates and carrier paths, for example `THIC`,
`TSIC`, `QIIC`, `TOIC`, `CIIC`, `THBS` / `THBI`, `BIQS` / `BIQI`, and
cleft-object `TOCL` / `IOCL` / `PPOCL` / `ROCL`.

### Problem

After those dedicated migrations, the broad non-inverted `SFsi*` and `OXi`
backstops no longer reject any additional agreed corpus example. Keeping them
would leave two stale PP rules that duplicate more local dictionary
constraints.

Rule 38 is different and remains active. Its selector `SFIsi` covers inverted
paths, and simple removal still accepts:

```text
*I wonder how important is it to turn off the computer.
```

### Implementation

Rules 37 and 39 are removed from `4.0.knowledge` without adding new
connectors. This is a redundancy removal, not a new grammar path. The
remaining rule 38 continues to protect the unresolved inverted case.

### Examples

Focused examples include:

```text
It is likely that he came.
I made it clear that he came.
*Joe is likely that he came.
*I made Anne clear that he came.
```

### Verification

Rules 37 and 39 were tested individually before removal. In both cases,
ordinary parser runs produced no error-set differences against the pre-removal
baseline for:

```sh
link-parser < ./data/en/corpus-knowledge.batch
link-parser < ./data/en/corpus-basic.batch
link-parser < ./data/en/corpus-fixes.batch
link-parser < ./data/en/corpus-fix-long.batch
```

## Rule 30: Retire The `AFdi` Comparative Filler-`It` Arm

**Status:** implemented; PP rule 30 has been removed from `4.0.knowledge`.

### Rule / Area

The removed PP rule was:

```text
AFdi , SFsi SFIsi OXi , "Complement requires 'it'30"
```

The affected area is comparative predicate clauses with filler/expletive
`it`, especially raw paths where `than.e` connected to a lower finite
predicate with `AFdi`:

```text
*It is more likely that Joe died than John is that Fred died.
```

### Problem

The old `than.e` entry exposed an `AFdi+` branch for comparative subordinate
predicates:

```text
LEi- & AFdi+ & THc+
```

This allowed a locally plausible raw linkage in which the outer clause had a
valid filler/expletive `it` path, while the lower comparative clause used an
ordinary subject:

```text
it --SFsi-- is --Pa-- likely --LEi-- than --AFdi-- is
John --Ss*s-- is
than --THc-- that Fred died
```

PP rejected this because the completed `AFdi` domain did not contain the
local `SFsi`, `SFIsi`, or `OXi` evidence required by rule 30. The outer
`it --SFsi-- is` relation is not the lower comparative predicate's
filler-`it` evidence.

### Implementation

The dictionary no longer exposes `AFdi` from `than.e`. The old branch:

```text
(MVti- or LEi-) & AFdi+ & {Pa+}
```

is removed, and the clausal comparative branch is narrowed from:

```text
(LEi- & {AFdi+}) & ...
```

to:

```text
LEi- & ...
```

The `AFdi` selector is also removed from the still-active expletive-`it`
backstop rules 37-39, because the current dictionary no longer forms completed
`AFdi` links. Valid examples in this area continue to parse through existing
certified complement or comparative paths, such as `THIC` / `THi`, `THBS` /
`THb`, and ordinary infinitival comparative analyses.

### Verification

Focused accepted-linkage comparison inspected the first three displayed
accepted linkages for:

```text
It is more likely that Joe died than that Fred died.
It is more likely that Joe died than it is that Fred died.
It is easier to ignore the problem than it is to solve it.
```

The accepted parses remain available without `AFdi`. The first two examples
use the existing certified filler/expletive-`it` complement paths; the
infinitival comparative example remains on its ordinary tough/infinitival
comparative path. The negative example above has no accepted zero-null
linkage after the migration.

The rule 30 migration was validated with ordinary parser runs:

```sh
link-parser < ./data/en/corpus-knowledge.batch
link-parser < ./data/en/corpus-basic.batch
link-parser < ./data/en/corpus-fixes.batch
link-parser < ./data/en/corpus-fix-long.batch
link-parser < ./data/en/corpus-failures.batch
```

Observed results:

```text
corpus-knowledge.batch: 0 errors
corpus-basic.batch: 88 errors
corpus-fixes.batch: 359 errors
corpus-fix-long.batch: 8 errors
corpus-failures.batch: 1496 errors
```

## Rule 21: License `TSi` Complements With Filler `It`

**Status:** implemented; PP rule 21 has been removed from `4.0.knowledge`.

### Rule / Area

The removed PP rule was:

```text
TSi , SFsi SFIsi OXi , "Complement requires 'it'21"
```

The grammatical area is the filler/expletive-`it` licensing of subjunctive
that-clause predicate complements:

```text
It is important that women be ready.
Is it important that women be ready?
I considered it important that women be ready.
```

In these constructions, the that-clause predicate relation is not licensed by
an ordinary lexical subject such as `Joe`; it requires the filler/expletive
`it` path.

### Problem

The old dictionary allowed predicates such as `important.a` and participial
verbs such as `proposed.v-d`, `requested.v-d`, `required.v-d`, and
`recommended.v-d` to expose `TSi+` directly. PP then checked whether the
completed `TSi` domain also contained one of the filler/expletive-`it`
relations `SFsi`, `SFIsi`, or `OXi`.

Without that PP check, locally well-formed raw linkages could attach an
ordinary subject to a `TSi` predicate and accept examples such as:

```text
*Joe is important that women be ready when they make these choices.
```

### Implementation

The dictionary now uses a dedicated uppercase certificate family:

```text
<tsi-verb>: TSIC- & TSi+;
```

The filler/expletive `it` entry can supply `TSIC+` on the direct subject,
inverted subject, and object-complement branches that correspond to the old
PP witnesses:

```text
SFsi  -> TSIC
SFIsi -> TSIC
OXi   -> TSIC
```

Predicate entries that still expose `TSi+` through the migrated branch now
consume `<tsi-verb>` instead of bare `TSi+`. The resulting constraint is local
to the selected predicate disjunct: if the predicate uses `TSi`, the same
linkage must also connect it to a filler/expletive `it` certificate.

### Implications

This is a dictionary replacement for rule 21. Valid `TSi` linkages may show an
additional `TSIC` link where the old grammar used PP to infer the same
condition after extraction. Ordinary `THi` that-clause complements and `TOi`
infinitival complements remain separate paths and are not licensed by this
rule.

Some accepted linkages for examples in this area can prefer an existing `THi`
analysis over a `TSi` analysis after sorting. That does not weaken the rule-21
migration: the dictionary constraint applies precisely when the selected
predicate branch creates a `TSi` link.

### Verification

Focused accepted-linkage comparison inspected the first three displayed
accepted linkages for:

```text
It is important that women be ready when they make these choices.
Is it important that women be ready?
I considered it important that women be ready.
```

The same grammatical constructions remain accepted. When a displayed linkage
uses `TSi`, it also carries `TSIC`; when a preferred displayed linkage uses
`THi`, it follows the pre-existing non-`TSi` complement path.

The rule 21 migration was validated with ordinary parser runs:

```sh
link-parser < ./data/en/corpus-knowledge.batch
link-parser < ./data/en/corpus-basic.batch
link-parser < ./data/en/corpus-fixes.batch
link-parser < ./data/en/corpus-fix-long.batch
link-parser < ./data/en/corpus-failures.batch
```

Observed results:

```text
corpus-knowledge.batch: 0 errors
corpus-basic.batch: 88 errors
corpus-fixes.batch: 359 errors
corpus-fix-long.batch: 8 errors
corpus-failures.batch: 1497 errors
```

## Rule 23: License `TOi` Complements With Filler `It`

**Status:** implemented; PP rule 23 has been removed from `4.0.knowledge`.

### Rule / Area

The removed PP rule was:

```text
TOi , SFsi SFIsi OXi , "Complement requires 'it'23"
```

The grammatical area is the filler/expletive-`it` licensing of infinitive
complements:

```text
It is easy to use the program.
It is necessary to use the program.
```

This is distinct from tough-subject infinitives such as:

```text
Our program is easy to use.
```

The latter construction is licensed by the subject being interpreted as the
object of the infinitive and uses a separate `TOt` path.

### Problem

The old dictionary exposed `<toi-verb>` as:

```text
TOi+ & IV+
```

PP then checked whether the completed `TOi` domain also contained one of the
filler/expletive-`it` relations `SFsi`, `SFIsi`, or `OXi`. Without that PP
check, raw linkages could use the `TOi` expletive-infinitive branch in places
where the sentence should instead use a gap-bearing tough-subject branch, or
be rejected. A representative bad path is:

```text
*Our program is easier to use it than to understand.
```

The bad linkage uses `easier.a-c --TOi-- to.r` and `easier.a-c --IV-- use.v`
while the infinitive still has an overt object `it`. There is no
filler/expletive `it` relation in the `TOi` domain, so the old PP rule
rejected it.

### Implementation

The dictionary now uses a dedicated uppercase certificate family:

```text
<toi-verb>: TOIC- & TOi+ & IV+;
```

The filler/expletive `it` entry can supply `TOIC+` on the direct subject,
inverted subject, and object-complement branches that correspond to the old
PP witnesses:

```text
SFsi  -> TOIC
SFIsi -> TOIC
OXi   -> TOIC
```

Predicate entries that select `<toi-verb>` now require this certificate in
the same predicate disjunct. Ordinary tough-subject uses remain on `TOt`, and
the comparative `TOic` / `TOfc` paths are not changed by this rule.

### Implications

This is a dictionary replacement for rule 23. Valid `TOi` linkages now show an
additional `TOIC` link where the old grammar used PP to infer the same
condition after extraction. The replacement also keeps `TOi` separate from
gap-bearing `TOt`, so retained-object examples are blocked before PP.

The explicit certificate link can change the preferred displayed linkage when
an older non-`TOi` analysis is also available. For examples such as `It is
easy to use the program`, the certified `TOi` analysis remains accepted and
contains `TOIC`, but cheaper `MVi` analyses can sort before it. This is a
ranking consequence of making the formerly implicit PP witness explicit in
the linkage, not a loss of the grammatical `TOi` path.

### Verification

Focused accepted-linkage comparison inspected the first three displayed
accepted linkages for:

```text
It is easy to use the program.
It is necessary to use the program.
Our program is easy to use.
```

The same grammatical constructions remain accepted. The `TOi` analyses for
the filler/expletive examples carry `TOIC`, though they may sort after cheaper
`MVi` alternatives. The tough-subject example continues through `TOt`.

The rule 23 migration was validated with ordinary parser runs:

```sh
link-parser < ./data/en/corpus-knowledge.batch
link-parser < ./data/en/corpus-basic.batch
link-parser < ./data/en/corpus-fixes.batch
link-parser < ./data/en/corpus-fix-long.batch
link-parser < ./data/en/corpus-failures.batch
```

Observed results:

```text
corpus-knowledge.batch: 0 errors
corpus-basic.batch: 88 errors
corpus-fixes.batch: 359 errors
corpus-fix-long.batch: 8 errors
corpus-failures.batch: 1496 errors
```

## Rule 22: License `QIi` Complements With Filler `It`

**Status:** implemented; PP rule 22 has been removed from `4.0.knowledge`.

### Rule / Area

The removed PP rule was:

```text
QIi , SFsi SFIsi OXi , "Complement requires 'it'22"
```

The grammatical area is the filler/expletive-`it` licensing of
question-clause predicate complements:

```text
It is unknown whether he came.
It is unclear whether he came.
I made it clear how to use the program.
I want it to be obvious how to use the program.
```

These constructions differ from ordinary subject predicates. A sentence such
as `*Joe is unknown how he came` has a locally possible predicate and
question-clause relation, but it lacks the filler/expletive `it` relation that
licenses the `QIi` complement.

### Problem

The old dictionary exposed `QIi+` directly on predicates such as `unknown.a`,
`unclear.a`, `obvious.a`, and `clear.a`. PP then checked whether the completed
`QIi` domain also contained one of the filler/expletive-`it` relations
`SFsi`, `SFIsi`, or `OXi`.

Without that PP check, raw linkages could attach an ordinary subject to the
`QIi` predicate:

```text
*Joe is unknown how he came.
*The answer is unknown why he came.
```

The local `is --Paf-- unknown --QIi-- how/why` shape is not sufficient by
itself; the predicate branch needs evidence that the clause uses filler or
expletive `it`.

### Implementation

The direct subject, inverted subject, and direct object-complement cases use a
dedicated certificate:

```text
<qii-verb>: QIIC- & QIi+;
```

The filler/expletive `it` entry can supply `QIIC+` on the same three witness
branches used by the old PP rule:

```text
SFsi  -> QIIC
SFIsi -> QIIC
OXi   -> QIIC
```

Direct object-complement adjectives need the opposite left-connector order
from direct subject predicates. In `It is unknown whether...`, the predicate
sees the closer `Paf-` link before the farther `QIIC-` link. In `I made it
clear how...`, the adjective sees the closer `QIIC-` link from `it` before the
farther `Paf-` link from the higher verb. The dictionary therefore includes
both orders for the affected adjective classes.

Object-raising `it to be ...` cases need a carrier rather than a direct
`QIIC` link, because a direct link from `it` to the lower adjective would
cross the infinitival predicate structure. The replacement uses a carrier
chain analogous to the existential-`there` carrier families:

```text
<qii-too-verb>: TQII+;
to.r: ... TQII- & IQII+ ...
be.v: ... IQII- & PQII+ ...
adjective: PQII- & QIi+
```

Thus `I want it to be obvious how...` carries the `OXi` evidence through
`want --TQII-- to --IQII-- be --PQII-- obvious`.

### Implications

This is a dictionary replacement for rule 22. Valid `QIi` linkages now show
one of the explicit certificate paths where the old grammar used PP to infer
the same condition after extraction. The direct `QIIC` path and the
object-raising `TQII` / `IQII` / `PQII` path are technical links whose purpose
is to encode filler/expletive-`it` licensing in the dictionary.

The object-raising path intentionally changes the visible carrier links in
that construction. Before this migration, accepted linkages used the ordinary
`TOo` / `IV` / `Ixt` / `Paf` shape and then relied on PP to verify the `OXi`
witness. The migrated path uses `TQII` / `IQII` / `PQII` to make the witness
explicit before PP.

### Verification

Focused accepted-linkage comparison inspected the first three displayed
accepted linkages for:

```text
It is unknown whether he came.
I made it clear how to use the program.
I want it to be obvious how to use the program.
```

The same grammatical constructions remain accepted. Direct subject and direct
object-complement examples preserve the public `QIi`, `SFsi`/`OXi`, and
predicate links while adding `QIIC`. The object-raising example remains
accepted through `OXi` and `QIi`, with the old ordinary infinitival carrier
replaced by `TQII` / `IQII` / `PQII`.

The rule 22 migration was validated with ordinary parser runs:

```sh
link-parser < ./data/en/corpus-knowledge.batch
link-parser < ./data/en/corpus-basic.batch
link-parser < ./data/en/corpus-fixes.batch
link-parser < ./data/en/corpus-fix-long.batch
link-parser < ./data/en/corpus-failures.batch
```

Observed results:

```text
corpus-knowledge.batch: 0 errors
corpus-basic.batch: 88 errors
corpus-fixes.batch: 359 errors
corpus-fix-long.batch: 8 errors
corpus-failures.batch: 1496 errors
```

## Rule 24: License `Ci` Complements With Filler `It`

**Status:** implemented; PP rule 24 has been removed from `4.0.knowledge`.

### Rule / Area

The removed PP rule was:

```text
Ci , SFsi SFIsi OXi , "Complement requires 'it'24"
```

The grammatical area is filler/expletive-`it` licensing of finite-clause
predicate complements:

```text
It is likely he came.
It is surprising he came.
I considered it likely he came.
I want it to be likely he came.
```

These constructions differ from ordinary subject predicates. A sentence such as
`*Joe is likely he came` has a locally possible predicate and finite-clause
relation, but it lacks the filler/expletive `it` relation that licenses the
`Ci` complement.

### Problem

The old dictionary exposed `Ci+ & CV+` directly on adjectives such as `likely`,
`surprising`, and related finite-clause predicates. PP then checked whether the
completed `Ci` domain also contained one of the filler/expletive-`it` relations
`SFsi`, `SFIsi`, or `OXi`.

Without that check, raw linkages could attach an ordinary subject or a
non-`it` object while still selecting the finite-clause predicate branch:

```text
*Joe is likely he came.
*I considered Anne likely he came.
```

The local `is --Paf-- likely --Ci-- he --CV-- came` shape is not sufficient by
itself; the predicate branch needs evidence that the clause uses
filler/expletive `it`.

### Implementation

The direct subject, inverted subject, and direct object-complement cases use a
dedicated certificate:

```text
<cii-verb>: CIIC- & {hHM+} & Ci+ & CV+;
```

The filler/expletive `it` entry can supply `CIIC+` on the same three witness
branches used by the old PP rule:

```text
SFsi  -> CIIC
SFIsi -> CIIC
OXi   -> CIIC
```

Direct object-complement adjectives need the opposite left-connector order from
direct subject predicates. In `It is likely he came`, the adjective sees the
closer `Paf-` link before the farther `CIIC-` link. In `I considered it likely
he came`, the adjective sees the closer `CIIC-` link from `it` before the
farther `Paf-` link from the higher verb. The dictionary therefore includes a
separate object-complement order:

```text
<cii-obj-verb>: CIIC- & (Paf- or dMJra-) & {hHM+} & Ci+ & CV+;
```

The `Ci` certificate deliberately excludes the `AF` predicate path. A focused
regression probe showed that allowing `AF` under the direct `Ci` certificate
accepted:

```text
*It is more likely that Joe died than John is that Fred died.
```

In that bad linkage, the initial filler `it` licensed `likely`, while `likely`
used a distant comparative `AF` link and took a separate `Ci` complement from
the later clause. Keeping `AF` out of the `Ci` certificate prevents an outer
filler `it` from licensing that comparative path.

Object-raising `it to be ...` cases need a carrier rather than a direct `CIIC`
link, because a direct link from `it` to the lower adjective would cross the
infinitival predicate structure. The replacement uses a carrier chain analogous
to the rule-22 `QIi` carrier:

```text
<cii-too-verb>: TCII+;
to.r: ... TCII- & ICII+ ...
be.v: ... ICII- & PCII+ ...
adjective: PCII- & Ci+ & CV+
```

Thus `I want it to be likely he came` carries the `OXi` evidence through
`want --TCII-- to --ICII-- be --PCII-- likely`.

### Implications

This is a dictionary replacement for rule 24. Valid `Ci` linkages now show one
of the explicit certificate paths where the old grammar used PP to infer the
same condition after extraction. The direct `CIIC` path and the object-raising
`TCII` / `ICII` / `PCII` path are technical links whose purpose is to encode
filler/expletive-`it` licensing in the dictionary.

The object-raising path intentionally changes the visible carrier links in that
construction. Before this migration, accepted linkages used the ordinary `TOo`
/ `IV` / `Ixt` / `Paf` shape and then relied on PP to verify the `OXi` witness.
The migrated path uses `TCII` / `ICII` / `PCII` to make the witness explicit
before PP.

### Verification

Focused accepted-linkage comparison inspected the first three displayed
accepted linkages for:

```text
It is likely he came.
I considered it likely he came.
I want it to be likely he came.
```

The same grammatical constructions remain accepted. Direct subject and direct
object-complement examples preserve the public `Ci`, `CV`, `SFsi`/`OXi`, and
predicate links while adding `CIIC`. The object-raising example remains
accepted through `OXi` and `Ci`, with the old ordinary infinitival carrier
replaced by `TCII` / `ICII` / `PCII`.

The rule 24 migration was validated with ordinary parser runs:

```sh
link-parser < ./data/en/corpus-knowledge.batch
link-parser < ./data/en/corpus-basic.batch
link-parser < ./data/en/corpus-fixes.batch
link-parser < ./data/en/corpus-fix-long.batch
link-parser < ./data/en/corpus-failures.batch
```

Observed results:

```text
corpus-knowledge.batch: 0 errors
corpus-basic.batch: 88 errors
corpus-fixes.batch: 359 errors
corpus-fix-long.batch: 8 errors
corpus-failures.batch: 1496 errors
```

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

The dictionary removes the naked `I*a+` fallback from `to.r`. Existing licensed
infinitival constructions continue to use their narrower dictionary paths.
When a valid construction used the old fallback, it must receive a local
license rather than restoring the broad `to.r` branch.

Comparative infinitives headed by `than.e` are one such valid case. They now
use an explicit local license on `than.e`:

```text
WTHAN- & TO+ & IV+
```

The `WTHAN` link is supplied by `way` / `ways` nouns. This lets `than.e`
connect to both `to.r` and the infinitival verb directly without accepting
arbitrary adjective comparatives with a broad `MVp` modifier link.

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
There is no nicer way than to leave now.
There is no nicer way to round off the evening than to have a quiet nightcap.
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

## Controlled-Subject Rules: `Sj` Requires `I#j`, `Sg` Requires `Pg`

**Status:** implemented; both PP rules have been removed from
`4.0.knowledge`.

### Rule / Area

The removed PP rules were:

```text
Sj , I#j , "Bad use of bare infinitive"
Sg , Pg  , "Bad use of present participle"
```

The grammatical area is controlled-subject complements, where an object or
object-like phrase supplies the logical subject of a following bare infinitive
or present participle.

### Problem

The old dictionary used `Sj` and `Sg` as controlled-subject links:

```text
him --Sj-- sing      paired with I*j
him --Sg-- breathing paired with Pg
```

PP then checked whether a linkage containing `Sj` also contained `I#j`, and
whether a linkage containing `Sg` also contained `Pg`.

### Overgeneration Cause

`Sj` and `Sg` are subscripted forms of ordinary subject `S`. Broad subject
connectors such as `S-` can match subscripted `S` forms, so a finite-subject
path could accidentally produce a PP-allowed-looking controlled-subject link.
This made simple PP-rule removal unsafe: raw linkages could contain `Sj` or
`Sg` without being part of the intended controlled complement.

### Implementation

The controlled-subject links now use dedicated uppercase connector families:

```text
SJI  controlled subject of a bare infinitive, paired with I*j
SGP  controlled subject of a present participle, paired with Pg
```

The subject-side object paths that used to offer `Sg+` or `Sj+` now offer
`SGP+` or `SJI+`. The embedded verb paths use `SJI- & I*j-` for bare
infinitives and `SGP- & Pg-` for present participles. This keeps the old
semantic relation but removes the broad-match interaction with finite `S`.

### Examples

Focused accepted examples include:

```text
You should hear John sing.
You should hear him sing.
I feel him breathing down my back.
John imagines himself singing from a mountaintop.
```

Focused rejected examples include:

```text
*You should hear him sings.
*I feel him breathes down my back.
*John imagines himself sings from a mountaintop.
```

### Verification

The controlled-subject migration was validated with ordinary parser runs:

```sh
link-parser < ./data/en/corpus-knowledge.batch
link-parser < ./data/en/corpus-basic.batch
link-parser < ./data/en/corpus-fixes.batch
link-parser < ./data/en/corpus-fix-long.batch
```

Expected results for the current documented state:

```text
corpus-knowledge.batch: 0 errors
corpus-basic.batch: 88 errors
corpus-fixes.batch: 361 errors
corpus-fix-long.batch: 8 errors
```

## Rule 67: Result-Clause `that` Requires A Degree Or `such` Witness

**Status:** implemented; the PP rule has been removed from `4.0.knowledge`.

### Rule / Area

The removed PP rule was:

```text
MVh , EExk EAxk D##k , "Incorrect use of that67"
```

The grammatical area is result-clause `that`, as in `so quickly that ...`,
`so big that ...`, and `such eloquence that ...`.

### Problem

The old `that.j-c` entry allowed a bare `MVh-` result-clause attachment. PP
then checked whether the linkage also contained a degree or determiner witness
with one of the historical names `EExk`, `EAxk`, or `D##k`. That witness test
was global within the PP domain rather than encoded in the local connector
geometry of the result-clause construction.

Without the PP rule, the bare `MVh-` branch accepted result-clause-shaped
linkages such as `likely that ...`, `quickly that ...`, or `big that ...`
without the degree expression that makes the construction grammatical.

### Overgeneration Cause

The old connector path separated the clause attachment from the licensing
degree phrase. The result-clause `that` could link through `MVh` even when the
modified adverb, adjective, or noun did not carry the required `so`,
`sufficiently`, or `such` relation.

### Implementation

The dictionary now replaces the bare `that.j-c [[MVh-]]` path with certified
alternatives:

```text
RTHAT- & MVh-
MVH-
```

Adverbial and noun result clauses keep the predicate-to-`that` `MVh` link and
add an `RTHAT` certificate from the modified adverbial or noun to `that.j-c`.
The degree or determiner relation to that modified word uses an uppercase
certificate family:

```text
so.e / sufficiently --EEXK-- adverb-or-degree-target --RTHAT-- that.j-c
such / such a / such an --DTHAT-- noun --RTHAT-- that.j-c
```

Adjective-headed result clauses cannot use a separate `RTHAT` link without
duplicating the same adjective-to-`that` endpoint pair, so they use the
combined `MVH` connector:

```text
so.e / sufficiently --EAXK-- adjective --MVH-- that.j-c
```

The older lowercase `EExk`, `EAxk`, and `D...k` links remain available for
ordinary non-result-clause uses such as `so quickly`, `so big`, and `such
eloquence`.

### Examples

Focused accepted examples include:

```text
He ran so quickly that he fell.
It went almost so well that we thought we won!
I love her so much that I can't let her go.
The shuttle is so big that it has to be carried.
She presented her case with such eloquence that we could only admire her.
```

Focused rejected examples include:

```text
*He ran quickly that he fell.
*The shuttle is big that it fell.
*She presented her case with eloquence that we could only admire her.
```

### Verification

The rule 67 migration was validated with ordinary parser runs:

```sh
link-parser < ./data/en/corpus-knowledge.batch
link-parser < ./data/en/corpus-basic.batch
link-parser < ./data/en/corpus-fixes.batch
link-parser < ./data/en/corpus-fix-long.batch
```

Expected results for the current documented state:

```text
corpus-knowledge.batch: 0 errors
corpus-basic.batch: 88 errors
corpus-fixes.batch: 361 errors
corpus-fix-long.batch: 8 errors
```

## Rule 68: License `B#m` Through Wh/Degree Dictionary Certificates

**Status:** implemented; the PP rule has been removed from `4.0.knowledge`.

### Rule / Area

The removed PP rule was:

```text
B#m , D##w H HA , "Bad use of gerund68"
```

The active grammatical area is wh/degree extraction with `B#m`; the old
message is a stale description of this rule. Representative accepted examples
include:

```text
Which dog did you chase?
How many do you want?
How many more times did you do it?
How many miles did you bike?
How much effort did you give it?
What degree of trust do you put in him?
How big a department is it?
```

### Background And Problem

The nearby `4.0.knowledge` comments record an older attempt to make `B#m`
restricted after `Rw` was introduced, followed by a reversion because the
restriction broke wh/degree questions such as `how many people you saw` and
`how efficient a program is it`.

Removing the PP rule without a dictionary replacement exposed two classes of
bad paths. First, ordinary quantity or determiner links could feed extracted
nouns without proving that the quantity phrase was wh/degree licensed. Second,
ordinary `B#m` relative/extraction branches could be selected after local noun
modifiers that did not carry any `D##w`, `H`, or `HA` evidence into the
extraction domain.

Observed bad examples included:

```text
*How fast programs are they.
*How much bigger dogs are they.
*How much more efficient programs are available.
```

### Old Mechanism

The PP rule rejected a completed domain if it contained a `B#m` link but did
not also contain a wh/degree witness link matching `D##w`, `H`, or `HA`.

### Overgeneration Cause

The old dictionary used ordinary links both for non-extracted noun phrases and
for branches that later selected `B#m`. For example, the unit-noun path used
ordinary `ND`:

```text
many --ND-- miles --Bpm-- bike
```

Likewise, ordinary `D**w`, `Dmc`, `Dmu`, and `EC` paths could appear in
branches that did not necessarily carry the domain witness needed by the old
PP rule. Locally each link was a valid dictionary link, so the overgeneration
was visible only after the completed-linkage domain scan.

The migration is deliberately not implemented as a global `B*m` connector
rewrite. `B*m` is used outside the old domain-scoped PP condition. A global
certificate on all `B*m` links would reject valid sentences that the old PP
rule did not reject.

### Implementation

The dictionary now uses a small family of wh/degree certificates for the
`B#m` extraction branches:

- `NDH` licenses unit-noun extraction from `H`, as in `How many miles did you
  bike?`.
- Bare `many` and `much` can expose the raw `Rw+ & B#m+` extraction branch
  only inside their `H-` disjuncts.
- `DWHs`, `DWHp`, and `DWHu` are wh/degree determiner certificates for
  singular, plural, and uncountable extraction nouns. They replace ordinary
  `D**w`, `Dmc`, or `Dmu` only on the certified extraction branch.
- `ECWH` carries an `H` certificate through `more` to a following `DWH` or
  `BWH` extraction target.
- `BWH` carries an `HA` certificate to the noun that selects the `B#m`
  extraction branch. Ordinary `HA` paths remain available without `BWH` for
  non-extraction degree phrases.

Ordinary quantity and determiner paths continue to use their established
links, including:

```text
Many people came.
You biked how many miles?
It was so big a dog that it filled the cage.
```

This makes the extraction branch carry the required wh/degree witness and
removes the need for the PP companion check.

### Examples

Focused examples are recorded in `corpus-knowledge.batch`:

```text
Which dog did you chase?
How many do you want?
How many more times did you do it?
How many miles did you bike?
How much of a man is he?
How much effort did you give it?
What degree of trust do you put in him?
He won't divulge what type it is.
How big a department is it?
*The telling John to leave was stupid.
*How much more efficient programs are available.
```

### Verification

Verification compared accepted/rejected outcomes with `lgerror` and used
focused accepted-linkage inspection against `master`. The migration preserves
the accepted zero-null analyses while intentionally changing the certificate
links on the migrated extraction branch. Examples include `ND -> NDH` for
unit quantities, `D**w`/`Dmc`/`Dmu -> DWH*` on certified extraction noun
phrases, and `EC -> ECWH` when `more` carries an `H` certificate.

```text
corpus-knowledge.batch: 0 errors
corpus-basic.batch: 88 errors
corpus-fixes.batch: 361 errors
corpus-fix-long.batch: 8 errors
```

## Rule 71 (`Ws`): Keep Subject How-Questions Out Of Extraction Paths

**Status:** implemented; the PP rule has been removed from `4.0.knowledge`.

### Rule / Area

The removed PP rule was:

```text
Ws , B#m Ca BT , "Question inversion violated71"
```

The grammatical area is sentence-opening `how` questions. A `Ws` question can
be a subject question:

```text
How many dogs ran?
How much sugar is needed?
```

but extracted-object and non-inverted clause continuations must use the
ordinary question or embedded-clause paths:

```text
How much money did you earn?
I wonder how many times you did it.
```

### Problem

Removing the PP rule without a dictionary replacement allowed `Ws` to combine
with extraction and non-inverted-clause continuations:

```text
*How much money you earn
*How many times you did it
*How much of the book you read
```

The historical `BT` criterion is stale in the current dictionary: no active
English dictionary endpoint uses a `BT` connector. The live rule-71 work is
therefore the distinction between safe `Ws` quantity/temporal paths and the
bad `B#m` / `Ca` continuations.

### Old Mechanism

PP rejected a completed domain when a `Ws` link co-occurred with `B#m`, `Ca`,
or `BT`. That caught the bad examples after extraction, but it also forced the
postprocessor to inspect a distinction that can be represented directly in
the dictionary.

### Overgeneration Cause

The old `how` entry used the same `H` or `EEh` relation for both `Wq` and
`Ws` starts. Once `how` selected `Ws`, downstream ordinary quantity and
temporal branches could still choose paths that belonged to extracted-object
or non-inverted-clause analyses. The local links were all dictionary-legal, so
the error was visible only to the PP domain scan.

### Implementation

The dictionary now splits the relevant `how` starts:

- `HWS` certifies `Ws` quantity subject questions and exposes only
  non-extraction continuations.
- `DWSu` carries the `HWS` branch to uncountable subject nouns without using
  broad `Dmu`, which can match wh-extraction variants.
- `EEHWS` certifies the focused temporal-fragment path `how long before ...`.

The ordinary `H` and `EEh` paths remain available for `Wq`, embedded `QI`, and
other non-`Ws` starts. The still-active rule-5 PP check treats `HWS` and
`EEHWS` as valid `Ws` companions, while rule 71 itself is no longer needed.

### Examples

Focused examples are recorded in `corpus-knowledge.batch`:

```text
How many people died?
How much sugar is needed?
How long before you got home?
Approximately how long before you got home?
How much money did you earn?
How many times did you do it?
*How much money you earn
*How many times you did it
*How much of the book you read
```

### Verification

Verification used focused accepted/rejected probes, `lgerror` corpus
comparison, and top-linkage inspection of representative accepted sentences.
The expected visible difference is that subject `Ws` how-quantity linkages use
`HWS`/`DWSu`, and `how long before ...` uses `EEHWS`, instead of ordinary
`H`/`Dmu`/`EEh` on those specific migrated paths.

```text
corpus-knowledge.batch: 0 errors
corpus-basic.batch: 88 errors
corpus-fixes.batch: 359 errors
corpus-fix-long.batch: 8 errors
```

## `Qd , MX`: Require Punctuation For Name-Based Direct Question Openers

**Status:** implemented; the PP rule has been removed from `4.0.knowledge`.

### Rule / Area

The removed PP rule was the ID-less subject-inversion check:

```text
Qd , MX , "Bad subject inversion"
```

The grammatical area is direct subject-auxiliary inversion after an opener.
Some valid direct questions use a left opener plus `Qd`, for example:

```text
Joe, are you ready?
Anyhow, am I right?
Which way did they go?
```

### Problem

The dictionary also allowed a capitalized name to act as a no-comma
directive opener. If such a name opened a `Qd` path, a following noun could
simultaneously use an `MX` relation, producing accepted raw linkages such as:

```text
*Joe doesn't matter what Ted does.
```

### Old Mechanism

PP rejected completed domains that contained both `Qd` and `MX`. That blocked
the bad capitalized-name path, but it also kept a rule for a distinction that
is local to the dictionary's opener analysis.

### Overgeneration Cause

`<directive-opener>` supplied a no-punctuation fallback for both `Qd` and `Wq`
openers. Given-name entries reused that ordinary opener expression, so `Joe`
could take the same no-comma `Wc- & Qd+` opener disjunct as legitimate
non-name opener heads. The resulting raw linkage was locally legal until PP
noticed that the same domain also contained an `MX` link.

### Implementation

The dictionary now uses a separate `<directive-opener-entity>` expression for
capitalized entities and given names. In that entity-specific expression,
`Qd` openers require punctuation (`Xc+ & Qd+`). The ordinary
`<directive-opener>` still permits its previous no-comma `Qd` behavior for
non-entity opener heads, preserving idiomatic wh-adverbial questions such as
`Which way did they go?`. The `Wq` opener fallback is unchanged.

### Examples

Focused examples are recorded in `corpus-knowledge.batch`:

```text
Joe, are you ready?
Anyhow, am I right?
It doesn't matter what Ted does.
*Joe doesn't matter what Ted does.
*Mary doesn't matter what Ted does.
```

### Verification

Accepted-linkage comparison against `master` for `Joe, are you ready?`,
`Anyhow, am I right?`, and `It doesn't matter what Ted does.` showed matching
public link rows for the first three accepted displayed linkages. The focused
regression checks also preserved `Which way did they go?` and
`Which way did you come?`.

```text
corpus-knowledge.batch: 0 errors
corpus-basic.batch: 88 errors
corpus-fixes.batch: 359 errors
corpus-fix-long.batch: 8 errors
corpus-failures.batch: 1498 errors
```

## Rule 62 (`Pa##j`): License Predicative-Adjective Objects With `OAJ`

**Status:** implemented; the PP rule has been removed from `4.0.knowledge`.

### Rule / Area

The removed PP rule was:

```text
Pa##j , Osm Op Ox Os*e OXi , "Bad predicative adj 62"
```

The grammatical area is object-oriented predicative-adjective complements:

```text
I want it green.
I want John sober.
The doctor declared him insane.
```

### Problem

The old dictionary let several verb classes expose `Pa**j+` together with a
broad object connector such as `O+` or an extracted-object fallback. That was
too broad for singular common-noun objects:

```text
*I want a gift inexpensive.
```

The raw linkage could locally connect the verb to `gift` with ordinary `Os`
and to the adjective with `Pa**j`. The PP rule then rejected it because the
same domain did not contain one of the allowed object-class links.

### Old Mechanism

PP accepted a `Pa##j` domain only if it also contained one of `Osm`, `Op`,
`Ox`, `Os*e`, or `OXi`. This allowed pronouns, plural objects, and named
entities while rejecting the singular common-noun object path.

### Overgeneration Cause

A verb-side macro listing the old allowed `O` subscripts is not safe in Link
Grammar. A positive connector such as `Os*e+` can still match an ordinary
`Os-` noun connector and produce an allowed-looking link name. That recreates
the same bad singular-object path that the PP rule was rejecting.

### Implementation

The dictionary now uses the uppercase `OAJ` connector for ordinary
predicative-adjective object licensing. The allowed object classes expose
`OAJ-`, and verb `Pa**j+` branches require `OAJ+` instead of broad `O+`,
`Ox+`, or extracted-object fallbacks.

The `it` entry also keeps its existing `OXi-` path. Verb `Pa**j+` branches
accept either `OAJ+` or `OXi+`: ordinary `I want it green` uses `OAJ`, while
complement-bearing expletive cases such as `I made it clear that I was angry`
use `OXi` so the still-active expletive-`it` PP rules continue to see their
expected witness.

### Examples

Focused examples are recorded in `corpus-knowledge.batch`:

```text
I want it green.
I want John sober.
I want them ready.
I want these green.
The doctor declared him insane.
*I want a gift inexpensive.
```

### Verification

Accepted-linkage comparison against the pre-migration baseline used
`-test=auto-next-linkage:3` with `!links`, `!limit=10000`, `!short=254`, and
`!null=0`. The first accepted linkages keep the same predicate-adjective
structure; the intended public-link change is that ordinary `Osm`, `Os*e`,
and `Ox` object rows become `OAJ`.

The migration was also validated with ordinary parser runs:

```text
corpus-knowledge.batch: 0 errors
corpus-basic.batch: 88 errors
corpus-fixes.batch: 361 errors
corpus-fix-long.batch: 8 errors
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

Rules 71, 72, 73, and the ID-less `Bad subject inversion` rule were migrated
later by dedicated dictionary changes.

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

## Rule 72: Split Filler-Subject `SF` From Ordinary Continuations

**Status:** implemented; PP rule 72 has been removed from `4.0.knowledge`.

### Rule / Area

The removed PP rule was:

```text
SF , I* PP* TO* Pa* Pam Pg* Pv* LE* AFd* MVta,
     "Bad use of 'filler' subject72"
```

The grammatical area is non-inverted filler-subject `SF` domains. A filler
subject path should not also license ordinary infinitival, predicative,
comparative, or modifier continuations in the same local domain. Those
continuations belong either to ordinary subjects or to explicitly certified
filler/expletive constructions.

### Problem

After the surrounding expletive-`it` migrations, simple deletion of rule 72
left two observable bad zero-null paths.

The first was an infinitival `to.r` path:

```text
*Absence to comply may result in dismissal.
```

The raw linkage treated `to.r` as the sentence subject with an `SFsx` link to
`may` and an ordinary `I` link to `comply`:

```text
Absence --COa--> to
to ------SFsx-- may
to ------I----- comply
may -----I----- result
```

The grammatical positive uses a local nominal infinitive relation instead:

```text
Failure --TOn-- to
Failure --IV--> comply
Failure --Ss--- may
```

The second bad path was a finite comparative that-clause:

```text
*It is more likely that Joe died than John is that Fred died.
```

After rule 30 retired `AFdi`, the remaining raw overgeneration used ordinary
`AFd`:

```text
likely --LE-- than
than ---AFd-- is
than ---THc-- that Fred died
John ---Ss--- is
```

The `AFd` link made the local connector structure look plausible before PP,
but the lower finite clause did not have a filler/expletive subject relation.

### Implementation

The dictionary replacement removes the overbroad direct `SFsx` continuation
from the affected infinitival `to.r` branch:

```text
to.r: ... {NT-} & I+ & (... no SFsx+ & <S-CLAUSE> ...)
```

Valid nominal infinitive subjects such as `Failure to comply may result...`
remain available through the local noun-side `TOn` / `IV` path.

The finite comparative branch on `than.e` is also split. The finite
that-clause arm no longer carries optional `AFd+`:

```text
(LE- or LEi-) & THc+
```

The ordinary `AFd+` comparative evidence remains available for infinitival
comparative continuations:

```text
((LE- & {AFd+}) or LEi-) &
  ((TOic+ & <inf-verb>) or (TOfc+ & <inf-verb>) or (TOtc+ & B+))
```

This keeps examples such as `It is easier to ignore the problem than it is to
solve it` while preventing the finite `AFd + THc` leak.

### Verification

Focused accepted-linkage comparison inspected the first three displayed
accepted linkages for:

```text
Failure to comply may result in dismissal.
It is easier to ignore the problem than it is to solve it.
It is more likely that Joe died than that Fred died.
```

The accepted parses remain available. `Failure to comply...` uses the
noun-side `TOn` / `IV` subject relation; the comparative infinitival example
keeps the ordinary `AFd` plus `TOic` / `TOfc` analyses; and the finite
that-clause comparative keeps the direct `LE` / `THc` path without `AFd`.
The two starred examples above have no accepted zero-null linkage after the
migration.

The rule 72 migration was validated with ordinary parser runs:

```sh
link-parser < ./data/en/corpus-knowledge.batch
link-parser < ./data/en/corpus-basic.batch
link-parser < ./data/en/corpus-fixes.batch
link-parser < ./data/en/corpus-fix-long.batch
link-parser < ./data/en/corpus-failures.batch
```

Observed results:

```text
corpus-knowledge.batch: 0 errors
corpus-basic.batch: 88 errors
corpus-fixes.batch: 359 errors
corpus-fix-long.batch: 8 errors
corpus-failures.batch: 1496 errors
```

## Rule 73: Separate Filler-It Inversion From Ordinary `I`

**Status:** implemented; the PP rule has been removed from `4.0.knowledge`.

### Rule / Area

The removed PP rule was:

```text
SFI , I* PP* TO* Pa* Pam Pg* Pv* LE* AFd* MVta,
      "Bad use of 'filler' subject73"
```

The grammatical area is inverted auxiliary questions with filler `it`, as in:

```text
Does it seem likely that Joe came?
Did it appear likely that Joe came?
```

### Problem

The old dictionary allowed the same auxiliary branch to combine `SFI` with
the ordinary lower-verb continuation `I*d+`. That was too broad: a raw linkage
could connect an inverted filler-`it` auxiliary to a lower predicate that did
not license expletive/filler `it`, and PP then had to reject the completed
domain if ordinary complement or modifier links appeared with `SFI`.

The bad local path looked syntactically plausible to the connector matcher:

```text
does --SFI-- it
does --I*d-- generic lower verb
```

But `I*d` is also needed for ordinary subject questions such as:

```text
Does Joe seem likely to come?
```

So the dictionary replacement must split the filler-`it` auxiliary path
without narrowing the ordinary-subject `I` path.

### Old Mechanism

PP rejected an `SFI` domain that also contained one of the broad ordinary
continuation or complement links:

```text
I* PP* TO* Pa* Pam Pg* Pv* LE* AFd* MVta
```

This was a completed-linkage backstop. It did not prevent the parser from
building and ranking raw candidates with the bad `SFI + I*` shape.

### Implementation

The `do`/`does`/`did` inverted auxiliary entries now split ordinary-subject
and filler-`it` continuations:

```text
ordinary subject question:  SI*  ... I*d+
filler-it question:         SFI* ... IFI+
```

Lower predicates that can participate in this filler-`it` raising path expose
`IFI-` through the same area that already accepts `If-`-style lower-verb
relations, for example the `seem` / `appear` class. Generic lower verbs keep
ordinary `I` continuations and therefore cannot be reached from an `SFI`
auxiliary branch.

`IFI` is an uppercase connector family rather than a subscripted variant of
`I`. A subscripted spelling such as `Ifi` would still match broad ordinary
`I` connectors, recreating the same leak that rule 73 had to catch.

### Implications

Valid filler-`it` raising questions remain accepted, but their accepted
linkages now show `IFI` where the older linkage used an ordinary `I*d`/`If`
continuation. Ordinary-subject raising questions keep the ordinary `I`
relation. Rule 72 remains active for the non-inverted `SF` filler-subject
case; this migration covers only rule 73's inverted `SFI` selector.

### Examples

Focused examples include:

```text
Does it seem likely that Joe came?
Does it appear likely that Joe came?
Did it seem likely that Joe came?
Does Joe seem likely to come?
*Does it act likely that Joe came?
*Did it act likely that Joe came?
```

### Verification

The rule 73 migration should be validated with ordinary parser runs:

```sh
link-parser < ./data/en/corpus-knowledge.batch
link-parser < ./data/en/corpus-basic.batch
link-parser < ./data/en/corpus-fixes.batch
link-parser < ./data/en/corpus-fix-long.batch
link-parser < ./data/en/corpus-failures.batch
```

Expected results at the time of this migration:

```text
corpus-knowledge.batch: 0 errors
corpus-basic.batch: 88 errors
corpus-fixes.batch: 359 errors
corpus-fix-long.batch: 8 errors
corpus-failures.batch: 1497 errors
```

Focused accepted-linkage comparison against the pre-migration baseline
inspected the first three displayed accepted linkages for:

```text
Does it seem likely that Joe came?
Does it appear likely that Joe came?
Did it seem likely that Joe came?
```

The same filler-`it` raising constructions remain accepted. The intentional
public-link difference is the new certificate link:

```text
I*d / If  -> IFI
```

## Redundant Bounded `r`-Domain PP Check

**Status:** implemented; the bounded `r` rule has been removed from
`4.0.knowledge`.

### Rule / Area

The removed PP rule was:

```text
r , "Unbounded r domain79"
```

The grammatical area is relative-domain boundedness. The active `r` domain
starters include relative links such as `R*`, `Mr`, and `MX#r`.

### Problem

The old PP rule rejected any `r` domain whose participating words fell to the
left of the domain root. In the current dictionary, this check did not carry
observable protection in the tracked corpora or in focused `!bad` probes, while
keeping it in the postprocessor would leave another rule in the metric
extraction blocker set.

The corresponding `s` bounded-domain rule is not redundant. Its removal accepts
known bad wh/inversion examples, so that rule remains active and is listed in
the deferred migration table above.

### Old Mechanism

`BOUNDED_RULES` is a PP-level domain-graph check rather than a link-pair
contains-one/contains-none check. It runs after a linkage has already been
extracted and its domains have been built. The `r` rule therefore could only
reject completed raw linkages after extraction.

### Implementation

The `r` entry is removed from `BOUNDED_RULES`. No new connector family is
introduced and no generated dictionary change is required.

### Examples

Focused relative-clause coverage is recorded in `corpus-knowledge.batch`:

```text
The syndicates whose activities add to the cost left.
The syndicates whose activities add to the cost of business left.
The syndicates whose activities add to the cost of doing business left.
```

### Verification

The removal was validated with ordinary parser runs. `lgerror` comparison
against the pre-removal outputs for `corpus-knowledge.batch`,
`corpus-basic.batch`, and `corpus-fixes.batch` showed no sentence-level
differences. `corpus-fix-long.batch` kept the same error count. The focused
relative-clause examples above were also compared against the reference branch
with `!links`, `!limit=10000`, `!short=254`, `!null=0`, and graph output
disabled; the first three accepted public link rows match. The total linkage
and no-PP-violation counts differ because the PP rule itself has been removed.

Expected results:

```text
corpus-knowledge.batch: 0 errors
corpus-basic.batch: 88 errors
corpus-fixes.batch: 359 errors
corpus-fix-long.batch: 8 errors
corpus-failures.batch: 1497 errors
```

`corpus-failures.batch` improves by one; the changed sentence is a long
relative-clause example headed by `whose nefarious activities add so much to
the cost of doing business in New York`.

## Redundant `CONTAINS_ONE` Question-Inversion Checks

**Status:** implemented for rules 8 and 9.

### Rule / Area

The removed PP rules were:

```text
Qd  , SI SFI SXI THRS THRP THRU , "S-V inversion required8"
PFc , SI SFI SXI                , "S-V inversion required9"
```

The grammatical area is direct subject-verb inversion. Rule 8 handled `Qd`,
including ordinary yes/no questions and the existential-`there` paths migrated
earlier. Rule 9 handled the historical `PFc` selector.

### Problem

After the existential-`there` migration, the current dictionary already
exposes `Qd` only through ordinary question/inversion paths or through the
agreement-specific `THRS` / `THRP` / `THRU` paths. Rule 8 therefore no longer
carries observable behavior in the tracked corpora.

Rule 9 is stale under the current dictionary. The only `PFc` connector
occurrences are positive occurrences on `as.e`, `as.e-c`, and `than.e`; there
is no matching negative `PFc` occurrence, so no completed linkage can contain a
`PFc` link. The PP rule therefore cannot reject any current linkage.

### Implementation

Rules 8 and 9 are removed from `4.0.knowledge`; no replacement connector
family is needed.

### Examples

Focused rule-8 examples are recorded in `corpus-knowledge.batch`:

```text
Does he drink?
Are you insane?
Is there a dog in the park?
Are there dogs in the park?
```

There is no focused rule-9 sentence because `PFc` is unreachable in the
current dictionary.

### Verification

The removals were validated with ordinary parser runs and `lgerror`
comparison against the pre-removal outputs for `corpus-knowledge.batch`,
`corpus-basic.batch`, `corpus-fixes.batch`, and `corpus-failures.batch`; all
comparisons had zero error differences.

```text
corpus-knowledge.batch: 0 errors
corpus-basic.batch: 88 errors
corpus-fixes.batch: 361 errors
corpus-fix-long.batch: 8 errors
corpus-failures.batch: 1499 errors
```

## Rule 7a: Bare-`what` `Wb` Requires An Inverted Verb

**Status:** implemented; PP rule 7a has been removed from `4.0.knowledge`.

### Rule / Area

The removed PP rule was:

```text
Wb , SI SFI SXI , "S-V inversion required7a"
```

The grammatical area is bare-`what` topic questions such as `What did you
think?` and idiomatic variants such as `What the hell were you thinking?`.

### Problem

The old `what` entry exposed `Wb-` through the same broad branch that also
exposed ordinary wh/opening links. That branch could attach `what` to a
following non-inverted clause with ordinary `R`, leaving PP to reject the
completed linkage later because no `SI`, `SFI`, or `SXI` inversion evidence
appeared in the same domain.

The bad sentence below demonstrates the rejected raw path:

```text
*What the outcome is, I'm sure he'll still be popular.
```

The invalid `Wb` analysis attached `what` to `is` through the same ordinary
`R` / `Bsw` path used by valid wh extractions, but the clause was not an
inverted question.

### Implementation

The dictionary now splits the `Wb` branch from the ordinary `what` branch.
Bare-`what` `Wb` requires a dedicated `RWB` link to the verb question path:

```text
LEFT-WALL --Wb-- what --RWB-- did --SI-- you
                 what --------BW-------- think
```

`RWB-` is exposed only through the same verb-question macros that expose the
ordinary `Rw-` relation. Non-`Wb` wh/opening paths keep ordinary `R`, so this
change does not attempt to solve the still-deferred `Wq` and `Ws` rules.

### Examples

Focused examples are recorded in `corpus-knowledge.batch`:

```text
What did you think?
What were you thinking?
What the hell were you thinking?
*What the outcome is, I'm sure he'll still be popular.
```

### Implications

This is a narrow replacement for rule 7a only. It turns the old after-the-fact
domain requirement into a local dictionary certificate on the `what`-to-verb
question path. Related `Wq` and `Ws` checks remain active and still need their
own dictionary treatment.

### Verification

The migration was validated with ordinary parser runs:

```sh
link-parser < ./data/en/corpus-knowledge.batch
link-parser < ./data/en/corpus-basic.batch
link-parser < ./data/en/corpus-fixes.batch
link-parser < ./data/en/corpus-fix-long.batch
link-parser < ./data/en/corpus-failures.batch
```

Results:

```text
corpus-knowledge.batch: 0 errors
corpus-basic.batch: 88 errors
corpus-fixes.batch: 359 errors
corpus-fix-long.batch: 8 errors
corpus-failures.batch: 1498 errors
```

Focused accepted-linkage comparison against `master` for `What did you
think?`, `What were you thinking?`, and `What the hell were you thinking?`
showed the expected replacement in the first displayed linkage: the old `Rw`
edge from `what` to the inverted verb becomes `RWB`. The next `Wq` and `Ws`
displayed linkages keep their old public link structure.

## Rule 3: Filler-It Inversion Needs Question Evidence

**Status:** implemented; PP rule 3 has been removed from `4.0.knowledge`.

### Rule / Area

The removed PP rule was:

```text
SFI##* , Wq Qd CQ PFc , "Bad use of s-v inversion3"
```

The grammatical area is subject-verb inversion with filler `it`, as in
matrix questions such as `Is it likely that Joe came?`.

### Problem

The ordinary dictionary path for `is.v` allowed the filler-it inversion
connector `SFIs+` through an optional question expression. That expression
also had an empty fallback, which was useful for non-inverted `SIs` paths but
too permissive for `SFIs`. After PP rule 3 was removed, the embedded question
below acquired a zero-null raw linkage:

```text
*I wonder how important is it to turn off the computer.
```

In that bad linkage, `wonder.v` takes `QI*d` to `how`, `how` links by `EAh`
to `important.a`, `important.a` links by `AF` to `is.v`, and `is.v` links by
`SFIsi` to `it`. No actual matrix-question root link appears in the same
domain, so the inversion is embedded where English requires ordinary order:

```text
I wonder how important it is to turn off the computer.
```

### Implementation

The dictionary now distinguishes optional question context from required
question-root context. `<verb-rq-required>` is the non-empty counterpart of
`<verb-rq>`: it keeps the real `Rw`, `RWB`, `Qd`, `Qp`, `Qw`, and `Qe`
question links but omits the empty fallback. The `is.v` filler-it inversion
branch uses this required expression for `SFIs+`, while ordinary `SIs`
branches keep the optional expression.

This preserves valid matrix filler-it questions such as `Is it likely that
Joe came?` and `Is it important to turn off the computer?`, but blocks the
embedded inversion path before PP.

### Examples

Focused examples are recorded in `corpus-knowledge.batch`:

```text
Is it important to turn off the computer?
Is it likely that Joe came?
I wonder how important it is to turn off the computer.
*I wonder how important is it to turn off the computer.
```

### Verification

The change was validated with focused positive and negative examples, plus
ordinary parser runs:

```text
corpus-knowledge.batch: 0 errors
corpus-basic.batch: 88 errors
corpus-fixes.batch: 359 errors
corpus-fix-long.batch: 8 errors
corpus-failures.batch: 1497 errors
```

Focused accepted-linkage comparison against `master` for `Is it important to
turn off the computer?`, `Is it likely that Joe came?`, and `I wonder how
important it is to turn off the computer.` showed the same top accepted link
structure, apart from previously migrated connector-name changes unrelated to
rule 3.

## Rule 4: `SXI` Also Licenses Fronted Locative Inversion

**Status:** implemented; PP rule 4 has been removed from `4.0.knowledge`.

### Rule / Area

The removed PP rule was:

```text
SXI , Wq Qd CQ PFc , "Bad use of s-v inversion4"
```

The grammatical area is inverted first-person singular `be` forms. The
historical PP rule treated `SXI` as if it always needed an overt question or
comparative marker in the same domain.

### Problem

The current dictionary also uses `SXI` in fronted locative and adverbial
inversion paths. For example, `here am I` links the fronted word to the verb
with `PFb`, and the verb links to `I` with `SXI`. That linkage is locally
well-formed and expresses the intended inversion, but it does not contain
`Wq`, `Qd`, `CQ`, or the stale `PFc` selector. PP rule 4 therefore rejected a
valid construction.

The failure-corpus sentence below became accepted after the rule was removed:

```text
I mean, here am I, chattering away to you about the outcome of the war
and you actually know, Cassie.
```

### Implementation

Rule 4 is removed from `4.0.knowledge`. No new connector is needed: the
dictionary already distinguishes the valid fronted-inversion path with `PFb`.
The removal lets that path stand without an after-the-fact PP requirement that
was too narrow for current dictionary usage.

### Examples

Focused examples are recorded in `corpus-knowledge.batch`:

```text
Here am I.
Here was I.
There was I.
```

### Verification

The removal was validated with ordinary parser runs and `lgerror` comparison
against the pre-removal outputs for `corpus-knowledge.batch`,
`corpus-basic.batch`, `corpus-fixes.batch`, and `corpus-failures.batch`.
`corpus-failures.batch` improved by one accepted sentence, the `here am I`
example above. The other comparisons had zero error differences.

```text
corpus-knowledge.batch: 0 errors
corpus-basic.batch: 88 errors
corpus-fixes.batch: 361 errors
corpus-fix-long.batch: 8 errors
corpus-failures.batch: 1498 errors
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

## Rule 44: Split Predicative `as.e-c` From Verb-Side `MVz`

**Status:** implemented; PP rule 44 has been removed from `4.0.knowledge`.

### Rule / Area

The removed PP rule was:

```text
MVz , D##y EAy EEy MVy EB#y , "Bad comparative44"
```

The grammatical area is comparative and parenthetical `as` clauses. The
selector `MVz` is used by comparative `as.e-c`, but the old PP rule was too
broad: it rejected valid parenthetical `as` clauses that do not contain the
listed degree witnesses.

### Problem

Simple deletion of rule 44 recovered useful valid examples:

```text
Ridiculous as it seems, the tale is true.
he earns as much as was expected
the claim, unclear as worded, deserves attention
```

but it also accepted the known bad comparative path:

```text
*I am as intelligent as John does.
```

The bad raw path used two unrelated `as` entries. The first attached
`as.e-c` to the verb with the ordinary `MVzp` modifier branch and then used
`Pa` to connect the adjective:

```text
am ----MVzp---- as.e-c ----Pa---- intelligent
```

The second `as` was the temporal-subordinate `as.#while` path:

```text
am ----MVSWH---- as.#while ----Cs/CV---- John does
```

That local shape was enough to look like a completed raw linkage after rule
44 was removed, but it is not a valid comparative `as ... as` construction.

### Implementation

The `as.e-c` predicative-adjective branch now uses a new uppercase
adjective-only connector:

```text
as.e-c: MVZP- & Pa+
```

`MVZP+` is exposed from the adjective opener path, where parenthetical
adjectival `as` clauses such as `unclear as worded` attach. Ordinary verbs
keep broad `@MV+` modifier paths, but they do not expose `MVZP+`; therefore
the bad `am --MVzp-- as.e-c --Pa-- intelligent` path is no longer built.

Other `as.e-c` comparative paths remain on their existing connectors. In
particular, `as much as was expected` still uses `MVz` with `CMPX`, and
`Ridiculous as it seems` still uses the `MVza` / `AFd` / `Cta` path.

### Examples

Focused accepted examples include:

```text
Ridiculous as it seems, the tale is true.
He earns as much as was expected.
The claim, unclear as worded, deserves attention.
```

Focused rejected examples include:

```text
*I am as intelligent as John does.
```

### Verification

The rule 44 migration was validated with:

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
corpus-fixes.batch: 355 errors
corpus-fix-long.batch: 8 errors
```

## Rule 47: Certify Comparative `MV#o` Object-Clause Paths

**Status:** implemented; PP rule 47 has been removed from `4.0.knowledge`.

### Rule / Area

The removed PP rule was:

```text
MV#o , D##m D##y Om Oy Jm Jy Am MX#m , "Bad comparative47"
```

The grammatical area is comparative object-clause continuation, including
`as.e-c` paths such as `as much as he did` and `the same as it did`, and the
corresponding `than.e` branch.

### Problem

Simple deletion of rule 47 left the `as.e-c` `MVzo- & Ct+ & Bc+` branch
available to ordinary broad modifier paths. That accepted the bad adjective
comparative:

```text
*I am as intelligent as John does.
```

In that raw path, the adjective supplied ordinary comparative `EAy` evidence,
but no object or measure witness such as `Oy`, `D##y`, or `D##m` was present
to license an `MV#o` object-clause continuation. The old PP rule rejected the
completed linkage after extraction.

### Implementation

The dictionary now uses the internal certificate connector `CMPO`. Comparative
object and measure paths that correspond to the old PP witnesses can expose
`CMPO+`, for example `Oy` / `Jy` paths on `much`, `many`, and `the_same`,
determiner-measure paths such as `Dmuy`, `Dmcy`, `Dmum`, and `Dmcm`, and
selected comparative adjective `Am` paths.

The `as.e-c` and `than.e` `MV#o` continuations require `CMPO-` next to their
ordinary `MVzo-` or `MVto-` connector:

```text
as.e-c: CMPO- & MVzo- & Ct+ & Bc+
than.e: CMPO- & MVto- & Ct+ & Bc+
```

The alternate connector order is also present for cases where the comparative
witness and modified predicate appear in the opposite left-connector order.
Plain adjective comparatives such as `as intelligent as John does` do not
expose `CMPO`, so they cannot use the `MV#o` branch.

### Examples

Focused accepted examples include:

```text
The coffee tastes the same as it did last year.
I did as much as he did.
I earned as much as John earned.
```

Focused rejected examples include:

```text
*I am as intelligent as John does.
```

### Verification

The rule 47 migration was validated with:

```sh
link-parser < ./data/en/corpus-knowledge.batch
link-parser < ./data/en/corpus-basic.batch
link-parser < ./data/en/corpus-fixes.batch
link-parser < ./data/en/corpus-fix-long.batch
link-parser < ./data/en/corpus-failures.batch
```

Expected results:

```text
corpus-knowledge.batch: 0 errors
corpus-basic.batch: 88 errors
corpus-fixes.batch: 355 errors
corpus-fix-long.batch: 8 errors
corpus-failures.batch: 1496 errors
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

The `BIh` rule is different from the neighboring `BIq` predicate rule: after
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

This removal applies only to `BIh`. The neighboring `BIq` predicate rule is
handled separately by the dedicated certificate connectors documented below.

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

## Rule 42: License `BIq` Predicate Complements In The Dictionary

**Status:** implemented; the PP rule has been removed from `4.0.knowledge`.

### Rule / Area

The removed PP rule was:

```text
BIq , S##q SI##q SFsi Ss#b SFIsi SIs#b ,
      "Bad use of predicate42"
```

The grammatical area is predicate wh-complements, as in:

```text
The question is who we should invite.
The issue has been whether we should leave.
This is who we should invite.
```

### Problem

The old dictionary exposed `BI+` through broad copular paths. PP then checked
the completed domain for a question-like subject or inverted subject link such
as `S##q`, `SI##q`, `Ss#b`, or `SIs#b`. This allowed locally well-formed raw
linkages to be built for ordinary nouns that do not license a predicate
wh-complement.

Simple PP-rule removal accepted bad paths such as:

```text
*The answer is who we should invite.
*The problem is who we should invite.
*The big mind on everybody's question is who killed OJ.
```

An initial dictionary prototype that moved `BI+` to a copular branch with
verb-side `Ss*q-` was unsafe. A generic noun subject can match a subscripted
verb-side connector and inherit the `Ss*q` link name, so subscripted `S`
connectors do not prove that the subject selected the question-license path.

### Implementation

Question-like nouns and pronouns now expose dedicated predicate-license
subject connectors:

```text
BIQS+   direct licensed subject
BIQI-   licensed inverted subject
```

The corresponding copular and auxiliary branches consume:

```text
BIQS-   direct licensed subject on the predicate verb
BIQI+   licensed inverted subject on the predicate verb
```

The broad `BI+` alternatives were removed from ordinary `be` paths and moved
into `vc-be-biq` variants that require the licensed subject path. Auxiliary
chains use dedicated uppercase certificate links:

```text
subject --BIQS-- modal --IBIQ-- be --BIq-- wh-complement
subject --BIQS-- have --PPBIQ-- been --BIq-- wh-complement
```

These are distinct uppercase connector families rather than subscripted
variants such as `S*q`, `I*q`, or `PP*q`. Link Grammar connector matching
would allow broad ordinary connectors to match subscripted variants, so
subscripted certificates cannot safely encode this condition.

### Implications

This is a dictionary replacement for rule 42. The accepted linkages are not
byte-for-byte identical to older master-style output: valid `BIq` predicates
now show `BIQS`, `BIQI`, `IBIQ`, or `PPBIQ` certificate links where master
used ordinary `Ss*q`, `Ss*b`, `I`, or `PP` links. The change is intentional
because those ordinary families could not safely enforce the predicate license
before postprocessing.

### Verification

Focused accepted-linkage comparison against `master` inspected the first three
displayed accepted linkages for:

```text
The question is who we should invite.
The question may be who we should invite.
The issue has been whether we should leave.
This is who we should invite.
```

The same `BIq` constructions remain accepted. The expected differences are the
new explicit certificate links:

```text
Ss*q  -> BIQS
Ss*b  -> BIQS
I     -> IBIQ
PP    -> PPBIQ
```

Focused bad examples now have no accepted zero-null linkage:

```text
*The answer is who we should invite.
*The problem is who we should invite.
*The big mind on everybody's question is who killed OJ.
```

The rule 42 migration was validated with ordinary parser runs:

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

## Rules 32s, 32p, 32u, 34, 35, 36: Encode Existential `there`

**Status:** implemented; these PP rules have been removed from
`4.0.knowledge`.

### Rule / Area

The removed PP rules required existential `there` subject links to have a
compatible object or complement link somewhere in the same linkage:

```text
SFst  , O*t Ost Omt Omm Bs#t B*#t Bc#t , "Bad use of 'there'32s"
SFp   , O*t Opt Omt Omm Bp#t B*#t Bc#t , "Bad use of 'there'32p"
SFu   , O*t Out Omt Omm Bp#t B*#t Bc#t , "Bad use of 'there'32u"
SFIst , O*t Ost Omt Bs#t B*#t Bc#t     , "Bad use of 'there'34"
SFIp  , O*t Opt Omt Bp#t B*#t Bc#t     , "Bad use of 'there'35"
OXt   , O#t B##t                        , "Bad use of 'there'36"
```

The grammatical area is agreement between existential `there` and the eventual
nominal complement. The complement can be separated from `there` by inversion,
modals, perfect auxiliaries, `going to be`, and raising predicates, so this is
a long-distance agreement constraint rather than a local relation between
adjacent words:

```text
There is a dog in the park.
There are dogs in the park.
There seems to appear to have been likely to be a problem.
```

### Problem

The old dictionary used broad `SFst`, `SFp`, `SFu`, `SFIst`, and `SFIp`
starter links from `there.r` to the first predicate. The object evidence could
appear much later, for example after modal, perfect, `going to be`, or
raising-predicate chains. PP therefore had to inspect the completed linkage
and reject locally well-formed but agreement-incompatible paths such as:

```text
*There is chasing dogs.
*There are a dog in the park.
*There seems to appear to have been likely to be problems.
```

Rule 36 was a related object-form backstop. The old dictionary also exposed a
standalone `OXt-` branch on `there.r`. That allowed ordinary transitive object
slots to treat locative or existential `there` as an object-like argument. PP
then rejected completed raw linkages where this `OXt` use did not carry an
existential object or filler-gap witness, for example the rejected raw path
inside `I want there quickly`. That sentence can still parse with ordinary
locative `MVp`; the migrated rule only removes the old object-like `OXt`
candidate.

### Implementation

Existential `there.r` now uses agreement-specific uppercase starter links:

```text
there --THRS-- singular predicate path
there --THRP-- plural predicate path
there --THRU-- uncountable predicate path
```

These links are certificate links: they still appear in public linkages, but
their role is to make the agreement state explicit in the LG connector graph.
Ordinary LG connector matching is local, so the singular/plural/uncountable
state must be carried through the intervening predicate chain until the final
object or complement branch can consume it.

The same families are used in the opposite direction for inverted questions,
so `Is there a dog?` and `Are there dogs?` do not need the old `SFIst` and
`SFIp` PP checks.

The agreement requirement is carried through common predicate chains by
dedicated connector families:

```text
there --THRS-- will --ITHRS-- be --Ost-- dog
there --THRS-- has --PPTHRS-- been --Ost-- problem
there --THRS-- is --PGTHRS-- going --TTHRS-- to --ITHRS-- be --Ost-- meeting
```

For raising and predicative-adjective paths, the certificate is propagated
through the intervening predicates instead of relying on a completed-linkage PP
scan:

```text
there --THRS-- seems --TTHRS-- to --ITHRS-- appear
appear --TTHRS-- to --ITHRS-- have --PPTHRS-- been
been --PATHRS-- likely --TTHRS-- to --ITHRS-- be --Ost-- problem
```

The final `be` object branches are split by agreement. Singular paths accept
singular-compatible object evidence, plural paths accept plural-compatible
object evidence, and uncountable paths accept uncountable-compatible evidence.
The broad `SF*` connector families remain in the dictionary for unrelated
grammar paths, but existential `there.r` and the related deictic `here` path no
longer use them for this construction.

Rule 36 is handled by deleting the standalone `there.r OXt-` alternative. The
valid tested uses that formerly appeared near this rule do not need `OXt`:
locative `there` continues to attach through ordinary modifier paths such as
`MVp`, and existential or presentational `there` uses the agreement-specific
`THR*` families above. Removing `OXt-` prevents raw object-like `there` paths
from being generated at all, so no replacement connector family is needed.

The agreement states are encoded as distinct uppercase families, for example
`THRS`, `THRP`, and `THRU`, rather than as subscripted forms such as `THRs`,
`THRp`, and `THRu`. The states are intended to be mutually exclusive. Using
uppercase-distinct names avoids accidental broad matching if a future bare
`THR` connector is introduced, since a bare connector would match subscripted
variants under ordinary LG connector matching.

The remaining PP rule 8 was extended to recognize `THRS`, `THRP`, and `THRU`
as valid subject-inversion evidence for `Qd` questions. This is a compatibility
update for the still-active S-V inversion checks, not a migration of those
checks.

### Implications

Accepted existential-`there` linkages now show explicit agreement-certificate
links instead of the old broad `SF*` links. This is intentional: subscripted
`SF` links did not carry enough information to prevent bad raw linkages before
postprocessing.

Some accepted linkages differ internally from older master-style output. For
example, `going to be` and `likely to be` existential paths now use `PGTHR*`,
`TTHR*`, `ITHR*`, and `PATHR*` certificate links. These links make the
agreement path explicit and allow the bad zero-null raw linkages to be removed
from the dictionary search space.

The THR carrier families are therefore a technical LG solution for enforcing a
real far-agreement condition before postprocessing. They are not hidden
implementation artifacts, because they are visible in linkages, but their
names encode agreement state and propagation stage rather than a single
ordinary syntactic relation.

### Verification

Focused accepted examples include:

```text
There is a dog in the park.
There are dogs in the park.
There is water on the table.
Is there a dog in the park?
Are there dogs in the park?
There is going to be an important meeting in January.
There has been a problem.
There have been problems.
There will be a dog.
Will there be a dog?
Does there seem to be a dog in the park?
There seems to appear to have been likely to be a problem.
```

Focused bad examples no longer have accepted zero-null linkages:

```text
*There is chasing dogs.
*There are a dog in the park.
*Are there a dog in the park?
*There has been problems.
*There seems to appear to have been likely to be problems.
*There seems to appear to have been likely to be stupid.
```

Focused accepted-linkage comparison against the pre-change dictionary inspected
the first three displayed accepted linkages for:

```text
There is a dog in the park.
There has been a problem.
There seems to appear to have been likely to be a problem.
I want there to be a problem.
```

The same constructions remain accepted. For the ordinary existential examples,
the expected differences are the new certificate links:

```text
SFst  -> THRS
PP    -> PPTHRS
TOf/I -> TTHRS/ITHRS
Paf   -> PATHRS
```

For `I want there to be a problem`, the preferred accepted linkages use `MVp`
for `there`. The pre-change dictionary also displayed an accepted `OXt`
alternative when the domain contained the infinitival object witness. That
object-like `there` analysis is intentionally removed; the ordinary locative
and infinitival analyses remain accepted.

The migration was validated with ordinary parser runs:

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
