# Thai Link Documentation

Copyright (C) 2021 Prachya Boonkwan  
National Electronics and Computer Technology Center, Thailand  
License: Creative Commons, Attribution (CC-BY)

This document elaborates the details of each link type for the Thai Link Grammar. These link types are listed alphabetically for convenience purposes as follows.

> [AJ](#aj)
[AM](#am)
[AT](#at)
[AV](#av)
[AX](#ax)
[CL](#cl)
[IJ](#ij)
[JN](#jn)
[JV](#jv)
[JP](#jp)
[LW](#lw)
[NU](#nu)
[NZ](#nz)
[O](#o)
[PC](#pc)
[PO](#po)
[PS](#ps)
[PT](#pt)
[PU](#pu)
[PZ](#pz)
[QF](#qf)
[RI](#ri)
[RW](#rw)
[S](#s)
[TP](#tp)
[VC](#vc)
[VZ](#vz)

However, the remaining of this document is organized with respect to the syntactic hierarchy:

1. Utterance
2. Sentence
3. Noun phrase
4. Verb phrase
5. Numeral phrase 
6. Coordinate structure 
7. Punctuation

----------

## Utterance

### LW

This *undirected* link connects the first element of an acceptable utterance to the left wall. There are three types of LW links: `LWs` (sentence), `LWn` (noun phrase), and `LWp` (preposition phrase).

`LWs` links the main verb of a complete sentence to the left wall. For example, ฉัน ไป โรงเรียน 'I go to school.' is parsed as follows.

```
    +-----LWs-----+
    |       +<-S<-+-->O->+
    |       |     |      |
LEFT-WALL ฉัน.pr ไป.v โรงเรียน.n
```

`LWn` links the core noun of a noun phrase to the left wall. For example, เครื่องบิน พาณิชย์ 30 ลำ 'thirty commercial airplanes' is parsed below.

```
              +----->NUnr---->+
    +---LWn---+-->AJj->+      +>CLn>+
    |         |        |      |     |
LEFT-WALL เครื่องบิน.n พาณิชย์.j 30.nu ลำ.cl
```

`LWp` links the preposition of a preposition phrase to the left wall. For example, ใน เรือน ปั้นหยา 'In the hip-roofed house' is parsed below.

```
    +---LWp--+->PO->+--NZ--+
    |        |      |      |
LEFT-WALL ใน.pan เรือน.n ปั้นหยา.n
```

### RW

This *undirected* link connects the last element of an acceptable utterance to the right wall. For every utterance, there must be an `RW` link from it to the right wall, such that it is accepted by the Thai Link Grammar.

```
    +-----LWs-----+--------RW-------+
    |       +<-S<-+-->O->+          |
    |       |     |      |          |
LEFT-WALL ฉัน.pr ไป.v โรงเรียน.n RIGHT-WALL
```

`RW` links typically do not appear unless the flag `!walls` is enabled in the Link Parser.

----------

## Sentence

### S

This directed link connects a subject to its main verb.

```
    +-----------LWs----------+
    |       +<-------S<------+
    |       +---NZ--+        +--VZ--+-->O->+
    |       |       |        |      |      |
LEFT-WALL คณะ.n นักเรียน.n เดินทาง.v มา.v เชียงใหม่.n
```

### O

This directed connects an object to its main verb.

```
    +-------LWs-------+--------->AVpr--------->+
    |        +<---S<--+--->O-->+---NZ---+      +->PO>+->PS>+
    |        |        |        |        |      |     |     |
LEFT-WALL เรา.pr รับประทาน.v อาหาร.n กลางวัน.n ที่.pan ร้าน.n นี้.pr
```

### VZ

This directed connects a verb to its modifying verb in a serial verb construction.

### VC

This directed connects a verb to its controlled verb.

### TP

This *undirected* connects a topicalized noun phrase to its main verb.

```
    +-----------------LWs-----------------+
    |       +--------------TP-------------+
    |       +-------PUst-------+          |
    |       +--NZ--+--NZ-+     |    +<-S<-+>AVw>+
    |       |      |     |     |    |     |     |
LEFT-WALL บ้าน.n เรือน.n ไทย.n _.pu ฉัน.pr ชอบ.v ที่สุด.r
```

### IJ

It links an interjection word to the left wall.

### PT

It links a particle word to the right wall.

----------

## Noun Phrase

### NZ

It connects a noun to its modifying noun in a serial noun construction.

### AJ

It connects a nominal modifier to its core noun.

### RI

It connects an implicit relative clause to its core noun.

### AT

It connects an attribute noun to its core noun.

### PS

It connects a possessive pronoun to its core noun.

### AM

It connects an attribute's modifier to its attributive verb.

----------

## Verb Phrase

### AX

It connects an auxiliary to its main verb. All kinds of auxiliary always <u>precede</u> the main verb.

### AV

It connects an adverbial to its main verb. All kinds of adverbial (except cohesive marker) always <u>follow</u> the main verb.

----------

## Preposition Phrase

### PO

It connects a preposition to its complementing noun phrase.

### PC

It connects a preposition to its complementing sentence.

### PZ

It connects a preposition to its modifying preposition in a serial preposition construction.

----------

## Numeral Phrase

### NU

It connects a numeral phrase to its modifiee.

### CL

It connects a classifier to its number.

### QF

It connects a numeral quantifier to its number.

----------

## Coordinate Structure

### JN

It connects a noun-phrase conjunct to its coordinator.

### JV

It connects a verb-phrase conjunct to its coordinator.

### JP

It connects a preposition-phrase conjunct to its coordinator.

----------

## Punctuation

### PU

It connects a punctuation mark to its modifiee.


