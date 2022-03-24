# Input Formats

This document described the input formats of the Link Grammar for Thai. It parses both raw texts and linguistically annotated texts in specific formats. The more information annotated to the text, the more accurate the parser becomes.

## Raw Text

In most cases, the Link parser should accept raw texts by default. For Thai, one line of raw text should be tokenized into words before being fed to the parser. For example, โรงเรียนแห่งนี้เปิดดำเนินกิจการมานานแล้ว 'This school has been established for a long time' is parsed as follows:

```
linkparser> โรงเรียน แห่ง นี้ เปิด ดำเนิน กิจการ มา นาน แล้ว
Found 174 linkages (174 had no P.P. violations)
	Linkage 1, cost vector = (UNUSED=0 DIS= 2.00 LEN=10)

    +--------------LWs-------------+
    |         +<---------S<--------+      +----->AVw---->+
    |         +->AJpr->+->PO->+    +--VZ--+-->O-->+      +>AVw>+>AVw>+
    |         |        |      |    |      |       |      |     |     |
LEFT-WALL โรงเรียน.n แห่ง.pnn นี้.pr เปิด.v ดำเนิน.v กิจการ.n มา.r  นาน.r แล้ว.r
```

## Multiple Sentences in One Line

The parser also accepts multiple short sentences in one line. Each sentence should be delimited by the end-of-sentence symbol `||` (and also `<EOS>`). This symbol is treated as the right wall and the left wall at the same time. For example, ผมได้ติดต่อเจ้าหน้าที่ไปแล้ว || แต่ยังไม่ได้ตามเรื่อง 'I contacted the staff previously || but I have not tracked the progress yet' is parsed as follows.

```
linkparser> ผม ได้ ติดต่อ เจ้าหน้าที่ ไป แล้ว || แต่ ยัง ไม่ ได้ ตาม เรื่อง
Found 4748 linkages (1000 of 1000 random linkages had no P.P. violations)
	Linkage 1, cost vector = (UNUSED=0 DIS= 1.00 LEN=18)

                                                        +--------->PC--------->+
    +--------LWs--------+------------RW------------+    |     +<------AXw<-----+
    |       +<----S<----+----->AVw----->+          |    |     |    +<---AXg<---+
    |       |     +<AXw<+-->O-->+       +>AVw>+    +-LWp+     |    |     +<AXw<+-->O->+
    |       |     |     |       |       |     |    |    |     |    |     |     |      |
LEFT-WALL ผม.pr ได้.x ติดต่อ.v เจ้าหน้าที่.n ไป.r  แล้ว.r || แต่.pvv ยัง.x ไม่.ng ได้.x  ตาม.v เรื่อง.n
```

## POS-Annotated Text

This parser also accepts linguistically annotated texts. Once tokenized into words, some of them can be annotated with POS tags to increase the accuracy. The format of POS annotation is *word*`.`*tag*. For example, เมื่อวานนี้มีคนมาติดต่อคุณครับ 'Yesterday, someone came to contact you' is parsed as follows.

```
linkparser> เมื่อวานนี้.n มี.ve คน.n มา.x ติดต่อ.v คุณ.pr ครับ.pt
Found 1 linkage (1 had no P.P. violations)
	Unique linkage, cost vector = (UNUSED=0 DIS= 0.00 LEN=12)

                          +---------------------PT--------------------+
    +---------LWs---------+---------->VE---------->+                  |
    |           +<---S<---+-->O-->+       +<--AXw<-+--->O--->+        |
    |           |         |       |       |        |         |        |
LEFT-WALL เมื่อวานนี้.n[!] มี.ve[!] คน.n[!] มา.x[!] ติดต่อ.v[!] คุณ.pr[!] ครับ.pt[!]
```

Note that each word is annotated with the Thai Link Grammar's POS tags.

## POS Tagset for Thai Link Grammar

| POS tag | Description |
|:--------|-------------|
| `n`     | Noun |
| `na`    | Attributive noun |
| `nt`    | Title noun |
| `v`     | Verb |
| `va`    | Attributive verb |
| `vg`    | Gerundial verb |
| `ve`    | Existential verb |
| `j`     | Adjective |
| `jl`    | Preceding adjective |
| `r`     | Adverb |
| `ra`    | Attribute-modifying adverb |
| `rc`    | Cohesive adverb |
| `qr`    | Adverb-modifying adverb |

| POS tag | Description |
|:--------|-------------|
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
| `pnn`   | Noun-modifying preposition taking a noun phrase |
| `pvn`   | Verb-modifying preposition taking a noun phrase |
| `pan`   | Preposition taking a noun phrase |
| `pnv`   | Noun-modifying preposition taking a verb phrase |
| `pvv`   | Verb-modifying preposition taking a verb phrase |
| `pav`   | Preposition taking a verb phrase |
| `pna`   | Noun-modifying preposition taking both a noun phrase and a verb phrase |
| `pva`   | Verb-modifying preposition taking a noun phrase and a verb phrase |
| `paa`   | Preposition taking a noun phrase and a verb phrase |
| `cn`    | Noun conjunction |
| `cv`    | Verb conjunction |
| `cp`    | Preposition conjunction |
| `cd`    | Discontinuous conjunction |
| `pt`    | Particle |
| `ij`    | Interjection |