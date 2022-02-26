# Thai Link Grammar

Copyright (C) 2021 Prachya Boonkwan  
National Electronics and Computer Technology Center, Thailand  
License: Creative Commons, Attribution (CC-BY)

This document summarizes all the link-types used in `4.0.dict` for Thai Link Grammar. They are classified with respect to the syntactic hierarchy.

## Utterance

| Link-types | Descriptions |
|:----------:|--------------|
| `LW`       | **Left wall.** It links the first element of an acceptable utterance to the left wall. |
| `LWs`      | **Left wall: sentence.** It links the main verb of a complete sentence to the left wall. |
| `LWn`      | **Left wall: noun phrase.** It links the core noun of a noun phrase to the left wall. |
| `LWp`      | **Left wall: preposition phrase.** It links the preposition of a preposition phrase to the left wall. |
| `RW`       | **Right wall.** It links the last element of an acceptable utterance to the right wall. |

## Sentence

| Link-types | Descriptions |
|:----------:|--------------|
| `S`        | **Subject.** It connects a subject to its main verb. |
| `O`	       | **Object.** It connects an object to its main verb. |
| `VZ`       | **Serial verb.** It connects a verb to its modifying verb in a serial verb construction. |
| `VC`       | **Controlled verb.** It connects a verb to its controlled verb. |
| `TP`       | **Topicalization.** It connects a topicalized noun phrase to its main verb. |
| `IJ`      | **Interjection.** It links an interjection word to the left wall. |
| `PT`      | **Particle.** It links a particle word to the right wall. |

## Noun Phrase

| Link-types | Descriptions |
|:----------:|--------------|
| `NZ`       | **Serial noun.** It connects a noun to its modifying noun in a serial noun construction. |
| `AJ`       | **Adjective.** It connects a nominal modifier to its core noun. |
| `AJj`      | **Simple adjective.** It connects an adjective to its core noun. |
| `AJr`      | **Relative clause.** It connects a relativizer to its core noun. |
| `AJv`      | **Attributive verb.** It connects an attributive verb that acts as an adjective to its core noun. |
| `AJp`      | **Preposition phrase.** It connects a preposition phrase to its core noun. |
| `RI`       | **Implicit relative clause.** It connects an implicit relative clause to its core noun. |
| `AT`       | **Attribute noun.** It connects an attribute noun to its core noun. |
| `PS`       | **Possessive pronoun.** It connects a possessive pronoun to its core noun. |
| `AM`       | **Attribute's modifier.** It connects an attribute's modifier to its attributive verb. |

## Verb Phrase

| Link-types | Descriptions |
|:----------:|--------------|
| `AX`       | **Auxiliary.** It connects an auxiliary to its main verb. All kinds of auxiliary always <u>precede</u> the main verb. |
| `AXw`      | **Simple auxiliary.** It connects an auxiliary verb to its main verb. |
| `AXg`      | **Negative auxiliary.** It connects a negator to its main verb. |
| `AV`       | **Adverbial.** It connects an adverbial to its main verb. All kinds of adverbial (except cohesive marker) always <u>follow</u> the main verb. |
| `AVp`      | **Preposition phrase.** It connects a preposition phrase to its main verb. |
| `AVw`      | **Simple adverb.** It connects an adverb to its main verb. |
| `AVg`      | **Negative adverb.** It connects a negative adverb to its main verb. |
| `AVn`      | **Numeral of noun phrase.** It connects a numeral modifier of a noun phrase to the main verb. |
| `AVv`      | **Numeral of verb phrase.** It connects a numeral modifier to the main verb. |
| `AVt`      | **Temporal phrase.** It connects a temporal phrase to the main verb. |
| `AVc`      | **Cohesive marker.** It connects a cohesive marker to the main verb. The cohesive markers always <u>precede</u> a sentence. |

## Preposition Phrase

| Link-types | Descriptions |
|:----------:|--------------|
| `PO`       | **Prepositional object.** It connects a preposition to its complementing noun phrase. |
| `PC`       | **Prepositional complement.** It connects a preposition to its complementing sentence. |
| `PZ`       | **Serial preposition.** It connects a preposition to its core preposition in a serial preposition construction. |
| `PZn`       | **Serial noun-consuming preposition.** It connects a preposition to its core noun-consuming preposition. |
| `PZv`       | **Serial verb-consuming preposition.** It connects a preposition to its core verb-consuming preposition. |

## Numeral Phrase

| Link-types | Descriptions |
|:----------:|--------------|
| `NU`       | **Numeral phrase.** It connects a numeral phrase to its modifiee. |
| `NUn`      | **Numeral phrase for noun.** It connects a numeral phrase to its core noun. |
| `NUv`      | **Numeral phrase for verb.** It connects a numeral phrase to its main verb. |
| `CL`       | **Classifier.** It connects a classifier to its number. |
| `CLn`      | **Classifier for nouns.** It connects a classifier for nouns to its number. |
| `CLv`      | **Classifier for verbs.** It connects a classifier for verbs to its number. |
| `QF`       | **Numeral quantifier.** It connects a numeral quantifier to its number. |

## Coordinate Structure

| Link-types | Descriptions |
|:----------:|--------------|
| `JN`       | **Conjoined noun phrase.** It connects a noun-phrase conjunct to its coordinator. |
| `JNl` | **Left-hand-side conjoined noun phrase.** It connects a noun-phrase conjunct on the left hand side to its coordinator. |
| `JNr` | **Right-hand-side conjoined noun phrase.** It connects a noun-phrase conjunct on the right hand side to its coordinator. |
| `JV`       | **Conjoined verb phrase.** It connects a verb-phrase conjunct to its coordinator. |
| `JVl`      | **Conjoined verb phrase.** It connects a verb-phrase conjunct on the left hand side to its coordinator. |
| `JVr`      | **Conjoined verb phrase.** It connects a verb-phrase conjunct on the right hand side to its coordinator. |
| `JP`       | **Conjoined preposition phrase.** It connects a preposition-phrase conjunct to its coordinator. |
| `JPl`      | **Conjoined preposition phrase.** It connects a preposition-phrase conjunct on the left hand side to its coordinator. |
| `JPr`      | **Conjoined preposition phrase.** It connects a preposition-phrase conjunct on the right hand side to its coordinator. |

## Punctuation

| Link-types | Descriptions |
|:----------:|--------------|
| `PU`       | **Punctuation mark.** It connects a punctuation mark to its modifiee. |
| `PUs`      | **White space.** It connects a white space to its modifiee. |
| `PUp`      | **Paiyal Noi 'ฯ'.** It connects a paiyal noi 'ฯ' to its modifiee. |
| `PUy`      | **Mai Yamok 'ๆ'.** It connects a mai yamok 'ๆ' to its modifiee. |