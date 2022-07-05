;
; Demo Atomese Dictionary.
;
; This file contains some example Atomese dictionary entries,
; written in the style of scheme expressions. This is NOT a great
; way to encode dictionaries, but it does give a general flavor
; of what Atomese looks like.
;
; It is intended that this file can be read in using the
; FileStorageNode provided by the AtomSpace.

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
