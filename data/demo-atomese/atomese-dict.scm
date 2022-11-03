;
; Demo Atomese Dictionary.
;
; This file contains some example Atomese dictionary entries,
; written in the style of scheme s-expressions. This is NOT a great
; way to encode dictionaries, but it does give a general flavor
; of what Atomese looks like.
;
; It is intended that this file can be read in using the
; FileStorageNode provided by the AtomSpace.
;
; CAUTION: IF YOU EDIT THIS FILE, BE SURE TO `MAKE INSTALL` AFTERWARDS.
; That is because the `storage.dict` is configured to get this file
; from the install location, and not the source tree!
;
; ---------------------------------------------------------------------
; The four sections below allow one and only one phrase to be parsed:
; "level playing field". These sections are directly converted into
; Link Grammar disjuncts having the same connectors, except that the
; LG connectors are single links, that stand in for the connected words.

(Section
	(Word "###LEFT-WALL###")
	(ConnectorSeq
		(Connector (Word "level") (ConnectorDir "+"))))

(Section
	(Word "level")
	(ConnectorSeq
		(Connector (Word "###LEFT-WALL###") (ConnectorDir "-"))
		(Connector (Word "playing") (ConnectorDir "+"))))

(Section
	(Word "playing")
	(ConnectorSeq
		(Connector (Word "level") (ConnectorDir "-"))
		(Connector (Word "field") (ConnectorDir "+"))))

(Section
	(Word "field")
	(ConnectorSeq
		(Connector (Word "playing") (ConnectorDir "-"))))

; ---------------------------------------------------------------------
; The data below allows a collection of very similar sentences
; to be parsed. They all have the form "Mary saw a bird", with
; a choice of subjects, objects, verbs and determiners possible,
; as listed below.

(Section
	(Word "###LEFT-WALL###")
	(ConnectorSeq
		(Connector (WordClass "person") (ConnectorDir "+"))))

(Member (Word "Mary") (WordClass "person"))
(Member (Word "John") (WordClass "person"))
(Member (Word "Olga") (WordClass "person"))
(Member (Word "Sasha") (WordClass "person"))

(Section
	(WordClass "person")
	(ConnectorSeq
		(Connector (Word "###LEFT-WALL###") (ConnectorDir "-"))
		(Connector (WordClass "verb") (ConnectorDir "+"))))

(Member (Word "saw") (WordClass "verb"))
(Member (Word "heard") (WordClass "verb"))

(Section
	(WordClass "verb")
	(ConnectorSeq
		(Connector (WordClass "person") (ConnectorDir "-"))
		(Connector (WordClass "animal") (ConnectorDir "+"))))

(Member (Word "bird") (WordClass "animal"))
(Member (Word "cat") (WordClass "animal"))
(Member (Word "dog") (WordClass "animal"))

(Section
	(WordClass "animal")
	(ConnectorSeq
		(Connector (WordClass "verb") (ConnectorDir "-"))
		(Connector (WordClass "determiner") (ConnectorDir "-"))))

(Member (Word "the") (WordClass "determiner"))
(Member (Word "a") (WordClass "determiner"))
(Member (Word "this") (WordClass "determiner"))
(Member (Word "that") (WordClass "determiner"))

(Section
	(WordClass "determiner")
	(ConnectorSeq
		(Connector (WordClass "animal") (ConnectorDir "+"))))

; ---------------------------------------------------------------------
; The LG 'cost' can be taken from any FloatValue located on a Section.
; The location of that cost is configurable in the dict file; the demo
; uses `(Predicate "*-Mutual Info Key cover-section")` as the location
; key, and specifies the zero-based index into the vector as 1.  The
; number is taken to be the MI, so minux the cost.  The larger the MI,
; the lower the cost.
;
; Reminder: the costs below can be views by saying `!!saw` and `!!the`
; at the LG command-line prompt.

(cog-set-value!
	(Section
		(WordClass "determiner")
		(ConnectorSeq
			(Connector (WordClass "animal") (ConnectorDir "+"))))
	(Predicate "*-Mutual Info Key cover-section")
	(FloatValue 0 3.1))

(cog-set-value!
	(Section
		(WordClass "verb")
		(ConnectorSeq
			(Connector (WordClass "person") (ConnectorDir "-"))
			(Connector (WordClass "animal") (ConnectorDir "+"))))
	(Predicate "*-Mutual Info Key cover-section")
	(FloatValue 555 2.4))  ; The `555` is meaningles and ignored.

