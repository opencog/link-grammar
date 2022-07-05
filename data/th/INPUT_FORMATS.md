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

**FYI:** As of 5 July 2022, the space is also treated as a candidate of sentence delimiters.

## POS-Annotated Text

This parser also accepts linguistically annotated texts. Once tokenized, some words in a sentence can be annotated with Link POS tags to increase the accuracy. The format of Link POS annotation is *word*`.`*tag*. For example, เมื่อวานนี้มีคนมาติดต่อคุณครับ 'Yesterday, someone came to contact you' is parsed as follows.

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

Note that each word is annotated with the Thai Link Grammar's POS tags. A full documentation on the tagsets can be [found here](https://github.com/kaamanita/link-grammar/blob/master/data/th/TAGSETS.md).

## LST20 Tagset

The parser also accepts texts annotated with the LST20 tagset, a large-scaled Thai corpus annotated with word boundaries, POS tags, named entities, clause boundaries, and sentence boundaries. The parser offers this compatibility as a means to bridge the gap between fundamental NLP tools and the Link Parser.

Once tokenized, some words in a sentence can be annotated with the LST20 tags. Due to some discrepancies with the Link Grammar, annotation with the LST20 tagset generally yields less accuracy than the Link POS tagset. The format of LST20 tags is *word*`@`*tag*. For example, เขาบอกนักเรียนให้นอนได้แล้ว 'He asks the students to go to bed' is parsed as follows.

```
linkparser> เขา@PR บอก@VV นักเรียน@NN ให้@PS นอน@VV ได้แล้ว@AV
Found 162 linkages (162 had no P.P. violations)
	Linkage 1, cost vector = (UNUSED=0 DIS= 1.00 LEN=5)

    +----------LWs----------+
    |           +<----S<----+----->O----->+---->AJpr--->+---->PC--->+---->AVw--->+
    |           |           |             |             |           |            |
LEFT-WALL เขา@PR[!].pr บอก@VV[!].v นักเรียน@NN[!].n ให้@PS[!].pna นอน@VV[!].v ได้แล้ว@AV[!].r
```

Note that each word is annotated with the LST20 POS tags.

Moreover, the parser also accepts LST20 named entity tags. For example, `<DTM>`*วันที่ 25 ธันวาคม*`</DTM>`ของทุกปีเป็นวันคริสต์มาส '25 December of every year is Christmas Day' is parsed as follows.

```
linkparser> linkparser> วันที่_25_ธันวาคม@DTM ของ@PS ทุก@AJ ปี@NN เป็น@VV วัน@NN คริสต์มาส@NN
Found 348 linkages (348 had no P.P. violations)
	Linkage 1, cost vector = (UNUSED=0 DIS= 1.00 LEN=10)

    +--------------------------------LWs--------------------------------+
    |               +<------------------------S<------------------------+
    |               |                +---------->PO--------->+          |
    |               +----->AJpr----->+            +<---AJj<--+          +---->O---->+------NZ-----+
    |               |                |            |          |          |           |             |
LEFT-WALL วันที่_25_ธันวาคม@DTM[!] ของ@PS[!].pnn ทุก@AJ[!].jl ปี@NN[!].n เป็น@VV[!].v วัน@NN[!].na คริสต์มาส@NN[!].n
```

Note that the phrase วันที่ 25 ธันวาคม is annotated with the named entity tag `DTM`. A full documentation on the tagsets can be [found here](https://github.com/kaamanita/link-grammar/blob/master/data/th/TAGSETS.md). The annotation guideline of LST20 Corpus can be [found here](https://arxiv.org/abs/2008.05055).
