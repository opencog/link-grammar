--
--
-- SQLite3 style SQL commands to create a table holding the various
-- dictionary bits and pieces.  Right now, this is just a very simple
-- table that holds all the disjuncts for a given word.
--

CREATE TABLE Morphemes
(
	-- For English, the 'morpheme' is the 'word'. A given morpheme
	-- may appear mutiple times in this table.
	morpheme TEXT,

	-- The subscripted form of the above.  The subscripted forms are
	-- always unique for the dictionary.
	subscript TEXT UNIQUE,

	-- The classname is the set that the subscripted 'word' belongs to.
	-- All members of the class share a common set of disjuncts, with
	-- a common set of costs.
	classname TEXT
);

CREATE TABLE Disjuncts
(
	-- All words/morphemes sharing this classname also share this
	-- disjunct and cost.
	classname TEXT,

	-- The standard Link Grammar disjunct, expressed as an ASCII string.
	-- The disjunct can be composed of the & operator, and the optional
	-- connectors i.e. {} and the multiple connector i.e. @. The and
	-- operator is NOT allowed. This means that the grouping parents () 
	-- must also not appear in the expression.  The cost operators [] are
	-- also not allowed; costs are to be expressed using the cost field.
	--
	-- An example of a valid disjunct:
	--     A+ & B- & {Ca*bc*f+} & @Mpqr-
	--
	-- An example of an INVALID disjunct:
	--     (A+ & B-) & {Ca*bc*f+ or [D-]} & @Mpqr-
	--
	disjunct TEXT,

	-- Cost of using this disjunct.
	cost REAL
);
