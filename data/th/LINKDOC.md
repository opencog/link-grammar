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
[JP](#jp)
[JV](#jv)
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

#### LWs

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

#### LWn

`LWn` links the core noun of a noun phrase to the left wall. For example, เครื่องบินพาณิชย์ 30 ลำ 'thirty commercial airplanes':

```
เครื่องบิน     พาณิชย์       30        ลำ
kʰrɯâŋ.bin  pʰaːnít     sǎːm.sib  lam
airplane    commercial  thirty    CL.vehicle
'thirty commercial airplanes'
```

This noun phrase is parsed below.

```
              +----->NUnr---->+
    +---LWn---+-->AJj->+      +>CLn>+
    |         |        |      |     |
LEFT-WALL เครื่องบิน.n พาณิชย์.j 30.nu ลำ.cl
```

#### LWp

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
เรา  รับประทาน     อาหาร    กลางวัน     ที่     ร้าน         นี้
raʊ  ráppratʰaːn  ʔaːhǎːn  klaːŋ.ʋan  tʰîː  ráːn        níː
we   eat          meal     afternoon  at    restaurant  this
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
ฉัน     ออก    เดิน   ช็อปปิ้ง     ที่     ห้าง   พารากอน
tɕʰǎn  ʔɔ̀ːk   dəːn  tɕɔ́ppîŋ   tʰîː  hâːŋ  pʰaːraːkɔ̂n
I      leave  walk  shopping  at    mall  Paragon
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
คุณภาพ        ชีวิต       ชาวไทย      ภูเขา      ก็     เป็น  ปัจจัย    สำคัญ
kʰunnapʰâːp  tɕʰiːʋít  tɕʰaːʊtʰaɪ  pʰuːkʰǎʊ  kɔ̂ː   pen  pàtɕaɪ  sǎmkʰan
quality      life      Thai        mountain  also  be   factor  crucial
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

Four nouns คุณภาพ 'quality', ชีวิต 'life', ชาวไทย 'Thai', and ภูเขา 'mountain' form a serial noun construction and are connected to each other via the `NZ` link. If any, the pre-noun modifier is linked to the first noun, while the post-noun modifiers are linked to the last noun.

### AJ

This directed link connects a nominal modifier to its core noun. There are five types of `AJ` links: `AJj` (simple adjective), `AJr` (relative clause), `AJp` (preposition phrase), and `AJv` (attributive verb).

#### AJj

`AJj` connects an adjective to its core noun. For example, รถยนต์คันก่อนหน้า เขาไปถึงหรือยัง 'The preceding car, has it arrived yet?':

```
รถยนต์    คัน          ก่อนหน้า    _  เขา   ไป   ถึง      หรือยัง
rót.jon  kʰan        kɔ̀ːn.nâː     kʰǎʊ  paɪ  tʰɯ̌ŋ    rɯ̌ːjaŋ
car      CL.vehicle  previous     he    go   arrive  QUES.yet
'The preceding car, has it arrived yet?'
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

The noun modifier คันก่อนหน้า 'preceding CL.vehicle' connects to the core noun รถยนต์ 'car' via the `AJj` link.

#### AJr

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

The relative pronoun ที่ is linked to the core noun สมุด 'book' via the link `AJr`.

#### AJp

`AJp` connects a preposition phrase to its core noun. For example, สินค้าบนรถไฟยังปลอดภัยอยู่ 'The goods on the train are still safe':

```
สินค้า    บน   รถไฟ     ยัง     ปลอดภัย      อยู่
sǐnkʰá  bon  rót.faɪ  jaŋ    plɔ̀ːt.pʰaɪ  jùː
goods   on   train    still  safe        yet
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

#### AJv

`AJv` connects an attributive verb that acts as an adjective to its core noun. For example, นักเรียนคนขยันกำลังเดินทาง 'The diligent student is traveling':

```
นักเรียน    คน         ขยัน            กำลัง    เดินทาง
nák.rian  kʰon       kʰayǎn         kamlaŋ  dəːn.tʰaːŋ
student   CL.person  ATTR.diligent  PROG    travel
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

Performing as an adjective, attributive verb ขยัน 'be diligent' is connected to the core noun นักเรียน 'student' via the `AJv` link.

### RI

This directed link connects an implicit relative clause to its core noun. For example, หม้อหุงข้าวใบนี้มีระบบอัตโนมัติ 'This rice cooker has an automatic mechanism':

```
หม้อ       หุง    ข้าว    ใบ            นี้     มี     ระบบ       อัตโนมัติ
mɔ̂ː       hǔŋ   kʰâːʊ  baɪ           níː   miː   rábòp      ʔàttànoːmát
saucepan  cook  rice   CL.container  this  have  mechanism  automatic
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

This directed link connects an attribute noun to its core noun. For example, ระบบขนาดใหญ่ย่อมมีความสลับซับซ้อน 'A large-scaled system always has complexity':

```
ระบบ    ขนาด     ใหญ่    ย่อม     มี     ความ    สลับซับซ้อน
rábòp   kʰánàːt  jàɪ    yɔ̂m     miː   kʰʷaːm  sàlàp.sápsɔ́ːn
system  scale    large  always  have  NOM     complex
'A large-scaled system always has complexity.'
```

This sentence is parsed as follows.

```
    +----------------LWs----------------+
    |        +<------------S<-----------+
    |        +->AT->+->AJv->+     +<AXw<+->O->+-->AJv-->+
    |        |      |       |     |     |     |         |
LEFT-WALL ระบบ.n ขนาด.na ใหญ่.va ย่อม.x  มี.v ความ.fx สลับซับซ้อน.va
```

Attribute modifier ขนาดใหญ่ 'large-scaled' is connected to the core noun ระบบ 'system' via the `AT` link.

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

This directed link connects an auxiliary to its main verb. All kinds of auxiliary always <u>**precede**</u> the main verb. There are three types of `AX` links: `AXw` (simple auxiliary), `AXg` (negator), and `AXq` (emphasis).

#### AXw

`AXw` connects an auxiliary verb to its main verb. For example, เขาจะเดินทางไปอุบลราชธานีอยู่แล้ว 'He would travel to Ubon Ratchathani anyway':

```
เขา   จะ   เดินทาง     ไป   อุบลราชธานี             อยู่แล้ว
he    FUT  travel     go   Ubon Ratchathani      already
kʰǎʊ  tɕà  dəːntʰa:ŋ  paɪ  ʔùbon.râːtɕʰátʰaːniː  jùːlǽːʊ
'He would travel to Ubon Ratchathani anyway.'
```

This sentence is parsed as follows.

```
    +---------LWs---------+
    |        +<-----S<----+      +------>AVw------>+
    |        |     +<-AXw<+--VZ--+-->O-->+         |
    |        |     |      |      |       |         |
LEFT-WALL เขา.pr จะ.x เดินทาง.v ไป.v อุบลราชธานี.n อยู่แล้ว.r
```

Auxiliary verb จะ 'will' is connected to the main verb เดินทาง 'travel' via the `AXw` link.

#### AXg

`AXg` connects a negator to its main verb. For example, เขาไม่ยอมไปตรวจโรคติดต่อ 'He did not have himself checked up for infectious diseases':

```
เขา   ไม่   ยอม      ไป   ตรวจ     โรค      ติดต่อ
kʰǎʊ  mâɪ  jɔːm     paɪ  truàt    rôːk     tìttɔ̀ː
he    NEG  consent  go   examine  disease  infect
'He did not have himself checked up for infectious diseases.'
```

This sentence is parsed as follows.

```
    +---------LWs--------+
    |        +<----S<----+
    |        |     +<AXg<+--VZ-+--VZ-+->O->+->RI->+
    |        |     |     |     |     |     |      |
LEFT-WALL เขา.pr ไม่.ng ยอม.v ไป.v ตรวจ.v โรค.n ติดต่อ.v
```

Negator ไม่ 'not' is connected to the main verb ยอม 'consent' via the `AXg` link.

#### AXq

`AXq` connects an emphasis marker to the main verb. For example, เราแทบจะไม่ได้ข่าวเกี่ยวกับเขาเลย 'We have not heard any news about him':

```
เรา  แทบ     ไม่   ได้       ข่าว    เกี่ยวกับ   เขา   เลย
raʊ  tʰæ̂ːp   mâɪ  dâɪ      kʰà:ʊ  kiàʊkàp  kʰǎʊ  ləːɪ
we   almost  NEG  receive  news   about    he    PART.EMPH
'We have not heard any news about him.'
```

This sentence is parsed as follows.

```
    +---------------LWs--------------+
    |        +<----------S<----------+
    |        |      +<------AXq<-----+-------------PT-------------+
    |        |      |     +<---AXw<--+---->AVpr--->+              |
    |        |      |     |    +<AXg<+->O>+        +-->PO->+      |
    |        |      |     |    |     |    |        |       |      |
LEFT-WALL เรา.pr แทบ.qr จะ.x ไม่.ng ได้.v ข่าว.n เกี่ยวกับ.pan เขา.n เลย.pt
```

Emphasis marker แทบ 'almost' is connected to the main verb ได้ 'receive' via the `AXq` link.

### AV

This directed link connects an adverbial to its main verb. All kinds of adverbial (except cohesive marker) always <u>**follow**</u> the main verb. There are seven types of `AV` links: `AVp` (preposition phrase), `AVw` (simple adverb), `AVg` (negative adverb), `AVn` (noun's numeral), `AVv` (verb's numeral), `AVt` (temporal phrase), and `AVc` (cohesive marker).

#### AVp

`AVp` connects a preposition phrase to its main verb. For example, เรานัดพบกันที่ปารีส 'We arrange to meet up in Paris':

```
เรา  นัด       พบ    กัน     ที่     ปารีส
raʊ  nát      pʰóp  kan    tʰîː  paːrîːs
we   arrange  meet  RECIP  at    Paris
'We arrange to meet up in Paris.'
```

This sentence is parsed as follows.

```
    +------LWs-----+    +-->AVpr->+
    |        +<-S<-+-VZ-+->O>+    +->PO->+
    |        |     |    |    |    |      |
LEFT-WALL เรา.pr นัด.v พบ.v กัน.n ที่.pan ปารีส.n
```

The preposition ที่ 'at' is connected to the main verb พบ 'meet' via the `AVp` link.

Two `AVp` links can also be constituted as a coordinate structure. For example, ครูวางหนังสือเอาไว้บนโต๊ะหรือใต้โต๊ะล่ะ 'Did you (teacher) put the book *on* <u>or</u> *under* the table?':

```
ครู       วาง    หนังสือ   เอา       ไว้         บน   โต๊ะ    หรือ  ใต้     โต๊ะ    ล่ะ
kʰruː    ʋaːŋ   nǎŋsɯ̌ː  ʔaʊ       ʋáɪ        bon  tó     rɯ̌ː  tâɪ    tó     lâ
teacher  place  book    ADV.hold  ADV.place  on   table  or   under  table  EMPH
'Did you (teacher) put the book on or under the table?'
```

This sentence is parsed as follows.

```
                 +----------------------------PT---------------------------+
                 |                   +------>AVpr------>+                  |
    +-----LWs----+---->AVw---->+     |     +<---AVpl----+                  |
    |       +<-S<+-->O->+      +>AVw>+     +->PO>+      +-AVpr>+->PO>+     |
    |       |    |      |      |     |     |     |      |      |     |     |
LEFT-WALL ครู.n วาง.v หนังสือ.n เอา.r ไว้.r บน.pan โต๊ะ.n หรือ.cp ใต้.paa โต๊ะ.n ล่ะ.pt
```

Two prepositions บน 'on' and ใต้ 'under' are coordinated with conjunction หรือ 'or'.

#### AVw

`AVw` connects a simple adverb to its main verb. For example, สายการบินนกแอร์มาถึงท่าอากาศยานอุบลราชธานีแล้ว 'Our plane (Nok-Air Airlines) has just arrived at Ubon Ratchathani Airport':

```
สายการบิน       นกแอร์    มา    ถึง      ท่าอากาศยาน          อุบลราชธานี            แล้ว
sǎːɪ.kaːn.bin  nók.ʔæː  maː   tʰɯ̌ŋ    tʰâː.ʔaːkàːtsàjaːn  ʔùbonrâːtɕʰátʰaːniː  lǽːʊ
airlines       Nok-Air  come  arrive  airport             Ubon Ratchathani     PAST
'Our plane (Nok-Air Airlines) has just arrived at Ubon Ratchathani Airport.'
```

This sentence is parsed as follows.

```
    +---------------LWs--------------+
    |          +<---------S<---------+------------>AVw------------>+
    |          +---NZ---+      +<AXw<+--->O-->+-----NZ----+        |
    |          |        |      |     |        |           |        |
LEFT-WALL สายการบิน.n นกแอร์.n มา.x  ถึง.v ท่าอากาศยาน.n อุบลราชธานี.n แล้ว.r
```

Past-tense adverb แล้ว is connected to the main verb ถึง 'arrive' via the `AVw` link.

#### AVg

`AVg` connects a negative adverb to its main verb. For example, นักเรียนจะกระทำอย่างนี้มิได้ 'Students, do not do this':

```
นักเรียน    จะ   กระทำ    อย่าง  นี้     มิได้
nák.rian  tɕà  kràtʰam  jàːŋ  níː   mí.dâɪ
student   FUT  do       like  this  IMP.NEG
'Students, do not do this.'
```

This sentence is parsed as follows.

```
    +----------LWs---------+
    |         +<-----S<----+------->AVgr------>+
    |         |      +<AXw<+->AVw->+>AVw>+     |
    |         |      |     |       |     |     |
LEFT-WALL นักเรียน.n จะ.x กระทำ.v อย่าง.r  นี้.r มิได้.ng
```

Negative adverb มิได้ is connected to the main verb กระทำ 'do' via the `AVg` link.

#### AVn

`AVn` connects a numeral modifier of a noun phrase to the main verb so as to avoid crossing links. For example, แม่ฝากซื้อกล้วยหอมไว้ 2 ลูก 'Mother has asked (me) to buy two bananas':

```
แม่      ฝาก   ซื้อ   กล้วยหอม     ไว้    2     ลูก
mæ̂ː     fàːk  sɯ́ː  kluâɪ.hɔ̌ːm  ʋáɪ   sɔ̌ːŋ  lûːk
mother  ask   buy  banana      EMPH  two   CL.fruit
'Mother has asked (me) to buy two bananas.'
```

This sentence is parsed as follows.

```
                       +------->AVnr------>+
    +-----LWs----+     +----->AVw---->+    |
    |       +<-S<+--VZ-+-->O->+       |    +>CLn>+
    |       |    |     |      |       |    |     |
LEFT-WALL แม่.n ฝาก.v ซื้อ.v กล้วยหอม.n ไว้.r 2.nu  ลูก.cl
```

Although numeral phrase สองลูก '2 CL.fruit' modifies กล้วยหอม 'banana', it forms an `AVn` link to the main verb ซื้อ 'buy' to avoid crossing links. Thai Link Grammar **<u>always</u>** preserves the projectivity of dependency structures.

#### AVv

`AVv` connects a numeral modifier to the main verb. For example, นักเรียนถูกลงโทษซ้ำ 2 ครั้ง 'The student was punished twice':

```
นักเรียน    ถูก     ลงโทษ      ซ้ำ     2     ครั้ง
nák.rian  tʰùːk  loŋ.tʰôːt  sám    sɔ̌ːŋ  kʰráŋ
student   PASS   punish     again  two   CL.frequency
'The student was punished twice.'
```

The sentence is parsed as follows.

```
    +----------LWs---------+
    |         +<-----S<----+--->AVvr-->+
    |         |      +<AXw<+->AVw>+    +>CLv>+
    |         |      |     |      |    |     |
LEFT-WALL นักเรียน.n ถูก.x ลงโทษ.v ซ้ำ.r 2.nu ครั้ง.cl
```

Numeral phrase สองครั้ง '2 CL.frequency' is connected to the main verb ลงโทษ 'punish' via the `AVv` link.

#### AVt

`AVt` connects a temporal phrase to the main verb. For example, เพื่อนจัดงานให้ฉันวันที่ 10 สิงหาคม 'My friends held me a party on the 10th of August':

```
เพื่อน    จัด        งาน    ให้   ฉัน     วัน   ที่        10   สิงหาคม
pʰɯân   tɕàt      ŋaːn   hâɪ  tɕʰǎn  ʋan  tʰîː     sìp  sǐŋhǎːkʰom
friend  organize  party  for  me     day  ORDMARK  ten  August
'My friends held me a party on the 10th of August.'
```

This sentence is parsed as follows.

```
                   +------------>AVtr----------->+
    +------LWs-----+--->AVpr-->+                 |
    |        +<-S<-+->O>+      +->PO>+           |
    |        |     |    |      |     |           |
LEFT-WALL เพื่อน.n จัด.v งาน.n ให้.pva ฉัน.pr วันที่_10_สิงหาคม[!]
```

Temporal phrase วันที่ 10 สิงหาคม 'the 10th of August' is connected to the main verb จัด 'organize' via the `AVt` link.

#### AVc

`AVc` connects a cohesive marker to the main verb. The cohesive markers always <u>**precede**</u> a sentence. For example, อย่างไรก็ตาม ข้าพเจ้าได้ติดตามการทำงานของทุกฝ่ายแล้ว 'However, I have followed up the operation of all departments':

```
อย่างไรก็ตาม       ข้าพเจ้า        ได้    ติดตาม      การ   ทำงาน     ของ    ทุก     ฝ่าย         แล้ว
jàːŋraɪ.kɔ̂ːtaːm  kʰâːpʰátɕâːʊ  dâːɪ  tìttaːm    kaːn  tʰamŋaːn  kʰɔ̌ːŋ  tʰúk   fàːɪ        lǽːʊ
however          I.FORMAL      PERF  follow up  NOM   operate   of     every  department  PAST
'However, I have followed up the operation of all departments.'
```

This sentence is parsed as follows.

```
    +-----------------LWs-----------------+
    |           +<----------AVcl<---------+------------------>AVw------------------>+
    |           |           +<-----S<-----+       +---->AJpr---->+---->PO---->+     |
    |           |           |       +<AXw<+-->O-->+->RI->+       |      +<AJj<+     |
    |           |           |       |     |       |      |       |      |     |     |
LEFT-WALL อย่างไรก็ตาม.rc ข้าพเจ้า.pr ได้.x ติดตาม.v การ.fx ทำงาน.v ของ.pnn ทุก.jl ฝ่าย.n แล้ว.r
```

Cohesive marker อย่างไรก็ตาม 'however' is connected to the main verb ติดตาม 'follow up' via the `AVc` link.

----------

## Preposition Phrase

### PO

This directed link connects a preposition to its complementing noun phrase. For example, บนเครื่องบินมีห้องน้ำหรือไม่ 'Are there any lavatories on the airplane?':

```
บน   เครื่องบิน      มี      ห้องน้ำ     หรือไม่
bon  kʰrɯâːŋ.bin  miː    hɔ̂ŋ.náːm  rɯ̌ːmâɪ
on   airplane     exist  lavatory  QUES
'Are there any lavatories on the plane?'
```

This sentence is parsed as follows.

```
    +----------LWs----------+
    |        +<----AVpl<----+------PT------+
    |        +-->PO->+      +->O->+        |
    |        |       |      |     |        |
LEFT-WALL บน.pan เครื่องบิน.n มี.v ห้องน้ำ.n หรือไม่.pt
```

Noun phrase เครื่องบิน 'airplane' is connected to the preposition บน 'on' via the `PO` link.

### PC

This directed link connects a preposition to its complementing sentence. For example, สมชายจึงเดินทางไปสกลนครทันที หลังจากได้ทราบข่าวดีของภรรยา 'Consequently, Somchai immediately travels to Sakon Nakhon after he has heard the good news about his wife':

```
สมชาย      จึง            เดินทาง     ไป   สกลนคร         ทันที          หลังจาก    ได้    ทราบ  ข่าว    ดี     ของ    ภรรยา
sǒmtɕʰaːɪ  tɕɯŋ          dəːntʰaːŋ  paɪ  sàkon.nákʰɔːn  tʰantʰiː     lǎŋtɕàːk  dâːɪ  sâːp  kʰàːʊ  diː   kʰɔ̌ːŋ  pʰanrájaː
Somchai    consequently  travel     go   Sakon Nakon    immediately  after     PERF  know  news   good  of     wife
'Consequently, Somchai immediately travels to Sakon Nakhon after he has heard the good news about his wife.'
```

This sentence is parsed as follows.

```
    +----------LWs---------+
    |        +<-----S<-----+      +---->AVw---->+        +----->PC---->+     +--->AJpr-->+
    |        |      +<-AXw<+--VZ--+-->O->+      +->AVpr->+       +<AXw<+->O->+>AJv>+     +-->PO->+
    |        |      |      |      |      |      |        |       |     |     |     |     |       |
LEFT-WALL สมชาย.n จึง.x เดินทาง.v ไป.v สกลนคร.n ทันที.r หลังจาก.pva ได้.x ทราบ.v ข่าว.n ดี.va ของ.pnn ภรรยา.n
```

Verb ทราบ 'know' in the subordinate clause is linked to preposition หลังจาก 'after' via the `PC` link.

### PZ

This directed link connects a preposition to its core preposition in a serial preposition construction. Thai allows double prepositions, as long as both of them take the same argument. There are two types of `PZ` links: `PZn` (noun-consuming preposition) and `PZv` (verb-consuming preposition).

#### PZn

`PZn` connects a preposition to its core noun-consuming preposition. For example, พ่อวางของไว้ที่บนบ้าน 'Father put (his) belongings in the house':

```
พ่อ      วาง   ของ        ไว้    ที่     บน   บ้าน
pʰɔ̂ː    ʋaːŋ  kʰɔ̌ːŋ      ʋáɪ   tʰîː  bon  bâːn
father  put   belonging  EMPH  at    on   house
'Father put (his) belongings in the house.'
```

This sentence is parsed as follows.

```
    +-----LWs----+--->AVw--->+--->AVpr-->+
    |       +<-S<+->O->+     |    +<-PZn<+->PO>+
    |       |    |     |     |    |      |     |
LEFT-WALL พ่อ.n วาง.v ของ.n ไว้.r ที่.pan บน.pan บ้าน.n
```

Preposition ที่ 'at' is connected to preposition บน 'on' via the `PZz` link.

#### PZv

`PZv` connects a preposition to its core verb-consuming preposition. For example, เราจำเป็นต้องสร้างตึกเพื่อให้เกิดการพัฒนาเมือง 'We are obliged to build (some) edifices so as to make the city development happen':

```
เรา  จำเป็น       ต้อง   สร้าง   ตึก       เพื่อ      ให้       เกิด          การ   พัฒนา        เมือง
raʊ  tɕampen     tɔ̂ŋ   sâːŋ   tɯ̀k      pʰɯâː    hâɪ      kə̀ːt         kaːn  pʰáttʰánaː  mɯaːŋ
we   be obliged  must  build  edifice  INFMARK  INFMARK  make happen  NOM   develop     city
'We are obliged to build some edifices so as to make the city development happen.'
```

This sentence is parsed as follows.

```
    +-------------LWs-------------+
    |        +<---------S<--------+
    |        |      +<----AXw<----+------->AVpr------>+
    |        |      |      +<-AXw<+->O->+     +<-PZv<-+->PC>+-->O->+->RI->+-->O->+
    |        |      |      |      |     |     |       |     |      |      |      |
LEFT-WALL เรา.pr จำเป็น.x ต้อง.x สร้าง.v ตึก.n เพื่อ.pav ให้.pva เกิด.v การ.fx พัฒนา.v เมือง.n
```

Preposition เพื่อ (infinitive marker) is connected to preposition ให้ (infinitive marker) via the `PZv` link.

----------

## Numeral Phrase

### NU

This directed link connects a numeral phrase to its head. There are two types of `NU` links: `NUn` (noun-modifying numeral) and `NUv` (verb-modifying numeral).

#### NUn

`NUn` connects a numeral phrase to its core noun. For example, นักเรียน 3 คนกำลังเดินมา 'Three students are coming':

```
นักเรียน    3      คน         กำลัง    เดิน   มา
nák.rian  sǎːm   kʰon       kamlaŋ  dəːn  maː
student   three  CL.person  PROG    walk  come
'Three students are coming.'
```

This sentence is parsed as follows.

```
    +----------------LWs----------------+
    |         +<-----------S<-----------+
    |         +>NUnr>+>CLn>+      +<AXw<+>AVw>+
    |         |      |     |      |     |     |
LEFT-WALL นักเรียน.n 3.nu  คน.cl กำลัง.x เดิน.v มา.r
```

Numeral phrase สามคน 'three CL.person' is connected to the core noun นักเรียน 'student' via the `NUn` link.

#### NUv

`NUv` connects a numeral phrase to its main verb. For example, แม่ตีน้อง 3 ครั้ง 'Mother hits the younger brother three times':

```
แม่      ตี    น้อง              3      ครั้ง
mæ̂ː     tiː  nɔ̂ːŋ             sǎːm   kʰráŋ
mother  hit  younger brother  three  CL.frequency
'Mother hits the younger brother three times.'
```

This sentence is parsed as follows.

```
    +----LWs----+-->AVvr-->+
    |       +<S<+->O>+     +>CLv>+
    |       |   |    |     |     |
LEFT-WALL แม่.n ตี.v น้อง.n 3.nu ครั้ง.cl
```

However, a verb-modifying numeral phrase may sometimes be connected to a noun phrase via the `NUv` link so as to avoid crossing links. For example, แม่ตีน้อง 3 ครั้งและพี่ 2 ครั้ง 'Mother hits the younger brother three times and the older brother twice':

```
แม่      ตี    น้อง              3      ครั้ง           และ  พี่              2     ครั้ง
mæ̂ː     tiː  nɔ́ːŋ             sǎːm   kʰráŋ         lǽ   pʰîː           sɔ̌ːŋ  kʰráŋ
mother  hit  younger brother  three  CL.frequency  and  older brother  two   CL.frequency
'Mother hits the younger brother three times and the older brother twice.'
```

This sentence is parsed as follows.

```
                +----------->O---------->+
    +----LWs----+    +--------JNl--------+
    |       +<S<+    +>NUvr>+>CLv>+      +-JNr+>NUvr>+>CLv>+
    |       |   |    |      |     |      |    |      |     |
LEFT-WALL แม่.n ตี.v น้อง.n  3.nu ครั้ง.cl และ.cn พี่.n   2.nu ครั้ง.cl
```

In this case, numeral phrases สามครั้ง 'three CL.frequency' and สองครั้ง 'two CL.frequency' are connected to their core nouns น้อง 'younger brother' and พี่ 'older brother', respectively, via the `NUv` link. Although these numeral phrases actually modify the main verb ตี 'hit', we connect them to the grammatical objects instead. Thai Link Grammar **<u>always</u>** preserves the projectivity of dependency structures.

### CL

This directed link connects a classifier to its number. There are two types of `CL` links: `CLn` (noun-modifying classifier) and `CLv` (verb-modifying classifier).

#### CLn

`CLn` connects a classifier for nouns to its number. For example, สมชายมีรถยนต์ 30 คัน 'Somchai has 30 cars':

```
สมชาย      มี    รถยนต์    30        คัน
sǒmtɕʰaːɪ  miː  rót.jon  sǎːm.sìp  kʰan
Somchai    has  car      thirty    CL.car
'Somchai has 30 cars.'
```

This sentence is parsed as follows.

```
    +------LWs-----+
    |        +<-S<-+->O->+>NUnr>+>CLn>+
    |        |     |     |      |     |
LEFT-WALL สมชาย.n มี.v รถยนต์.n 30.nu คัน.cl
```

The noun-modifying classifier คัน 'CL.car' is connected to the number สามสิบ 'thirty' via the `NUn` link.

#### CLv

`CLv` connects a classifier for verbs to its number. For example, พอลเดินสำรวจพื้นที่ถึง 3 รอบ 'Paul walked around and explored the area for three rounds':

```
พอล    เดิน   สำรวจ    พื้นที่        ถึง          3      รอบ
pʰɔːl  dəːn  sǎmruàt  pʰɯ́ːntʰîː  tʰɯ̌ŋ        sǎːm   rɔ̂ːp
Paul   walk  explore  area       QUANT.EMPH  three  CL.frequency
'Paul walked around and explored the area for three rounds.'
```

This sentence is parsed as follows.

```
    +-----LWs-----+      +------->AVvr------>+
    |       +<-S<-+--VZ--+-->O->+      +<QFl<+>CLv>+
    |       |     |      |      |      |     |     |
LEFT-WALL พอล.n เดิน.v สำรวจ.v พื้นที่.n ถึง.qfl 3.nu รอบ.cl
```

The verb-modifying classifier รอบ 'CL.frequency' is connected to the number สาม 'three' via the `QF` link.

### QF

This directed link connects a numeral quantifier to its number. For example, นักเรียนเกือบ 100 คนมีอาการท้องเสีย 'Almost 100 students are having a case of diarrhea':

```
นักเรียน    เกือบ          100          คน         มี     อาการ    ท้องเสีย
nák.rian  kɯàːp         nɯ̀ŋrɔ́ːɪ      kʰon       miː   ʔaːkaːn  tʰɔ́ːŋ.siǎ
student   QUANT.APPROX  one hundred  CL.person  have  symptom  diarrhea
'Almost 100 students are having a case of diarrhea.'
```

This sentence is parsed as follows.

```
    +-----------------LWs-----------------+
    |         +<------------S<------------+
    |         +----->NUnr----->+          |
    |         |        +<-QFl<-+>CLn>+    +->O->+-->AJv->+
    |         |        |       |     |    |     |        |
LEFT-WALL นักเรียน.n เกือบ.qfl 100.nu คน.cl มี.v อาการ.n ท้องเสีย.va
```

Quantifier เกือบ 'almost' is connected to the number หนึ่งร้อย 'one hundred' via the `QF` link.

----------

## Coordinate Structure

### JN

This *undirected* link connects a noun-phrase conjunct to its coordinator. For example, พ่อและแม่ต่างปกป้องลูก 'Each father and mother is protecting the children':

```
พ่อ      และ  แม่      ต่าง       ปกป้อง    ลูก
pʰɔ̂ː    lǽ   mæ̂ː     tàːŋ      pòkpɔ̂ŋ   lûːk
father  and  mother  ADV.each  protect  child
'Each father and mother are protecting the children.'
```

This sentence is parsed as follows.

```
    +--------------LWs--------------+
    |             +<-------S<-------+
    |       +-JNl-+-JNr-+    +<-AXw<+-->O->+
    |       |     |     |    |      |      |
LEFT-WALL พ่อ.n และ.cn แม่.n ต่าง.x ปกป้อง.v ลูก.n
```

Both conjuncts พ่อ 'father' and แม่ 'mother' are connected to the conjunction และ 'and' via the `JN` link.

### JV

This *undirected* link connects a verb-phrase conjunct to its coordinator. For example, เขาจะทำงานหรือนอนพักล่ะ 'Is he going to work or take a rest?':

```
เขา   จะ   ทำงาน      หรือ  นอน    พัก    ล่ะ
kʰǎʊ  tɕà  tʰam.ŋaːn  rɯ̌ː  nɔːn   pʰák  lâ
he    FUT  work       or   sleep  rest  EMPH
'Is he going to work or take a rest?'
```

This sentence is parsed as follows.

```
    +---------LWs--------+
    |        +<----S<----+
    |        |     +<AXw<+--JVl--+-JVr-+--VZ-+-PT-+
    |        |     |     |       |     |     |    |
LEFT-WALL เขา.pr จะ.x ทำงาน.v หรือ.cv นอน.v พัก.v ล่ะ.pt
```

Via the `JV` link, the first verb ทำงาน 'work' is connected to the conjunction หรือ 'or', and the conjunction, to the second verb นอน 'sleep'.

### JP

This *undirected* link connects a preposition-phrase conjunct to its coordinator. For example, หนังสือบนโต๊ะและใต้เก้าอี้ 'the books on the table and under the chair':

```
หนังสือ   บน   โต๊ะ    และ  ใต้     เก้าอี้
nǎŋsɯ̌ː  bon  tó     lǽ   tâɪ    kâʊʔîː
book    on   table  and  under  chair
'the books on the table and under the chair'
```

This sentence is parsed as follows.

```
             +------->AJpr------->+
             |       +-----JPl----+
    +---LWn--+       +->PO>+      +--JPr-+->PO->+
    |        |       |     |      |      |      |
LEFT-WALL หนังสือ.n บน.pan โต๊ะ.n และ.cp ใต้.pan เก้าอี้.n
```

Both conjuncts บนโต๊ะ 'on the table' and ใต้เก้าอี้ 'under the chair' are connected to the conjunction และ 'and' via the `JP` link.

### JD

This *undirected* link connects a discontinuous conjunction to its core conjunction. For example, ฉันซื้อทั้งหนังสือและสมุด 'I bought both books and notebooks':

```
ฉัน     ซื้อ   ทั้ง    หนังสือ   และ  สมุด
tɕʰǎn  sɯ́ː  tʰáŋ  nǎŋsɯ̌ː  lǽ   sàmùt
I      buy  both  book    and  notebook
'I bought both books and notebooks.'
```

This sentence is parsed as follows.

```
                  +-------->O-------->+
    +-----LWs-----+    +------JD------+
    |       +<-S<-+    |      +--JNl--+-JNr-+
    |       |     |    |      |       |     |
LEFT-WALL ฉัน.pr ซื้อ.v ทั้ง.cd หนังสือ.n และ.cn สมุด.n
```

Discontinous conjunction ทั้ง 'both' is connected to the main conjunction และ 'and' via the `JD` link.

----------

## Punctuation

### PU

This *undirected* link connects a punctuation mark to its head. There are three types of `PU` links: `PUs` (white space), `PUp` (paiyal noi), and `PUy` (mai yamok).

#### PUs

`PUs` connects a white space to its head. For example, เราซื้อสมุด หนังสือ และดินสอ 'We bought notebooks, books, and pencils':

```
เรา  ซื้อ   สมุด       _  หนังสือ   _  และ  ดินสอ
raʊ  sɯ́ː  sàmùt        nǎŋsɯ̌ː     lǽ   dinsɔ̌ː
we   buy  notebook     book       and  pencil
'We bought notebooks, books, and pencils.'
```

This sentence is parsed as follows.

```
                              +--------JNr-------+
    +------LWs-----+---->O--->+     +-----JNl----+
    |        +<-S<-+    +-JNl-+     |      +-PUs-+--JNr-+
    |        |     |    |     |     |      |     |      |
LEFT-WALL เรา.pr ซื้อ.v สมุด.n _.cn หนังสือ.n _.pu และ.cn ดินสอ.n
```

The white space is connected to the conjunction และ 'and' via the `PUs` link.

#### PUp

`PUp` connects a paiyal noi 'ฯ' (ไปยาลน้อย) to its head. For example, กรุงเทพฯ เป็นเมืองหลวงของประเทศไทย 'Bangkok is the capital of Thailand':

```
กรุงเทพ     ฯ             _  เป็น  เมืองหลวง   ของ    ประเทศ    ไทย
kruŋ.tʰêp  (PAIYAL NOI)     pen  mɯaŋ.luǎŋ  kʰɔ̌ːŋ  pràtʰêːt  tʰaɪ
Bangkok    ABBRMARK         be   capital    of     country   Thailand
'Bangkok is the capital of Thailand.'
```

This sentence is parsed as follows.

```
    +------------LWs-----------+
    |         +<-------S<------+
    |         +--PUp-+-PUs+    +--->O-->+->AJpr->+-->PO-->+--NZ--+
    |         |      |    |    |        |        |        |      |
LEFT-WALL กรุงเทพ.n ฯ.pu _.pu เป็น.v เมืองหลวง.n ของ.pnn ประเทศ.n ไทย.n
```

The paiyal noi ฯ is connected to the noun กรุงเทพ 'Bangkok' via the `PUp` link.

#### PUy

`PUy` connects a mai yamok 'ๆ' (ไม้ยมก) to its head. For example, เด็กๆ ก็ยังไม่เข้าใจอยู่ดี 'The children still do not understand yet':

```
เด็ก    ๆ            _  ก็     ยัง   ไม่   เข้าใจ       อยู่ดี
dèk    (MAI YAMOK)     kɔ̂ː   jaŋ  mâɪ  kʰâʊ.tɕaɪ   jùːdiː
child  REDUP           also  yet  NEG  understand  still
'The children still do not understand yet.'
```

This sentence is parsed as follows.

```
    +------------------LWs------------------+
    |       +<--------------S<--------------+
    |       |              +<------AXw<-----+
    |       |              |    +<---AXw<---+
    |       +-PUy-+-PUs+   |    |    +<-AXg<+->AVw>+
    |       |     |    |   |    |    |      |      |
LEFT-WALL เด็ก.n ๆ.pu _.pu ก็.x ยัง.x ไม่.ng เข้าใจ.v อยู่ดี.r
```

The mai yamok ๆ is connected to the core noun เด็ก 'child' via the `PUy` link.
