--
-- demo.sql
--
-- Inserts some word and disjunct data into the database.
-- This is for demonstration purposed only: it inserts just enough to
-- parse the sentences "this is a test" "this is a dog" "this is
-- a cat", "that is a test", "that is another dog", etc.  The disjuncts
-- are a small subset of the correct ones for English.
--
-- To create a new database, simply say:
--    cat dict.sql | sqlite3 dict.db
-- To populate it with the demo data:
--    cat demo.sql | sqlite3 dict.db
--

INSERT INTO Morphemes VALUES ('LEFT-WALL', 'LEFT-WALL', 'LEFT-WALL');
INSERT INTO Disjuncts VALUES ('LEFT-WALL', 'Wd+ & WV+', 0.0);

INSERT INTO Morphemes VALUES ('this', 'this.p', 'this');
INSERT INTO Disjuncts VALUES ('this', 'Wd- & Ss*b+', 0.1);

INSERT INTO Morphemes VALUES ('is', 'is.v', 'is');
INSERT INTO Disjuncts VALUES ('is', 'Ss- & WV- & O*m+', 0.214159265358979);

INSERT INTO Morphemes VALUES ('a', 'a', '(article)');
INSERT INTO Morphemes VALUES ('another', 'another', '(article)');
INSERT INTO Disjuncts VALUES ('(article)', 'Ds+', 0.1);

INSERT INTO Morphemes VALUES ('test', 'test.n', '(noun)');
INSERT INTO Morphemes VALUES ('dog', 'dog.n', '(noun)');
INSERT INTO Morphemes VALUES ('cat', 'cat.n', '(noun)');
INSERT INTO Disjuncts VALUES ('(noun)', 'Ds- & Os-', 0.0);