; ---------------------------------------------------------------------
;
; Crazy data used for unit testing, to veryify connector order.
; This should allow the sentence "1 2 3 fountain 4 5 6" to parse,
; with a fountain from the center to each of the numbers.
; The name comes from the following parse diagram:
;
;        +-------A-------+
;        |     +----D----+----I---+
;        |     | +---E---+---H--+ |
;        |     | | +--F--+--G-+ | |
;        |     | | |     |    | | |
;    LEFT-WALL 1 2 3 fountain 4 5 6
;
; The expected disjunct on the word 'fountain' is
;
;      F- & E- & D- & A- & G+ & H+ & I+
;

(Section
	(Word "###LEFT-WALL###")
	(ConnectorSeq
		(Connector (Word "fountain") (ConnectorDir "+"))))

(Section
	(Word "fountain")
	(ConnectorSeq
		(Connector (Word "###LEFT-WALL###") (ConnectorDir "-"))
		(Connector (Word "1") (ConnectorDir "-"))
		(Connector (Word "2") (ConnectorDir "-"))
		(Connector (Word "3") (ConnectorDir "-"))
		(Connector (Word "4") (ConnectorDir "+"))
		(Connector (Word "5") (ConnectorDir "+"))
		(Connector (Word "6") (ConnectorDir "+"))))

(Section (Word "1")
	(ConnectorSeq (Connector (Word "fountain") (ConnectorDir "+"))))

(Section (Word "2")
	(ConnectorSeq (Connector (Word "fountain") (ConnectorDir "+"))))

(Section (Word "3")
	(ConnectorSeq (Connector (Word "fountain") (ConnectorDir "+"))))

(Section (Word "4")
	(ConnectorSeq (Connector (Word "fountain") (ConnectorDir "-"))))

(Section (Word "5")
	(ConnectorSeq (Connector (Word "fountain") (ConnectorDir "-"))))

(Section (Word "6")
	(ConnectorSeq (Connector (Word "fountain") (ConnectorDir "-"))))

; ---------------------------------------------------------------------
; Disjuncts can also be constructed from individual word-pairs. Using
; these will results in a Maximum Spanning Tree (MST) or Maximum Planar
; Graph (MPG) parse. Such disjuncts are not as "rigid" as those specified
; by sections; instead they may have a variable number of connectors
; in arbitrary order, preserving only the word order in the parsed
; sentence.
;
; When used with existing disjuncts (controllable in the configuration
; file), the use of word-pairs can supplement a poor selection of
; disjuncts by adding additional (optional) links determined by the word
; pairs.
;
; An example vocabulary is given below.  This allows sentences such as
; "the fish jumped out of the water" to be parsed, as well as shorter
; sentences made up of these same words, such as "the fish jumped" and
; "jumped out", "out of water". There is no grammatical structure to what
; can be parsed in this way, other than that there must be some word-pair
; occurring in proper sentence order. Thus, for example "water jumped"
; will NOT parse, because there is no word pair with "water" coming before
; "jumped".
;
; The configuration file enables the automatic insertion of "ANY" links
; into disjuncts. Thus, sentences like "Olga saw the fish" will also parse,
; because "Olga" and "saw" are in the dictionary (above), and are decorated
; with "ANY" connector types. These connect to "ANY" connectors that are
; added to word-pair disjuncts.
;
(Evaluation (Predicate "wrdpr") (List (Word "###LEFT-WALL###") (Word "jumped")))
(Evaluation (Predicate "wrdpr") (List (Word "###LEFT-WALL###") (Word "fish")))
(Evaluation (Predicate "wrdpr") (List (Word "the") (Word "fish")))
(Evaluation (Predicate "wrdpr") (List (Word "fish") (Word "jumped")))
(Evaluation (Predicate "wrdpr") (List (Word "jumped") (Word "out")))
(Evaluation (Predicate "wrdpr") (List (Word "out") (Word "water")))
(Evaluation (Predicate "wrdpr") (List (Word "of") (Word "water")))
(Evaluation (Predicate "wrdpr") (List (Word "the") (Word "water")))
(Evaluation (Predicate "wrdpr") (List (Word "out") (Word "of")))

; Set costs on two of the pairs.
(cog-set-value!
	(Evaluation (Predicate "wrdpr") (List (Word "the") (Word "fish")))
	(Predicate "*-Mutual Info Key-*")
	(FloatValue 0 3.1))

(cog-set-value!
	(Evaluation (Predicate "wrdpr") (List (Word "jumped") (Word "out")))
	(Predicate "*-Mutual Info Key-*")
	(FloatValue 0 4.2))

;
; ---------------------------------------------------------------------
