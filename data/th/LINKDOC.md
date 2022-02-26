# Thai Link Documentation

Copyright (C) 2021 Prachya Boonkwan  
National Electronics and Computer Technology Center, Thailand  
License: Creative Commons, Attribution (CC-BY)

This document elaborates the details of each link type for the Thai Link Grammar. These link types are listed alphabetically for convenience purposes as follows.

> [AJ](#aj)
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

1. [Utterance](#utterance)
2. [Sentence](#sentence)
3. [Noun phrase](#noun-phrase)
4. [Verb phrase](#verb-phrase)
5. [Numeral phrase](#numeral-phrase)
6. [Coordinate structure](#coordinate-structure)
7. [Punctuation](#punctuation)

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

This directed link connects an object to its main verb.

```
    +-------LWs-------+--------->AVpr--------->+
    |        +<---S<--+--->O-->+---NZ---+      +->PO>+->PS>+
    |        |        |        |        |      |     |     |
LEFT-WALL เรา.pr รับประทาน.v อาหาร.n กลางวัน.n ที่.pan ร้าน.n นี้.pr
```

### VZ

This *undirected* link connects a verb to its modifying verb in a serial verb construction.

```
    +-----LWs-----+
    |       +<-S<-+--VZ-+--VZ--+>AVpr>+->PO>+---NZ--+
    |       |     |     |      |      |     |       |
LEFT-WALL ฉัน.pr ออก.v เดิน.v ช็อปปิ้ง.v ที่.paa ห้าง.n พารากอน.n
```

### VC

This directed link connects a verb to its controlled verb.

```
    +--------LWs--------+----->VC----->+
    |          +<---S<--+-->O->+       +-->O-->+---NZ---+
    |          |        |      |       |       |        |
LEFT-WALL นักการเมือง.n ถูก.ps นักข่าว.n ซักถาม.v เรื่อง.n งบประมาณ.n
```

### TP

This *undirected* link connects a topicalized noun phrase to its main verb.

```
    +-----------------LWs-----------------+
    |       +--------------TP-------------+
    |       +-------PUst-------+          |
    |       +--NZ--+--NZ-+     |    +<-S<-+>AVw>+
    |       |      |     |     |    |     |     |
LEFT-WALL บ้าน.n เรือน.n ไทย.n _.pu ฉัน.pr ชอบ.v ที่สุด.r
```

### IJ

This *undirected* link connects an interjection word to the left wall.

```
    +--------------LWs--------------+
    |        +----------IJ----------+
    |        |          +<----S<----+--------PT-------+
    |        +-PUs-+    |     +<AXw<+>AVw>+>AVw>+     |
    |        |     |    |     |     |     |     |     |
LEFT-WALL เฮ้ย.ij _.pu มัน.pr จะ.x  เป็น.v ไป.r  ได้.r หรือ.pt
```

### PT

This *undirected* link connects a particle word to the right wall.

```
    +---------LWs--------+
    |        +<----S<----+------PT------+
    |        |     +<AXw<+->O->+        |
    |        |     |     |     |        |
LEFT-WALL เธอ.pr เคย.x ไป.v โคราช.n หรือยัง.pt
```

----------

## Noun Phrase

### NZ

This *undirected* link connects a noun to its modifying noun in a serial noun construction.

```
    +--------------------LWs-------------------+
    |        +<---------------S<---------------+
    |        +--NZ--+---NZ--+---NZ--+    +<AXw<+-->O->+->AJv>+
    |        |      |       |       |    |     |      |      |
LEFT-WALL คุณภาพ.n ชีวิต.n ชาวไทย.n ภูเขา.n ก็.x  เป็น.v ปัจจัย.n สำคัญ.va
```

### AJ

This directed link connects a nominal modifier to its core noun. There are five types of `AJ` links: `AJj`, `AJr`, `AJp`, `AJv`, and `AJt`.

`AJj` connects an adjective to its core noun.

```
    +-----------------LWs-----------------+
    |        +-------------TP-------------+
    |        +----->AJj---->+             +-----PT-----+
    |        |      +<-CLn<-+       +<-S<-+>AVw>+      |
    |        |      |       |       |     |     |      |
LEFT-WALL รถยนต์.n คัน.cl ก่อนหน้า.j เขา.pr ไป.v  ถึง.r หรือยัง.pt
```

`AJr` connects a relativizer to its core noun.

```
    +----------------LWs----------------+
    |       +<------------S<------------+
    |       |     +--->PC--->+          |
    |       +>AJr>+    +<-S<-+>AVw>+    +->O->+
    |       |     |    |     |     |    |     |
LEFT-WALL สมุด.n ที่.rl ฉัน.pr ซื้อ.v  มา.r อยู่.v ที่ไหน.pr
```

`AJp` connects a preposition phrase to its core noun.

```
    +-------------------LWs------------------+
    |        +<--------------S<--------------+
    |        |                   +<---AXw<---+
    |        +>AJpr>+->PO->+     |    +<-AXw<+->AVw->+
    |        |      |      |     |    |      |       |
LEFT-WALL สินค้า.n บน.pan รถไฟ.n ยัง.x คง.x ปลอดภัย.va อยู่.r
```

`AJv` connects an attributive verb that acts as an adjective to its core noun.

```
    +------------------LWs-----------------+
    |         +<-------------S<------------+
    |         +---->AJv---->+              |
    |         |      +<-CLn<+      +<-AXw<-+
    |         |      |      |      |       |
LEFT-WALL นักเรียน.n คน.cl ขยัน.va กำลัง.x เดินทาง.v
```

### RI

This directed link connects an implicit relative clause to its core noun.

```
    +----------------LWs---------------+
    |       +<------------S<-----------+
    |       +--------->AJj-------->+   |
    |       +->RI>+->O>+     +<CLn<+   +->O->+->AJv->+
    |       |     |    |     |     |   |     |       |
LEFT-WALL หม้อ.n หุง.v ข้าว.n ใบ.cl  นี้.j มี.v ระบบ.n อัตโนมัติ.va
```

### AT

This directed link connects an attribute noun to its core noun.

```
    +----------------LWs----------------+
    |        +<------------S<-----------+
    |        +->AT->+->AJv->+     +<AXw<+->O->+-->AJv->+-->AVw->+->AVw->+
    |        |      |       |     |     |     |        |        |       |
LEFT-WALL ระบบ.n ขนาด.na ใหญ่.va ย่อม.x  มี.v ความ.n สลับซับซ้อน.va เป็น.r ธรรมดา.r
```

### PS

This directed link connects a possessive pronoun to its core noun.

```
    +------------LWs-----------+
    |          +<------S<------+
    |          +--->PS-->+     +->O>+--NZ--+
    |          |         |     |    |      |
LEFT-WALL คอมพิวเตอร์.n เขา.pr ติด.v เชื้อ.n ไวรัส.n
```

----------

## Verb Phrase

### AX

This directed link connects an auxiliary to its main verb. All kinds of auxiliary always <u>precede</u> the main verb. There are two types of `AX` links: `AXw` and `AXg`.

`AXw` connects an auxiliary verb to its main verb.

```
    +---------LWs--------+
    |       +<-----S<----+      +------>AVw----->+
    |       |     +<-AXw<+--VZ--+-->O-->+        +>AVw>+
    |       |     |      |      |       |        |     |
LEFT-WALL เขา.n จะ.x เดินทาง.v ไป.v อุบลราชธานี.n อยู่.r  แล้ว.r
```

`AXg` connects a negator to its main verb.

```
    +---------LWs--------+
    |        +<----S<----+
    |        |     +<AXg<+--VZ-+--VZ-+->O->+->RI->+
    |        |     |     |     |     |     |      |
LEFT-WALL เธอ.pr ไม่.ng ยอม.v ไป.v ตรวจ.v โรค.n ติดต่อ.v
```

### AV

This directed link connects an adverbial to its main verb. All kinds of adverbial (except cohesive marker) always <u>follow</u> the main verb. There are seven types of `AV` links: `AVp`, `AVw`, `AVg`, `AVn`, `AVv`, `AVt`, and `AVc`.

`AVp` connects a preposition phrase to its main verb.

```
    +--------LWs--------+
    |        +----TP----+-->AVpr-->+
    |        |     +<-S<+->O>+     +->PO->+
    |        |     |    |    |     |      |
LEFT-WALL เรา.pr นัด.n พบ.v กัน.pr ที่.pan ปารีส.n
```

`AVw` connects an adverb to its main verb.

```
    +---------------LWs--------------+
    |          +<---------S<---------+------------>AVw------------>+
    |          +---NZ---+      +<AXw<+--->O-->+-----NZ----+        |
    |          |        |      |     |        |           |        |
LEFT-WALL สายการบิน.n นกแอร์.n มา.x  ถึง.v ท่าอากาศยาน.n อุบลราชธานี.n แล้ว.r
```

`AVg` connects a negative adverb to its main verb.

```
    +----------LWs---------+
    |         +<-----S<----+------->AVgr------>+
    |         |      +<AXw<+->AVw->+>AVw>+     |
    |         |      |     |       |     |     |
LEFT-WALL นักเรียน.n จะ.x กระทำ.v อย่าง.r  นี้.r มิได้.ng
```

`AVn` connects a numeral modifier of a noun phrase to the main verb.

```
                       +------->AVnr------>+
    +-----LWs----+     +----->AVw---->+    |
    |       +<-S<+--VZ-+-->O->+       |    +>CLn>+
    |       |    |     |      |       |    |     |
LEFT-WALL แม่.n ฝาก.v ซื้อ.v กล้วยหอม.n ไว้.r 2.nu  ลูก.cl
```

`AVv` connects a numeral modifier to the main verb.

```
    +----------LWs---------+
    |         +<-----S<----+--->AVvr-->+
    |         |      +<AXw<+->AVw>+    +>CLv>+
    |         |      |     |      |    |     |
LEFT-WALL นักเรียน.n ถูก.x ลงโทษ.v ซ้ำ.r 2.nu ครั้ง.cl
```

`AVt` connects a temporal phrase to the main verb.

```
                   +------------>AVtr----------->+
    +------LWs-----+--->AVpr-->+                 |
    |        +<-S<-+->O>+      +->PO>+           |
    |        |     |    |      |     |           |
LEFT-WALL เพื่อน.n จัด.v งาน.n ให้.pva ฉัน.pr วันที่_10_สิงหาคม[!]
```

`AVc` connects a cohesive marker to the main verb. The cohesive markers always <u>precede</u> a sentence.

```
    +-----------------LWs-----------------+
    |           +<----------AVcl<---------+------------------>AVw------------------>+
    |           |           +<-----S<-----+       +---->AJpr---->+---->PO---->+     |
    |           |           |       +<AXw<+-->O-->+->RI->+       |      +<AJj<+     |
    |           |           |       |     |       |      |       |      |     |     |
LEFT-WALL อย่างไรก็ตาม.rc ข้าพเจ้า.pr ได้.x ติดตาม.v การ.fx ทำงาน.v ของ.pnn ทุก.jl ฝ่าย.n แล้ว.r
```

----------

## Preposition Phrase

### PO

This directed link connects a preposition to its complementing noun phrase.

```
    +----------LWs----------+
    |        +<----AVpl<----+------PT------+
    |        +-->PO->+      +->O->+        |
    |        |       |      |     |        |
LEFT-WALL บน.pan เครื่องบิน.n มี.v ห้องน้ำ.n หรือไม่.pt
```

### PC

This directed link connects a preposition to its complementing sentence.

```
    +----------LWs---------+
    |        +<-----S<-----+      +---->AVw---->+        +----->PC---->+     +--->AJpr-->+
    |        |      +<-AXw<+--VZ--+-->O->+      +->AVpr->+       +<AXw<+->O->+>AJv>+     +-->PO->+
    |        |      |      |      |      |      |        |       |     |     |     |     |       |
LEFT-WALL สมชาย.n จึง.x เดินทาง.v ไป.v สกลนคร.n ทันที.r หลังจาก.pva ได้.x ทราบ.v ข่าว.n ดี.va ของ.pnn ภรรยา.n
```

### PZ

This directed link connects a preposition to its core preposition in a serial preposition construction. There are two types of `PZ` links: `PZn` and `PZv`.

`PZn` connects a preposition to its core noun-consuming preposition.

```
    +-----LWs----+--->AVw--->+--->AVpr-->+
    |       +<-S<+->O->+     |    +<-PZn<+->PO>+
    |       |    |     |     |    |      |     |
LEFT-WALL พ่อ.n วาง.v ของ.n ไว้.r ที่.pan บน.pan บ้าน.n
```

`PZv` connects a preposition to its core verb-consuming preposition.

```
    +-------------LWs-------------+
    |        +<---------S<--------+
    |        |      +<----AXw<----+------->AVpr------>+
    |        |      |      +<-AXw<+->O->+     +<-PZv<-+->PC>+-->O->+->RI->+-->O->+
    |        |      |      |      |     |     |       |     |      |      |      |
LEFT-WALL เรา.pr จำเป็น.x ต้อง.x สร้าง.v ตึก.n เพื่อ.pav ให้.pva เกิด.v การ.fx พัฒนา.v เมือง.n
```

----------

## Numeral Phrase

### NU

This directed link connects a numeral phrase to its modifiee.

### CL

This directed link connects a classifier to its number.

### QF

This directed link connects a numeral quantifier to its number.

----------

## Coordinate Structure

### JN

This *undirected* link connects a noun-phrase conjunct to its coordinator.

### JV

This *undirected* link connects a verb-phrase conjunct to its coordinator.

### JP

This *undirected* link connects a preposition-phrase conjunct to its coordinator.

----------

## Punctuation

### PU

This *undirected* link connects a punctuation mark to its modifiee.


