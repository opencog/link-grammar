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
[JD](#jd)
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

`LWs` links the main verb of a complete sentence to the left wall. For example, ฉันไปโรงเรียน 'I go to school':

```
ฉัน     ไป   โรงเรียน
tɕʰǎn  paɪ  roːŋ.rian
I      go   school
'I go to school.'
```

This sentence is parsed as follows.

```
    +-----LWs-----+
    |       +<-S<-+-->O->+
    |       |     |      |
LEFT-WALL ฉัน.pr ไป.v โรงเรียน.n
```

`LWn` links the core noun of a noun phrase to the left wall. For example, เครื่องบินพาณิชย์ 30 ลำ 'thirty commercial airplanes':

```
เครื่องบิน     พาณิชย์       30        ลำ
kʰrɯâŋ.bin  pʰaːnít     sǎːm.sib  lam
airplane    commercial  thirty    CL
'thirty commercial airplanes'
```

This noun phrase is parsed below.

```
              +----->NUnr---->+
    +---LWn---+-->AJj->+      +>CLn>+
    |         |        |      |     |
LEFT-WALL เครื่องบิน.n พาณิชย์.j 30.nu ลำ.cl
```

`LWp` links the preposition of a preposition phrase to the left wall. For example, ใน เรือน ปั้นหยา 'in the hip-roofed house':

```
ใน   เรือน   ปั้นหยา
naɪ  rɯaːn  pânjǎː
in   house  hip-roofed
'in the hip-roofed house'
```

This preposition phrase is parsed below.

```
    +---LWp--+->PO->+--NZ--+
    |        |      |      |
LEFT-WALL ใน.pan เรือน.n ปั้นหยา.n
```

### RW

This *undirected* link connects the last element of an acceptable utterance to the right wall. For every utterance, there must be an `RW` link from it to the right wall, such that it is accepted by the Thai Link Grammar. For example, ฉันไปโรงเรียน 'I go to school':

```
ฉัน     ไป   โรงเรียน
tɕʰǎn  paɪ  roːŋ.rian
I      go   school
'I go to school.'
```

This sentence is parsed as follows.

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

This directed link connects a grammatical subject to its main verb. For example, คณะนักเรียนเดินทางมาเชียงใหม่ 'A group of students are traveling to Chiang Mai':

```
คณะ    นักเรียน    เดินทาง      มา    เชียงใหม่
kʰáná  nák.rian  dəːn.tʰaːŋ  maː   tɕʰiaŋ.màɪ
group  student   travel      come  Chiang Mai
'A group of students are traveling to Chiang Mai.'
```

This sentence is parsed as follows.

```
    +-----------LWs----------+
    |       +<-------S<------+
    |       +---NZ--+        +--VZ--+-->O->+
    |       |       |        |      |      |
LEFT-WALL คณะ.n นักเรียน.n เดินทาง.v มา.v เชียงใหม่.n
```

The `S` link connects the grammatical subject คณะนักเรียน 'a group of students' to the main verb เดินทาง 'travel'.

### O

This directed link connects a grammatical object to its main verb. For example, เรารับประทานอาหารกลางวันที่ร้านนี้ 'We had lunch at this restaurant':

```
เรา  รับประทาน     อาหาร    กลางวัน     ที่     ร้าน   นี้
raʊ  ráppratʰaːn  ʔaːhǎːn  klaːŋ.ʋan  tʰîː  ráːn  níː
we   eat       meal   afternoon  at  restaurant  this
'We had lunch at this restaurant.'
```

This sentence is parsed as follows.

```
    +-------LWs-------+--------->AVpr--------->+
    |        +<---S<--+--->O-->+---NZ---+      +->PO>+->PS>+
    |        |        |        |        |      |     |     |
LEFT-WALL เรา.pr รับประทาน.v อาหาร.n กลางวัน.n ที่.pan ร้าน.n นี้.pr
```

The `O` link connects the grammatical object อาหารกลางวัน 'lunch' to its main verb รับประทาน 'eat'.

### VZ

This *undirected* link connects a verb to its modifying verb in a serial verb construction. For example ฉันออกเดินช็อปปิ้งที่ห้างพารากอน 'I am out for shopping at Paragon Mall':

```
ฉัน    ออก    เดิน   ช็อปปิ้ง     ที่     ห้าง   พารากอน
tɕǎn  ʔɔ̀ːk   dəːn  tɕɔ́ppîŋ   tʰîː  hâːŋ  pʰaːraːkɔ̂n
I     leave  walk  shopping  at    mall  Paragon
'I am out for shopping at Paragon Mall.'
```

This sentence is parsed as follows.

```
    +-----LWs-----+
    |       +<-S<-+--VZ-+--VZ--+>AVpr>+->PO>+---NZ--+
    |       |     |     |      |      |     |       |
LEFT-WALL ฉัน.pr ออก.v เดิน.v ช็อปปิ้ง.v ที่.paa ห้าง.n พารากอน.n
```

Three verbs ออก 'leave', เดิน 'walk', and ช็อปปิ้ง 'shop' form a serial verb construction and are connected via the `VZ` link. If any, the grammatical subject is always connected to the first verb, while post-verb modifiers are attached to the last verb.

### VC

This directed link connects a verb to its controlled verb. For example, นักการเมืองถูกนักข่าวซักถามเรื่องงบประมาณ 'The politician is questioned by the news reporters about the budget':

```
นักการเมือง       ถูก     นักข่าว          ซักถาม      เรื่อง    งบประมาณ
nák.kaːn.mɯaːŋ  tʰùːk  nák.kʰàːʊ      sák.tʰǎːm  rɯâːŋ   ŋóp.pràmaːn
politician      PASS   news reporter  question   matter  budget
'The politician is questioned by the news reporters about the budget.'
```

This sentence is parsed as follows.

```
    +--------LWs--------+----->VC----->+
    |          +<---S<--+-->O->+       +-->O-->+---NZ---+
    |          |        |      |       |       |        |
LEFT-WALL นักการเมือง.n ถูก.ps นักข่าว.n ซักถาม.v เรื่อง.n งบประมาณ.n
```

The controlled verb ซักถาม 'question' is connected to the passive marker ถูก.

### TP

This *undirected* link connects a topicalized noun phrase to its main verb. For example, บ้านเรือนไทย ฉันชอบที่สุด 'Traditional Thai houses, I like the most':

```
บ้าน    เรือน               ไทย   _  ฉัน     ชอบ     ที่สุด
bâːn   rɯaːn              tʰaɪ     tɕʰǎn  tɕʰɔ̂:p  tʰîːsùt
house  traditional house  Thai     I      like    most
'Traditional Thai houses, I like the most.'
```

This sentence is parsed as follows.

```
    +-----------------LWs-----------------+
    |       +--------------TP-------------+
    |       +-------PUst-------+          |
    |       +--NZ--+--NZ-+     |    +<-S<-+>AVw>+
    |       |      |     |     |    |     |     |
LEFT-WALL บ้าน.n เรือน.n ไทย.n _.pu ฉัน.pr ชอบ.v ที่สุด.r
```

The topicalized object บ้านเรือนไทย 'Traditional Thai houses' is connected to its main verb ชอบ 'like' via the `TP` link.

### IJ

This *undirected* link connects an interjection word to the left wall. For example, เฮ้ย มันจะเป็นไปได้หรือ 'Whoa, how could it happen?'

```
เฮ้ย   _  มัน   จะ    เป็นไป    ได้    หรือ
hə́ɪ      man  tɕà   pen.paɪ  dâːɪ  rɯ̌ː
whoa     it   will  happen   ABLE  QUES
'Whoa, how could it happen?'
```

This sentence is parsed as follows.

```
    +--------------LWs--------------+
    |        +----------IJ----------+
    |        |          +<----S<----+-----PT-----+
    |        +-PUs-+    |     +<AXw<+->AVw>+     |
    |        |     |    |     |     |      |     |
LEFT-WALL เฮ้ย.ij _.pu มัน.pr จะ.x เป็นไป.v ได้.r หรือ.pt
```

The interjection เฮ้ย 'whoa' is connected to its main verb via the `IJ` link.

### PT

This *undirected* link connects a particle word to the right wall. For example, เธอเคยไปโคราชหรือยัง 'Have you been to Khorat yet?':

```
เธอ   เคย    ไป   โคราช     หรือยัง
tʰəː  kʰəːɪ  paɪ  kʰoːrâːt  rɯ̌ːjaŋ
you   PERF   go   Khorat    QUES.yet
'Have you been to Khorat yet?'
```

This sentence is parsed as follows.

```
    +---------LWs--------+
    |        +<----S<----+------PT------+
    |        |     +<AXw<+->O->+        |
    |        |     |     |     |        |
LEFT-WALL เธอ.pr เคย.x ไป.v โคราช.n หรือยัง.pt
```

The particle หรือยัง 'yet?' is connected to its main verb ไป 'go' via the `PT` link.

----------

## Noun Phrase

### NZ

This *undirected* link connects a noun to its modifying noun in a serial noun construction. For example, คุณภาพชีวิตชาวไทยภูเขาก็เป็นปัจจัยสำคัญ 'The life quality of Thai hill tribes is also a crucial factor':

```
คุณภาพ        ชีวิต       ชาวไทย      ภูเขา         ก็     เป็น  ปัจจัย    สำคัญ
kʰunnapʰâːp  tɕʰiːʋít  tɕʰaːʊtʰaɪ  pʰuːkʰǎʊ     kɔ̂ː   pen  pàtɕaɪ  sǎmkʰan
quality      life      Thai        hill tribes  also  be   factor  crucial
'The life quality of Thai hill tribes is also a crucial factor.'
```

This sentence is parsed as follows.

```
    +--------------------LWs-------------------+
    |        +<---------------S<---------------+
    |        +--NZ--+---NZ--+---NZ--+    +<AXw<+-->O->+->AJv>+
    |        |      |       |       |    |     |      |      |
LEFT-WALL คุณภาพ.n ชีวิต.n ชาวไทย.n ภูเขา.n ก็.x  เป็น.v ปัจจัย.n สำคัญ.va
```

Four nouns คุณภาพ 'quality', ชีวิต 'life', ชาวไทย 'Thai', and ภูเขา 'hill' form a serial noun construction and are connected to each other via the `NZ` link. If any, the pre-noun modifier is linked to the first noun, while the post-noun modifiers are linked to the last noun.

### AJ

This directed link connects a nominal modifier to its core noun. There are five types of `AJ` links: `AJj`, `AJr`, `AJp`, `AJv`, and `AJt`.

`AJj` connects an adjective to its core noun. For example, รถยนต์คันก่อนหน้า เขาไปถึงหรือยัง 'The leading car, has it arrived yet?':

```
รถยนต์    คัน    ก่อนหน้า    _  เขา   ไป   ถึง      หรือยัง
rót.jon  kʰan  kɔ̀ːn.nâː     kʰǎʊ  paɪ  tʰɯ̌ŋ    rɯ̌ːjaŋ
car      CL    previous     he    go   arrive  QUES.yet
'The leading car, has it arrived yet?'
```

The sentence is parsed as follows.

```
    +-----------------LWs-----------------+
    |        +-------------TP-------------+
    |        +----->AJj---->+             +-----PT-----+
    |        |      +<-CLn<-+       +<-S<-+>AVw>+      |
    |        |      |       |       |     |     |      |
LEFT-WALL รถยนต์.n คัน.cl ก่อนหน้า.j เขา.pr ไป.v  ถึง.r หรือยัง.pt
```

The noun modifier คันก่อนหน้า 'previous CL' connects to the core noun via the `AJj` link.

`AJr` connects a relativizer to its core noun. For example, สมุดที่ฉันซื้อมาอยู่ที่ไหน 'Where is the book I have bought?':

```
สมุด    ที่     ฉัน     ซื้อ   มา        อยู่   ที่ไหน
sàmùt  tʰîː  tɕʰǎn  sɯ́ː  maː       jùː  tʰîː.nǎɪ
book   REL   I      buy  ADV.come  be   where
'Where is the book I have bought?'
```

The sentence is parsed as follows.

```
    +----------------LWs----------------+
    |       +<------------S<------------+
    |       |     +--->PC--->+          |
    |       +>AJr>+    +<-S<-+>AVw>+    +->O->+
    |       |     |    |     |     |    |     |
LEFT-WALL สมุด.n ที่.rl ฉัน.pr ซื้อ.v  มา.r อยู่.v ที่ไหน.pr
```

The relative pronoun ที่ is linked to the core noun via the link `AJr`.

`AJp` connects a preposition phrase to its core noun. For example, สินค้าบนรถไฟยังปลอดภัยอยู่ 'The goods on the train are still safe':

```
สินค้า    บน   รถไฟ    ยัง     ปลอดภัย      อยู่
sǐnkʰá  bon  rótfaɪ  jaŋ    plɔ̀ːt.pʰaɪ  jùː
goods   on   train   still  safe        yet
'The goods on the train are still safe.'
```

The sentence is parsed as follows.

```
    +-------------------LWs------------------+
    |        +<--------------S<--------------+
    |        |                   +<---AXw<---+
    |        +>AJpr>+->PO->+     |    +<-AXw<+->AVw->+
    |        |      |      |     |    |      |       |
LEFT-WALL สินค้า.n บน.pan รถไฟ.n ยัง.x คง.x ปลอดภัย.va อยู่.r
```

Preposition บน 'on' is linked to the core noun สินค้า 'goods' via the `AJp` link.

`AJv` connects an attributive verb that acts as an adjective to its core noun. For example, นักเรียนคนขยันกำลังเดินทาง 'The diligent student is traveling':

```
นักเรียน    คน    ขยัน            กำลัง    เดินทาง
nák.rian  kʰon  kʰayǎn         kamlaŋ  dəːn.tʰaːŋ
student   CL    ATTR.diligent  PROG    travel
'The diligent student is traveling.'
```

This sentence is parsed as follows.

```
    +------------------LWs-----------------+
    |         +<-------------S<------------+
    |         +---->AJv---->+              |
    |         |      +<-CLn<+      +<-AXw<-+
    |         |      |      |      |       |
LEFT-WALL นักเรียน.n คน.cl ขยัน.va กำลัง.x เดินทาง.v
```

Acting as an adjective, attributive verb ขยัน 'be diligent' is connected to the core noun นักเรียน 'student' via the `AJv` link.

### RI

This directed link connects an implicit relative clause to its core noun. For example, หม้อหุงข้าวใบนี้มีระบบอัตโนมัติ 'This rice cooker has an automatic mechanism':

```
หม้อ       หุง    ข้าว    ใบ   นี้     มี     ระบบ       อัตโนมัติ
mɔ̂ː       hǔŋ   kʰâːʊ  baɪ  níː   miː   rábòp      ʔàttànoːmát
saucepan  cook  rice   CL   this  have  mechanism  automatic
'This rice cooker has an automatic mechanism.'
```

This sentence is parsed as follows.

```
    +----------------LWs---------------+
    |       +<------------S<-----------+
    |       +--------->AJj-------->+   |
    |       +->RI>+->O>+     +<CLn<+   +->O->+->AJv->+
    |       |     |    |     |     |   |     |       |
LEFT-WALL หม้อ.n หุง.v ข้าว.n ใบ.cl  นี้.j มี.v ระบบ.n อัตโนมัติ.va
```

The implicit relative clause หุงข้าว 'cook rice' is connected to the core noun หม้อ 'pot' via the `RI` link.

### AT

This directed link connects an attribute noun to its core noun. For example, ระบบขนาดใหญ่ย่อมมีความสลับซับซ้อน 'A big-scaled system always has complexity':

```
ระบบ    ขนาด     ใหญ่  ย่อม     มี     ความ    สลับซับซ้อน
rábòp   kʰánàːt  jàɪ  yɔ̂m     miː   kʰʷaːm  sàlàp.sápsɔ́ːn
system  scale    big  always  have  NOM     complex
'A big-scaled system always has complexity.'
```

This sentence is parsed as follows.

```
    +----------------LWs----------------+
    |        +<------------S<-----------+
    |        +->AT->+->AJv->+     +<AXw<+->O->+-->AJv-->+
    |        |      |       |     |     |     |         |
LEFT-WALL ระบบ.n ขนาด.na ใหญ่.va ย่อม.x  มี.v ความ.fx สลับซับซ้อน.va
```

Attribute modifier ขนาดใหญ่ 'big-scaled' is connected to the core noun ระบบ 'system' via the `AT` link.

### PS

This directed link connects a possessive pronoun to its core noun. For example, คอมพิวเตอร์เขาติดเชื้อไวรัส 'His computer is infected with a virus':

```
คอมพิวเตอร์    เขา      ติด           เชื้อ     ไวรัส
kʰɔmpʰíʊtə̂ː  kʰǎʊ     tìt          tɕʰɯáː  ʋaɪrás
computer     POSS.he  be infected  germ    virus
'His computer is infected with a virus.'
```

This sentence is parsed as follows.

```
    +------------LWs-----------+
    |          +<------S<------+
    |          +--->PS-->+     +->O>+--NZ--+
    |          |         |     |    |      |
LEFT-WALL คอมพิวเตอร์.n เขา.pr ติด.v เชื้อ.n ไวรัส.n
```

Possessive pronoun เขา 'his' is connected to the core noun คอมพิวเตอร์ 'computer' via the `PS` link.

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

Two `AVp` links can also be constituted as a coordinate structure.

```
                 +----------------------------PT---------------------------+
                 |                   +------>AVpr------>+                  |
    +-----LWs----+------->AVw------->+     +<---AVpl----+                  |
    |       +<-S<+-->O->+--NZ--+     |     +->PO>+      +-AVpr>+->PO>+     |
    |       |    |      |      |     |     |     |      |      |     |     |
LEFT-WALL ครู.n วาง.v หนังสือ.n เอา.n ไว้.r บน.pan โต๊ะ.n หรือ.cp ใต้.pan โต๊ะ.n ล่ะ.pt
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

This directed link connects a numeral phrase to its head. There are two types of `NU` links: `NUn` and `NUv`.

`NUn` connects a numeral phrase to its core noun.

```
    +----------------LWs----------------+
    |         +<-----------S<-----------+
    |         +>NUnr>+>CLn>+      +<AXw<+>AVw>+
    |         |      |     |      |     |     |
LEFT-WALL นักเรียน.n 3.nu  คน.cl กำลัง.x เดิน.v มา.r
```

`NUv` connects a numeral phrase to its main verb.

```
                +----------->O---------->+
    +----LWs----+    +--------JNl--------+
    |       +<S<+    +>NUvr>+>CLv>+      +-JNr+>NUvr>+>CLv>+
    |       |   |    |      |     |      |    |      |     |
LEFT-WALL แม่.n ตี.v น้อง.n  3.nu ครั้ง.cl และ.cn พี่.n   2.nu ครั้ง.cl
```

### CL

This directed link connects a classifier to its number. There are two types of `CL` links: `CLn` and `CLv`.

`CLn` connects a classifier for nouns to its number.

```
    +------LWs-----+
    |        +<-S<-+->O->+>NUnr>+>CLn>+
    |        |     |     |      |     |
LEFT-WALL สมชาย.n มี.v รถยนต์.n 30.nu คัน.cl
```

`CLv` connects a classifier for verbs to its number.

```
    +-----LWs-----+      +------->AVvr------>+
    |       +<-S<-+--VZ--+-->O->+      +<QFl<+>CLv>+
    |       |     |      |      |      |     |     |
LEFT-WALL พอล.n เดิน.v สำรวจ.v พื้นที่.n ถึง.qfl 3.nu รอบ.cl
```

### QF

This directed link connects a numeral quantifier to its number.

```
    +-----------------LWs-----------------+
    |         +<------------S<------------+
    |         +----->NUnr----->+          |
    |         |        +<-QFl<-+>CLn>+    +->O->+-->AJv->+
    |         |        |       |     |    |     |        |
LEFT-WALL นักเรียน.n เกือบ.qfl 100.nu คน.cl มี.v อาการ.n ท้องเสีย.va
```

----------

## Coordinate Structure

### JN

This *undirected* link connects a noun-phrase conjunct to its coordinator.

```
    +--------------LWs--------------+
    |             +<-------S<-------+
    |       +-JNl-+-JNr-+    +<-AXw<+-->O->+
    |       |     |     |    |      |      |
LEFT-WALL พ่อ.n และ.cn แม่.n ต่าง.x ปกป้อง.v ลูก.n
```

### JV

This *undirected* link connects a verb-phrase conjunct to its coordinator.

```
    +---------LWs--------+
    |        +<----S<----+
    |        |     +<AXw<+--JVl--+-JVr-+--VZ-+-PT-+
    |        |     |     |       |     |     |    |
LEFT-WALL เขา.pr จะ.x ทำงาน.v หรือ.cv นอน.v พัก.v ล่ะ.pt
```

### JP

This *undirected* link connects a preposition-phrase conjunct to its coordinator.

```
             +------->AJpr------->+
             |       +-----JPl----+
    +---LWn--+       +->PO>+      +--JPr-+->PO->+
    |        |       |     |      |      |      |
LEFT-WALL หนังสือ.n บน.pan โต๊ะ.n และ.cp ใต้.pan เก้าอี้.n
```

### JD

This *undirected* link connects a discontinuous conjunction to its core conjunction.

```
                  +-------->O-------->+
    +-----LWs-----+    +------JD------+
    |       +<-S<-+    |      +--JNl--+-JNr-+
    |       |     |    |      |       |     |
LEFT-WALL ฉัน.pr ซื้อ.v ทั้ง.cd หนังสือ.n และ.cn สมุด.n
```

----------

## Punctuation

### PU

This *undirected* link connects a punctuation mark to its head. There are three types of `PU` links: `PUs`, `PUp`, and `PUy`.

`PUs` connects a white space to its head.

```
                              +--------JNr-------+
    +------LWs-----+---->O--->+     +-----JNl----+
    |        +<-S<-+    +-JNl-+     |      +-PUs-+--JNr-+
    |        |     |    |     |     |      |     |      |
LEFT-WALL เรา.pr ซื้อ.v สมุด.n _.cn หนังสือ.n _.pu และ.cn ดินสอ.n
```

`PUp` connects a paiyal noi 'ฯ' to its head.

```
    +------------LWs-----------+
    |         +<-------S<------+
    |         +--PUp-+-PUs+    +--->O-->+->AJpr->+-->PO-->+--NZ--+
    |         |      |    |    |        |        |        |      |
LEFT-WALL กรุงเทพ.n ฯ.pu _.pu เป็น.v เมืองหลวง.n ของ.pnn ประเทศ.n ไทย.n
```

`PUy` connects a mai yamok 'ๆ' to its head.

```
    +------------------LWs------------------+
    |       +<--------------S<--------------+
    |       |              +<------AXw<-----+
    |       |              |    +<---AXw<---+
    |       +-PUy-+-PUs+   |    |    +<-AXg<+->AVw>+
    |       |     |    |   |    |    |      |      |
LEFT-WALL เด็ก.n ๆ.pu _.pu ก็.x ยัง.x ไม่.ng เข้าใจ.v อยู่ดี.r
```