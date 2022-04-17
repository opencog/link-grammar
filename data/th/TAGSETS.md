# POS Tagset for Thai Link Grammar

## Link POS Tagset

### Content Words

There are 13 content-word types as shown below.

| POS tag | Description |
|:-------:|:------------|
| `n`     | Noun |
| `na`    | Attributive noun |
| `nt`    | Title noun |
| `v`     | Verb |
| `va`    | Attributive verb |
| `vg`    | Noun-modifying verb |
| `ve`    | Existential verb |
| `j`     | Adjective |
| `jl`    | Preceding adjective |
| `r`     | Adverb |
| `ra`    | Attribute-modifying adverb |
| `rc`    | Cohesive adverb |
| `rq`    | Emphasizing adverb |

### Function Words

There are 22 function-word types as shown below.

| POS tag | Description |
|:-------:|:------------|
| `x`     | Auxiliary |
| `fx`    | Nominalizing prefix |
| `ngl`   | Preceding negator |
| `ngr`   | Succeeding negator |
| `nu`    | Number |
| `nuv`   | Movable number |
| `om`    | Ordinal marker |
| `cln`   | Noun-modifying classifier |
| `clv`   | Verb-modifying classifier |
| `qfl`   | Preceding quantifier |
| `qfr`   | Succeeding quantifier |
| `ps`    | Passive marker |
| `pr`    | Pronoun |
| `rl`    | Relative pronoun |
| `sm`    | Subclause marker |
| `p`     | Preposition |
| `cn`    | Noun conjunction |
| `cv`    | Verb conjunction |
| `cp`    | Preposition conjunction |
| `cd`    | Discontinuous conjunction |
| `pt`    | Particle |
| `ij`    | Interjection |

### Subcategorization of Prepositions

The prepositions are subcategorized into nine subtypes.

| POS tag | Description |
|:-------:|:------------|
| `pnn`   | Noun-modifying preposition taking a noun phrase |
| `pvn`   | Verb-modifying preposition taking a noun phrase |
| `pan`   | Preposition taking a noun phrase |
| `pnv`   | Noun-modifying preposition taking a verb phrase |
| `pvv`   | Verb-modifying preposition taking a verb phrase |
| `pav`   | Preposition taking a verb phrase |
| `pna`   | Noun-modifying preposition taking both a noun phrase and a verb phrase |
| `pva`   | Verb-modifying preposition taking a noun phrase and a verb phrase |
| `paa`   | Preposition taking a noun phrase and a verb phrase |

## LST20 Tagsets

The LST20 Corpus is a large-scaled Thai corpus annotated with word boundaries, POS tags, named entities, clause boundaries, and sentence boundaries. There are two tagsets that can be used with the Link Parser: POS and named entities. Its annotation guideline and data statistics can be [found here](https://arxiv.org/abs/2008.05055).

### LST20 POS Tagset

| POS Tag | Description |
|:-------:|:------------|
| `NN`    | **Noun:** person, place, thing, abstract concept, and proper name |
| `VV`    | **Verb:** action, state, occurrence, and word that forms the predicate part |
| `AJ`    | **Adjective:** attribute, modifier, and description of a noun |
| `AV`    | **Adverb:** word that modifies or qualifies an adjective, verb, or another adverb |
| `AX`    | **Auxiliary:** tense, aspect, mood, and voice |
| `CC`    | **Connector:** conjunction and relative pronoun |
| `CL`    | **Classifier:** class or measurement unit to which a noun or an action belongs |
| `FX`    | **Prefix:** inflectional prefix (nominalizer, adjectivizer, adverbializer, and courteous verbalizer), and derivational prefix |
| `IJ`    | **Interjection:** exclamation word |
| `NG`    | **Negator:** word of negation |
| `NU`    | **Number:** quantity for counting and calculation |
| `PA`    | **Particle:** politeness, intention, belief, and question |
| `PR`    | **Pronoun:** word used to refer to an element in the discourse |
| `PS`    | **Preposition:** location, comparison, instrument, exemplification |
| `PU`    | **Punctuation:** punctuation mark |
| `XX`    | **Others:** unknown category |

### Named Entity Tagset

| NE Tag | Description |
|:------:|:------------|
| `TTL`  | **Title:** family relationship, social relationship, and permanent title |
| `DES`  | **Designation:** position and professional title |
| `PER`  | **Person:** name of a person or family |
| `ORG`  | **Organization:** name of organization, office, and company |
| `LOC`  | **Location:** name of land according to geo-political borders |
| `BRN`  | **Brand:** name of brand, product, and trademark |
| `DTM`  | **Date and Time:** time or a specific period of time |
| `MEA`  | **Measurement:** measurement unit and quantity of things |
| `NUM`  | **Number:** the number of a measurement unit |
| `TRM`  | **Terminology:** domain-specific word |