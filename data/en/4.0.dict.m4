dnl
dnl Macro version of the 4.0.dict file. This is file is used to simplify
dnl the maintenance of the verb definitions; it defines a handful of
dnl macros to deal with the case of conjoined verbs. Pre-process this
dnl file with the m4 macro pre-processor to create 4.0.dict
dnl That is, at the command line, run:
dnl     m4 4.0.dict.m4 > 4.0.dict
dnl
dnl the comment delimiter for link-grammar data files is %
changecom(`%')
 %***************************************************************************%
 %                                                                           %
 %       Copyright (C) 1991-1998  Daniel Sleator and Davy Temperley          %
 %       Copyright (c) 2003  Peter Szolovits and MIT.                        %
 %       Copyright (c) 2008-2020  Linas Vepstas                              %
 %       Copyright (c) 2013  Lian Ruiting                                    %
 %                                                                           %
 %  See file "README" for information about commercial use of this system    %
 %                                                                           %
 %***************************************************************************%

#define dictionary-version-number 5.10.2;
#define dictionary-locale         en_US.UTF-8;

% The default largest disjunct cost to consider during parsing.
#define max-disjunct-cost         2.7;
#define panic-max-disjunct-cost   4.0;

 % _ORGANIZATION OF THE DICTIONARY_
 %
 % I. NOUNS
 % II. PRONOUNS
 % III. DETERMINERS
 % IV. NUMERICAL EXPRESSIONS
 % V. VERBS
 %    A. Auxiliaries; B. Common verb types; C. complex intransitive verbs;
 %    D. complex intransitive verbs; E. complex verbs taking [obj] +
 %    [complement]; F. idiomatic verbs
 % VI. PREPOSITIONS
 % VII. TIME AND PLACE EXPRESSIONS
 % VIII. QUESTION-WORDS AND CONJUNCTIONS
 % IX. ADJECTIVES
 % X. COMPARATIVES AND SUPERLATIVES
 % XI. ADVERBS
 %    A. Mainly adjectival; B. Mainly post-verbal; C. Post-verbal/pre-verbal;
 %    D. Post-verbal/pre-verbal/openers; E. Post-verbal/openers;
 %    F. Pre-verbal/openers
 % XII. MISCELLANEOUS WORDS AND PUNCTUATION
 %
 %
 % TODO:
 % To-do: many verb simple past-tense forms include ({@E-} & A+) to
 % make them adjective-like. Strictly speaking, these should probably
 % copied into words.adj.1 and treated like common adjectives, right?
 %
 % Many nouns in words.n.4 are treated as "mass or count". The side
 % effect is that mass nouns are inconsistently treated as sometimes
 % singular, sometimes plural. e.g. words.n.3 gets <noun-rel-s> &
 % <noun-main-m>. This is a kind-of ugly tangle, it should really
 % be sorted out so that links are properly marked as s, p or m.
 % This is mostly fixed, except that some uses of <noun-main-m>
 % remain, below.

% Capitalization handling (null effect for now- behave as empty words).
<1stCAP>: ZZZ-;
<nonCAP>: ZZZ-;

% Null links. These are used to drop the requirement for certain words
% to appear during parsing. Currently, only one null is defined: the
% no-det-null. It is used to make determiners optional; this allows for
% the parsing of newspaper headlines and clipped technical speech
% (e.g. medical, engineering, where determiners are often dropped).

% A definition using the "headline" dialect.
<no-det-null>: [()]headline;

% NOUNS

% The marker-entity is used to identify identity names.
% The marker-common-entity is used to identify all common nouns
% and adjectives that might appear in entity names:
% e.g. "Great Southern Federal Bank and Railroad" or "Aluminum Bahrain"
% These markers are used by the tokenizer, to help identify entities,
% which can appear capitalized at the start of sentences.  (Actually,
% at this time, only <marker-common-entity> is used. We keep around
% <marker-entity> just in case :-)
<marker-entity>: XXXENTITY+;
<marker-common-entity>: XXXGIVEN+;

% The RJ links connect to "and"; the l,r prevent cross-linking
<clause-conjoin>: dRJrc- or dRJlc+;

% {@COd-} : "That is the man who, in Joe's opinion, we should hire"
% {hCO-} & {[@hCO-]}: first one is free, additional ones cost.
%        "Despite newspaper reports to the contrary, Mary did stuff"
% {dHM-}: hesitation marker: "I told him that uh he's wrong"
<clause-head>:
  ({@hCOd- or dHM-} & (C- or <clause-conjoin>)) or
  ({hCO-} & {[@hCO-]} & Wd-);

<CLAUSE>:   {<clause-head> or [Rn-]};
<S-CLAUSE>: {<clause-head>};
<CLAUSE-E>: {<clause-head> or Re-};

% Post-nominal qualifiers, complete with commas, etc.
% We give these a small cost, so that they don't hide quotational
% complements (i.e. so that "blah blah blah, he said" doesn't
% get the MX link at lower cost than the CP link...)
<post-nominal-x>:
  [{[B*j+]} & Xd- & Xc+ & MX-]0.1;

<post-nominal-s>:
  [{[Bsj+]} & Xd- & Xc+ & MX-]0.1;

<post-nominal-p>:
  [{[Bpj+]} & Xd- & Xc+ & MX-]0.1;

<post-nominal-u>:
  [{[Buj+]} & Xd- & Xc+ & MX-]0.1;

% noun-main-x -- singular or plural or mass.
<noun-main-x>:
  (S+ & <CLAUSE>) or SI- or J- or O-
  or <post-nominal-x>;

% noun-main-s -- singular
% XXX FIXME: <noun-main-?> is often used with <noun-rel-?> and noun-rel
% has a R+ & B+ on it. The problem here is that R+ & B+ should not
% be used with the J- and sometimes not with the O-.  So, for example,
%   "*I still remember the room I kissed him"
% "room" is the object, and the "I" should not be relative. But then
%   "I still remember the girl I kissed"
% ... The correct fix would be to disallow (B- & O+) on "kissed".
% ... but how? It was introduced by "What are the chances..." sentences
%     which place a (B- & O+) into <vc-trans>. A complicated fix might be
%     to provide some subscript on R that propagates to and blocks B+ ...
% ... More generally, the valid constructions are those with a zero-that
%     in them: "I still remember the girl [that] I kissed" and the invalid
%     one's would not have a zero-that. Which requires fixing that.j-r...
%
% Even relative-with- can be OK:
%   "there are men with ties that sparkle" -- this has a J link.
% Is the man sparkling, or is the tie sparkling?
%
% (Js- & {Mf+}): Allows constructions involving "of" to link locally,
%     e.g. "Got it from the Abbey of Stratford Langthorne"
%     links "of" to "Abbey" instead of "it".
%
% (Js- & {Jk-}): Allows back-link to conjunction:
%     e.g. "... to look at and listen to everything"
%
% Ss*s+: blocks links to Ss*t- which accepts singular subject, but
%        requires plural object.  See "are.v" for examples.
%
<noun-main-s>:
  (Ss*s+ & <CLAUSE>) or SIs- or (Js- & ({Jk-} or {Mf+})) or Os-
  or <post-nominal-s>;

% noun-main-e: used for proper names (named entities)
% Os*e- is used to allow certain adjectival uses.
% Os*e- & Sj+: subject of bare infinitive. "You should hear John sing"
<noun-main-e>:
  (Ss*s+ & <CLAUSE>) or SIs- or (Js- & {Mf+}) or (Os*e- & {Sg+ or Sj+})
  or <post-nominal-s>;

% noun-main-p -- plural
<noun-main-p>:
  (Sp+ & <CLAUSE>) or SIp- or Jp-
  or Op-
  or <post-nominal-p>;

% noun-main-u -- u == uncountable
% TODO: alter this to use Su+, SIu- someday. likewise Buj+
% Doing this requires adding Su- links to many entries
<noun-main-u>:
  (Ss+ & <CLAUSE>) or SIs- or Ju- or Ou-
  or <post-nominal-s>;

% noun-main-m -- m == mass
% TODO: get rid of this someday.
% To get rid of this, any noun that uses this needs to be split into
% two: the countable form, which will used <noun-main-s> and the
% uncountable form, which will use <noun-main-u>
<noun-main-m>:
  (Ss+ & <CLAUSE>) or SIs- or Jp- or Os-
  or <post-nominal-s>;

% Used only for this.p, that.j-p
% (Jd- & Dmu- & Os-): they have plenty of this
% (Jd- & Dmu- & {Wd-} & Ss+): "not enough of this was used"
% Js-: "how did you learn of this?"
% ALx- & Os-: "I want just this"
<noun-main-h>:
  (Jd- & Dmu- & Os-)
  or (Jd- & Dmu- & {Wd-} & Ss*b+)
  or (Ss*b+ & <CLAUSE>)
  or SIs*b-
  or Js-
  or [Os-]0.5
  or (ALx- & Os-)
  or <post-nominal-x>;

<noun-main2-x>:
  J- or O-
  or <post-nominal-x>;

<noun-main2-s>:
  Js- or Os-
  or <post-nominal-s>;

% Xd- or [[()]] allows parsing of "I have no idea what that is."
% without requiring comma after "idea"
<noun-main2-s-no-punc>:
  Js- or Os-
  or ({[Bsj+]} & (Xd- or [[()]]) & Xc+ & MX-);

<noun-main2-p>:
  Jp- or Op-
  or <post-nominal-p>;

<noun-main2-m>:
  Jp- or Os-
  or <post-nominal-s>;

% @M+: "The disability of John means he is slow"
<noun-rel-x>: {@M+} & {R+ & B+ & {[[@M+]]}} & {@MX+};
<noun-rel-s>: {@M+} & {R+ & Bs+ & {[[@M+]]}} & {@MXs+};
<noun-rel-p>: {@M+} & {R+ & Bp+ & {[[@M+]]}} & {@MXp+};

% [@AN-].1: add a tiny cost so that A- is preferred to AN- when there
% is a choice. The is because some nouns are also listed as adjectives,
% and we want to use the adjective version A- link in such cases.
% [@AN- & @A-] has cost so that G links are preferred.
% {[@AN-].1} & {@A- & {[[@AN-]]}};
<noun-modifiers>:
  (@A- & {[[@AN-]]})
  or [@AN-]0.1
  or ([[@AN-].1 & @A-] & {[[@AN-]]})
  or ();

<nn-modifiers>:
  (@A- & {[[@AN-]]})
  or [@AN-]0.1
  or ([[@AN-].1 & @A-] & {[[@AN-]]});


% conjoined nouns or noun-phrases.
% The l and r prevent two nouns from hooking up directly, they
% must hook up to a conjunction (and, or) in the middle.
% SJl == connect to left
% SJr == connect to right
% SJ*s == singular
% SJ*p == plural
% SJ*u == mass
%
% M+: "gloom of night and heat will not stop me"
% The "of night" can connect to the left noun, but rarely to the right noun
% because it should then connect to the "and", not the right noun.
% but then: "neither heat nor gloom of night shall stop me"
% Looks like only a proper semantic decision can determine the correct
% parse here ...
%
% Add cost to M+, so that "a number of recommendations and suggestions"
% "of" gets priority in modifying the and.j-n instead of "recommendations".
% However, this cost then causes the following to parse incorrectly:
% "...went to hell yesterday and heaven on Tuesday."
% Arghh... only semantic disambiguation will work here.
<noun-and-s>: ({@M+} & dSJls+) or ({[@M+]} & dSJrs-);
<noun-and-p>: ({[@M+]0.4 or Mp+} & dSJlp+) or ({[@M+]1.4 or [Mp+]} & dSJrp-);
<noun-and-u>: ({[@M+]0.4 or Mp+} & dSJlu+) or ({[@M+]1.4 or [Mp+]} & dSJru-);
<noun-and-x>: ({[@M+]0.4 or Mp+} & dSJl+) or ({[@M+]1.4 or [Mp+]} & dSJr-);
<noun-and-p,u>:
  ({[@M+]0.4 or Mp+} & dSJlp+) or ({[@M+]1.4 or [Mp+]} & dSJrp-) or
  ({[@M+]0.4 or Mp+} & dSJlu+) or ({[@M+]1.4 or [Mp+]} & dSJru-);

% It might be a good idea to change these to be {Rw+ or Qe+} since
% there are many how-questions that are relativized in this same way.
%
% Btm is used for time expressions.  It is used to constrain the verb
% "last.v" which only makes sense for time.
<rel-clause-x>: {Rw+} & B*m+;
<rel-clause-s>: {Rw+} & Bsm+;
<rel-clause-p>: {Rw+} & Bpm+;
<rel-clause-t>: {Rw+} & Btm+;

% TOf+ & IV+:  "there is going to be a meeting", "there appears to be a bug"
% TOn+ & IV+:  "there are plots to hatch", "there is a bill to sign"
% TOt+ & B+: this is one where B makes the link
% TO+ & Xc+: allows null-infinitive: "Because I want to."
<inf-verb>: IV+;
<to-verb>:  TO+ & IV+;
<tof-verb>: TOf+ & IV+;
<toi-verb>: TOi+ & IV+;
<ton-verb>: TOn+ & IV+;
<too-verb>: TOo+ & IV+;
<tot-verb>: TOt+ & B+;
<null-verb>: TO+ & Xc+;

% hHM+: hesitation marker: "I told him that uh he's wrong"
<embed-verb>: {hHM+} & Ce+ & CV+;
<that-verb>: {hHM+} & Cet+ & CV+;
<subcl-verb>: {hHM+} & Cs+ & CV+;
<advcl-verb>: {hHM+} & Ca+ & CV+;
<fitcl-verb>: {hHM+} & Ci+ & CV+;
<porcl-verb>: {hHM+} & Cr+ & CV+;
<thncl-verb>: {hHM+} & Cc+ & CV+;
% We don't handle Ct,Cta in the above, because the AF and B link plays
% the role of CV, connecting to the head-verb.

% Fronted prepositional and participle phrases, used with
% subject-object inversion.  Wp connect to preps only.
% Why is there a cost? How big should the cost be?
% PFd+: prevent links to PFt-
<fronted>: [dWp- & (dPFb+ or dPFd+)]0.1;

% The use of COa here needs to be carefully re-examined;
%   it is used much too freely.
% COa+ is used to block links to COd-
% Wc- & Xc+ & Qd+: subject-object inversion: "anyhow, am I right?"
%       This gets a fairly stiff cost if the comma is missing.
% Wc- & Xc+ & Wq+: "O Mary, what have I done?"
% Wc- & MG+: "O Yahweh of Armies, how long will you not have mercy?"
<directive-opener>:
  {[[Wa-]]} &
    (({OH-} & Wc- & {MG+} & (Xc+ or [()]1.2) & (Qd+ or Wq+)) or
    ({Xd-} & {OH-} & (Xc+ or [[()]]) & [dCOa+]));

% Xc+ & EI+: connect to imperatives (infinitive verbs): "Anyhow, don't"
<directive-adverb>: (Xc+ & EI+);

% Xc+ & S**i+: connect to imperatives (infinitive verbs): "John, don't"
% OH-: "Oh Sue, let's go!"
<directive-subject>: ({OH-} & Xc+ & S**i+);

% Just pure singular entities, no mass nouns
% The CAPITALIZED-WORDS rule is triggered by regex matching, and
% applies to all capitalized words that are not otherwise found in
% the dictionary.
% ({[[@MX+]]} & AN+) comes from postposed modifiers:
% "Codon 311 (Cys --> Ser) polymorphism"
%
% We do NOT tag these with <marker-entity>, as this messes up first-word
% processing in tokenize.c.  So for example, we do *not* want "There"
% in "There they are" tagged as an entity, just because it's capitalized.
% We really do want to force the lower-case usage, because the lower case
% is in the dict, and it's the right word to use. (The only entities that
% should be tagged as such are those that are in the dicts, in their
% capitalized form, e.g. "Sue.f" female given name as opposed to "sue.v"
% verb in the sentence "Sue went to the store.")
%
% To help discourage capitalized use when the lower-case is in the dict,
% we give a slight cost to [<noun-rel-s> & (JG- or <noun-main-s>)] to
% discourage use as a common noun, so that the lower-case version can
% play this role.  Likewise th cost on [AN+].
%
% The cost on AN+ also discourages crazy AN links to noun cognates of verbs:
% e.g.  "The Western Railroad runs through town" -- don't want AN to runs.n.
%
% MX+ & <noun-main-s>: country names: "...went to Paris, France"
%
<INITIALS> <ALL-UPPER> <entity-singular>:
  ({NM+} & ({G-} & {[MG+]} &
    (({DG- or [GN-]0.5 or [@A- & @AN-]0.5 or [{@A-} & {D-}] or ({@A-} & Jd- & Dmc-)} &
        ((<noun-rel-s> & (JG- or <noun-main-s>))
        or <noun-and-s>
        or YS+
        ))
      or ({[[@MX+]]} & [AN+]) or G+)))
  or (MXs+ & (<noun-main-s> or <noun-and-s>))
  or ({@A- or G-} & {D-} & Wa- & {NM+})
  or [<directive-opener> or <directive-subject>]0.2;

% As above, but with a tiny extra cost, so that a dictionary word is
% preferred to the regex match (i.e. for a common noun starting a
% sentence). However, the other regex matches (e.g. MC-NOUN-WORDS)
% should have a cost that is even higher (so that we take the
% capitalized version before we take any other matches.)
<CAPITALIZED-WORDS>: [<entity-singular>]0.06;

% Capitalized words that seem to be plural (by ending with an s, etc)
% -- But not all words that end with an 's' are plural:
% e.g. Cornwallis ... and some of these can take a singular determiner:
% "a Starbucks"
<PL-CAPITALIZED-WORDS>:
  ({NM+} & {G-} & {[MG+]} &
    (({DG- or [[GN-]] or [[{@A-} & ({Dmc-} or {Ds-})]] or ({@A-} & Jd- & Dmc-) } &
        ([<noun-rel-x> & (JG- or <noun-main-x>)]
        or <noun-and-x>
        or YS+
        or YP+
        ))
      or AN+
      or G+))
  or ({@A- or G-} & {D-} & Wa- & {NM+})
  or <directive-opener>
  or <directive-subject>;

% capitalized words ending in s
% -- hmm .. proper names not used anywhere right now, has slot for plural ... !!??
<proper-names>:
  ({G-} & {[MG+]} & (({DG- or [[GN-]] or [[{@A-} & {D-}]]} &
    (({@MX+} & (JG- or <noun-main-s>)) or YS+ or YP+)) or AN+ or G+));

% "Tom" is a given name, but can also be a proper name, so e.g.
% "The late Mr. Tom will be missed." which needs A-, D- links
%
% <noun-and-s> is tricky when used with [[...]] connectors.
% Careful for bad parses of
% "This is the dog and cat Pat and I chased and ate"
% "actress Whoopi Goldberg and singer Michael Jackson attended the ceremony"
%
% Some given names cause problems, though. See tom.n-u, bob.v, frank.a
% frank.v, Frank.b, An.f In.f So.f below.
%
% Wa-: A single exclamation: "Tom!  Hey, Tom! Oh, hello John!"
% A- & OH- & Wa-: "O lovely Rose Marie!"
% OH- & SIs-: "Tell me, O Muse, of that ingenious hero"
<given-names>:
  {G-} & {[MG+]} &
    (({DG- or [GN-]2.1 or [[{@A-} & {D-}]]} &
      (({@MX+} & {NMr+} & (JG- or <noun-main-e> or <noun-and-s>))
        or YS+
        or YP+))
    or AN+
    or ({@A-} & {OH-} & Wa-)
    or (OH- & SIs-)
    or G+);

% Whole, entire entities, cannot participate in G links
% because the entire entity has already been identified.
<entity-entire>:
  ({DG- or [[GN-]] or [[{@A-} & {D-}]]} &
    (({@MX+} & <noun-main-s>) or <noun-and-s> or YS+))
  or Wa-
  or AN+;

% Words that are also given names
% Cannot take A or D links.
% Art Bell Bill Bob Buck Bud
%
% The bisex dict includes names that can be given to both
% men and women.
/en/words/entities.given-bisex.sing
/en/words/entities.given-female.sing
/en/words/entities.given-male.sing
/en/words/entities.goddesses
/en/words/entities.gods:
  <marker-entity> or <given-names> or <directive-opener> or <directive-subject>;

% Given names An In So interfere with misc words -- give them a cost.
An.f In.f So.f: [[<given-names>]];

% Special handling for certain given names. These are words that have a
% lower-case analog in the dictionary, and are also used in upper-case
% form in an "idiomatic name" e.g. Vatican_City.  Without the below,
% this use of "City" would prevent it from being recognized in other
% (non-idiomatic) proper name constructions, e.g. New York City.
/en/words/entities.organizations.sing:
  <marker-entity> or <entity-singular>;

/en/words/entities.locations.sing:
  <marker-entity> or <entity-singular>;

% The apostrophe triggers incorrect guessing, so we explicitly list this here.
Hallowe'en:
  <marker-entity> or <entity-singular>;

% Jd- & Dmu- & Wa-: "A lot of money!"
% Jd- & Dmu- & Wa- & Mp+: "A lot of money for so little!"
<noun-assert>:
    ({Dm-} & Wa-)
    or (Jd- & Dmu- & Wa- & {Mp+});

% words.n.4: nouns that can be mass or countable
% allocation.n allotment.n alloy.n allure.n alteration.n alternation.n
% piano.n flute.n belong here, because of "He plays piano"
%
% This class has now been eliminated: nouns are either singular, plural
% or mass. If they can be more than one, then they are listed separately
% in each class e.g. words.n.1 and/or words.n.2 and/or words.n.3, etc.
% (Only a few screwball exceptions below; should be fixed ...)
<noun-mass-count>:
  <noun-modifiers> &
    (({NM+} & AN+)
    or ({NM+ or ({Jd-} & D*u-)} & <noun-rel-s> & (<noun-main-m> or <rel-clause-s>))
    or ({NM+ or ({Jd-} & D*u-)} & <noun-and-p,u>)
    or (YS+ & {D*u-})
    or (GN+ & (DD- or [()]))
    or Us-
	 or <noun-assert>);

<GREEK-LETTER-AND-NUMBER> pH.i x.n: <noun-mass-count>;

% Same as pattern used in words.n.4 -- mass nouns or countable nouns
<generic-singular-id>: <noun-mass-count>;

% Pattern used for words.n.2.s
% Similar to <common-noun>, but with different determiners for number
% agreement.
% ({{Dmc-} & Jd-} & Dmc-) : "I gave him a number of the cookies"
% want "Dmc-" on both to avoid "this cookies"

<generic-plural-id>:
  <noun-modifiers> &
    ([[AN+]]
    or ({NM+ or ({{Dmc-} & Jd-} & Dmc-)} &
      <noun-rel-p> & (<noun-main-p> or <rel-clause-p>))
    or ND-
    or ({NM+ or Dmc-} & <noun-and-p>)
    or dSJrp-
    or (YP+ & {Dmc-})
    or (GN+ & (DD- or [()]))
    or Up-
    or <noun-assert>);

% Number abbreviations: no.x No.x
% pp. paragraph, page -- art: article
% RR: rural route
% NM+ & AN+: "Please use a No. 2 pencil"
No.x no.x Nos.x nos.x
Nr.x Nrs.x nr.x nrs.x
Num.x num.x pp.x
Art.x art.x RR.x rr.x :
  (Xi+ or [[()]]) & {NM+} & AN+;

% Explicitly include the period at the end of the abbreviation.
Adj..x Adm..x Adv..x Asst..x Atty..x Bart..x Bldg..x Brig..x Bros..x Capt..x Cie..x
Cmdr..x Col..x Comdr..x Con..x Corp..x Cpl..x DR..x Dr..x Drs..x Ens..x Ft..x
Gen..x Gov..x Hon..x Hr..x Hosp..x HMS..x Insp..x Lieut..x Lt..x MM..x MR..x MRS..x
MS..x Maj..x Messrs..x Mlle..x Mme..x Mr..x Mrs..x Ms..x Msgr..x Mt..x Op..x
Ord..x Pfc..x Ph..x Prof..x Pvt..x Rep..x Reps..x Res..x Rev..x Rt..x
Sen..x Sens..x Sfc..x Sgt..x Sr..x St..x Supt..x Surg..x:
  G+;

% Period is missing in the abbreviation! Accept, but with a cost.
Adj.x Adm.x Adv.x Asst.x Atty.x Bart.x Bldg.x Brig.x Bros.x Capt.x Cie.x
Cmdr.x Col.x Comdr.x Con.x Corp.x Cpl.x DR.x Dr.x Drs.x Ens.x Ft.x
Gen.x Gov.x Hon.x Hr.x Hosp.x HMS.x Insp.x Lieut.x Lt.x MM.x MR.x MRS.x
MS.x Maj.x Messrs.x Mlle.x Mme.x Mr.x Mrs.x Ms.x Msgr.x Mt.x Op.x
Ord.x Pfc.x Ph.x Prof.x Pvt.x Rep.x Reps.x Res.x Rev.x Rt.x
Sen.x Sens.x Sfc.x Sgt.x Sr.x St.x Supt.x Surg.x:
  [G+];

% Street addresses, company abbreviations
% Cost on Xi+: don't link to period, if we've already got one!
St.y St..y Ave.y Ave..y Av.y Av..y Pl.y Pl..y Ct.y Ct..y Dr.y Dr..y
Gr.y Gr..y Ln.y Ln..y Rd.y Rd..y Rt.y Rt..y
Blvd.y Blvd..y Pkwy.y Pkwy..y Hwy.y Hwy..y
AG.y Assn.y Assn..y
Corp.y Corp..y Co.y Co..y Inc.y Inc..y PLC.y
Pty.y Pty..y Ltd.y Ltd..y LTD.y Bldg.y Bldg..y and_Co GmBH.y
et_al et_al. :
  ({[X-]} & G-) & {[[Xi+]]} & {[MG+]} &
    (({DG- or [[GN-]] or [[{@A-} & {D-}]]} &
      (({@MX+} & (JG- or <noun-main-s>)) or
      <noun-and-s> or
      YS+ or
      YP+)) or
    AN+ or
    G+);


% Titles, e.g. Joe Blow, Esq. or Dr. Smarty Pants, Ph.D.
% Gack. See absurdly large collection at:
% http://en.wikipedia.org/wiki/List_of_post-nominal_letters
% The generic case of strings of all-uppercase are handled
% by the regexes ALL-UPPER and INITIALS. The below are mixed-case
% or other very common degrees.
<ALL-UPPER>.y <INITIALS>.y
Jr.y Jr..y Sr.y Sr..y Esq.y Esq..y
AB.y A.B..y AIA.y A.I.A..y
BA.y B.A..y BFA.y B.F.A..y BS.y B.S..y BSc.y B.Sc..y
CEng.y CEng..y CFA.y CPA.y CPL.y CSV.y
DD.y D.D..y DDS.y D.D.S..y DO.y D.O..y D.Phil..y D.P.T..y
Eng.D..y
JD.y J.D..y  KBE.y K.B.E..y LLD.y LL.D..y
MA.y M.A..y MBA.y M.B.A..y MD.y M.D.y MFA.y M.F.A..y
MS.y M.S..y MSc.y M.Sc..y
OFM.y
PE.y P.E..y Pfc.y Pharm.D..y
PhD.y Ph.D.y Ph.D..y
RA.y R.A..y RIBA.y R.I.B.A..y RN.y R.N..y Sgt.y
USMC.y USN.y:
  {Xi+} & {Xd- & {Xc+}} & G- & {[MG+]} &
    (({DG- or [[GN-]] or [[{@A-} & {D-}]]} &
      (({@MX+} & (JG- or <noun-main-s>)) or
      <noun-and-s> or
      YS+ or
      YP+)) or
    AN+ or
    G+);

% The generic category for strings containing a hyphen
<PART-NUMBER>.n
<ALL-UPPER>.n
<HYPHENATED-WORDS>.n:
  [[({@AN-} & {@A-} &
    (({NM+ or D-} &
      ((<noun-rel-x> & (<noun-main-x> or <rel-clause-x>))
      or <noun-and-x>))
     or U-
     or ({D-} & Wa-)))
  or ((YS+ or YP+) & {@AN-} & {@A-} & {D-})]];

% NOUNS --------------------------------------------------------
% Nouns typically take determiners (a, the). The minor flags are:
% D link: determiners: D1234
% position 1 can be s, m for singular, mass
% position 2 can be c for count, u for uncountable
% position 3 can be k,m,y for comparatives, w for questions.
% position 4 can be c for consonant, v for vowel, x for long-distance.

% words.n.1-vowel words.n.1-const: Common nouns
% activist.n actor.n actress.n actuary.n ad.n adage.n adagio.n adapter.n
% The naked SJr- allows article to be skipped in conjunction (and,or)
% constructions ("the hammer and sickle")
%
% ({NMa+} & AN+): He takes vitamin D supplements.
%         ..... but most common nouns can't do this, so this is weird.
%
% XXX TODO FIXME: there are many gerund-like nouns in here (e.g. "reading")
% which screw things up when linking to "be" (e.g. "I have to be reading now")
% by appearing as objects (O-) connector when really the verb form (Pg-)
% is what should be happening. So rip these words out... (similar remarks for
% words.n.3)
% [Wa-]0.02: give a very mild preference to words that could be verbs
%            (and thus imperatives) e.g. Smile!
% Ds- & Wa- & Mf+: "A single man of large fortune"
<common-noun>:
  <noun-modifiers> &
    (({NMa+} & AN+)
    or ((NM+ or ({[NM+]1.5} & (Ds- or <no-det-null>)))
      & ((<noun-rel-s> & (<noun-main-s> or <rel-clause-s>))
        or <noun-and-s>))
    or <noun-and-s>
    or (YS+ & Ds-)
    or (GN+ & (DD- or [()]))
    or Us-
    or ({Ds-} & [Wa-]0.02 & {Mf+}));

% Preliminary experimental split for supporting a/an phonetic change
% for common nouns starting with vowels or consonant's.
% XXX not yet fully tested; seems over-complicated.
<common-phonetic>:
  (<noun-modifiers> &
    (<noun-and-s>
    or (GN+ & (DD- or [()]))
    or Us-
    or ({Ds-} & [Wa-]0.05 & ({Mf+} or {NM+}))))
  or (<nn-modifiers> &
    (({NMa+} & AN+)
    or ((NM+ or ({[NM+]1.5} & (Ds**x- or <no-det-null>)))
      & ((<noun-rel-s> & (<noun-main-s> or <rel-clause-s>))
        or <noun-and-s>))
    or (YS+ & Ds**x-)
    ));

<common-vowel-noun>:
  <common-phonetic>
  or (({NMa+} & AN+)
    or ((NM+ or ({[NM+]1.5} & (Ds**v- or <no-det-null>)))
      & ((<noun-rel-s> & (<noun-main-s> or <rel-clause-s>))
        or <noun-and-s>))
    or (YS+ & Ds**v-));

<common-const-noun>:
  <common-phonetic>
  or (({NMa+} & AN+)
    or ((NM+ or ({[NM+]1.5} & (Ds**c- or <no-det-null>)))
      & ((<noun-rel-s> & (<noun-main-s> or <rel-clause-s>))
        or <noun-and-s>))
    or (YS+ & Ds**c-));

/en/words/words.n.1-vowel
/en/words/entities.prof.vowel :
  <marker-common-entity> or <common-vowel-noun>;
/en/words/words.n.1-const
/en/words/entities.prof :
  <marker-common-entity> or <common-const-noun>;

% XXX FIXME this is nearly identical to words.n.t !?
/en/words/entities.people.vowel :
  <marker-common-entity> or <common-vowel-noun> or <directive-subject>;

/en/words/entities.people :
  <marker-common-entity> or <common-const-noun> or <directive-subject>;

% XXX eliminate this, it serves no purpose, sort into vowels and consts.
/en/words/words.n.1.gerund :
  <marker-common-entity> or <common-noun>;

% Common plural nouns ending in "s"
% allocations.n allotments.n allowances.n alloys.n allures.n allusions.n
/en/words/words.n.2.s :
  <marker-common-entity> or <generic-plural-id>;

<PL-GREEK-LETTER-AND-NUMBER>: <generic-plural-id>;

% plural nouns not ending in "s"
% almost exactly identical to <generic-plural-id> except that there is
% a YS+ instead of a YP+, uses a <noun-and-s> instead of <noun-and-p>
%
% {Jd-}: allows a "a flock of birds" to act as determiner.
%
% aircraft.p bacteria.p bellmen.n buffalo.p businessmen.n chairmen.n
/en/words/words.n.2.x :
  <marker-common-entity> or
  (<noun-modifiers> &
    ([[AN+]]
    or (GN+ & (DD- or [()]))
    or Up-
    or ({Dmc-} & Wa-)
    or ({NM+ or ({Jd-} & Dmc-)} &
      ((<noun-rel-p> & (<noun-main-p> or <rel-clause-p>)) or <noun-and-s>))
    or (YS+ & {Dmc-})
    ));

% XXX should probably eliminate <noun-and-p,u> and replace by <noun-and-u>
% but this requires other spread-out changes
%
%  ({{Dmu-} & Jd-} & Dmu-):  "Drink a pint of this beer"
% XXX: perhaps the above belongs on <noun-main-u> ??? If so,
% then we should also fix up similar connectors on "these", "those", "it",
% "them" etc; see below, search for Jd-
<mass-noun>:
  <noun-modifiers> &
    (AN+
    or (GN+ & (DD- or [()]))
    or Up-
    or ({Dmu-} & Wa-)
    or ({NM+ or ({{Dmu-} & Jd-} & Dmu-)}
      & ((<noun-rel-s> & (<noun-main-u> or <rel-clause-s>)) or <noun-and-p,u>))
    or (YS+ & {Dmu-})
    );

% XXX FIXME: this has only partial phonetic support.  I guess the Dm+ need to
% be fixed up as well.
% [Wa-]0.02: give a very mild preference to words that could be verbs
%            (and thus imperatives) e.g. Worry!
<mass-phonetic>:
  <noun-modifiers> &
    ((GN+ & (DD- or [()]))
    or Up-
    or [<noun-assert>]0.02
    or ({NM+ or ({{Dmu-} & Jd-} & Dmu-)}
      & ((<noun-rel-s> & (<noun-main-u> or <rel-clause-s>)) or <noun-and-p,u>))
    or (YS+ & {Dmu-})
    );

% If PH is possible, then it is preferred. See PH below for explanation.
<wantPHc>: [PHc-]-0.1 or ();
<wantPHv>: [PHv-]-0.1 or ();
<wantPH>: {PH-};

<mass-vowel-noun>:
  <mass-phonetic>
  or (AN+ & <wantPHv>)
  or (<nn-modifiers> & AN+);

<mass-const-noun>:
  <mass-phonetic>
  or (AN+ & <wantPHc>)
  or (<nn-modifiers> & AN+);

% nouns that are mass only
% absolutism.n absorption.n abstinence.n abundance.n academia.n
/en/words/words.n.3-vowel /en/words/words.lang.3-vowel:
  <marker-common-entity> or <mass-vowel-noun>;
/en/words/words.n.3-const /en/words/words.lang.3-const:
  <marker-common-entity> or <mass-const-noun>;

% Argh. Special case: allow a post-posed Latin modifier.
% "He has diabetes mellitus".
% "Sarah's office was ransacked by persons unknown."
diabetes.n persons.n-u:
	<mass-const-noun> & Pa+;

% Gonna treat these as mass nouns, not sure if this is correct.
% "She wished me goodnight" "She wishes you well"
adieu.n-u bye.n-u farewell.n-u fare-thee-well good-bye.n-u goodbye.n-u
good-night.n-u goodnight.n-u welcome.n-u well.n-u:
  <mass-noun>;

% Disambiguation: Want to cost this so that it doesn't interfere with
% given name "Tom".
tom.n-u: [<marker-common-entity> or <mass-noun>];

% Nouns that are also adjectives (e.g. red.a) and so we don't want to
% allow these to take AN+ links (we want to have red.a get used instead).
% But we do need these as nouns, so as to parse 'she prefers red'.
% However, assign a cost, so that 'her shoes are red' gets red.a (with
% the Pa link) preferred over red.n (with the O link).
%
% Doesn't seem to need a noun-and-x to make this work ...
% In other respects, these are kind-of-like mass nouns...
auburn.n black.n blue.n brown.n green.n gray.n ochre.n
pink.n purple.n red.n
tawny.n ultramarine.n umber.n yellow.n:
  <marker-common-entity>
  or (<noun-modifiers> & (
    (GN+ & (DD- or [()]))
    or Up-
    or ({Dmu-} & Wa-)
    or ({Dmu-} & <noun-rel-s> & ([<noun-main-m> or <rel-clause-s>]))
    or (YS+ & {Dmu-})
    ));

% US state names and abbreviations
% NM N.M. NY N.Y. NC N.C. ND N.D. Ohio Okla.
/en/words/entities.us-states.sing:
  <marker-entity>
  or ({G-} & {DG- or [[GN-]] or [[{@A-} & {D-}]]} &
    (({MG+} & {@MX+} & (JG- or <noun-main-s> or <noun-and-s>))
    or G+
    or ({[[MG+]]} & (AN+ or YS+ or YP+))))
  or (Xc+ & Xd- & G- & AN+)
  or Wa-;

% SINGULAR ENTITIES FOR ENTITY EXTRACTION
% This must appear after other categories so it doesn't interfere with those.
/en/words/entities.national.sing:
<marker-entity> or <entity-singular>;

% Enable parsing of "Mother likes her"
% Informal only, see formal version below.
auntie.f dad.m daddy.m granny.f
granddad.m grandpa.f grandpop.m mom.f mommy.f
pop.m papa.m poppy.m pops.m sis.f:
  <entity-singular>;

% XXX FIXME: unfortunately, this doubles the number of parses for many
% things, e.g. colliding with mother.n-f
% MX-: Shem, brother of Jopheth, left the village.
aunt.f brother.m father.m grandmother.f grandfather.m mother.f
sister.f uncle.m child.s son.m daughter.f grandson.m granddaughter.f
granduncle.m grandaunt.f:
  <entity-singular>
  or (OF+ & {@MV+} & Xd- & Xc+ & MX*a-);

alter_ego au_pair mise_en_scene faux_pas non_sequitur fait_accompli
modus_vivendi head_of_state tour_de_force:
  (<noun-modifiers> &
    ((Ds- & <noun-rel-s> & (<noun-main-s> or <rel-clause-s>)) or
    ({Ds-} & <noun-and-s>) or
    Us- or
    (YS+ & Ds-) or
    (GN+ & (DD- or [()])))) or
  AN+;

kung_fu joie_de_vivre op_art noblesse_oblige force_majeure
lese_majesty lese_majeste lèse_majesty lèse_majesté lèse-majesté leze_majesty
a_must time_of_day time_of_year top_dollar year_end
breach_of_contract sleight_of_hand power_of_attorney word_of_mouth
reason_of_selection carte_blanche:
  (<noun-modifiers> &
    (({Dmu-} & <noun-rel-s> & (<noun-main-m> or <rel-clause-s>)) or
    ({Dmu-} & <noun-and-u>) or
    Um- or
    Wa- or
    (YS+ & {Dmu-}) or
    (GN+ & (DD- or [()])))) or
  AN+;

% XXX FIXME plurals:
% lese_majesties or lèse_majestés

% title nouns (president, chairman)
% auditor.t bailiff.t broker.t buyer.t candidate.t captain.t cardinal.t
% Ou-: "He was made knight by the crown"
% XXX this is nearly identical to entities.people !?
/en/words/words.n.t:
  <noun-modifiers> & {@M+}
    & (BIt- or (Xd- & Xc+  & MX-) or Ou- or TI-);

% Almost identical to below.
% Ds- & {NM+} & <noun-rel-x> &..: "the number 12 is a lucky number"
%    above has cost to give "a number of" priority.
number.n:
  (<nn-modifiers> & (
    [Ds**x- & {NM+} & <noun-rel-x> & (<noun-main-x> or B*x+)]
    or ({Ds**x-} & {NM+} & <noun-and-x>)
  ))
  or (
    [Ds**c- & {NM+} & <noun-rel-x> & (<noun-main-x> or B*x+)]
    or ({Ds**c-} & {NM+} & <noun-and-x>)
  )
  or AN+;

% Almost identical to above.
% Differing in strange ways from <common-noun>
majority.n minority.n bunch.n batch.n bulk.n handful.n group.n:
  (<nn-modifiers> & (
    [Ds**x- & <noun-rel-x> & (<noun-main-x> or B*x+)]
    or ({Ds**x-} & <noun-and-x>)
  ))
  or (
    [Ds**c- & <noun-rel-x> & (<noun-main-x> or B*x+)]
    or ({Ds**c-} & <noun-and-x>)
  )
  or AN+;

% Numerical identifiers
% NM+ & AN+ : "Please use a number 2 pencil"
%             "He has type 2 diabetes"
number.i batch.i group.i type.i:
  NM+ & AN+;

% Identical to <common-noun>, except that D- costs extra
<costly-common-noun>:
  <noun-modifiers> &
    (AN+
    or ((NM+ or [[{[NM+]1.5} & (Ds- or <no-det-null>) ]] )
      & ((<noun-rel-s> & (<noun-main-s> or <rel-clause-s>))
        or <noun-and-s>))
    or dSJrs-
    or (YS+ & Ds-)
    or (GN+ & (DD- or [()]))
    or Us-
    or ({Ds-} & Wa-));

% -----------------------------------------------------------------
% determiner constructions, with a dangling of: "a number of", "a lot of"
% "I have a number of cookies"
% "a pride of lions" "a litter of kittens" all take determiners
% Some of these commonly modify count nouns, other mass nouns.
% {A-}: "a vast expanse" "a large flock"
% All of these "measure" nouns can also act as common nouns, but
% we want to give these a cost, so that they don't get the first choice.
/en/words/measures.1:
  (OFd+ & Dm+ & {A-} & D-)
  or <marker-common-entity>
  or <costly-common-noun>;

% determiner constructions, with a dangling of:
% "I have bags of money"
% NIn+ needed for money amounts
% {Dmcn-}: "two kilograms of ..."
% The [<generic-plural-id>] is from words.n.2.s
/en/words/measures.2:
  (OFd+ & (NIn+ or Dm+) & {A-} & {Dmcn-})
  or <marker-common-entity>
  or [<generic-plural-id>];

<kind-of>:
  ({@AN-} & @A- & U+ &
    ((Ds**x- & <noun-rel-s> & (<noun-main-s> or <rel-clause-s>))
    or ({Ds**x-} & <noun-and-s>)
    or Us-))
  or (U+ &
    ((Ds**c- & <noun-rel-s> & (<noun-main-s> or <rel-clause-s>))
    or ({Ds**c-} & <noun-and-s>)
    or Us-));

% [<kind-of>]: costly, so that the {Jd-} link for measures.1 is preferred.
kind_of:
  [<kind-of>]
  or EA+
  or EE+
  or Em+
  or Wa-;

% [<kind-of>]: costly, so that the {Jd-} link for measures.1 is preferred.
% Em+: "she sort of hangs from his neck"
type_of sort_of breed_of species_of:
  [<kind-of>]
  or [Us-]
  or Em+
  or [Wa-];

% This gets a cost, so that the {Jd-} link for measures.2 is preferred.
kinds_of types_of sorts_of breeds_of species_of:
  [{{@AN-} & @A-} & U+ &
    (({Dmc-} & <noun-rel-p> & (<noun-main-p> or <rel-clause-p>))
    or ({Dmc-} & <noun-and-p>)
    or Up-)];

% A+: "It has twice the percent value"
percent.u parts.u:
  (<noun-modifiers> &
    ((ND- & {DD-} & <noun-rel-x> & (<noun-main-x> or B*x+)) or
    <noun-main-p> or
    (ND- & {DD-} & <noun-and-x>) or
    U-))
  or (ND- & (OD- or AN+ or YS+))
  or ({E- or EA-} & A+);

% Weight and voume in chemistry
% "An example would be a 5% weight per volume NaCl solution."
weight.u volume.u:
  AN- & Mp+ & AN+;

% "It is a a 5%(w/v) NaCl solution."
% "We measure the concentration in v/v %"
v/v vol/vol volume/volume w/v weight/vol weight/volume:
  ({{Xd- & Xc+} & AN-} & AN+)
  or EA+;

% -----------------------------------------------------------------
% This set of disjuncts should probably be split up and refined.
% "shame.n", "crux.n" are here because they need the Ss*t connector
% to pick up "that" in "The crux of it is that we must act first."
% However, report.n and sign.n and remark.n, etc. do not seem to
% need this connector ...
%
% ({NM+} & {Ss+} & Wd-): "Hypothesis 2: The door on the left hides the prize."
% "Problem: How do you convince your customer that you are on the right path?"
%
<Dsv>: Ds**v- or Ds**x-;
<Dsc>: Ds**c- or Ds**x-;

% Vowel-only form of the below
argument.n impression.n allegation.n announcement.n assertion.n
accusation.n idea.n assumption.n implication.n
indication.n inkling.n amount.n answer.n:
  <noun-modifiers> & (
    AN+
    or (<Dsv> & {@M+} & {(TH+ or (R+ & Bs+)) & {[[@M+]]}} & {@MXs+} &
      (<noun-main2-s> or
      (Ss*t+ & <CLAUSE>) or
      SIs*t- or
      <rel-clause-s>))
    or ({<Dsv>} & <noun-and-s>)
    or dSJrs-
    or (YS+ & <Dsv>)
    or ({NM+} & {Ss+} & Wd-)
    or (GN+ & (DD- or [()]))
    or Us-);

attestation.n:
  (<noun-modifiers> & (({D*u-} & {@M+} & {(TH+ or (R+ & Bs+)) & {[[@M+]]}} & {@MXs+} & (<noun-main2-m> or (Ss*t+ & <CLAUSE>) or SIs*t- or Bsm+)) or Us- or (YS+ & {D*u-}) or (GN+ & (DD- or [()])))) or AN+;

% consonant-only form of the above.
report.n sign.n conclusion.n complaint.n position.n restriction.n
notion.n remark.n proclamation.n reassurance.n saying.n possibility.n
problem.n claim.n result.n statement.n hunch.n concept.n hypothesis.n
message.n premonition.n prerequisite.n prereq.n pre-req.n pre-requisite.n
corequisite.n co-requisite.n coreq.n co-req.n truism.n fallacy.n
proposition.n prospect.n presupposition.n supposition.n finding.n
crux.n shame.n thing.n bet.n guess.n:
  <noun-modifiers> & (
    AN+
    or (<Dsc> & {@M+} & {(TH+ or (R+ & Bs+)) & {[[@M+]]}} & {@MXs+} &
      (<noun-main2-s> or
      (Ss*t+ & <CLAUSE>) or
      SIs*t- or
      <rel-clause-s>))
    or ({<Dsc>} & <noun-and-s>)
    or dSJrs-
    or (YS+ & <Dsc>)
    or ({NM+} & {Ss+} & Wd-)
    or (GN+ & (DD- or [()]))
    or Us-);

% Vowel form of the below
acknowledgment.n acknowledgement.n understanding.n assurance.n
awareness.n opinion.n explanation.n expectation.n insistence.n:
  (<noun-modifiers> & (
    ({(D*u*v- or D*u*x-)} & {@M+} & {(TH+ or (R+ & Bs+)) & {[[@M+]]}} & {@MXs+} & (
      <noun-main2-m>
      or (Ss*t+ & <CLAUSE>)
      or SIs*t-
      or <rel-clause-s>))
    or ({(D*u*v- or D*u*x-)} & <noun-and-u>)
    or Us-
    or (YS+ & {D*u-})
    or (GN+ & (DD- or [()]))))
  or AN+;

% Consonant for of the above.
proof.n doubt.n suspicion.n hope.n knowledge.n relief.n disclosure.n
fear.n principle.n concern.n philosophy.n risk.n threat.n conviction.n
theory.n speculation.n news.n belief.n contention.n thought.n myth.n
discovery.n rumor.n probability.n fact.n feeling.n comment.n process.n
perception.n sense.n realization.n view.n consensus.n notification.n
rule.n danger.n warning.n suggestion.n:
  (<noun-modifiers> & (
    ({(D*u*c- or D*u*x-)} & {@M+} & {(TH+ or (R+ & Bs+)) & {[[@M+]]}} & {@MXs+} & (
      <noun-main2-m>
      or (Ss*t+ & <CLAUSE>)
      or SIs*t-
      or <rel-clause-s>))
    or ({(D*u*c- or D*u*x-)} & <noun-and-u>)
    or Us-
    or (YS+ & {D*u-})
    or (GN+ & (DD- or [()]))))
  or AN+;

evidence.n reasoning.n likelihood:
  (<noun-modifiers> &
    (({Dmu-} & {@M+} & {(TH+ or (R+ & Bs+)) & {[[@M+]]}} & {@MXs+} &
      (<noun-main2-m> or
      (Ss*t+ & <CLAUSE>) or
      SIs*t- or
      <rel-clause-s>)) or
    ({Dmu-} & <noun-and-u>) or
    Up- or
    (YS+ & {Dmu-}) or
    (GN+ & (DD- or [()])))) or
  AN+;

ideas.n opinions.n statements.n beliefs.n facts.n arguments.n
principles.n theories.n philosophies.n signs.n impressions.n
conclusions.n contentions.n complaints.n proofs.n doubts.n
suspicions.n allegations.n reports.n claims.n announcements.n
positions.n risks.n hopes.n explanations.n restrictions.n threats.n
thoughts.n myths.n feelings.n discoveries.n rumors.n comments.n
realizations.n probabilities.n remarks.n notions.n convictions.n
hunches.n assumptions.n concepts.n hypotheses.n assertions.n
expectations.n implications.n perceptions.n proclamations.n
reassurances.n fears.n sayings.n senses.n messages.n disclosures.n
accusations.n views.n concerns.n understandings.n acknowledgments.n
acknowledgements.n possibilities.n premonitions.n prerequisites.n
prereqs.n pre-reqs.n pre-requisites.n
corequisites.n co-requisites.n coreqs.n co-reqs.n
provisos.n truisms.n fallacies.n assurances.n speculations.n
propositions.n prospects.n presuppositions.n inklings.n suppositions.n
findings.n amounts.n rules.n dangers.n warnings.n indications.n
answers.n suggestions.n:
  (<noun-modifiers> &
    (({{Jd-} & Dmc-} & {@M+} & {(TH+ or (R+ & Bp+)) & {[[@M+]]}} & {@MXp+} &
      (<noun-main2-p> or
      (Sp*t+ & <CLAUSE>) or
      SIp*t- or
      <rel-clause-p>)) or
    ({Dmc-} & <noun-and-p>) or
    Up- or
    (YP+ & {Dmc-}) or
    (GN+ & (DD- or [()])))) or
  [[AN+]];

request.n requirement.n condition.n recommendation.n provision.n stipulation.n:
  (<noun-modifiers> &
    (({D*u-} & {@M+} & {(TH+ or TS+ or (R+ & Bs+)) & {[[@M+]]}} & {@MXs+} &
      (<noun-main2-m> or
      (Ss*t+ & <CLAUSE>) or
      SIs*t- or
      <rel-clause-s>)) or
    ({D*u-} & <noun-and-u>) or
    Us- or
    (YS+ & {D*u-}) or
    (GN+ & (DD- or [()])))) or
  AN+;

% {Jd-} : "a number of conditions"
requests.n requirements.n conditions.n recommendations.n provisions.n
stipulations.n:
  (<noun-modifiers> &
    (({{Jd-} & Dmc-} & {@M+} & {(TH+ or TS+ or (R+ & Bp+)) & {[[@M+]]}} & {@MXp+} &
      (<noun-main2-p> or
      (Sp*t+ & <CLAUSE>) or
      SIp*t- or
      <rel-clause-p>)) or
    ({Dmc-} & <noun-and-p>) or
    Up- or
    (YP+ & {Dmc-}) or
    (GN+ & (DD- or [()])))) or
  [[AN+]];

% (NM+ & Ss+ & Wd-): "Proposal 2: Hire a plumber"
excuse.n decision.n proposal.n attempt.n plan.n plot.n pledge.n urge.n
mission.n right.n desire.n mandate.n promise.n option.n campaign.n
offer.n vow.n permit.n impetus.n proclivity.n propensity.n move.n
vote.n bill.n:
  (<noun-modifiers> &
    ((Ds- & {@M+} & {(<ton-verb> or (R+ & Bs+)) & {[[@M+]]}} & {@MXs+} &
      (<noun-main-s> or
      <rel-clause-s>)) or
    ({Ds-} & <noun-and-s>) or
    Us- or
    (YS+ & Ds-) or
    (GN+ & (DD- or [()])) or
    ({NM+} & Ss+ & Wd-))) or
  AN+;

propension.n:
  (<noun-modifiers> & ((Ds- & {@M+} & {(<ton-verb> or (R+ & Bs+)) & {[[@M+]]}} & {@MXs+} & (<noun-main-s> or Bsm+)) or Us- or (YS+ & Ds-) or (GN+ & (DD- or [()])))) or AN+;

% <noun-rel-uto>: somewhat like <noun-rel-s> but with more stuff.
% {Jd-}: "a large amount of effort"
<noun-rel-uto>:
  {{Jd-} & D*u-} & {@M+} & {(<ton-verb> or (R+ & Bs+)) & {[[@M+]]}} & {@MXs+};

% Like other generic noun types, but can take "to"
% "He took the effort to make amends"
<noun-to>:
  <noun-modifiers> &
    (AN+
    or ((<noun-rel-uto> & (<noun-main-m> or <rel-clause-s>)) or <noun-and-p,u>)
    or ({D*u-} & <noun-and-u>)
    or (YS+ & {D*u-})
    or (GN+ & (DD- or [()]))
    or Us-
    );

failure.n haste.n refusal.n reluctance.n pressure.n willingness.n
responsibility.n intent.n temptation.n readiness.n effort.n
determination.n capacity.n unwillingness.n need.n will.n eagerness.n
opportunity.n commitment.n ambition.n ability.n order.n obligation.n
incentive.n panache.n balls.n-u cojones.n-u:
  <noun-to>
  or <noun-assert>;

% Regex-based guessing of unknown words, ending in -ity -acy -ance
<NOUN-TO-WORDS>.n:
  <noun-to>;

% Nouns formerly classified as mass nouns (words.n.3) but can take "to"
% Unlike mass nouns, cannot take numeric modifiers.
% <to-verb>: "The inability to laugh signifies trouble"
% "He had the honesty to come clean"
% "He had the impudence to laugh"
% "The solution had the acidity to eat through walls"
/en/words/words.n.3.y: <noun-to>;

% {Jd-}: "tons of offers"
% TH+: What are the CHANCES that this could happen?
excuses.n decisions.n proposals.n failures.n efforts.n attempts.n
refusals.n pledges.n urges.n missions.n rights.n desires.n needs.n
ambitions.n capacities.n mandates.n promises.n abilities.n options.n
commitments.n intents.n opportunities.n plans.n plots.n
responsibilities.n chances.n campaigns.n offers.n pressures.n
obligations orders.n temptations.n vows.n permits.n impetuses.n
proclivities.n propensities.n moves.n votes.n bills.n incentives.n:
  (<noun-modifiers> &
    (({{Jd-} & Dmc-} & {@M+} & {(<ton-verb> or TH+ or (R+ & Bp+)) & {[[@M+]]}} & {@MXp+} &
      (<noun-main-p> or
      <rel-clause-p>)) or
    ({Dmc-} & <noun-and-p>) or
    Up- or
    (YP+ & {Dmc-}) or
    (GN+ & (DD- or [()])))) or
  [[AN+]];

% WTF, the next batch below are like above, but with assorted exceptions ...
chance.n:
  (<noun-modifiers> &
    (({D*u-} & {@M+} & {(<ton-verb> or TH+ or (R+ & Bs+)) & {[[@M+]]}} & {@MXs+} &
      (<noun-main-s> or
      <rel-clause-s>)) or
    ({D*u-} & <noun-and-s>) or
    Us- or
    (YS+ & {D*u-}) or
    (GN+ & (DD- or [()])))) or
  AN+;

% ({NM+} & Ss+): "Question 2: Was he present at the meeting?"
% Wd-: "Question: How do you convince your customer that you are on the right path?"
question.n issue.n:
  (<noun-modifiers> &
    ((Ds- & {@M+} & {R+ & Bs+ & {[[@M+]]}} & {@MXs+} &
      (<noun-main2-s> or
      (Ss*q+ & <CLAUSE>) or
      SIs*q- or
      <rel-clause-s>)) or
    ({Ds-} & <noun-and-s>) or
    Us- or
    (YS+ & Ds-) or
    ({NM+} & Ss+) or
    Wd- or
    (GN+ & (DD- or [()])))) or
  AN+;

questions.n issues.n:
  (<noun-modifiers> &
    (({{Jd-} & Dmc-} & {@M+} & {R+ & Bp+ & {[[@M+]]}} & {@MXp+} &
      (<noun-main2-p> or
      (Sp*q+ & <CLAUSE>) or
      SIp*q- or
      <rel-clause-p>)) or
    ({Dmc-} & <noun-and-p>) or
    Up- or
    (YP+ & {Dmc-}) or
    (GN+ & (DD- or [()])))) or
  AN+;

reason.n:
  (<noun-modifiers> &
    (({D*u-} & {@M+} & {TH+ or <embed-verb> or <ton-verb> or WY+ or (R+ & Bs+)} & {@MXs+} &
      (<noun-main2-s> or
      (Ss*t+ & <CLAUSE>) or
      SIs*t- or
      <rel-clause-s>)) or
    ({D*u-} & <noun-and-s>) or
    Us- or
    (YS+ & {D*u-}) or
    (GN+ & (DD- or [()])))) or
   AN+;

reasons.n:
  (<noun-modifiers> &
    (({{Jd-} & Dmc-} & {@M+} & {TH+ or <embed-verb> or <ton-verb> or WY+ or (R+ & Bp+)} & {@MXp+} &
      (<noun-main-p> or
       <rel-clause-p>)) or
    ({Dmc-} & <noun-and-p>) or
    Up- or
    (YP+ & {Dmc-}) or
    (GN+ & (DD- or [()])))) or
  [[AN+]];

way.n:
  (<noun-modifiers> &
    ((Ds- & {@M+} & {<ton-verb> or TH+ or <embed-verb> or (R+ & Bs+)} & {@MXs+} &
      (MVa- or
      <noun-main-s> or
      <rel-clause-s>)) or
    ({Ds-} & <noun-and-s>) or
    Us- or
    (YS+ & {D*u-}) or
    (GN+ & (DD- or [()])))) or
  AN+;

% NMa+: "Go to place X on the map."
place.n:
  (<noun-modifiers> &
    ((Ds- & {@M+} & {<ton-verb> or TH+ or <embed-verb> or (R+ & Bs+)} & {@MXs+} &
      ([[MVa-]] or
      <noun-main-s> or
      <rel-clause-s>)) or
    ({Ds-} & <noun-and-s>) or
    (<noun-main-m> & {NMa+}) or
    Us- or
    (YS+ & {D*u-}) or
    (GN+ & (DD- or [()])))) or
  AN+;

% NMa+: "Go to spot X on the map."
spot.n:
  <marker-common-entity> or (<common-noun> & {NMa+});


% NMa+: "It all happens at time T."
time.n:
  (<noun-modifiers> &
    (({D*u-} & {@M+} & {<ton-verb> or WN+ or TH+ or <embed-verb> or (R+ & Bs+)} & {@MXs+} &
      ([[MVa-]] or
      (<noun-main-m> & {NMa+}) or
      <rel-clause-s>)) or
    ({D*u-} & <noun-and-x>) or
    Us- or
    <noun-assert> or
    (YS+ & {D*u-}) or
    (GN+ & (DD- or [()])))) or
  AN+;

ways.n:
  (<noun-modifiers> &
    (({{Jd-} & Dmc-} & {@M+} & {<ton-verb> or TH+ or <embed-verb> or (R+ & Bp+)} & {@MXp+} &
      (MVa- or
      <noun-main-p> or
      <rel-clause-p>)) or
    ({Dmc-} & <noun-and-p>) or
    Up- or
    (YP+ & {Dmc-}) or
    (GN+ & (DD- or [()])))) or
  [[AN+]];

places.n:
  (<noun-modifiers> &
    (({{Jd-} & Dmc-} & {@M+} & {<ton-verb> or TH+ or <embed-verb> or (R+ & Bp+)} & {@MXp+} &
      ([[MVa-]] or
      <noun-main-p> or
      <rel-clause-p>)) or
    ({Dmc-} & <noun-and-p>) or
    Up- or
    (YP+ & {Dmc-}) or
    (GN+ & (DD- or [()])))) or
  [[AN+]];

% Dmc- & Wa-: "Many times!" "How many times?"
times.n:
  (<noun-modifiers> &
    (({{Jd-} & Dmc-} & {@M+} & {<ton-verb> or WN+ or TH+ or <embed-verb> or (R+ & Bp+)} & {@MXp+} &
      ([[MVa-]] or
      <noun-main-p> or
      <rel-clause-p>)) or
    ({Dmc-} & <noun-and-p>) or
    Up- or
    (YP+ & {Dmc-}) or
    (GN+ & (DD- or [()]))))
  or (Dmc- & Wa-)
  or [[AN+]];

% ====================================================================
%PRONOUNS

% MXs+: "he, the shop owner, ..."
% SIpx-: "I would have done that were he with me"
she he:
  {[[R+ & Bs+]]} & (({MXs+} & Ss+ & <CLAUSE>) or SIs- or SIpx- or dSJls+);

% subject of bare infinitive:
% "You should hear him sing" -- sing is the bare infinitive,
% and "him" is the "subject" of that infinitive...
<bare-inf>: (J- or Ox-) & {Sg+ or Sj+};

% The E- is for "It's either us or them" ... not ideal, but OK
% See also me.p below.
me him:
  <bare-inf> or ({[[E-]]} & dSJl+) or dSJr- or Wa-;

% DD+: "... how us two should work together"
us:
  <bare-inf> or ({[[E-]]} & dSJl+) or dSJr- or DD+;

% Jd- & Dmc-: "I have a lot of them" and J- is cost so that this comes first.
% (Jd- & Dmc- & Sp+):  "Many of them could be saved"
% Why is there a cost on <bare-inf> ???
them:
  [<bare-inf>]0.5
  or ({Jd- & Dmc-} & Ox-)
  or (Jd- & Dmc- & {Wd-} & Sp+)
  or ({[[E-]]} & dSJl+)
  or dSJr-;

% Ox-: "John made himself familiar with the drawings"
%    The Ox is needed for post-processing
myself yourself himself herself itself themselves
ourselves yourselves thyself oneself one's one’s:
  <bare-inf> or E+ or MVa-;

each_other: ((J- or O-) & {Sg+ or Sj+}) or YS+;

his:
  DP+
  or ({AL-} & {@L+} & (D+ or DD+))
  or [<noun-main-x>]
  or Wa-;

% J-: "... with her"
her:
  <bare-inf>
  or DP+
  or ({AL-} & {@L+} & (D+ or DD+))
  or Wa-;

others:
  {{Jd-} & Dmc-} & ((<noun-rel-p> & <noun-main-p>) or <noun-and-p>);

mine.p yours theirs.p hers ours:
  <noun-main-x>
  or Wa-
  or dSJl+
  or dSJr-;

% yisser yousser ye'r: Irish English second-person possessive --
% https://en.wikipedia.org/wiki/Irish_English
its my.p your their.p our thy.p yisser.p yousser ye'r:
  DP+
  or ({AL-} & {@L+} & (D+ or DD+));

% Possessive version of me
% Cost on D, DD: avoids use as determiner on "Make me coffee"
me.p:
  DP+
  or [{AL-} & {@L+} & (D+ or DD+)];

% [<CLAUSE> & Pg+]: "you leaving already?"
% Wa-: "You!"
% Ds+: "you asshole!"
you:
  Wa- or <bare-inf>
  or (Sp+ & <CLAUSE>)
  or SIp-
  or dSJlp+
  or [<CLAUSE> & Pg+]
  or [Ds+];

thou: Sp+ & <CLAUSE>;

y'all y’all yall you_all: (Sp+ & <CLAUSE>) or SIp-;

% basilect you
% Pg+: "yo leavin' already?" verb "are" is missing.
% youse yous yis ye: second-person plural -- Irish English --
% https://en.wikipedia.org/wiki/Irish_English
youse yous yis ye ya yo:
  <bare-inf> or (Sp+ & <CLAUSE>) or SIp-
  or (<CLAUSE> & Pg+);

% (Jd- & Dmu- & Sp+):  "much of it could be saved"
% (Jd- & Dmu- & {Wd-} & S+): "all of it was saved"
% (Jd- & Dmu- & Os-):  "there was enough of it"
% (Osm- & {@M+}): "is this it?" "Do you have it ready?"
%    "leave it alone" "make it rain"
%   (the m prevents links as indirect object)
% MX+ & Ss+: "it -- fried rice -- is very popular"
% XXX FIXME: why does [J-] have a cost???
it:
   [J-]0.5
   or (Osm- & ({@M+} or {Sg+ or Sj+}))
   or (Jd- & Dmu- & Os-)
   or (Jd- & Dmu- & {Wd-} & S+)
   or ({MX+} & (Ss+ or SFsi+) & <CLAUSE>)
   or SIs- or SFIsi- or OXi- or Vp-
   or dSJls+ or dSJrs-;

% O*c: "we will go faster than they"
% MXp+: "they, the twins, did it"
they:
  ({[[R+ & Bp+]]} & (({MXp+} & Sp+ & <CLAUSE>) or SIp-)) or
  Ox- or
  dSJlp+ or dSJrp-;

% DD+: "... how we two should work together"
% MXp+: "We, the undersigned, ..."
we:
  ({MXp+} & Sp+ & <CLAUSE>)
  or Ox-
  or DD+
  or SIp- or dSJlp+ or dSJrp-;

% XXX why is this marked plural (Sp, SIp) ?? a cheap hack to make "I've" work?
% We use <marker-entity> here to prevent lower-case mangling by
% the tokenizer.
% SJrp-: allows only "...and I", disallows "I and ..."
% MXs+: "I, Joseph, rang the bell"
I.p:
  <marker-entity>
  or ((({MXs+} & Sp*i+) or SX+) & <CLAUSE>)
  or SIp*i-
  or dSJr-
  or dSJl+
  or SXI-;

them_all us_all you_all: <bare-inf>;
% it_all gets a cost when used as direct object (Ox) to avoid
% inappropriate parse "Please paint it all white"
it_all: [[Ox-]] or J-;

something someone somebody:
  {EL+} & (({Pa+} & <noun-rel-s> & {@MXs+} & <noun-main-s>) or <noun-and-s> or YS+ or Wa-);

nothing no_one nobody:
  {EN-} & {EL+} & (({Pa+} & <noun-rel-s> & {@MXs+} & <noun-main-s>) or <noun-and-s> or YS+ or Wa-);

everything everyone anyone everybody anybody anything:
  {EN-} & {EL+}
    & (((({Pa+} & <noun-rel-s>) or CX+) & {@MXs+} & <noun-main-s>) or <noun-and-s> or YS+ or Wa-);

% EL-: "what the fuck happened?" "what else happened?"
% Amazing how profanity works this way...
else the_fuck the_shit the_piss
the_bleep
in_heck in_the_heck the_heck
in_hell in_the_hell the_hell bloody_hell dirty_hell
in_goddamn_hell in_the_godammn in_goddamned_hell in_the_godammned
in_bleeping_hell in_bloody_hell in_dirty_hell
in_holy_shit the_holy_shit the_shit holy_crap the_holy_crap
goddamn goddamned damn.ee
for_christ_sake for_christs_sake for_Christ_sake for_Christs_sake
for_christ's_sake for_Christ's_sake
in_Jesus in_Jesus_name in_Jesus'_name in_God's_name in_Lord's_name
in_Lords_name in_the_Lord's_name the_bejesus
the_almighty almighty_lord the_almighty_lord
the_piss_Christ in_piss_Christ:
  EL-;

% Like above, but more polite, and with commas:
% "What, exactly, do you want?"
exactly.ee precisely.ee really.ee:
  {Xc+ & {Xd-}} & EL-;

% E+: "Why didn't you fucking say so?"
fucking.e bleeping.e flat_out:
  EL- or E+;

% ====================================================================
% DETERMINERS

% (L+ & (AJld+ or AJrd-)): "...is the biggest and the baddest thug."
the:
  ({AL-} & {@L+} & (D+ or DD+)) or
  DG+ or
  (TR- & U+) or
  (L+ & (dAJld+ or dAJrd-));

% this as a pronoun
this.p:
  <noun-main-h>
  or EA+
  or [<noun-and-s>]0.5;

% this as a determiner
% {ALx-} & DD+: "Give me just this one thing"
this.d:
  ({ALx-} & (D*u+ or DD+))
  or DTn+
  or Wa-;

% "Just the regulars are here"
just.d only.d:
  {MVa-} & ALx+;

% [[<noun-main-p>]] costs so that ditransitive verbs don't suffer:
% "I taught these mice to jump", taught is ditransitive, we don't want
% "these" to be the object.  See also "those"
% (Jd- & Dmu- & Op-): "I gave him a number of these"
% (Jd- & Dmu- & {Wd-} & Sp+): "a number of these were found"
% ALx- & Op-: "I want only these"
these:
  ({ALx-} & (Dmc+ or DD+))
  or ({ALx-} & Op-)
  or (Jd- & Dmu- & Op-)
  or (Jd- & Dmu- & {Wd-} & Sp+)
  or [[<noun-main-p>]]
  or <noun-and-p>
  or Wa-;

% [[<noun-main-p>]] costs so that ditransitive verbs don't suffer,
% and get the D+ link instead of the O- link.
% See also "these"
those:
  ({AL-} & (Dmc+ or DD+))
  or ({ALx-} & Op-)
  or (Jd- & Dmu- & Op-)
  or (Jd- & Dmu- & {Wd-} & Sp+)
  or (<noun-rel-p> & ([[<noun-main-p>]] or dRJlr+ or dRJrr-))
  or <noun-and-p>
  or Wa-;

% "Them there beans ought to be picked"
them_there:
  Dm+ or Wa-;

% (Wa- & {Mp+}): "Both of them."
% XJa+: "Both June and Tom are coming"
both.d:
  Dmc+
  or XJa+
  or E+
  or ({M+ or (ALx+ & Jp+)} & <noun-main-p>)
  or <noun-and-p>
  or (Wa- & {Mp+});

both.a: Paf- or dAJra-;

% half: prefer NJ+ over noun-main-x, because half is almost surely
% modifying something, instead of being a direct object, subject, etc.
% EZ+: "If you love Mr. Darcy half as well as I do"
half:
  {EN-} &
    (NJ+
    or EZ+
    or [(({DD-} & {@Mp+ or (R+ & B+)}) or (AL+ & J+)) & <noun-main-x>]);

% OFd+ & Dmc+: "I drank many of the beers"
% Naked H-: "How many?"
% H- & EC+: "How many more?"
many:
  (H- & (Dmc+ or ND+ or NIn+ or EC+ or [()]))
  or (AM- & (Dmcy+ or Oy- or Jy-))
  or ({EE-} & (ND+ or NIn+))
  or ({DD-} & {EAx-} & Dmc+)
  or (OFd+ & Dmc+)
  or ((({EAx-} & {DD-}) or H-) & <noun-rel-p> & ([<noun-main-p>] or <rel-clause-p>));

% A naked <noun-main2-x> costs more than one with other links,
% so that ditransitive verbs don't get spurious links to all.a
% XXX can this be tightened up??
% <noun-main2-x> costs no mater what, so that Ofd+ is preferred.
% [E+]0.5: all modifying a verb probably is not right.
% Wa-: "All the people!" as a response to a question.
% {MVa-} & ALx+: "I want all these"
all.a:
  ({EN-} & (
    [E+]0.5
    or Dm+
    or NIc+
    or (ALx+ & (Jp+ or Ju+) & Wa-)
    or (
      (@M+ or (ALx+ & (Jp+ or Ju+)) or (R+ & B+) or EL+)
      & [<noun-main2-x>])
    or (
      {@M+ or (ALx+ & (Jp+ or Ju+)) or (R+ & B+) or EL+}
      & (
        [[<noun-main2-x>]]
        or (S**t+ & <CLAUSE>)
        or SI**t-))
    or <noun-and-x>))
  or ({MVa-} & ALx+)
  or DTa+;

all_that: EA+ or EE+ or (<noun-rel-s> & <noun-main-s>);
all_this: (<noun-rel-s> & <noun-main-s>) or <noun-and-s>;

all_those all_these: [[<noun-rel-p> & <noun-main-p>]] or <noun-and-p>;

% Ds*w-: "you want which one?"
% ({Ds-} & Wa-): "That one."
one:
  NA+ or
  NMw- or
  NN+ or
  ({EN-} & NIfn+) or
  ({NA-} & {EN-} & (({DD-} & Ds+) or ({{@A- & {[[@AN-]]}} & Ds-} &
    (YS+ or
    (<noun-rel-s> & (<noun-main-s> or <rel-clause-s>)) or
    <noun-and-s>))))
  or Ds*w-
  or NIm+
  or NSn+
  or (NA- & ND+)
  or DTi+
  or (NA- & Xd- & TY- & Xc+)
  or ({Ds-} & Wa-);

ones:
  {@A- & {[[@AN-]]}} & {Dmc-} &
    (YP+ or
    (<noun-rel-p> & <noun-main-p>) or
    <noun-and-p>);

any:
  ({EN-} &
    (D+ or
    DD+ or
    (<noun-rel-x> & <noun-main-x>) or
    <noun-and-x>)) or
  EC+;

% PHc+ : must link to consonant immediately to the right
% PHv+ : must link to vowel immediately to the right
% The plain [()]0.2 allows stupid users to write "a apple" "an banana"
% without utterly, miserably failing.  The cost is very low, right now,
% because not everything has been sorted into vowels & consonants.
%
% A lot of work is needed to make this really work correctly.  In
% particular, the wat <noun-modifiers> works needs to be redesigned.
% That is, if a noun uses (A- & Ds-) then it should be (A- & Ds**x-)
% but if the A- is not there, but the Ds is, then it must have either
% Ds**c or Ds**v. it will take a lot of work to restructure to do this.
% What a pain...  The quasi-alternative is to say: if PH is allowed,
% then it is preferred i.e. PH- has a negative cost.
%
% XXX FIXME Someday, remove the [()]0.2 entirely to force agreement.
<PHc> : PHc+ or [()]0.2;
<PHv> : PHv+ or [()]0.2;
%
% See also a.eq below.
a:  ({(AA- & HA-) or ALa- or [[Lf+]]} & (Ds**c+ or (<PHc> & Ds**x+)))
  or NN+ or NSa+ or NIm+;

an: ({(AA- & HA-) or ALa- or [[Lf+]]} & (Ds**v+ or (<PHv> & Ds**x+)))
  or NN+ or NSa+ or NIm+;

such: (ND- & Dmc+) or Dm*k+;
such_a:  Ds*kc+ or (<PHc> & Ds*kx+);
such_an: Ds*kv+ or (<PHv> & Ds*kx+);


% "all of the time". These are all temporal modifiers: use MVw ("when")
% and use OFw to force linkage only to time exprs.
% OFw+ & Xc+ CO+: "Some of the time, I really  hate it."
<adv-of>: (MVw- & OFw+) or (OFw+ & Xc+ & dCO+);

% Form-a-cycle with "as". The cost is arranged so that the cycle is
% preferred, if it is possible to make one. So:
% (MVa- & EZ-): "... worked first as a teacher"
<adv-as>: MVa- or [MVa- & EZ+]-0.1;

a_lot:
  [[<noun-and-p>]]
%  or [[(<noun-rel-p> & <noun-main-p>)]]
  or EC+ or <adv-as> or <adv-of> or Wa-;

% OFd+ & Dmc+: "I ate a few of the cookies."
few:
  (OFd+ & Dmc+ & {A-} & (D- or EA-))
  or ({EA- or EF+ or ({EA-} & DD-)} &
     (Dmc+ or [<noun-rel-p> & <noun-main-p>] or <noun-and-p> or Wa-));

a_couple:
%  [[<noun-rel-p> & <noun-main-p>]] or
  <noun-and-p> or Wa-;

% Numeric modifier: "a couple of thousand dollars"
a_couple_of:
  NN+ or ND+ or NIn+;

% ECa+: "I had a few too many"
a_few:
  NN+
  or ND+
  or NIn+
  or ECa+
  or Wa-
  or ({EN-} &
    (Dmc+
    or ND+
    or NIn+
    or [[<noun-rel-p> & <noun-main-p>]]));

% OFd+ & Dm+: "I ate some of the cookies"; cost to <nou>, <adv-of> so
% that this comes first.
% <adv-of>: "I saw him some of the time"
% {COw+} & <adv-of>:
some:
  D+
  or (OFd+ & Dm+ & {EC-})
  or EN+
  or MF+
  or [<noun-rel-x> & <noun-main-x>]
  or <noun-and-x>
  or [[<adv-of>]]
  or Wa-;

little.i:
  ({EE-} & (<adv-as> or <advcl-verb> or Qe+))
  or (OFd+ & Dm+ & {A-} & D-)
  or ({EEx- or H-}
    & (Dmu+
      or [<noun-rel-s> & (<noun-main-s> or <rel-clause-s>)]
      or <noun-and-s>))
  or (AM- & (Dmuy+ or MVy- or Oy- or Jy-))
  or [[{Ds-} & <adv-of>]];

% "he likes you most" has no determiner, just uses MVa-.
% {OFd+}: "most of them"
most:
  ({OFd+} & Dm+)
  or EA+
  or MF+
  or [EE+]
  or [<noun-rel-x> & <noun-main-x>]
  or <noun-and-x>
  or [<adv-of>]
  or [{DD-} & MVa- & {Mp+}];

the_most:
  ({OFd+} & Dm+)
  or EE+
  or [<noun-rel-x> & <noun-main-x>]
  or <adv-as>;

% "a part.n" should cover most cases.  Perhaps [[OF+ & <noun-main-s>]] should be
% removed??  Anyway, it's costed to give OFd+ priority. Likewise, should probably
% retire <adv-of> as well, right?
part.i:
  (OFd+ & Dm+)
  or [[OF+ & <noun-main-s>]]
  or <noun-and-s>
  or [[{Ds-} & <adv-of>]];

all.e:
  (OFd+ & Dm+)
  or [[<adv-of>]];

% "he likes you least of all" has no determiner, just uses MVa-.
% A+: "I shall retain every least word"
least.a: EA+ or [A+];
least.e: ({DD-} & MVa- & {Mp+}) or <adv-as>;

none:
  (OFd+ & Dm+)
  or [<noun-rel-x> & <noun-main-x>]
  or <noun-and-x>
  or [[<adv-of>]]
  or Wa-;

% costly <adv-of> so that OFd+ is preferred.
rest.i:
  [[DD- & <adv-of>]];

plenty:
  (OFd+ & Dm+)
  or ({@M+} & ([<noun-main-x>] or <noun-and-x>))
  or [[<adv-of>]]
  or [MVa-]
  or Wa-;

% SJr-: "someone or other would always complain"
other:
  Dmu+ or
  dSJr- or
  ({ND-} & {DD-} & Dmc+) or
  (DD- & (Ds+ or DD+ or <noun-main-s> or <noun-and-s>));

one_other every_other: <noun-main-s> or <noun-and-s> or Ds+;
any_other no_other: <noun-main-s> or <noun-and-s> or D+;
all_other: Dm+;
most_other: Dmc+;
quite_a : Ds**c+ or (<PHc> & Ds**x+);
quite_an : Ds**v+ or (<PHv> & Ds**x+);
one_such not_every: Ds+;
some_other no_such: D+;
every.d: {EN-} & (Ds+ or DTn+ or [[NIc+]]);

another:
  (OFd+ & Dm+)
  or Ds+
  or NIc+
  or [<noun-rel-s> & <noun-main-s>]
  or <noun-and-s>
  or YS+
  or Wa-;

one_another: (<noun-rel-s> & <noun-main-s>) or <noun-and-s> or YS+;

each:
  (OFd+ & Dm+)
  or Ds+
  or [<noun-rel-s> & <noun-main-s>]
  or <noun-and-s>
  or DTn+
  or E+
  or <adv-as>
  or Wa-;

no.misc-d: ({EN-} & D+) or EC+;

a_little:
  [<noun-rel-s> & <noun-main-s>]
  or <noun-and-s>
  or EA+ or EC+ or EE+ or <adv-as> or Wa-;

a_great_deal:
  EC+
  or <adv-as>
  or (OFd+ & Dmu+)
  or [<noun-rel-s> & <noun-main-s>]
  or <noun-and-s>
  or Wa-;

many_more a_few_more a_couple_more plenty_more a_lot_more:
  Dmcc+
  or (OFd+ & Dm+)
  or [<noun-rel-p> & <noun-main-p>]
  or <noun-and-p>
  or Wa-;

some_more:
  <adv-as>
  or Dm+
  or (OFd+ & Dmu+)
  or [<noun-rel-x> & <noun-main-x>]
  or <noun-and-x>
  or Wa-;

one_more:
  Ds+ or (<noun-rel-s> & <noun-main-s>) or <noun-and-s> or Wa-;

not_many:
  ({OFd+} & Dmc+)
  or [<noun-rel-p> & <noun-main-p>]
  or Wa-;

not_much:
  ({OFd+} & Dmu+)
  or [<noun-rel-x> & <noun-main-u>]
  or Wa-;

not_all not_everything:
  ({OFd+} & Dm+)
  or (((ALx+ & Jp+) or <noun-rel-x>) & [S+] & <CLAUSE>)
  or Wa-;

not_one:
  Ds+
  or (OFd+ & Dm+)
  or (<noun-rel-s> & [Ss+] & <CLAUSE>)
  or Wa-;

enough.n:
  (OFd+ & Dmu+)
  or [{OF+} & <noun-main-s>]
  or <noun-and-s>
  or Wa-;

% EF-: "He is good enough" but *He is very good enough
enough.r: EF-;

enough.a: ({@E-} & Pa- & {Pg+ or Os+ or @MV+}) or ({@E-} & Dm+);

% Wi-: "Enough rough-housing! Enough!"
% S**i- & Wi-: John, enough!
enough.ij: {Xd- & (EI- or S**i-)} & Wi- & {Pg+};

not_enough:
  (OFd+ & Dm+)
  or [{OF+} & <noun-main-s>]
  or <noun-and-s>
  or Wa-;

% =================================================================
% NUMERICAL EXPRESSIONS
% Numeric ranges, e.g. "15 to 20". The "to" (or hyphen) acts as a
% number in a range, and the requirements should match NUMBER.
% The NIf connectors (second subscript position is "n" for number
% and "u" for unit) allow "15 to 20", "15 to 20 kDa" and
% "15 kDa to 20 kDa", but not "15 kDa to 20".
% Allowing EC+ for "two to threefold more abundant". This allows also the
% nonsense "two to three more abundant", but this is likely harmless.
-.j-ru --.j-ru ---.j-ru or.j-ru to.j-ru ->.j-ru -->.j-ru:
  (NIfn- & {NIr-} & NItn+ & (
    NM- or EC+ or MVp-
    or NN+ or [[NF+]]
    or ({EN- or NIc-} & (ND+ or OD-
      or ({{@L+} & DD-} & (Dmcn+
        or (<noun-rel-p> & [<noun-main-p>])))))))
  or (NIfu- & {NIr-} & NItu+ & (
    ((<noun-rel-x> & (<noun-main-x> or Bsm+)) or (Us- & {Mp+}))
    or AN+ or Yd+ or Ya+))
  or (NIfp- & {NIr-} & NItp+ & (
    NM- or AN+ or ({Xc+ & Xd-} & Ma-)
    or <fronted>
    or MVp- or Pp- or FM-
    or (Xc+ & Xd- & (MVx- or MX-))));

% MVa+: "survivors differ from comparisons (5 vs. 6, respectively)"
and.j-ru vs.j-ru vs..j-ru versus.j-ru v.j-ru v..j-ru:
  (NIfn- & {NIr-} & NItn+ & (NM- or Jp- or EQ- or
    (NN+ or [[NF+]] or ({EN- or NIc-} & (ND+ or OD- or
      ({{@L+} & DD-} & (Dmcn+ or (<noun-rel-p> & [<noun-main-p>])))))))
  & {{Xca+} & MVa+}) or
  (NIfu- & {NIr-} & NItu+ &
    (((<noun-rel-x> & (<noun-main-x> or Bsm+)) or (Us- & {Mp+})) or AN+ or Yd+ or Ya+));

% and.j-sum is used in numerical sums: "It's a hundred and two in the shade."
% It's a hundred 'n two in the shade."
and.j-sum 'n.j-sum: NA- & NA+;

% For number, cost is added to the <noun-main-*> roles to prioritize
% postmodifier and numeric determiner roles.
% [[A+]]: "the five seat washed out"
%
two three four five six seven eight nine ten eleven twelve thirteen
fourteen fifteen sixteen seventeen eighteen nineteen
twenty twenty-one twenty-two twenty-three twenty-four
twenty-five twenty-six twenty-seven twenty-eight twenty-nine
thirty thirty-one thirty-two thirty-three thirty-four
thirty-five thirty-six thirty-seven thirty-eight thirty-nine
forty forty-one forty-two forty-three forty-four
forty-five forty-six forty-seven forty-eight forty-nine
fifty fifty-one fifty-two fifty-three fifty-four
fifty-five fifty-six fifty-seven fifty-eight fifty-nine
sixty sixty-one sixty-two sixty-three sixty-four
sixty-five sixty-six sixty-seven sixty-eight sixty-nine
seventy seventy-one seventy-two seventy-three seventy-four
seventy-five seventy-six seventy-seven seventy-eight seventy-nine
eighty eighty-one eighty-two eighty-three eighty-four
eighty-five eighty-six eighty-seven eighty-eight eighty-nine
ninety ninety-one ninety-two ninety-three ninety-four
ninety-five ninety-six ninety-seven ninety-eight ninety-nine
oh-one oh-two oh-three oh-four oh-five oh-six oh-seven oh-eight oh-nine
o-one o-two o-three o-four o-five o-six o-seven o-eight o-nine
zero-one zero-two zero-three zero-four zero-five zero-six zero-seven zero-eight zero-nine
four-score
several:
  NA+ or
  NMw- or
  ({EN-} & (NIfn+ or NItn-)) or
  NN+ or
  NW+ or
  ({EN- or NIc- or NA-} & (ND+ or NIn+ or
    ({{@L+} & DD-} & (Dmcn+ or (<noun-rel-p> & [<noun-main-p>]))))) or
  (NA- & {<noun-rel-p> & <noun-main-p>}) or
  (NA- & Xd- & TY- & Xc+)
  or Wa-
  or [[A+]];

oh.zero: (NA- & NA+);
zero.n: (NA- & NA+) or NN+ or Ds+ or (<noun-rel-s> & <noun-main-s>) or Wa-;

% the generic "number" category
% AN+ is needed for date-ranges
% FRACTIONS are simple fractions
% Ditto for fractions ...
% Not clear why we use Dmcn+ here, instead of allowing nouns to take ND-
% as effectively Dmcn and ND are the "same thing" more or less.
%
% ({ND+} & NIfn+) or (NItn- & {ND+}): "between 7:30AM and 9:30AM"
<NUMBERS> <FRACTION>:
  NMn-
  or ({EN-} & (({ND+} & NIfn+) or (NItn- & {ND+})))
  or NN+
  or [[NF+]]
  or [[AN+]]
  or ({EN- or NIc-} & (ND+ or NIn+ or OD- or
    ({{@L+} & DD-} & (Dmcn+ or (<noun-rel-p> & [<noun-main-p>])))))
  or ({Wd-} & EQt+)
  or EQt-
  or ((Wd- or NMn-) & NIa+)
  or Wa-;

% HMS-TIME consists of HH:MM:SS(AM|PM) type expressions
% and should probably have a narrower set of parse patterns than numbers in
% general.  e.g. should not have EQ links XXX todo -- fix this.
<HMS-TIME>: <NUMBERS> & {TZ+};

% Allowing postposed roman numerals only for now.
% e.g "Pope Pious XII"
<ROMAN-NUMERAL-WORDS>.rn:
  NMr-
  or ((Wd- or NMr-) & NIa+);

% nouns that look like roman numerals. Limited requirements to avoid
% excessive ambiguity.
<ROMAN-NUMERAL-WORDS>.n: {@MX+} & (<noun-main-s>);

% NMa-: Allow post-posed letter modifiers: e.g. "Vitamin A"
% Wd- & NIa+: Allow numbered, bulleted lists: "B: Press button firmly"
% Ju-: "It runs from T until the end"
%
% <marker-entity>: without this, the tokenizer destroys the upper-case,
%   when it occurs as the first letter in the sentence.
A.id B.id C.id D.id E.id F.id G.id H.id J.id K.id L.id M.id N.id
O.id P.id Q.id R.id S.id T.id U.id V.id W.id X.id Y.id Z.id:
  <marker-entity>
  or [NMa-]0.5
  or [(Wd- or NMa-) & NIa+]0.5
  or Ju-;

% Avoid having I.id interfere with pronoun I.
I.id: [[<marker-entity> or NMa- or (Wd- & NIa+)]];

% Variables: "suppose that X or Y is responsible."
A.n B.n C.n D.n E.n F.n G.n H.n J.n K.n L.n M.n N.n P.n Q.n R.n
S.n T.n U.n V.n W.n X.n Y.n Z.n:
  <marker-entity>
  or <noun-main-s>
  or <noun-and-s>;

% Given-name initials:
A. B. C. D. E. F. G. H. I. J. K. L. M. N.
O. P. Q. R. S. T. U. V. W. X. Y. Z. :
  <marker-entity>
  or ({G-} & G+);

% Days of month
% Note, however, this over-rides generic numbers in this range
% and so is a superset of the generic numbers disjuncts
% The following should match NUMBERS with the addition of "or TM-".
2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28
29 30 31:
<NUMBERS> or TM- or [[G+]];

% Ordinals - day-of-month expressions.
% Used only in expressions such as "December 2nd"
% Must use regex here as well, to avoid conflict with other regexes
first.ti second.ti third.ti fourth.ti fifth.ti sixth.ti seventh.ti eighth.ti
ninth.ti tenth.ti eleventh.ti twelfth.ti thirteenth.ti fourteenth.ti fifteenth.ti
sixteenth.ti seventeenth.ti eighteenth.ti nineteenth.ti twentieth.ti
twenty-first.ti twenty-second.ti twenty-third.ti twenty-fourth.ti twenty-fifth.ti
twenty-sixth.ti twenty-seventh.ti twenty-eighth.ti twenty-ninth.ti thirtieth.ti
thirty-first.ti <DAY-ORDINALS>.ti: TM-;

% For YEAR-DATE year numbers
% AN+ is given a cost, because <date-id> attaches incorrectly to
% measurements of various kinds, where the number is not actually a
% date, and ND is the preferred linkage.
% This needs cleanup, I think ...!?
<date-id>:
  NMd-
  or ({EN-} & (NIfn+ or NItn-))
  or NN+
  or [AN+]
  or Wa-
  or ((Xd- & TY- & Xc+) or TY-)
  or ({EN- or NIc-}
    & (ND+
      or OD-
      or ({{@L+} & DD-}
         & ([[Dmcn+]]
           or ((<noun-rel-x> or TA-) & (JT- or IN- or [[<noun-main-x>]]))))));

% Years w/o apostrophe: e.g. 47 Ford Fairlane or 57 Chevy
01 02 03 04 05 06 07 08 09: <date-id> or [[G+]];
% 10-31 are month-days, treated above.
32 33 34 35 36 37 38 39
40 41 42 43 44 45 46 47 48 49
50 51 52 53 54 55 56 57 58 59
60 61 62 63 64 65 66 67 68 69
70 71 72 73 74 75 76 77 78 79
80 81 82 83 84 85 86 87 88 89
90 91 92 93 94 95 96 97 98 99:
  <NUMBERS> or <date-id> or [[G+]];

% the DECADE-DATE regex matches 1950s 1950's 1950’s etc.
% A+: It's an old 50's love song
<DECADE-DATE>
'00s '10s '20s '30s '40s '50s '60s '70s '80s '90s
‘00s ‘10s ‘20s ‘30s ‘40s ‘50s ‘60s ‘70s ‘80s ‘90s
00's 10's 20's 30's 40's 50's 60's 70's 80's 90's:
  ({TA-} & DG- & (IN- or [[<noun-main-x>]]))
  or A+;

% year numbers
% 1910 1911 1912 1913 1914 1915 1916 1917 1918 1919
<YEAR-DATE>: <NUMBERS> or <date-id> or [[G+]];

% Years: e.g. '47 Ford Fairlane or '57 Chevy
'00 '01 '02 '03 '04 '05 '06 '07 '08 '09
'10 '11 '12 '13 '14 '15 '16 '17 '18 '19
'20 '21 '22 '23 '24 '25 '26 '27 '28 '29
'30 '31 '32 '33 '34 '35 '36 '37 '38 '39
'40 '41 '42 '43 '44 '45 '46 '47 '48 '49
'50 '51 '52 '53 '54 '55 '56 '57 '58 '59
'60 '61 '62 '63 '64 '65 '66 '67 '68 '69
'70 '71 '72 '73 '74 '75 '76 '77 '78 '79
'80 '81 '82 '83 '84 '85 '86 '87 '88 '89
'90 '91 '92 '93 '94 '95 '96 '97 '98 '99: <date-id> or [[G+]];


1:
  NMn-
  or ({EN-} & (NIfn+ or NItn-))
  or NN+
  or [[NF+]]
  or ({EN- or NIc-} & (ND+ or NIm+ or OD- or
    ({{@L+} & DD-} & (D**n+ or (<noun-rel-x> & [<noun-main-x>])))))
  or TM-
  or NSn+
  or ((Wd- or NMn-) & NIa+)
  or ({Wd-} & EQt+) or EQt-;
%%%%% or [[G- & (({MXs+} & <noun-main-s>) or G+ or AN+ or YS+)]]

0:
  NMn-
  or ({EN-} & (NIfn+ or NItn-))
  or NN+
  or [[NF+]]
  or ({EN- or NIc-} & (ND+ or NIn+ or OD- or
    ({{@L+} & DD-} & (Dmcn+ or [[Ds+]] or (<noun-rel-p> & [<noun-main-p>])))))
  or ((Wd- or NMn-) & NIa+)
  or NSn+;
%%%%% or [[G- & (({MXs+} & <noun-main-s>) or G+ or AN+ or YS+)]]

% TODO: no numbers or related expressions below this point take the new NM
% connector, although e.g. "point 1/2" would appear reasonable. Go through
% these and add NM- analogously to other numbers as applicable.

twenties thirties forties fifties sixties seventies eighties nineties
hundreds.cnt:
  ({NA-} & {TA-} & DG- & (IN- or [[<noun-main-x>]])) or
  <mass-noun>;

% teens could be above or teenagers (words.n.2)
teens:
  ({TA-} & DG- & (IN- or <noun-main-x>)) or
  <mass-noun>;

hundred thousand half-thousand million half-million quarter-million
billion half-billion quarter-billion trillion half-trillion
quarter-trillion dozen half-dozen
bajillion bazillion gadzillion gagillion gajillion gazillion godzillion
jillion.a jizillion kabillion kajillion katrillion killion umptillion
zillion.n:
  NN- & (
    NNy+
    or NMw-
    or NA+
    or ({EN- or NIc-} & (
      ND+
      or NIn+
      or OD-
      or ({{@L+} & DD-} &  (Dmcn+ or (<noun-rel-p> & <noun-main-p>))))));

half_a_dozen half_a_million:
({EN- or NIc-} & (ND+ or NIn+ or OD- or ({{@L+} & DD-} &
(Dmcn+ or (<noun-rel-p> & <noun-main-p>)))));

%  Dmcx-: prevents linkage to DMcn "*5 millions attended" but "Many millions attended"
dozens scores.a hundreds.a thousands millions billions trillions
bajillions bazillions gadzillions gagillions gajillions gazillions
godzillions jillions jizillions kabillions kajillions katrillions
killions umptillions zillions.a:
  [{DD- or Dmcx-} & <noun-rel-p> & <noun-main-p>]
  or (OFd+ & (ND+ or NIn+ or Dm+));

% OFd+ & Dm+: "tens of years ago ..."
tens:
  ({DD-} & OF+ & <noun-main-p>)
  or (OFd+ & Dm+);

% XXX FIXME: noun-main-x has O- in it, and that's just wrong, here.
1/2 3/4 2/3 1/3 1/4 1/8 3/8 5/8 7/8 ½ ⅓ ⅔ ¼ ¾ ⅛ ⅜ ⅝ ⅞:
  ({NNx-} & NNy+)
  or NF+
  or NIe+
  or ({NNx-} & {EN- or NIc-}
    & (ND+ or NIn+ or OD-
      or ({DD-} & ([[Ds+]] or Dmcn+ or (<noun-rel-x> & <noun-main-x>)))));

and_a_half:
  (NW- or NSn- or ND-) &
    (NNy+ or ({EN- or NIc-} & (ND+ or NIn+ or ({DD-} &
    (Dmcn+ or (<noun-rel-p> & <noun-main-p>))))));

quarter.i:
  NS- & {EN-} & (NF+ or (<noun-rel-x> & <noun-main-x>));
thirds.m fourths.m quarters.m fifths.m sixths.m sevenths.m eighths.m
ninths.m tenths.m:
  NW- & {EN-} & (NF+ or (<noun-rel-x> & <noun-main-x>));

first.a:
  L- or Pa- or E+ or TT+ or A+ or [Jp-]
  or <adv-as>
  or ({Xc+ & {Xd-}} & dCO+)
  or ((DD- or [[NSa-]]) & <noun-rel-x> & {<ton-verb>} & <noun-main-x>);

last.a dead_last dead_fucking_last DFL:
  L-
  or Pa-
  or <adv-as>
  or ({Xc+ & {Xd-}} & dCO+)
  or DTi+
  or TT+
  or (DD- & <noun-rel-x> & {<ton-verb>} & <noun-main-x>)
  or A+
  or [Jp-];

second.a:
  L- or Pa- or <adv-as> or ({Xc+ & {Xd-}} & dCO+) or
  (DD- & <noun-rel-x> & {<ton-verb>} & <noun-main-x>) or NR+ or A+;

% This uses the L link for superlatives, but leads to strange parses:
% "We celebrated their eleventh anniversary" parses differently
% than "tenth anniversary". XXX this should be fixed, I suppose ...

% A+: "fifteenth century Italy"
% Jp-: "Mike finished in first place, and John in third."
third.a fourth.a fifth.a sixth.a seventh.a eighth.a ninth.a tenth.a :
  L- or Pa- or NR+ or A+ or Jp-
  or <adv-as>
  or ({Xc+ & {Xd-}} & dCO+)
  or (NS- & {EN-} & NF+)
  or (((NS- & <noun-rel-x> & {EN-}) or (DD- & <noun-rel-x> & {<ton-verb>})) & <noun-main-x>);

% NS-: "I gave him a third of the loot."
eleventh.a twelfth.a thirteenth.a fourteenth.a fifteenth.a
sixteenth.a seventeenth.a eighteenth.a nineteenth.a
twentieth.a
twenty-first.a twenty-second.a twenty-third.a
twenty-fourth.a twenty-fifth.a twenty-sixth.a
twenty-seventh.a twenty-eighth.a twenty-ninth.a
thirtieth.a
thirty-first.a thirty-second.a thirty-third.a
thirty-fourth.a thirty-fifth.a thirty-sixth.a
thirty-seventh.a thirty-eighth.a thirty-ninth.a
fourtieth.a
fourty-first.a fourty-second.a fourty-third.a
fourty-fourth.a fourty-fifth.a fourty-sixth.a
fourty-seventh.a fourty-eighth.a fourty-ninth.a
fiftieth.a
fifty-first.a fifty-second.a fifty-third.a
fifty-fourth.a fifty-fifth.a fifty-sixth.a
fifty-seventh.a fifty-eighth.a fifty-ninth.a
sixtieth.a
sixty-first.a sixty-second.a sixty-third.a
sixty-fourth.a sixty-fifth.a sixty-sixth.a
sixty-seventh.a sixty-eighth.a sixty-ninth.a
seventieth.a
seventy-first.a seventy-second.a seventy-third.a
seventy-fourth.a seventy-fifth.a seventy-sixth.a
seventy-seventh.a seventy-eighth.a seventy-ninth.a
eightieth.a
eighty-first.a eighty-second.a eighty-third.a
eighty-fourth.a eighty-fifth.a eighty-sixth.a
eighty-seventh.a eighty-eighth.a eighty-ninth.a
ninetieth.a
ninety-first.a ninety-second.a ninety-third.a
ninety-fourth.a ninety-fifth.a ninety-sixth.a
ninety-seventh.a ninety-eighth.a ninety-ninth.a:
  Pa- or NR+ or A+ or Jp-
  or <adv-as>
  or ({Xc+ & {Xd-}} & dCO+)
  or (NS- & {EN-} & NF+)
  or (((NS- & <noun-rel-x> & {EN-}) or (DD- & <noun-rel-x> & {<ton-verb>})) & <noun-main-x>);

% Miscellaneous ordinal numbers, adjectival usage
% prefer G+ over A+ in general, as these are typically parts of names.
% ({Ds-} & AJla+): "he is in either the X or the Y battalion"
1º.a 2º.a 3º.a 4º.a 5º.a 6º.a 7º.a 8º.a 9º.a
<DAY-ORDINALS>.a <ORDINALS>.a:
  Pa- or
  MVa- or
  ({Xc+ & {Xd-}} & dCO+) or
  (DD- & <noun-rel-x> & {<ton-verb>} & <noun-main-x>) or
  NR+ or
  G+ or
  [A+] or
  ({Ds-} & dAJla+) or
  ({Ds-} & dAJra-);

% "Next on our list..." are ordinals.
% XXX should be converted to regex...
% Note also another list of ordinals below, used for time expressions.
first.ord next.ord last.ord second.ord third.ord fourth.ord fifth.ord
sixth.ord seventh.ord eighth.ord ninth.ord tenth.ord eleventh.ord
twelfth.ord thirteenth.ord fourteenth.ord fifteenth.ord sixteenth.ord
seventeenth.ord eighteenth.ord nineteenth.ord
twentieth.ord
twenty-first.ord twenty-second.ord twenty-third.ord
twenty-fourth.ord twenty-fifth.ord twenty-sixth.ord
twenty-seventh.ord twenty-eighth.ord twenty-ninth.ord
thirtieth.ord
thirty-first.ord thirty-second.ord thirty-third.ord
thirty-fourth.ord thirty-fifth.ord thirty-sixth.ord
thirty-seventh.ord thirty-eighth.ord thirty-ninth.ord
fourtieth.ord
fourty-first.ord fourty-second.ord fourty-third.ord
fourty-fourth.ord fourty-fifth.ord fourty-sixth.ord
fourty-seventh.ord fourty-eighth.ord fourty-ninth.ord
fiftieth.ord
fifty-first.ord fifty-second.ord fifty-third.ord
fifty-fourth.ord fifty-fifth.ord fifty-sixth.ord
fifty-seventh.ord fifty-eighth.ord fifty-ninth.ord
sixtieth.ord
sixty-first.ord sixty-second.ord sixty-third.ord
sixty-fourth.ord sixty-fifth.ord sixty-sixth.ord
sixty-seventh.ord sixty-eighth.ord sixty-ninth.ord
seventieth.ord
seventy-first.ord seventy-second.ord seventy-third.ord
seventy-fourth.ord seventy-fifth.ord seventy-sixth.ord
seventy-seventh.ord seventy-eighth.ord seventy-ninth.ord
eightieth.ord
eighty-first.ord eighty-second.ord eighty-third.ord
eighty-fourth.ord eighty-fifth.ord eighty-sixth.ord
eighty-seventh.ord eighty-eighth.ord eighty-ninth.ord
ninetieth.ord
ninety-first.ord ninety-second.ord ninety-third.ord
ninety-fourth.ord ninety-fifth.ord ninety-sixth.ord
ninety-seventh.ord ninety-eighth.ord ninety-ninth.ord
<DAY-ORDINALS>.ord <ORDINALS>.ord :
  (Wd- & {M+} & Ss*o+);

% TODO: un-parenthesized cases, e.g.
% - preparations of 5 x 10(8) cfu/ml are made
% - the strength was in the order of gerE > cotD > yfhP P2 > yfhP P1
% also remember "-->"

A.eq B.eq C.eq D.eq E.eq F.eq G.eq H.eq I.eq J.eq K.eq L.eq M.eq
N.eq O.eq P.eq Q.eq R.eq S.eq T.eq U.eq V.eq W.eq X.eq Y.eq Z.eq
a.eq b.eq c.eq d.eq e.eq f.eq g.eq h.eq i.eq j.eq k.eq l.eq m.eq
n.eq o.eq p.eq q.eq r.eq s.eq t.eq u.eq v.eq w.eq x.eq y.eq z.eq:
  EQt+ or EQt-;

fiscal.i: TY+ & <noun-main-s>;

or_so: ND- & {{@L+} & DD-} & (Dmcn+ or (<noun-rel-p> & <noun-main-p>));

% Allows parsing of "dollars per day" or "mL/sec" but is somewhat
% inconsistent with the equation parsing otherwise described below.
% XXX overall, eqn parsing could be strengthened.
per "/.per": Us+ & Mp-;


%VERBS

% The Xd- link to the comma is strongly required, here. Making the comma
% optional will lead to a broad variety of bad/insane linkages, simply
% because @MV+ is soe widely used.
%
% Xd- & (Xp+ or RW+) & MVg-: "John left, carrying a dog"
<MX-PHRASE>: Xd- & (Xc+ or Xp+ or RW+) & (MX*p- or MVg-);

<OPENER>: {Xd-} & Xc+ & [dCOp+]0.2;

% <coord>: connects to coordinating conjunction.
<coord>: VC-;

% These are the verb-form expressions for ordinary verbs.
%
% The general patterns here are:
% <verb-wall> : links verb to wall or to controlling phrase.
% <verb-s>    : links verbs to singular subjects
% <verb-pl>   : links verbs to plural subjects
% <verb-i>    : links to infinitive
% <verb-pl,i> : to plural subjects or infinitives
% <verb-sp>   : to singular or plural subject
% <verb-pp>   : to past-participles
% <verb-sp,pp>: to singular or plural subject or past-participles
% <verb-pg>   : to gerunds
% <verb-si>   : subject inversion
% <verb-co>   : optional subjects to imperatives (commands)
% <verb-sic>  : imperative-subject inversion

% <verb-wall>: these connect to the head verb:
% WV connects the wall to the head-verb,
% CV connects the dominating clause to the head verb of the dependent clause.
% IV connects infinitives to the head-verb
% VC connects the head-word to a subsequent coordinating conjunction.
%
% There are some other such connectors that don't quite fit this pattern:
% AF, Z, and in many cases B (for example TOt+ & B+). For this reason, we
% have to have a costly null [()] below, although we would really really
% like to get rid of it.  But that would take a lot of Z and B and AF link
% fiddling about, so we have to live with this for now.
%
% Also: CP-, Eq+ and COq+ all connect to verbs, and are so disjoined
% with <verb-wall>
%
<verb-wall>: ((dWV- or dCV- or dIV-) & {VC+}) or [()];
% <verb-wall>: (dWV- or dCV- or dIV-) & {VC+};

<mv-coord>: {@MV+} & {VC+};

% <of-coord>: "You were informed of this when?"
<of-coord>: OF+ & {{Xc+} & QN+} & <mv-coord>;

% When we are done, remove the option costly NULL below.
<WALL>: hWV+ or [[()]];
% <WALL>: hWV+;

% Pv- & <verb-wall>: "a historic new law was passed"
% Pv- & no wall: "John felt vindicated"
% The problem here is that for passives (i.e. to-be), The Pv should get the wall
% but in the other cases it should not. We could/should tighten this up by using
% Pvp+ on to-be, using Pvv for the others, and demand the wall only for Pvp.
% XXX FIXME, the above needs fixing.
%
% <verb-pp>: PP- & WV-: "I have seen it".
% <verb-pg>: Pg- is naked, no verb-wall: "I like eating bass."
% <verb-pg>: Sg- & Pg-: "I feel him breathing down my back"
% Wg-: "Rode a bike today"
%
% XXX FIXME: for certain transitive verbs, we really want verb-ico to be
% in the form (I- & B- & <verb-wall>)  for example: "that I did not know".
%
<verb-s>:    {@E-} & ((Ss- & {hPFt-} & <verb-wall>) or (RS- & Bs-) or Wg-);
<verb-pl>:   {@E-} & ((Sp- & {hPFt-} & <verb-wall>) or (RS- & Bp-) or Wg-);
<verb-sp>:   {@E-} & ((S- & {hPFt-} & <verb-wall>) or (RS- & B-) or Wg-);
<verb-pp>:   {@E-} & PP- & (<verb-wall> or [()]);
<verb-pg>:   {@E-} & (({[Sg-]-0.2} & Pg-) or Mg- or Wg-);
<verb-sp,pp>: <verb-sp> or <verb-pp>;

% Pv- & OFj+: "knowledge was gained of the activities"
<verb-pv>:
  ({@E-} & ((Pv- & {hPFt-} & {<verb-wall>} & {OFj+}) or Mv-) & <mv-coord>)
  or ({EA-} & Wx-);
<verb-pvk>:
  ({@E-} & ((Pv- & {hPFt-} & {<verb-wall>} & {K+}) or Mv-) & <mv-coord>)
  or ({EA-} & Wx-);
<verb-pv-b>: {@E-} & ((Pv- & {hPFt-} & {<verb-wall>}) or Mv- or ({EA-} & Wx-));

% used only in "as <past-participle>" constructions, which behave
% kind of like manner adverbial phrases.  Only certain verbs have
% this "manner" format, but they are not well-sorted out.
%
% Sa*v-: "He did it as expected"
% Sa*v- & MXsr-: "the movie, as filmed, is too long"
%        The cost on MXsr+ is to give MVs preference for
%        "She sang well, as planned"
% {MVz+}: "the man, as acquiescing as he was, set a bad precedent."
% Pv- & CV- & MXsr-: "The accused, as shall be proven, is innocent"
% S- & CV- & MXsr-: "The accused, as I suspected, is innocent"
% I*x- & CV- & MXsr-: "The accused, as I will show, is innocent"
%     The x on I*x blocks I*v, so that  Pv- is used.
<verb-manner>:
  ((Sa*v- or EAy- or ({E-} & Pv- & CV-)) & {Xd-} & {[MXsr-]0.1 & {MVz+}} & {Xc+})
  or ({E-} & S- & CV- & {Xd-} & MXsr- & {Xc+})
  or ({E-} & I*x- & CV- & {Xd-} & MXsr- & {Xc+});

% [A+]0.5 cost: "He was driven home" should NOT use driven.v as adjective!
% From what I can tell, <verb-manner> can be used anywhere that
% <verb-adj> can be... except for said.v-d
<verb-adj>:
  (({@E-} or {@EA-} or {@AN-}) & ([A+]0.5 or Pa-))
  or <verb-manner>;

% Wi- & {NM+}: imperative numbered lists: "Step 5. Do this."
% S**i- & Wi-: "John, stop that!"; the comma is mandatory.
% EI- & Wi-: "Hey, stop that!"
% CO- & Wi-: "Finally, move it back".
% [CO-]: cost because <verb-pl,i> & O+ occurs in many verbs, and
%        allows a really weird subject-object inversion to proceed:
%        e.g. "In the corner lay it" with it as object. That's just
%        wrong... but this requires lots of places to fix.
% VJrpi-: "I aim to do something and also to help."
% I & [()]: costly, if there is no wall, but when does this happen?
%       I'm confused, examples are needed... verb-pl,i is used everywhere...
%       I want the wall, e.g. for "Did you see in which room?"
%       which uses subject inversion and "did see" for the infinitive.
% Qa- & <verb-wall>: Why-action questions: "why go there?"
<verb-i>:    {@E-} & I- & (<verb-wall> or VJrpi-);
<verb-co>:   {@E-} & {Xd- & (EI- or S**i-)} & Wi-;
<verb-sic>:  {@E-} & Wi- & Xc+ & SI*i+ & {Xc+};
<verb-why>:  Qa- & <verb-wall>;

% Sj- & I*j-: subject of bare infinitive: "you should hear him talk!"
% <verb-wall> or [()]0.5: This is used with <b-minus> which already
%                         has a <verb-wall> in it (maybe) so we don't
%                         need a second one. So don't be harsh.
<verb-ico>:  {@E-} & ((I- & (<verb-wall> or VJrpi- or [()]0.5)) or
                      (Sj- & I*j-) or
                      <verb-why> or
                      ({(Xd- & (EI- or S**i-)) or [{Xd-} & hCO-]} & Wi- & {NM+}) or
                      (Wi- & Xc+ & SI*i+ & {Xc+})
                     ) & {@E-};
<verb-pl,i>:  <verb-pl> or <verb-ico>;

<verb-si>:   {@E-} & hPF- & {<verb-wall>} & hSI+;
<verb-sip>:  {@E-} & hPF- & {<verb-wall>} & hSIp+;

% <b-minus> is meant to be a generic replacement in the naked B- in
% many transitive verb constructions.  For questions, we need to force
% a verb-wall connector; this is what the (B*w- or B*m-) & <verb-wall>
% part does. For the other B- forms, we don't need the wall.  To force
% the wall, we just list all the others.
% XXX FIXME: the verb-i above may need to be changed to make the wall
% optional, because "Which dog did you chase" requires a I- & B*m- & WV-
% By contrast, "Who do you think Bill will bring?" requires a
% I- & CV- & B*w- & WV- on bring: that is, two walls.
%
% B*d-: "Whatever you want to do is fine" can't have a wall.
% B*w- "that, I did not know" needs a wall, but
%      "Pizza, which most people love, is not very healthy" can't have a wall.
%      so the wall on B*w- is optional.
% XXX FIXME -- most of the naked B- below should probably be <b-minus>

<b-minus>:
  B*d- or B*j-
  or (B*w- & (<verb-wall> or [()]0.5))
  or (B*m- & <verb-wall>);

<verb-ge-nos>:
  {@E-} & (
    <MX-PHRASE>
    or <OPENER>
    or ({[DP-]} & SIs*g-)
    or [DP- & J-]
    or [<fronted>]);

<verb-ge>:
  <verb-ge-nos>
  or ({@E-} & {[DP-]} & Ss*g+ & <CLAUSE>);

% ({[[Ds-]]} & OF+) : "a running of the bulls" "a polishing of prose"
% AJ: allows use of gerunds as adjectives: "described as smooth and obliging"
% <noun-and-u>: allows use of gerunds as nouns.
<verb-ge-d>:
  (<noun-modifiers> &
    (Dmu- or ((Dmu- or [[Ds-]]) & OF+) or ()) &
    ((<noun-rel-s> & {@MXs+} &
      ((Ss+ & <CLAUSE>) or SIs- or Os- or J-)) or
      dAJra- or dAJla+ or
      <noun-and-u>)) or
  [[AN+]];

<verb-pg,ge>:
  {@E-} & (
    <MX-PHRASE>
    or <OPENER>
    or ({[DP-]} & ((Ss*g+ & <CLAUSE>) or SIs*g-))
    or [DP- & J-]
    or [<fronted> & {@MV+}]
    or Mg-
    or Pg-
    or Wg-);

<verb-phrase-opener>:
  {@E-} & {@MV+} & (
    <MX-PHRASE>
    or <OPENER>
    or [<fronted> & {@MV+}]);

% Relative clause, or question.
% Qw- & <verb-wall>: "Where are they?" -- verb must connect to wall.
%      ([<verb-wall>]-0.1 or ()): prefer wall to participle.
% Qe-: "How many times did you do it?"
% Qd-: "Does he drink?" -- Qd connects directly to wall.
% {CO-} & Qd-: "By the way, does he drink?"
% Iq-: "The big question is did he do it?"
% Xd- & Iq-: "The big question is, did he do it?"
<verb-rq>:
  Rw-
  or ({{Xd-} & Iq-}
      & (Qd- or Qp- or ((Qw- or Qe-) & ([<verb-wall>]-0.1 or ()))))
  or [()];
% Just like above, but no aux, should always be anded with I+.
% The idea here is that the verb on the other end of the I+ will
% connect to the wall.
<verb-rq-aux>:
  Rw- or (({{Xd-} & Iq-} or {hCOw-}) & (Qd- or Qw- or Qp- or Qe-)) or [()];

% These are the verb-form expressions for special verbs that can take
% filler-"it" as a subject.

<verb-s-pl,i>:
  {@E-} & (((Sp- or If-) & <verb-wall>)
     or (RS- & Bp-) or <verb-co> or <verb-why>);
<verb-s-s>: {@E-} & (((Ss- or SFsi-) & <verb-wall>) or (RS- & Bs-));
<verb-s-sp,pp>: {@E-} & (((S- or SFsi- or PPf-) & <verb-wall>) or (RS- & B-));
<verb-s-sp>: {@E-} & (((S- or SFsi-) & <verb-wall>) or (RS- & B-));
<verb-s-pp>: {@E-} & PPf- & <verb-wall>;
<verb-s-pg>: {@E-} & (Pgf- or Mg-);
<verb-s-pv>: {@E-} & ((Pvf- & <verb-wall>) or Mv-) & <mv-coord>;
<verb-s-pv-b>: {@E-} & ((Pvf- & <verb-wall>) or Mv-);

% These are the verb-form expressions for special verbs that can take
% either filler-"it" or filler-"there" as a subject.
% These are used almost exclusively with auxiliary verbs.
% This is why they don't have & <verb-wall> in them: we don't want the
% auxiliary attaching to the wall, we want only the main verb doing this.
% The Ss- or Sp- prevent attachments to Sa- for "as.e" phrases.
<verb-x-pl,i>: {@E-} & (Sp- or SFp- or If- or (RS- & Bp-) or <verb-co>);
<verb-x-s>: {@E-} & (Ss- or SFs- or (RS- & Bs-));
<verb-x-s,u>: {@E-} & (Ss- or SFs- or SFu- or (RS- & Bs-));
<verb-x-sp,pp>: {@E-} & (Ss- or Sp- or SF- or PPf- or (RS- & B-));
<verb-x-sp>: {@E-} & (Ss- or Sp- or SF- or (RS- & B-) or Wg-);
<verb-x-pp>: {@E-} & PPf- & <verb-wall>;
<verb-x-pg>: {@E-} & (Pgf- or Mg- or Wg-);

% No verb-wall for <verb-x-pg>: "Is there going to be a problem?"
<verb-x-pg,ge>:
  <verb-x-pg>
  or ({@E-} & (
    <MX-PHRASE>
    or <OPENER>
    or ({[DP-]} & ((Ss*g+ & <CLAUSE>) or SIs*g-))
    or [DP- & J-]
    or [<fronted> & {@MV+}]) & <verb-wall>);

% Almost identical to the above, except that the verb attaches to the
% wall. We cannot use verb-s for this, since the SFsi prevents the parse
% of sentences like  "there appears to be a problem".
% If- blocks the Ix+ on would, be
<verb-y-pl,i>: {@E-} & (((Sp- or SFp- or If-) & <verb-wall>) or (RS- & Bp-) or <verb-co>);
<verb-y-s>: {@E-} & (((Ss- or SFs-) & <verb-wall>) or (RS- & Bs-));
<verb-y-s,u>: {@E-} & (((Ss- or SFs- or SFu-) & <verb-wall>) or (RS- & Bs-));
<verb-y-sp,pp>: {@E-} & (((S- or SF- or PPf-) & <verb-wall>) or (RS- & B-));
<verb-y-sp>: {@E-} & (((S- or SF-) & <verb-wall>) or (RS- & B-));

% Almost like above, but without the RS- & B-
<verb-z-s,u>: {@E-} & (Ss- or SFs- or SFu-) & <verb-wall>;

% conjoined verbs.
% VJr == left
% VJl == right
% VJd == ditransitive
%
% VJ*s == singular subject
% VJ*p == plural subject
% VJ*g == conjoined gerunds "He was running and jumping"
% VJ*h == past participle (PP- link) "He had run and jumped"
%
% The following control whether the conjunction can take an object.
% The conjunction should take an object if both verbs are transitive,
% e.g. "I saw and greeted Sue", which should parse as
% "I (saw and greeted) Sue".
% VJ**i == intransitive
% VJ**t == transitive
%
% s == singular, pl == plural, sp == singular or plural
% g == gerund
<verb-and-s->: {@E-} & dVJrs-;
<verb-and-s+>: {@E-} & dVJls+;
<verb-and-pl->: {@E-} & dVJrp-;
<verb-and-pl+>: {@E-} & dVJlp+;
<verb-and-sp->: {@E-} & dVJr-;
<verb-and-sp+>: {@E-} & dVJl+;
% <verb-and-sp-i-> is use only with auxiliaries, and thus it needs the
% Ift- connector for predicative adjectives:
% "he is known to get frustrated and to become miserable"
<verb-and-sp-i->: {@E-} & {Ift-} & dVJr*i-;
<verb-and-sp-i+>: {@E-} & dVJl*i+;
<verb-and-sp-t->: {@E-} & dVJr*t-;
<verb-and-sp-t+>: {@E-} & dVJl*t+;
<verb-and-pg->: {@E-} & dVJrg-;
<verb-and-pg+>: {@E-} & dVJlg+;
<verb-and-had->: {@E-} & (dVJrh- or dMJrv-);
<verb-and-had+>: {@E-} & (dVJlh+ or dMJlv+);

<verb-and-sp-t>:
  <verb-and-sp-t-> or
  <verb-and-sp-t+>;

% Verb macros for automatically conjoining verbs.
%
% Many of these use a cost on $1 to encourage any MV links to attach to
% the "and.j-v" instead of to the individual verb.  Unfortunately, this
% can often be too broad: so, we *want* transitive objects to attach
% to the local verb.  The appropriate fix seems to be to remove the cost
% here, and add a tiny cost to those MV's that are being incorrectly attached.
% Or maybe lower the cost here?  We already have a 0.2 case, below...
% XXX TODO: do the above, as they show up...
%
% plural-or-infinitive macro;
%   "Scientists sometimes may repeat experiments or use groups."
%   Want "groups" to connect to "use", not "and".
% (($1) or [()]) & <verb-and-pl+>):
%    "they might supplement or replace anticoagulants"
%    The first verb is expecting an object, but there isn't one.
% ([$1]0.1 or [()]) & <verb-and-pl+>):
%    "I love cats and dogs"
%    Do not be too eager in attaching an object to the first verb.
% ({@E-} & hXd- & dWi- & ($1) & hXc+):
%    Parenthetical phrases: "(please refer to the list below)"
%    Getting the parenthesis cannot be achieved by hacking <verb-ico>
%    or <verb-pl,i> and must be done here.
% (dWi- & ($1) & Xc+ & SI*i+ & {Xc+}): "Bring out the Ring, Frodo!"
% Sp- & SI*i+: "you hold my beer, John"
%        but w/cost to avoid bad parse of "They shelter in huts, woods and rocks"
% Xd- & EI- & SI*i+: "hey, thank you, sir!"
define(`VERB_PLI',`'
  ((<verb-pl,i> & ($1)) or
  ({@E-} & {([Sp-] or (Xd- & EI-))} & dWi- & ($1) & Xc+ & SI*i+ & {Xc+} & {@MV+}) or
  ({@E-} & hXd- & dWi- & ($1) & hXc+) or
  (<verb-and-pl-> & (($1) or ())) or
  (([$1]0.1 or [()]) & <verb-and-pl+>)))

% Generic singular intransitive form
define(`VERB_x_S',`'
  (($1 & ($2)) or
  (<verb-and-s-> & ([$2] or ())) or
  (($2) & <verb-and-s+>)))

% singular present tense macro; same comments as above...
define(`VERB_S_I',`'VERB_x_S(<verb-s>,$1))

% Generic intransitive form
define(`VERB_x_I',`'
  (($1 & ($2)) or
  (<verb-and-sp-i-> & ([$2] or ())) or
  (($2) & <verb-and-sp-i+>)))

% Generic transitive form
% ([$2]0.2 or ()): we want the modifiers to act on the conjunction, usually:
% for example: "We neither ate nor drank for three days"
define(`VERB_x_T',`'
  (($1 & ($2)) or
  (<verb-and-sp-i-> & ([$2]0.2 or ())) or
  (($2) & <verb-and-sp-i+>) or
  <verb-and-sp-t>))

% present tense, but allows transitive connections to 'and'
define(`VERB_S_T',`'VERB_x_T(<verb-s>, $1))

% past tense macro, intransitive variation
define(`VERB_SPPP_I',`'VERB_x_I(``<verb-sp,pp>'',$1))

% past tense macro, transitive variation
define(`VERB_SPPP_T',`'VERB_x_T(``<verb-sp,pp>'', $1))

% Same as above, but without the PP link
define(`VERB_SP_I',`'VERB_x_I(<verb-sp>,$1))

define(`VERB_SP_T',`'VERB_x_T(<verb-sp>, $1))

% as above but for past participles
define(`VERB_PP',`'
  ((<verb-pp> & ($1)) or
  (<verb-and-had-> & ([$1] or ())) or
  (($1) & <verb-and-had+>)))

% the filler-it  variation of the above rules.
define(`VERB_S_PLI',`'VERB_x_T(``<verb-s-pl,i>'', $1))

% This may allow overly broad 'and' constructions.
define(`VERB_X_S',`'VERB_x_S(<verb-x-s>,$1))

% This may allow overly broad 'and' constructions.
% I haven't completely verified this one, it may be buggy..
define(`VERB_X_PLI',`'VERB_x_I(``<verb-x-pl,i>'',$1))

% This may allow overly broad 'and' constructions.
define(`VERB_Y_S',`'VERB_x_S(<verb-y-s>,$1))

define(`VERB_Y_SPPP',`'VERB_x_I(``<verb-y-sp,pp>'',$1))

% This may allow overly broad 'and' constructions.
% I haven't completely verified this one, it may be buggy..
define(`VERB_Y_PLI',`'VERB_x_I(``<verb-y-pl,i>'',$1))

define(`VERB_S_S',`'VERB_x_T(<verb-s-s>,$1))
define(`VERB_S_SP',`'VERB_x_T(<verb-s-sp>,$1))
define(`VERB_S_SPPP',`'VERB_x_T(``<verb-s-sp,pp>'',$1))

% AUXILIARY VERBS

% O+ & <verb-wall>: "did" is not an auxiliary, and so needs the wall.
% O+ & QN+: "you did that when?"
% QN+: "you did what?"
<vc-do>:
  ((<b-minus>
    or (O+ & {@MV+} & <verb-wall> & {{Xc+} & QN+})
    or ({@MV+} & <verb-wall> & QN+)
    or [[@MV+ & O*n+]]
    or Vd+
    or ({N+} & (CX- or [[()]]))) & <mv-coord>)
  or ({N+} & I*d+);

% Below uses <verb-and-sp-i> but maybe <verb-and-pl> is better?
% I- & CV-: "What did John say you should do?"
% Naked I*d-: "How do you do?", "what is there to do?"
% I*t- & O+: forces use of object in to-do expressions.
%            "Are you really going to do it to them?"
% VJrpi-: "I aim to help and also to do something"
% SIp+ & N+: "Do you not want to know?"
% naked SIp+: "do you?"
% Wi- & I*d+ & Xc+ & SI*i+: "please do tell, John"; comma is required!
%
% I*d- & <b-minus> & O+: "How many years did you do it?"
% Hang on, the above also enables a zero-word parse ...
% "what are the chances [that] Sherlock could do it?"
%
do.v:
  ({@E-} & (Sp- or SFp- or (RS- & Bp-) or <verb-co> or <verb-why>) & <vc-do>)
  or (<verb-and-sp-i-> & ([<vc-do>] or ()))
  or (<vc-do> & <verb-and-sp-i+>)
  or ((SIp+ or SFIp+) & {N+} &
     ((<verb-rq-aux> & {N+} & {I*d+ or <verb-wall>}) or CQ-))
  or (<verb-co> & {I*d+} & Xc+ & SI*i+ & {Xc+})
  or ({@E-} & I*t- & O+ & IV- & <mv-coord>)
  or ({@E-} & I- &
    (<b-minus> or O+ or [[@MV+ & O*n+]] or CX-) &
    <mv-coord> &
    (<verb-wall> or VJrpi-))
  or ({@E-} & I*d- & <b-minus> & O+)
  or ({@E-} & I*d- & <verb-wall>);

% Ss- & <verb-wall>: "so it does!"
% Ss- & <verb-wall> & @MV+: "he does as he pleases."
does.v:
  VERB_X_S(<vc-do>)
  or ({@E-} & Ss- & <verb-wall> & <mv-coord>)
  or ((SIs+ or SFIs+) & ((<verb-rq-aux> & {N+} & {I*d+ or <verb-wall>}) or CQ-));

% Ss- & <verb-wall> & @MV+: "he did as he pleased."
% <verb-x-sp> & <verb-wall>: "I sure wish I did"
did.v-d:
  (<verb-x-sp> & <vc-do>)
  or (<verb-x-sp> & <verb-wall>)
  or ({@E-} & Ss- & <verb-wall> & <mv-coord>)
  or (<verb-and-sp-i-> & <vc-do>) or (<vc-do> & <verb-and-sp-i+>)
  or ((SI+ or SFI+) & ((<verb-rq-aux> & {N+} & {I*d+ or <verb-wall>}) or CQ-));
%
% XXX why not <vc-do> here ?
% <verb-pv-b>: "I want it done." "I want the job done"
% Os+ & @MV+: "I've done that chore many times"
% O+ & I*j+: "I would have done that, had you been there"
<vc-done>:
  <b-minus>
  or (O+ & <mv-coord> & {I*j+})
  or [[@MV+ & O*n+]]
  or Vd+;

% <verb-pv-b> & <vc-done>: Pv- & B-: "he fixed what damage there had been done"
done.v:
  VERB_PP(<vc-done>)
  or <verb-phrase-opener>
  or (<verb-pv-b> & {<vc-done>})
  or (S- & <verb-wall>);

% Pa- & Pg+: "I am done working", "I am through being mad"
done.a finished.a through.a:
  ({@E-} & Pa- & {Pg+ or @MV+})
  or (dAJra- & {@MV+})
  or ({@MV+} & dAJla+);

doing.v: <verb-pg> & (O+ or <b-minus> or [[@MV+ & O*n+]] or Vd+) & <mv-coord>;
doing.g: ((O+ or <b-minus> or [[@MV+ & O*n+]] or Vd+) & {@MV+} & <verb-ge>) or <verb-ge-d>;
better.i fine.i ok.i okay.i OK.i poorly.i well.i: {EE-} & Vd-;

% <verb-wall>: "I know he didn't", "I know they don't"
% Wi-: "Don't!"
% EI- & Wi-: "In total, don't!"
% Wi- & I*d+: "Don't do that!"
% SIp+: "don't you?" "why don't you?"
don't don’t:
  (((<verb-rq-aux> & (SIp+ or SFIp+) & (I*d+ or <verb-wall>))
    or ({@E-} & (Sp- or SFp- or (RS- & Bp-)))) & (I*d+ or <verb-wall> or [[()]]))
  or (<verb-co> & {I*d+});

doesn't doesn’t:
  ((<verb-rq-aux> & (SIs+ or SFIs+) & (I*d+ or <verb-wall>)) or <verb-x-s>)
     & (I*d+ or <verb-wall> or [[()]]);

didn't.v-d didn’t.v-d:
  ((<verb-rq-aux> & (SI+ or SFI+)) or <verb-x-sp>)
     & (I*d+ or <verb-wall> or [[()]]);

daren't mayn't shan't oughtn't mightn't
daren’t mayn’t shan’t oughtn’t mightn’t:
  ({{EI-} & Q- & <verb-wall>} & (SI+ or SFI+) & (I+ or <verb-wall>)) or
  (<verb-x-sp> & (I+ or <verb-wall> or [[()]]));

% Cost on {[[MV+]]}: prefer to have prep modifiers modify something else:
% e.g. "I have a report on sitcoms": "on" modifies "report", not "have"
% However, "I have a date with Bob": we want "with" to MVp modify have,
% and not Mp modify "date"... XXX this is all very broken ATM...
%
% <to-verb> & <verb-wall>: "I have to verb-inf" allows "have" to connect to wall.
% [TO+]: allows null-infinitive: "Because I have to."
% N+ & O+: "I have not an instant to lose."
<vc-have>:
  ({@MV+} & (<to-verb> or [TO+]) & <verb-wall>)
  or ({N+} & (B- or (O+ & <verb-wall>)) & {@MV+} & {[I*j+ or Pv+]})
  or (([[@MV+ & O*n+]] or CX- or [[()]]) & <mv-coord>)
  or ({N+} & PP+);

have.v:
  VERB_X_PLI(<vc-have>)
  or ((SIp+ or SFIp+) & ((<verb-rq> & PP+) or CQ-));

% I've they've you've we've: PP+ & <CLAUSE>;
% I- & PP+: "she would've said so".
’ve 've:
  (Sp- & (PP+ or O+))
  or (If- & PP+);

% The splitter will not two suffixes, so we need this
% abbreviation for would-have: "She'd've said so"
% Give it a cost to that "She 'd've said so" can split first.
'd've.#would-have ’d’ve.#would-have:
  [S- & (PP+ or O+)]0.3;

has.v:
  VERB_X_S(<vc-have>)
  or ((SIs+ or SFIs+) & ((<verb-rq> & {PP+ or <verb-wall>}) or CQ-));

% <verb-x-sp> & <verb-wall>: "I sure wish I had"
% Sa*a- & PPf+: "as had been agreed, the work began on Monday"
had.v-d:
  ((SI+ or SFI+) & ((<verb-rq> & PP+) or CQ-)) or
  (Sa*a- & PPf+) or
  (<verb-x-sp> & <vc-have>) or
  (<verb-x-sp> & <verb-wall>) or
  (<verb-and-sp-i-> & <vc-have>) or (<vc-have> & <verb-and-sp-i+>) or
  (<verb-x-pp> &
    (<to-verb> or
    ((O+ or <b-minus>) & {@MV+} & {[I*j+ or Pv+]}) or
    (([[@MV+ & O*n+]] or CX-) & <mv-coord>))) or
  [[(SI*j+ or SFI**j+) & PP+ & ((Xd- & VCq- & Xc+) or VCq- or ({{Xd-} & Xc+} & dCOp+))]];

%we'd they'd I'd he'd she'd you'd: (PP+ or ({Vw+} & I+)) & <CLAUSE>;

% S- & I+: "I'd love to"
% RS- & Bs- & PP+: "He looked at the girl who'd been knitting"
’d 'd:
  (S- & (PP+ or I+))
  or (RS- & Bs- & PP+);

having.v: <verb-pg> & <vc-have>;
having.g: (<vc-have> & <verb-ge>) or <verb-ge-d>;

% PP is disjoined with <verb-wall> because when PP is used, has/have/had
% is an auxiliary verb, an should not get a wall connection!
hasn't hasn’t:
  ((<verb-rq> & (SIs+ or SFIs+)) or (<verb-x-s>))
  & (PP+ or ((([[O+]] & <mv-coord>) or [[()]]) & <verb-wall>));

haven't haven’t:
  ((<verb-rq> & (SIp+ or SFIp+))
     or ({@E-} & (Sp- or SFp- or (RS- & Bp-))))
  & (PP+ or ((([[O+]] & <mv-coord>) or [[()]]) & <verb-wall>));

hadn't.v-d hadn’t.v-d:
  ((<verb-rq> & (SI+ or SFI+))
     or ({@E-} & (S- or SFs- or SFp- or (RS- & B-))))
  & (PP+ or ((([[O+]] & <mv-coord>) or [[()]]) & <verb-wall>));

% -----------------------------------------------------------
% "to be"
% Give [K+] a cost so as to prefer Pp+ in general
%<vc-be>:
%  ({@EBm+} & (((O*t+ or [B**t-] or [K+] or BI+ or OF+ or PF- or
%      (Osi+ & R+ & Bs+) or
%      (Opi+ & R+ & Bp+) or
%      [[()]]) & {@MV+}) or
%    (Pp+ & {THi+ or @MV+}) or
%    THb+ or
%    <to-verb> or
%    Pa+)) or
%  ({N+} & (AF- or Pv+ or I*v+)) or
%  (({N+} or {Pp+}) & Pg*b+);

% But first, some wack idiomatic phrases that behave like "to be"
% Ix- & Pv+: "You may rest assured"
rest.w: Ix- & Pv+;

% no-obj is costly but allows "if it weren't for Sally"
% [Cet+]: elided (silent "that"): "my guess is the door on the left hides the prize."
% which really should be: "my guess is [that] the door on the left hides the prize."
% Except that this breaks lots of stuff ... Arghhh.
%
% (<verb-wall> & BI+): "The question is who we should invite?"
% (<verb-wall> & OF+): "The are of a single mind."
% [{CV-} & B**t-]: "How fast a program does he think it is?"
% CV- optional to parse: "How efficient a program is it?"
% O*i+ & R+ & Bs+ & <verb-wall>: "I believe it was John"
%
% [Pv+].1: this gives Pv+ a fractional cost, so that Pa+ is preferred
%     over Pv+ whenever the same word appears as both adjective and verb.
%     For example, "injured.a" vs. injured.v-d" in "the player is injured",
%     which should get Pa+ not Pv+.
% MV+ & Pv+: "I was by then dominated by my aunt"
% MV+ & Pa+: "I was, before Friday, quite unhappy."
% Pa+ & {<verb-wall>}: the wall is optional: "A player who is injured
%     must leave the field" cannot take a wall.
% [Pa+]0.05: prefer gerund over adjective.
% [I*v+].5: the cost should maybe be even higher, to avoid linking
%     past-tense 'were' to infinitives. "The rooms were let."
% PFb- & <verb-wall> & Pa+: "cheaper than direct, slime is greener."

<vc-be-no-obj>:
  ({@EBm+} & ((
      ([{CV-} & B**t-]
      or (B**t- & <verb-wall>)
      or [K+]
      or (<verb-wall> & BI+)
      or (<verb-wall> & OF+)
      or (Osi+ & R+ & Bs+ & <verb-wall>)
      or (Opi+ & R+ & Bp+ & <verb-wall>)
      or ([()]0.666 & <verb-wall>)) & <mv-coord>)
    or (<verb-wall> & Pp+ & {THi+ or @MV+})
    or THb+
    or <to-verb>
    or (PFb- & <verb-wall> & {Pa+})
    or ({MV+} & [Pa+]0.05 & {<verb-wall>})))
  or ({N+} & ((AF- & <verb-wall>) or ({MV+} & [Pv+].1) or [I*v+].5))
  or (({N+} or {Pp+}) & Pg*b+ & <verb-wall>);

% Identical to above, but no wall.  This is used only in "and.j-v"
% constructions, so that WV links to the "and.j-v" instead of "be".
% [Pg*b+]-0.05: "I was getting hot", prefer Pg+ over O+
<vc-be-no-obj-no-wall>:
  ({@EBm+} & ((([B**t-] or [K+] or BI+ or OF+ or PFb- or
      (Osi+ & R+ & Bs+) or
      (Opi+ & R+ & Bp+) or
      [[()]]) & <mv-coord>) or
    (Pp+ & {THi+ or @MV+}) or
    THb+ or
    <to-verb> or
    Pa+)) or
  ({N+} & (AF- or [Pv+].1 or I*v+)) or
  (({N+} or {Pp+}) & [Pg*b+]-0.05);

% O*m+ allows "If only there were more!"
% THb+ allows "It is your fault that you're a failure."
% The @MV+ allows modifiers (e.g. prep modifiers) to attach both to
% the object and to the copula. Example:
% "There is neither pine nor apple in the pineapple."
% "There is no ham in the hamburger, and neither pine nor apple in the pineapple."
<vc-be-obj>:
  {hHM+} & {@EBm+} & (O*t+ or O*m+) & {@MV+} & {THb+};

<vc-be-obj-p>:
  {hHM+} & {@EBm+} & (Opt+ or Omm+) & {@MV+} & {THb+};

<vc-be-obj-sp>:
  {hHM+} & {@EBm+} & (Ost+ or Opt+ or Omm+) & {@MV+} & {THb+};

<vc-be-obj-u>:
  {hHM+} & {@EBm+} & Out+ & {@MV+} & {THb+};

<vc-be>:         <vc-be-no-obj> or (<vc-be-obj> & <verb-wall>);
<vc-be-sp>:      <vc-be-no-obj> or (<vc-be-obj-sp> & <verb-wall>);
<vc-be-no-wall>: <vc-be-no-obj-no-wall> or <vc-be-obj>;
<vc-be-and>:     <vc-be-no-wall>;


% Colon can be used as a synonym for "is"
% "The answer to your question: yes"
% "The capital of Germany: Berlin"
":.v":
  <verb-x-s> & <vc-be>;

% verb-x-s,u: "there is blood on his hands"
% Unfortunately, this allows "There is chasing dogs", which pairs SFu to Op
% and has to be removed via post-processing.
% EQ: "Everyone knows that 2 + 2 is 4"
% <verb-rq> & (SIs*x+ or SFIs+):  "Is it in place?")
% It does not use a wall, because Qd connects to the wall already.
% SIs*x blocks SIs*g: "*There is chasing dogs"
% Sa*a- & Pv+: "..., as is agreed."
is.v:
  (<verb-x-s,u> & <vc-be>)
  or (<verb-and-s-> & <vc-be-and>)
  or (<vc-be-and> & <verb-and-s+>)
  or ({<verb-rq>} & (SIs*x+ or SIs*b+ or SFIs+) & {<vc-be>})
  or (Sa*a- & Pv+)
  or (Ss*w- & <verb-wall> & Pp+ & TO+ & IV+)
  or (EQ*r- & S- & <verb-wall> & EQ*r+);

% Similar to above, but no S-O inversion, and no equation.
% Also, a cost, so that possessive 's is preferred.
% Also, this can be a contraction for "has": "he has" -> "he's"
% <verb-x-s,u> & PP+: "He's gone to Boston"  (viz "He has gone to Boston")
% But also, some contractions are prohibited:
% *That's just the kind of person he's -- so: Ss- & PF- not allowed
% SIs+ & PFb-: "Where's the ball?"
% hHM+: "I think it's uh from Mars"
's.v ’s.v:
  [(<verb-z-s,u> & {hHM+} &
    (({@EBm+} & (((
        (O*t+ & <verb-wall>)
        or [K+ & <verb-wall>]
        or BI+
        or OF+
        or (Osi+ & R+ & Bs+)
        or (Opi+ & R+ & Bp+))
      & {@MV+} & {THb+})
      or (Pp+ & {THi+ or @MV+})
      or THb+
      or (<to-verb> & <verb-wall>)
      or (Pa+ & <verb-wall>)))
    or ({Pp+} & Pg+)
    or Pv+
    or PP+))
  or (SIs+ & (Qw- or Qp-) & <verb-wall> & {Pa+})
  or (<verb-and-s-> & <vc-be-and>)
  or (<vc-be-and> & <verb-and-s+>)];

% are.v:
%  (({@E-} & (Spx- or SFp- or (RS- & Bp-))) or
%    (<verb-rq> & (SIpx+ or SFIp+))) & <vc-be>;

% Don't allow are.v with uncountable noun objects.
% Ss*t- & <vc-be-obj-p>:  (requires a plural object)
%        "What he wants are the cats"
%        "What John loves about this movie are the sound effects"
% Qd- & (SIpx+ or SFIp+) & <vc-be-and> (no walls here:
% for questions: "Are you insane?" "Are you the one?"
% Spx- & <vc-be-no-obj-no-wall>: "They are getting hot"
are.v:
  ({@E-} & (Spx- or SFp- or (RS- & Bp-)) & (<vc-be-sp> or <vc-be-no-obj-no-wall>))
  or ({@E-} & Ss*t- & <vc-be-obj-p> & <verb-wall>)
  or ({<verb-rq>} & SIpx+ & {<vc-be>})
  or ({<verb-rq>} & SFIp+ & {<vc-be-obj>});

%we're they're I'm you're:
%(({@EBm+} & (((O*t+ or K+ or BI+ or OF+ or Pp+) & {@MV+}) or <to-verb> or Pa+)) or
%({N+} & (Pg+ or Pv+))) & <CLAUSE>;

% they're we're
% [Pg+ or Pv+]-0.05: slight preference for P+ over O+ if possible.
’re 're:
  Spx- & (({@EBm+} & ((((O*t+ & <verb-wall>) or K+ or BI+ or OF+ or Pp+) & <mv-coord>) or <to-verb> or (Pa+ & <verb-wall>) )) or
    ({N+} & [Pg+ or Pv+]-0.05));

% yisser: "you're" Irish English
% Interesting -- no way to put a WV- link in here ...
yisser.v: (Pa+ & Wd-);

% prefer a wall, if we can find it.
<vc-be-opt-wall>:
  <vc-be> or [<vc-be-no-wall>]0.03;

% Q-: "How was the movie?"
% Sa*a- & Pv+: "..., as was promised."
% SX & no wall: "I was getting hot"
was.v-d:
  (<verb-x-s,u> & <vc-be>)
  or (<verb-and-s-> & <vc-be-and>)
  or (<vc-be-and> & <verb-and-s+>)
  or (Sa*a- & Pv+)
  or ({@E-} & SX- & <vc-be-opt-wall>)
  or (<verb-rq> & (SFIs+ or SIs+ or SXI+) & {<vc-be>});

% XXX probably should be verb-and-sp-i- etc !?
% Spx- & no-wall: "They were getting hot"
% [<verb-and-sp-> & <vc-be-and>]0.6: cost, so that other things are
%                                    tried, before VJ kicks in.
%       Example: I would have done that were he with me.
were.v-d:
  (({@E-} & (Spx- or SFp- or [[Ss-]] or [[SX- & <verb-wall>]] or (RS- & Bp-))) & <vc-be-opt-wall>)
  or (<verb-rq> & (SIpx+ or SFIp+) & (<vc-be-opt-wall> or <verb-wall>))
  or [<verb-and-sp-> & <vc-be-and>]0.6
  or (<vc-be-and> & <verb-and-sp+>)
  or [[(SI*j+ or SFI**j+) & <vc-be> & ((Xd- & VCq- & Xc+) or VCq- or ({{Xd-} & Xc+} & dCOp+))]];

% Ss*w-: allows Wh subjects: "Who am I?"
% SX- & no-wall: "I am getting hot"
am.v:
  ({@E-} & SX- & <vc-be-opt-wall>)
  or (<verb-rq> & SXI+ & {<vc-be>})
  or (Ss*w- & <vc-be>)
  or (<verb-and-sp-> & <vc-be-and>)
  or (<vc-be-and> & <verb-and-sp+>);

% I'm == I am
’m 'm:
  SX- & (({@EBm+} & (((O*t+ or K+ or BI+ or OF+ or Pp+) & <mv-coord>) or <to-verb> or Pa+)) or
    ({N+} & (Pg+ or Pv+))) & <verb-wall>;

% S*x- used for passive participles: "this action be taken".
% S**i- & Wi-: "Robert, be there at 5:00"
% XXX I used verb-and-sp-i- but maybe this is wrong ..
% "Show me my notes and be nice about it."
% ({@E-} & I- & B- & O+):
%   "What are the chances that Einstein could really be a genius?"
% Ix- & <verb-wall>: "He is as smart as I expected him to be."
% Ix- & <vc-be>: "I'm sure he'll still be popular."
% Ix- & no-wall: "They will be getting hot"
be.v:
  ({@E-} & ((<verb-co> & <verb-wall>) or <verb-why> or [S*x-]) & <vc-be>)
  or ({@E-} & Ix- & <verb-wall>)
  or ({@E-} & Ix- & <vc-be-opt-wall>)
  or (<verb-and-sp-i-> & ([<vc-be-and>] or ()))
  or (<vc-be> & <verb-and-sp-i+>)
  or ({@E-} & I- & B- & O+ & <verb-wall>);

% no-wall: "They have been getting hot"
been.v: {@E-} & PPf- & <vc-be-opt-wall>;

% S- & Pa+ & Xc+ & <embed-verb>: "The knife being dull, he ..."
% (S- & Xd- & MVg- & Pa+) "..., the knife being dull."
being.v:
  ((({@EBm+} &
      (((O*t+ or [B**t-] or Pp+ or K+ or OF+ or BI+ or <to-verb> or THb+)
         & <mv-coord>)
       or Pa+))
     or AF- or Pv+)
   & <verb-pg,ge>)
   or <verb-ge-d>
   or (S- & Pa+ & Xc+ & <embed-verb> & <verb-wall>)
   or (S- & Xd- & MVg- & Pa+ & <verb-wall>);

isn't isn’t:
  (<verb-x-s> & <vc-be>)
  or (<verb-rq> & (SIs+ or SFIs+) & {<vc-be>});

% merge of isn't, aren't
% "ain't you gonna go?"
% no-wall: "They ain't gettin hot"
ain't ain’t:
  (<verb-x-sp> & <vc-be-opt-wall>)
  or (<verb-rq> & (SI+ or SFI+) & {<vc-be>});

% no-wall: "It wasn't getting hot"
wasn't.v-d wasn’t.v-d:
  ({@E-} & (Ss- or (SX- & <verb-wall>) or SFs- or (RS- & Bs-)) & <vc-be-opt-wall>)
  or (<verb-rq> & (SI*+ or SXI+ or SFIs+) & {<vc-be>});

% no-wall: "They aren't getting hot"
aren't aren’t:
  (({@E-} & (Spx- or SFp- or (RS- & Bp-))) & <vc-be-opt-wall>)
  or (<verb-rq> & (SIpx+ or SFIp+) & {<vc-be>});

% [[Ss-]]: "If it weren't for Joe, ..."
% no-wall: "They weren't getting hot"
weren't.v-d weren’t.v-d:
  (({@E-} & (Spx- or SFp- or [[Ss-]] or (RS- & Bp-))) & <vc-be-opt-wall>)
  or (<verb-rq> & (SIpx+ or SFIp+) & {<vc-be>});

% XXX probably should be verb-and-sp-i- etc !?
% No <verb-wall> here, these are almost entirely just auxiliary verbs.
% Except ... "You know you can", "You know you must"
% Sa*a- & Ix+: "..., as shall be proven"
% SI+ & N+ & I+: "how long will you not have mercy?"
will.v can.v may.v must.v could.v might.v shall.v shalt.v:
  ((SI+ or SFI+) & ((<verb-rq-aux> & {N+} & (I+ or <verb-wall>)) or CQ-))
  or ({N+} & <verb-x-sp> & (I+ or (CX- & <mv-coord>) or <verb-wall> or [[()]]))
  or (Sa*a- & Ix+)
  or (<verb-and-sp-> & {N+} & {@E-} & I+)
  or ({N+} & {@E-} & I+ & <verb-and-sp+>);

% "I sure wish I could."
could.v-d:
  <verb-x-sp> & <verb-wall>;

%I'll he'll she'll we'll they'll you'll it'll: I+ & <CLAUSE>;
’ll 'll: S- & I+;

% <verb-wall>: "You know you should."
should.v:
  ((SI+ or SFI+) & ((<verb-rq-aux> & (I+ or <verb-wall>)) or CQ-)) or
  ({N+} & <verb-x-sp> & (I+ or (CX- & <mv-coord>) or <verb-wall> or [[()]])) or
  (<verb-and-sp-> & I+) or (I+ & <verb-and-sp+>) or
  [[(SI*j+ or SFI**j+) & I+ & ((Xd- & VCq- & Xc+) or VCq- or ({{Xd-} & Xc+} & dCOp+))]];

% idiomatic: "You had better give back that telephone!"
had_better: S- & I+;

% <verb-wall>: "I sure wish he would."
would.v:
  ((SI+ or SFI+) & ((<verb-rq-aux> & {N+ or Vw+} & I+) or CQ-)) or
  ({N+} & <verb-x-sp> & (({RT+} & I+) or (CX- & <mv-coord>) or <verb-wall> or [[()]])) or
  (<verb-and-sp-> & I+) or (I+ & <verb-and-sp+>);

% TO+: "I ought to."
ought.v:
  ((<verb-rq> & (SI+ or SFI+)) or <verb-x-sp> or <verb-and-sp->)
    & {<to-verb> or (N+ & I+) or TO+}
    & <verb-wall>;

% <verb-wall>: "I know I won't."
% SI+ & <verb-wall>: "why couldn't he?"
won't can't mustn't couldn't shouldn't cannot needn't
won’t can’t mustn’t couldn’t shouldn’t needn’t:
  (<verb-rq-aux> & (SI+ or SFI+) & (I+ or <verb-wall>)) or
  (<verb-x-sp> & (I+ or <verb-wall> or [[()]])) or
  (<verb-and-sp-> & {@E-} & I+) or
  ({@E-} & I+ & <verb-and-sp+>) or
  Wa-;

% <verb-wall>: "I know I wouldn't."
% SI+ & <verb-wall>: "why wouldn't he?"
wouldn't wouldn’t:
  (<verb-rq-aux> & (SI+ or SFI+) & {RT+} & (I+ or <verb-wall>)) or
  (<verb-x-sp> & (({RT+} & I+) or <verb-wall> or [[()]])) or
  (<verb-and-sp-> & {@E-} & (({RT+} & I+) or [[()]])) or
  ({@E-} & (({RT+} & I+) or [[()]]) & <verb-and-sp+>);

% ===================================================================
% EQUATIONS ETC.
%
% The below is just barely enough to parse just very simple equations
% and expressions, nothing complex -- no general math or anything like
% that. Relations are treated as "is.v", taking a subject and requiring
% an object (For example, "I think that x = 4", "I think that x is 4").

% Relations
% "verb" use. Two types: one is a synonym for "is", such as
% "I think that x = 4".
% The other is parenthetical remarks:
% e.g. "( p < 0.01 )" for "( p is less than 0.01 )"
% The parenthetical remarks must be offset by parenthesis, and
% must link back to main clause with MV or MX.
=.v <.v >.v =<.v >=.v ==.v ":.eq" eq.v ne.v lt.v lte.v le.v gt.v gte.v ge.v
equiv.v sim.v simeq.v approx.v ~.v ～.v
equals.eq
is_less_than is_greater_than is_equal_to
is_less_than_or_equal_to is_greater_than_or_equal_to:
  (<verb-x-s> & <vc-be>)
  or (EQ*r- & {S-} & <verb-wall> & EQ*r+)
  or (EQ*r-
     & {Xd-}
     & ([O+] or ({EQ*r-} & EQ*r+ & {Xc+}))
     & (MX- or MVa-));

% Binary operators:
% these occur in "simple" expressions
*.v "/.v" +.v -.v x.v:
  ([S- & <verb-wall>] or EQ-) &  ([O+] or EQ+) &
  (Xd- & Xc+ & (MX- or MVa-));

% Binary operators, strict:
% Here EQt attaches only to terms, which may be numbers or letters.
% By contrast, EQrr can only attach to relations (=, < > etc.)
% Negative cost is used to establish operator precedence;
% viz, these are lower cost than relations.
+.eq -.eq *.eq "/.eq" x.eqn plus.eq minus.eq times.eq divided_by
±.eq "+/-.eq":
  (EQt+ & EQt- & ([EQrr- or EQrr+]0.1 or AN+ or NIfn+ or NItn-))
  or (EQt+ & Xc+ & EQt- & Xd- & ([EQrr- or EQrr+]0.1 or AN+ or NIfns+ or NItn-))
  or (EQt- & Xd- & EQt+ & EQt- & Xc+)
  or (Xd- & EQt+ & EQt- & Xc+ & EQt+);

% turnstiles, entailment, implication, assignment
->.eq -->.eq "|-.eq" "|=.eq" ":-.eq" ":=.eq" <-.eq <--.eq :
  (S- & O+ ) & (AN+ or (Xd- & Xc+ & MX-)) & <verb-wall>;

% "adverb" use, e.g. "< 10" for "less than 10"
=.eq <.e =<.e <=.e >.e >=.e +.e -.e <<.e >>.e x.e:
   EN+;

mean.eq avg.eq avg..eq average.eq sum.eq difference.eq total.eq:
  EQt+;

% ===================================================================
% COMMON VERB CATEGORIES (The last number of the file name indicates
% the verb form: 1=plural-infinitive, 2=singular, 3=past("ed"),
% 4=progressive("-ing"), 5=gerund("-ing".)

% abbreviations for ditransitive and optionally ditransitive verbs
% ditransitive verbs take a direct and indirect object
% e.g. "I gave her a rose"
% B- & O+ & O*n+: "What are the chances you'll give her a kiss?"
% O+ & @MV+ & O*n+: "I gave him for his birthday a very expensive present"
% The above is costly because the above is an awkward construction...
<vc-ditrans>:
  (O+ & {[[@MV+]]} & O*n+ & {VJd+})
  or ({@E-} & <b-minus> & O+ & O*n+);

<vc-opt-ditrans>:
  (O+ & {{[[@MV+]]} & O*n+} & {VJd+})
  or ({@E-} & <b-minus> & O+ & O*n+);

<vc-opt-ditrans-costly>:
  (O+ & {{[[@MV+]]} & [O*n+]} & {VJd+})
  or ({@E-} & <b-minus> & O+ & O*n+);

% -----------------------------------------------------------
% common intransitive verbs
% QN+: "you arrived how?"
<vc-intrans>: <mv-coord> or QN+;

% XXX Hmmm. There is a fair number of verbs in here that are "weakly"
% transitive, i.e. are transitive in various rare usages:
% "I ambled the horse", "I hydroplaned the car", etc.  Per theory of
% functional grammar and related, transitivity should be taken as
% variable, so e.g. we could assign a high cost to the transitive use
% of some of these verbs ... Note that most of will never be transitive
% in the active present tense: one never says "it rains cats and dogs".
% I'm too lazy to manually sort through this, so, instead, there is a
% high-cost {[[O+]]} added to some of these.
%
% There are a fair number of verbs in her that can be used as
% imperatives, although most cannot (or would be awkward). So that
% also could be sorted out.
%
% accrue.v ache.v acquiesce.v ad-lib.v adhere.v adjoin.v alight.v
%
% <verb-sip>: "here rest the remains of St. Stephen"
% XXX Some but not all of these verbs allow inversion; basically,
% anything that is not locative or directive won't allow inversion.
%
/en/words/words.v.1.1:
  VERB_PLI({[[O+]]} & <vc-intrans>)
  or <verb-sip>;

% accounts.v accrues.v aches.v acquiesces.v ad-libs.v adheres.v
% <verb-si>: Locative subj-obj inversion "far out in the sea lives a fish"
% XXX Some but not all of these verbs allow inversion; basically,
% anything that is not locative or directive won't allow inversion.
%
/en/words/words.v.1.2:
  VERB_S_I(<vc-intrans>)
  or <verb-si>;

% accounted.v accrued.v ached.v acquiesced.v ad-libbed.v adhered.v
% Pa+: "He ad-libbed, uninterrupted."
/en/words/words.v.1.3:
  VERB_SPPP_I(({[[O+]]} & <vc-intrans>)
    or ({Xc+} & Pa+))
  or <verb-si>;

% <verb-pv>: "It was rusted closed"
rusted.v-d:
  VERB_SPPP_I(<vc-intrans>)
  or <verb-pv>;

fundheld.v-d strove.v-d: VERB_SPPP_I(<vc-intrans>);

% accruing.v aching.v acquiescing.v ad-libbing.v adhering.v adjoining.v
/en/words/words.v.1.4:
  ({[[O+]]} & <vc-intrans> & <verb-pg,ge>) or <verb-adj> or <verb-ge-d>;

arisen.v: {@E-} & PP- & {@MV+} & <verb-wall>;

% --------------------------------------------------------------
% intransitive verbs that can take particles like "up" and "out"
% FYI, "bulge" is actually in words.v.6, because one can say
% "the tank bulged out 6 inches on one side"

<vc-bulge>: {K+} & <mv-coord>;

% barge.v booze.v bottom.v bow.v branch.v breeze.v brim.v bulge.v cave.v
/en/words/words.v.5.1: VERB_PLI(<vc-bulge>);
/en/words/words.v.5.2: VERB_S_I(<vc-bulge>) or <verb-si>;
/en/words/words.v.5.3:
  VERB_SPPP_I(<vc-bulge>)
  or <verb-adj>
  or <verb-si>;

slunk.v-d: VERB_SPPP_I(<vc-bulge>) or <verb-si>;

lay.v-d: VERB_SP_I(<vc-bulge>) or <verb-si>;

lain.v: VERB_PP(<vc-bulge>);

/en/words/words.v.5.4:
  (<vc-bulge> & <verb-pg,ge>) or
  <verb-adj> or
  <verb-ge-d>;

% --------------------------------------------------------------
% irregular

<vc-listen>: {K+} & <mv-coord>;

% <verb-focus>: commands/requests to focus attention on action.
%               The action is a bare infinitive, thus I*j+.
% I- & MVp+ & I*j+: "go listen to them play music"
% Wi- & MVp+ & I*j+: "Just listen to them talk like that!"
<verb-focus>:
  {@E-} & (I- or Wi-) & {<verb-wall>} & MVp+ & I*j+;
listen.v:
  VERB_PLI(<vc-listen>)
  or <verb-focus>;

listens.v: VERB_S_I(<vc-listen>) or <verb-si>;
listened.v-d:
  VERB_SPPP_I(<vc-listen>)
  or <verb-adj>
  or <verb-si>;

listening.v:
  (<vc-listen> & <verb-pg,ge>) or
  <verb-adj> or
  <verb-ge-d>;


% I*g: "Come walk with me".
<vc-come>:
  ({(K+ & {Pa+}) or Pv+ or [[Pg+]] or I*g+ or <b-minus> or QN+} & <mv-coord>)
  or ({@MV+} & Pa+);
come.v:
  VERB_PLI(<vc-come>)
  or VERB_PP(<vc-come>)
  or ({@E-} & Ix- & O*t+)
  or <verb-sip>;
comes.v: VERB_S_I(<vc-come>) or <verb-si>;
came.v-d: VERB_SPPP_I(<vc-come>) or <verb-si>;

% <b-minus>: "he got what he had coming"
coming.v:
  (<vc-bulge> & <verb-pg,ge>)
  or VERB_SPPP_T(<b-minus>)
  or <verb-adj>
  or <verb-ge-d>;

% --------------------------------------------------------------
% optionally transitive verbs
% abdicate.v abide.v abort.v accelerate.v acclimate.v acclimatize.v
% O+ & QN+: "he anchors the bolts how?"
<vc-tr,intr>:
  ({O+ or <b-minus> or [[@MV+ & O*n+]]} & <mv-coord>)
  or ({O+} & QN+);

/en/words/words.v.2.1: VERB_PLI(`<vc-tr,intr>');
/en/words/words.v.2.2: VERB_S_T(`<vc-tr,intr>');

% Put a very small cost on A+ to allow verb conjunctions to come first.
% <verb-manner>: only about half of the words in words.v.2.3 should get
% this, the other half clearly should not. I'm too lazy to sort it out,
% right now.
% <verb-adj> has <verb-manner> as a part of it.
% Pa+: "he paced, worried"
% <verb-pv> & Pa+: "she was posed reclining"
/en/words/words.v.2.3:
  VERB_SPPP_T(`<vc-tr,intr> or ({Xc+} & Pa+)')
  or (<verb-pv> & {{Xc+} & Pa+})
  or (<verb-manner> & O+ & Xc+)
  or <verb-adj>
  or <verb-phrase-opener>;

% Pa+: "The vase landed, unbroken"
landed.v-d crashed.v-d crash-landed.v-d:
  VERB_SPPP_T(`<vc-tr,intr> or ({Xc+} & Pa+)')
  or <verb-pv>
  or <verb-adj>
  or <verb-phrase-opener>;

/en/words/words.v.2.4:
  <verb-pg> & <vc-tr,intr>;

% [A+]0.5: avoid gerunds as adjectives...
/en/words/words.v.2.5:
  (<vc-tr,intr> & <verb-ge>) or <verb-adj> or <verb-ge-d>;

shrank.v-d withdrew.v-d sank.v-d forgave.v-d hove.v-d
spoilt.v-d unbent.v-d overfed.v-d:
  VERB_SPPP_T(`<vc-tr,intr>') or
  <verb-adj>;

shrunk.v withdrawn.v sunk.v forgiven.v:
  VERB_PP(`<vc-tr,intr>') or
  <verb-pv> or
  <verb-adj> or
  <verb-phrase-opener>;

let.w sublet.v:
  VERB_PLI(`<vc-tr,intr>');

let.w-d sublet.v-d:
  VERB_SPPP_T(`<vc-tr,intr>')
  or <verb-pv>
  or <verb-adj>;

hurt.v-d thrust.v-d broadcast.v-d outbid.v-d:
  VERB_SPPP_T(`<vc-tr,intr>')
  or (<verb-ico> & <vc-tr,intr>)
  or <verb-pv>
  or <verb-adj>
  or <verb-phrase-opener>;

% <vc-ditrans>: "She bid him adieu"
% O+ & I+: "she bade him sit down"
bid.v-d bade.v-d:
  VERB_SPPP_T(`<vc-tr,intr> or <vc-ditrans> or (O+ & {@MV+} & I+)')
  or (<verb-ico> & <vc-tr,intr>)
  or <verb-pv>
  or <verb-adj>
  or <verb-phrase-opener>;

% --------------------------------------------------------------

% vc-judge: a lot like vc-delcare, except optionally transitive.
<vc-judge>: <vc-tr,intr> or
  ((O+ or <b-minus>) & ({@MV+} & Pa**j+));

judge.v: VERB_PLI(`<vc-judge>');
judges.v: VERB_S_T(`<vc-judge>');
judged.v-d:
  VERB_SPPP_T(`<vc-judge>')
  or (<verb-pv> & {{@MV+} & Pa+})
  or <verb-adj>
  or <verb-phrase-opener>;
judging.v:
  <verb-pg> & <vc-judge>;
judging.g:
  (<vc-judge> & <verb-ge>) or <verb-adj> or <verb-ge-d>;

% --------------------------------------------------------------

<vc-rise>: {K+ or OD+} & <mv-coord>;
rise.v fall.v:VERB_PLI(<vc-rise>);
rises.v falls.v: VERB_S_I(<vc-rise>) or <verb-si>;
rose.v-d fell.v-d: VERB_SPPP_I(<vc-rise>) or <verb-si>;

risen.v: VERB_PP(<vc-rise>);

fallen.v:
  VERB_PP(<vc-rise>) or
  <verb-adj>;
rising.v falling.v:
  (<vc-rise> & <verb-pg,ge>) or
  <verb-adj> or
  <verb-ge-d>;

% --------------------------------------------------------------
% words.v.6: Optionally transitive verbs that can take particles
% like "up" and "out" -- see also words.v.8
% average.v back.v bail.v bang.v beam.v beef.v bellow.v bend.v bind.v
% The [[@MV+ & O*n+]] term allows some bad parses, e.g. allows:
%   "she walked out of the room two minutes"
%
% ({@E-} & B- & O+):
%   "What are the chances Sherlock PLAYED tennis?"
%    but
%   "*The man whom I play tennis is here"  ... arghh
%
% ({@E-} & B- & O+ & K+):
%   "What are the chances she will DRIVE him up to the farm?"
%
% No Pa links here: *they're building a skyscraper tall
%                   *ours works more elegant than yours
%
% [A+]0.5: He was xxx'ed there  should have xxx as verb not adjective.
%
% QN+: "you bike how many miles?"
% O+ & QN+: "you saw it when?"
<vc-fill>:
  (((K+ & {[[@MV+]]} & (O*n+ or ({Xc+} & (Pa+ or Pv+))))
    or ({O+ or <b-minus>} & {K+})
    or [[@MV+ & O*n+]]
  ) & <mv-coord>)
  or ({O+} & {K+} & QN+);

/en/words/words.v.6.1:
  VERB_PLI(<vc-fill>);

/en/words/words.v.6.2: VERB_S_T(<vc-fill>);

% <verb-si>: "Above him hung a lamp"
%  However, not every verb listed would be used like that.
/en/words/words.v.6.3:
  VERB_SPPP_T(<vc-fill>) or
  (<verb-pv-b> & {K+} & <mv-coord>) or
  <verb-adj> or
  <verb-si> or
  ({K+} & <verb-phrase-opener>);

split.v-d spread.v-d fit.v-d shut.v-d cast.v-d:
  VERB_SPPP_T(<vc-fill>)
  or (<verb-ico> & <vc-fill>)
  or <verb-pvk>
  or <verb-adj>
  or ({K+} & <verb-phrase-opener>);

ate.v-d bit.v-d blew.v-d broke.v-d drank.v-d
flew.v-d froze.v-d hid.v-d stole.v-d
rang.v-d rode.v-d sprang.v-d stalked.v-d woke.v-d
 test-drove.v-d forbore.v-d oversaw.v-d throve.v-d shrove.v-d
countersank.v-d outgrew.v-d strode.v-d offsetted.v-d overthrew.v-d
partook.v-d begot.v-d overdid.v-d smote.v-d stank.v-d quick-froze.v-d
backbit.v-d awoke.v-d redid.v-d chidded.v-d reran.v-d rived.v-d
befell.v-d outrode.v-d betrode.v-d outdid.v-d ridded.v-d
deep-froze.v-d forbad.v-d deep-freezed.v-d retook.v-d interwove.v-d
bespoke.v-d underwent.v-d slew.v-d overdrew.v-d overcame.v-d
outwore.v-d foreknew.v-d wove.v-d trod.v-d outwent.v-d:
  VERB_SPPP_T(<vc-fill>);

bitten.v blown.v broken.v drunk.v
eaten.v flown.v frozen.v hidden.v ridden.v rung.v
sprung.v swum.v woken.v stolen.v
befallen.v interwoven.v overborne.v outgone.v outgrown.v
test-driven.v outdone.v shrunken.v countersunk.v
bespoken.v underlain.v partaken.v redone.v overdone.v
outridden.v deep-frozen.v bestrid.v undergone.v
outbidden.v shorn.v outworn.v bestridden.v stunk.v borne.v
slain.v woven.v riven.v backbitten.v overlain.v bestrewn.v
forborne.v quick-frozen.v browbeaten.v aquitted.v
overseen.v smitten.v overdrawn.v striven.v thriven.v
shriven.v backslidden.v retaken.v trodden.v chidden.v
begotten.v sown.v sewn.v sawn.v hewn.v cloven.v
foreknown.v overthrown.v strewn.v awoken.v bidden.v
stridden.v slain_dead:
  VERB_PP(<vc-fill>)
  or <verb-pvk>
  or <verb-adj>
  or ({K+} & <verb-phrase-opener>);

/en/words/words.v.6.4:
  (<verb-pg> & <vc-fill>) or
  <verb-and-pg-> or <verb-and-pg+>;

% [A+]0.5: "I like eating bass": eating is not the adjective, here.
% <verb-ge-d> & {K+}: "I hope you don't mind my horning in"
/en/words/words.v.6.5:
  (<vc-fill> & <verb-ge>) or
  (<verb-ge-d> & {K+}) or
  <verb-adj>;

frizz.v frizzle.v prink.v slough.v scuff.v tog.v swot.v:
  VERB_PLI(<vc-fill>);

% Disambiguation: add a cost so that Bob the given name is preferred
% to bob the verb: "Bob lives in China".
bob.v:
  [ VERB_PLI(<vc-fill>) ]0.2;

% ------------------------------------------------------------
% just like <verb-pg> & <vc-fill>, except that "and" is the one
% XXX TODO review this ...
and.v-fill:
  ((hVJlg- & hVJrg+) & (Pg- or Mg- or ({Xd-} & VJrg-))) & <vc-fill>;

% ------------------------------------------------------------
% Just like vc-fill above, but includes predicative adjectives:
% Pa+ link: "The truck ran uncontrolled"
% Pa**j link: "The thugs beat him senseless" "You are driving me crazy"
%     "Make it nice and soft"
%
% ({@E-} & B- & O+):
%   "What are the chances Sherlock could really CATCH the criminal?"
% ({@E-} & B- & O+ & Pa**j+):
%   "What are the chances she will really DRIVE him crazy?"
% ({@E-} & B- & O+ & K+):
%   "What are the chances she will DRIVE him up to the farm?"
% QN+: "you run how many miles?"
% QN+ & MV+: "you ran to her, when?"
<vc-run>:
  ((K+ & {[[@MV+]]} & O*n+)
    or Pa+
    or ({O+ or <b-minus>} & {K+})
    or ((O+ or <b-minus>) & ({@MV+} & Pa**j+))
    or ({@E-} & <b-minus> & O+ & {Pa**j+ or K+})
    or ({O+} & {K+} & {@MV+} & {Xc+} & QN+)
    or [[@MV+ & O*n+]]
  ) & <mv-coord>;

catch.v drive.v strike.v:
  VERB_PLI(<vc-run>);

% special case: run-present-tense + run-beaten-driven
run.v:
  VERB_PLI(<vc-run>)
  or VERB_PP(<vc-run>)
  or <verb-pvk>
  or <verb-adj>
  or ({K+} & <verb-phrase-opener>);

runs.v beats.v catches.v drives.v strikes.v:
  VERB_S_T(<vc-run>)
  or <verb-si>;

% <verb-pv>: "He was struck by the bus"
% <verb-pvk> with K+: "He was caught up in his work"
% Pa+: "He was struck dumb"
ran.v-d caught.v-d drove.v-d struck.v-d:
  VERB_SPPP_T(<vc-run>)
  or (<verb-pvk> & {Pa+})
  or <verb-si>;

% XXX is all this stuff really necessary?
beat.v-d:
  VERB_SPPP_T(<vc-run>)
  or (<verb-ico> & <vc-run>)
  or <verb-pvk>
  or <verb-adj>
  or ({K+} & <verb-phrase-opener>);

% [A+]0.5: avoid bad "He was driven home"
% Pa+: "He was driven green with envy"
beaten.v driven.v stricken.v:
  VERB_PP(<vc-run>)
  or (<verb-pvk> & {Pa+})
  or <verb-adj>
  or ({K+} & <verb-phrase-opener>);

running.v beating.v catching.v driving.v striking.v:
  (<verb-pg> & <vc-run>) or
  <verb-and-pg-> or <verb-and-pg+>;

% [A+]0.5: "I like eating bass": eating is not the adjective, here.
running.g beating.g catching.g driving.g striking.g:
  (<vc-run> & <verb-ge>) or
  <verb-ge-d> or
  <verb-adj>;

% ------------------------------------------------------------
% common transitive verbs
% abandon.v abase.v abbreviate.v abduct.v abet.v abhor.v abolish.v
%
% ({@E-} & B- & O+):
%    "What are the chances Sherlock could really solve a crime?"
% The (Os+ or Op+) is used to block:
%    *This is the man we love him
%    *I still remember the room I kissed him
% O+ & QN+: "you hit the shot how many times?"
<vc-trans>:
  ((O+
   or <b-minus>
   or [[@MV+ & O*n+]]
   or ({@E-} & <b-minus> & (Os+ or Op+))
  ) & <mv-coord>)
  or ({O+} & QN+);

/en/words/words.v.4.1 : VERB_PLI(<vc-trans>);
/en/words/words-medical.v.4.1: VERB_PLI(<vc-trans>);

/en/words/words.v.4.2: VERB_S_T(<vc-trans>);
/en/words/words-medical.v.4.2: VERB_S_T(<vc-trans>);

% <verb-manner> is too broad for most of these, but is OK for many.
% <verb-manner> is part of <verb-adj>
% I'm not sure I like Pa in verb-adj but there it is and widely used ...
/en/words/words.v.4.3:
  VERB_SPPP_T(<vc-trans>)
  or (<verb-pv> & {{Xc+} & Pa+})
  or (<verb-sp> & Xc+ & Pa+)
  or <verb-adj>
  or <verb-phrase-opener>;

% !?XXX many of the new additions fail some common sentences, such as:
% The candle guttered. It plummeted to the bottom. Prices plummeted!
/en/words/words-medical.v.4.3:
  VERB_SPPP_T(<vc-trans>)
  or (<verb-pv> & {{Xc+} & Pa+})
  or <verb-adj>
  or <verb-phrase-opener>;

/en/words/words.v.4.4
/en/words/words-medical.v.4.4:
  <verb-pg> & <vc-trans>;

/en/words/words.v.4.5
/en/words/words-medical.v.4.5:
  (<vc-trans> & <verb-ge>) or <verb-ge-d>;

forsook.v-d overrode.v-d overtook.v-d rewrote.v-d undid.v-d
overran.v-d mistook.v-d underwrote.v-d:
  VERB_SP_T(<vc-trans>);

% I*d- & <b-minus> & O+: "how many more times did you hit her?"
hit.v-d misread.v-d shed.v-d rid.v-d overcome.v-d
overrun.v-d upset.v-d undercut.v-d:
  VERB_SPPP_T(<vc-trans>)
  or (<verb-ico> & <vc-trans>)
  or ({@E-} & I*d- & <b-minus> & O+)
  or <verb-pv>
  or <verb-adj>
  or <verb-phrase-opener>;

forsaken.v overridden.v overtaken.v rewritten.v undone.v
beset.v mistaken.v underwritten.v:
  VERB_PP(<vc-trans>) or
  <verb-pv> or
  <verb-adj> or
  <verb-phrase-opener>;

% Unlikely transitive verb. Here, we single out 'frank', as that is a
% rare verb, and it causes conflicts with the given name 'Frank.b'.
% For example: "Frank felt vindicated".
<vc-trans-unlikely>:
  ([O+]1.5
   or <b-minus>
   or [[@MV+ & O*n+]]
   or ({@E-} & <b-minus> & O+)
  ) & <mv-coord>;

frank.v : VERB_PLI(<vc-trans-unlikely>);

% -----------------------------------------------------------------
% The simplest transitive, ditransitive constructions
% i.e. must take an object
% Almost exactly like words.v.4 above, but ditrans
<vc-corral>:
  <vc-trans>
  or <vc-ditrans>;

corral.v crown.v decant.v ink.v intone.v rope.v:
  VERB_PLI(<vc-corral>);
corrals.v crowns.v decants.v inks.v intones.v ropes.v:
  VERB_S_T(<vc-corral>);
corralled.v-d crowned.v-d decanted.v-d inked.v-d
intoned.v-d roped.v-d:
  VERB_SPPP_T(<vc-corral>)
  or <verb-pv>
  or <verb-adj>
  or <verb-phrase-opener>;
corralling.v crowning.v decanting.v inking.v intoning.v roping.v:
  <verb-pg> & <vc-corral>;
corralling.g crowning.g decanting.g inking.g intoning.g roping.g:
  (<vc-corral> & <verb-ge>) or <verb-ge-d>;

% The simplest optionally-transitive, and ditransitive constructions
<vc-bake>:
  {@MV+}
  or <vc-trans>
  or <vc-ditrans>;

bake.v dictate.v kiss.v slice.v:
  VERB_PLI(<vc-bake>);
bakes.v dictates.v kisses.v slices.v:
  VERB_S_T(<vc-bake>);

% A+: "she gave him some sliced bread"
baked.v-d sliced.v-d:
  VERB_SPPP_T(<vc-bake>)
  or <verb-pv>
  or <verb-adj>
  or <verb-phrase-opener>;
dictated.v-d kissed.v-d:
  VERB_SPPP_T(<vc-bake>)
  or <verb-pv>
  or <verb-phrase-opener>;
baking.v dictating.v kissing.v slicing.v:
  <verb-pg> & <vc-bake>;
baking.g dictating.g kissing.g slicing.g:
  (<vc-bake> & <verb-ge>) or <verb-ge-d>;

% O+ & K+: "She buttered him up"
% ditrans: "She buttered him some toast"
% A+: "She gave him some buttered bread"
<vc-butter>:
  <vc-trans>
  or ((O+ & K+) & <mv-coord>)
  or <vc-ditrans>;
butter.v: VERB_PLI(<vc-butter>);
butters.v: VERB_S_I(<vc-butter>);
buttered.v-d:
  VERB_SPPP_I(<vc-butter>)
  or <verb-pv>
  or <verb-adj>;
buttering.v:
  <verb-pg> & <vc-butter>;
buttering.g:
  (<vc-butter> & <verb-ge>) or <verb-ge-d>;

% -----------------------------------------------------------------
% words.v.8: Transitive verbs that can take particles like "up" and "out"
% auction.v bandy.v bar.v batten.v bite.v block.v blot.v blurt.v
% See also words.v.6 for optionally transitive verbs.
% XXX TODO: should be reviewed, as some of them are optionally transitive
%
% B- & O+ & {K+}:
%    what are the chances she will TRACK him down to the farm?
% Pa+: "he cut out after fifth period"
% K+ & Pa+: "it washed up, unbroken"
% QN+: "you kicked how many balls?"
<vc-kick>:
  (((K+ & {[[@MV+]]} & O*n+)
    or ((O+ or <b-minus>) & {K+})
    or (<b-minus> & O+ & {K+})
    or ({K+} & {Xc+} & Pa+)
    or [[@MV+ & O*n+]]) & <mv-coord>)
  or ({K+} & QN+);

/en/words/words.v.8.1: VERB_PLI(<vc-kick>);
/en/words/words.v.8.2: VERB_S_T(<vc-kick>);

threw.v-d tore.v-d wore.v-d
overate.v-d over-ate.v-d forewent.v-d oversewed.v-d forswore.v-d
foreswore.v-d forwent.v-d: VERB_SPPP_T(<vc-kick>);

shaken.v thrown.v torn.v worn.v
forgone.v curretted.v forsworn.v oversewn.v over-eaten.v
 foresworn.v overeaten.v:
  VERB_PP(<vc-kick>) or
  (<verb-pv-b> & {K+} & <mv-coord>) or
  <verb-adj> or
  ({K+} & <verb-phrase-opener>);

% <verb-manner>: only about half of the words in words.v.8.3 should get
% this, the other half clearly should not. I'm too lazy to sort it out,
% right now.
% Pa+: "it washed up unbroken"
/en/words/words.v.8.3:
  VERB_SPPP_T(<vc-kick>) or
  (<verb-pv-b> & {K+} & <mv-coord>) or
  <verb-adj> or
  ({K+} & <verb-phrase-opener>);

cut.v-d:
  VERB_SPPP_T(<vc-kick>)
  or (<verb-ico> & <vc-kick>)
  or (<verb-pv-b> & {K+} & <mv-coord>)
  or (<verb-manner> & O+ & Xc+)
  or <verb-adj>
  or ({K+} & <verb-phrase-opener>);

/en/words/words.v.8.4: <verb-pg> & <vc-kick>;
% <verb-ge-nos>: <vc-kick> has O+ in it; so must not have S+ in <verb-ge>
%            "Carrying the box was a small child"
/en/words/words.v.8.5:
  (<vc-kick> & <verb-ge-nos>)
  or <verb-ge-d>;

% --------------------------------------------------------------
<vc-raise>: (((O+ or <b-minus>) & {OD+}) or [[@MV+ & O*n+]]) & <mv-coord>;
raise.v lower.v up.v: VERB_PLI(<vc-raise>);
raises.v lowers.v ups.v: VERB_S_T(<vc-raise>);
raised.v-d lowered.v-d upped.v-d:
  VERB_SPPP_T(<vc-raise>) or
  <verb-pv> or
  <verb-adj> or
  <verb-phrase-opener>;
raising.v lowering.v upping.v: <verb-pg> & <vc-raise>;
raising.g lowering.g upping.g: (<vc-raise> & <verb-ge>) or <verb-ge-d>;

% much like words.v.2.1, except can have "TO" link.
% tending.g remains in words.v.2.5
%   "I tended for years to believe that shepherds tend sheep."
% TO+ & Xc+: allows null-infinitive: "Yes, it sometimes tends to"
<vc-tend>:
  (<vc-tr,intr> & {<to-verb>})
  or (<mv-coord> & <null-verb>);
tend.v: VERB_PLI(<vc-tend>);
tends.v: VERB_S_T(<vc-tend>);
tended.v-d:
  VERB_SPPP_T(<vc-tend>) or
  <verb-pv> or
  <verb-adj> or
  <verb-phrase-opener>;
tending.v: <verb-pg> & <vc-tend>;

% INTRANSITIVE COMPLEX VERBS (those that do not take O+)

% TO+ & Xc+: allows null-infinitive: "I would never hesitate to"
<vc-consent>:
  ({@MV+} & {<to-verb>})
  or (<mv-coord> & <null-verb>);
consent.v endeavor.v hesitate.v proceed.v aspire.v purport.v:
  VERB_PLI(<vc-consent>);
consents.v endeavors.v hesitates.v proceeds.v aspires.v purports.v:
  VERB_S_I(<vc-consent>);
consented.v-d endeavored.v-d hesitated.v-d proceeded.v-d
aspired.v-d purported.v-d:
  VERB_SPPP_I(<vc-consent>);
consenting.v endeavoring.v hesitating.v proceeding.v aspiring.v purporting.v:
  (<vc-consent> & <verb-pg,ge>) or <verb-ge-d>;


<vc-deign>: {@MV+} & <to-verb>;
endeavour.v condescend.v deign.v: VERB_PLI(<vc-deign>);
endeavours.v condescends.v deigns.v: VERB_S_I(<vc-deign>);
endeavoured.v-d condescended.v-d deigned.v-d: VERB_SPPP_I(<vc-deign>);
endeavouring.v condescending.v deigning.v: (<vc-deign> & <verb-pg,ge>) or
<verb-ge-d>;

% QN+: "it happened when?"
% MV+ & QN+: "That happened to her, when?"
<vc-happen>: {@MV+} & {<to-verb> or THi+ or ({Xc+} & QN+)} & {VC+};

% I*d- & <b-minus>: "how many more times will it happen"
happen.v occur.v:
   VERB_Y_PLI(<vc-happen>)
   or (I*d- & <b-minus>);
happens.v occurs.v: VERB_Y_S(<vc-happen>);
happened.v-d occured.v-d occurred.v-d: VERB_Y_SPPP(<vc-happen>);
happening.v occuring.v occurring.v: (<vc-happen> & <verb-pg,ge>) or <verb-ge-d>;

% <vc-please>: a subset of <vc-paraph>, used for urges/desires
% Allows "he does as he pleases" type constructions, using the
% CP link in a paraphrasing-like way.
% O+: "... as it pleases him"
% Pv-: "... as it was hoped"  (XXX why not PP-, here ???)
<vc-please>:
  {E-} & (S- or PP- or Pv-) & <verb-wall> & {Xd-} & [dCPu-]-0.05 &
    ({O+ & <mv-coord>} or {@MV+ & Xc+});

% ditransitive
<vc-wish>:
  ({@MV+} & {TH+ or <embed-verb> or RSe+ or <to-verb>}) or
  <vc-ditrans>;
wish.v: VERB_PLI(<vc-wish>);
wishes.v: VERB_S_I(<vc-wish>) or <vc-please>;
wished.v-d: VERB_SPPP_I(<vc-wish>) or <vc-please>;
wishing.v: <verb-pg> & <vc-wish>;
wishing.g: (<vc-wish> & <verb-ge>) or <verb-ge-d>;

% The O+ target is to handle "I hope so", but really, we should have
% a special-case for this (i.e. a new minor letter).
% See also <vc-think> for the same problem.
% TO+ & Xc+: allows null-infinitive: "Yes, I would hope to"
<vc-hope>:
  ({@MV+} & {TH+ or <embed-verb> or RSe+ or <to-verb>})
  or (<mv-coord> & <null-verb>)
  or [[O+ & {@MV+}]];

hope.v agree.v pretend.v swear.v pray.v protest.v vow.v vote.v:
  VERB_PLI(<vc-hope>);
hopes.v agrees.v pretends.v swears.v prays.v protests.v vows.v votes.v :
  VERB_S_I(<vc-hope>) or <vc-please>;
pretended.v-d prayed.v-d:
  VERB_SPPP_I(<vc-hope>) or <vc-please>;

% The (<verb-s-pv> & THi+) allows "it is hoped that ..." to parse.
% Naked Pv+: "..., as was hoped."
hoped.v-d protested.v-d voted.v-d vowed.v-d:
  VERB_SPPP_T(<vc-hope>)
  or (<verb-s-pv> & {THi+})
  or <vc-please>
  or <verb-manner>;

% Naked Pv+: "..., as was agreed."
agreed.v-d:
  VERB_SPPP_T(<vc-hope>)
  or (<verb-pv> & {TH+})
  or <vc-please>
  or <verb-manner>;

swore.v-d: VERB_SP_T(<vc-hope>);
sworn.v: VERB_PP(<vc-hope>) or <verb-adj> or <vc-please>;

hoping.v agreeing.v pretending.v swearing.v praying.v
protesting.v vowing.v voting.v:
(<vc-hope> & <verb-pg,ge>) or <verb-ge-d>;

<verb-fronted>: {@E-} & S- & hPFt- & <verb-wall>;

% XXX Why is there a cost on Pv+ ?? "John appeared vindicated"
% N+: "It appears not"
% <verb-fronted>: "so it seems", "so it appears"
% Ix- & PF- & <verb-wall>: "so it would seem"
% TO+ & Xc+: allows null-infinitive: "yes, it appears to"
<vc-appear>:
  {@MV+} & {(Pa+ & <verb-wall>)
    or <tof-verb>
    or <null-verb>
    or THi+
    or AF-
    or N+
    or [{Xc+} &Pv+]};
appear.v: VERB_Y_PLI(<vc-appear>) or (Ix- & PF- & <verb-wall>);
appears.v: VERB_Y_S(<vc-appear>) or <verb-fronted>;
appeared.v-d: VERB_Y_SPPP(<vc-appear>) or <verb-fronted>;
appearing.v: (<vc-appear> & <verb-x-pg,ge>) or <verb-ge-d>;

% XXX Why is there a cost on Pv+ ?? "John seemed vindicated"
% N+: "It seems not"
% <verb-si>: "so seems it"
% TO+ & Xc+: allows null-infinitive: "yes, it would seem to"
<vc-seem>:
  {@MV+} &
    ((Pa+ & <verb-wall>)
    or <tof-verb>
    or <null-verb>
    or LI+
    or THi+
    or AF-
    or N+
    or [Pv+]);
seem.v: VERB_Y_PLI(<vc-seem>) or (Ix- & PF- & <verb-wall>);
seems.v: VERB_Y_S(<vc-seem>) or <verb-fronted> or <verb-si>;
seemed.v-d: VERB_Y_SPPP(<vc-seem>) or <verb-fronted> or <verb-si>;
seeming.v: (<vc-seem> & <verb-x-pg,ge>) or <verb-ge-d>;

% qi-pref: give slight cost preference to QI over MV, for example:
% "I do not care where you put it"
% "I wonder when he will come"
<QI+pref>: [QI+]-0.05;
<QN+pref>: [QN+]-0.10;

<vc-care>: {@MV+} & {<to-verb> or <QI+pref> or <QN+pref>};
care.v: VERB_PLI(<vc-care>);
cares.v: VERB_S_I(<vc-care>);
cared.v-d: VERB_SPPP_I(<vc-care>);
caring.v: (<vc-care> & <verb-pg,ge>) or <verb-ge-d>;

% O+: "I suppose so"
<vc-assert>: ({@MV+} & (TH+ or RSe+ or Z- or <embed-verb>)) or O+;
assert.v contend.v remark.v retort.v intimate.v exclaim.v
conjecture.v allege.v surmise.v opine.v insinuate.v suppose.v:
  VERB_PLI(<vc-assert>) or <verb-manner>;

asserts.v contends.v remarks.v retorts.v intimates.v exclaims.v
conjectures.v alleges.v surmises.v opines.v insinuates.v supposes.v:
  VERB_S_T(<vc-assert>) or <verb-manner>;

retorted.v intimated.v exclaimed.v conjectured.v
surmised.v-d opined.v-d insinuated.v-d: VERB_SPPP_I(<vc-assert>);
asserted.v-d contended.v-d remarked.v-d:
  VERB_SPPP_T(<vc-assert>) or
  (<verb-pv> & THi+) or
  <verb-adj>;

alleged.v-d:
  VERB_SPPP_T(<vc-assert>) or
  (<verb-pv> & THi+) or
  <verb-adj>;

supposed.v-d:
  VERB_SPPP_T(<vc-assert>)
  or (<verb-s-pv> & {<tof-verb> or THi+ or Z-})
  or <verb-adj>;

asserting.v contending.v remarking.v retorting.v intimating.v
exclaiming.v conjecturing.v alleging.v surmising.v opining.v insinuating.v
supposing.v:
  (<vc-assert> & <verb-pg,ge>) or <verb-ge-d>;

<vc-muse>: {@MV+} & {TH+};
theorize.v attest.v fantasize.v muse.v speculate.v concur.v:
  VERB_PLI(<vc-muse>);
theorizes.v attests.v fantasizes.v muses.v speculates.v concurs.v:
  VERB_S_I(<vc-muse>);
attested.v-d fantasized.v-d mused.v-d speculated.v-d concurred.v-d:
  VERB_SPPP_I(<vc-muse>);
theorized.v-d:  VERB_SPPP_I(<vc-muse>) or (<verb-pv> & THi+);
theorizing.v attesting.v fantasizing.v musing.v speculating.v concurring.v:
  (<vc-muse> & <verb-pg,ge>) or
  <verb-ge-d>;

<vc-reply>: ({@MV+} & {TH+ or <embed-verb>});
reply.v argue.v sigh.v testify.v comment.v respond.v reason.v:
  VERB_PLI(<vc-reply>);
replies.v argues.v sighs.v
testifies.v comments.v responds.v reasons.v:
  VERB_S_T(<vc-reply>);
replied.v-d sighed.v-d commented.v-d responded.v-d:
  VERB_SPPP_I(<vc-reply>);

% The (<verb-s-pv> & THi+) allows "it is reasoned that ..." to parse.
argued.v-d reasoned.v-d testified.v-d:
  VERB_SPPP_T(<vc-reply>) or
  (<verb-s-pv> & THi+);

replying.v arguing.v sighing.v testifying.v commenting.v responding.v
reasoning.v:
  (<vc-reply> & <verb-pg,ge>) or
  <verb-ge-d>;

<vc-brag>: {@MV+} & {<embed-verb> or TH+ or RSe+ or <of-coord>};
boast.v brag.v complain.v hint.v mutter.v whisper.v:
  VERB_PLI(<vc-brag>);
boasts.v brags.v complains.v hints.v mutters.v whispers.v:
  VERB_S_I(<vc-brag>);
boasted.v-d bragged.v-d complained.v-d hinted.v-d muttered.v-d whispered.v-d:
  VERB_SPPP_I(<vc-brag>) or (<verb-s-pv> & THi+);
boasting.g bragging.g complaining.g hinting.g muttering.g whispering.g:
  (<vc-brag> & <verb-ge>) or <verb-ge-d>;
boasting.v bragging.v complaining.v hinting.v muttering.v whispering.v:
  <verb-pg> & <vc-brag>;

<vc-dream>: {@MV+} & {<embed-verb> or TH+ or RSe+ or <of-coord> or BW-};
dream.v: VERB_PLI(<vc-dream>);
dreams.v: VERB_S_I(<vc-dream>);
dreamt.v-d dreamed.v-d: VERB_SPPP_I(<vc-dream>);
dreaming.g: (<vc-dream> & <verb-ge>) or <verb-ge-d>;
dreaming.v: <verb-pg> & <vc-dream>;

% The O+ is to handle "do you think so, too?", however, a special
% target for objects like "so" or "it" would be better...
% "hope.v" has the same problem.
% O+ & O*n: "She will think it an act of kindness."
% O+ & Pa**j: "She will think it true."
% TO+ & Xc+: allows null-infinitive: "No, I did not think to"
<vc-think>:
  ({@MV+} & {<embed-verb> or TH+ or RSe+ or Z- or <of-coord> or BW-})
  or (<mv-coord> & <null-verb>)
  or (O+ & {@MV+} & {O*n+ or Pa**j+});

think.v: VERB_PLI(<vc-think>);
thinks.v: VERB_S_T(<vc-think>);

% <verb-s-pv> & Cet+: (phantom that) "It was previously thought they were wrong."
thought.v-d:
  VERB_SPPP_T(<vc-think>)
  or (<verb-s-pv> & {<that-verb> or THi+ or Z-});

thinking.g: (<vc-think> & <verb-ge>) or <verb-ge-d>;
thinking.v: <verb-pg> & <vc-think>;

% B-: "what does it matter?"
<vc-matter>:
  ({@MV+} & {THi+ or QIi+})
  or <b-minus>;
matter.v: VERB_S_PLI(<vc-matter>);
matters.v: VERB_S_S(<vc-matter>);
mattered.v-d: VERB_SPPP_I(<vc-matter>);
mattering.v: (<vc-matter> & <verb-pg,ge>) or <verb-ge-d>;

<vc-suffice>: {@MV+} & {THi+};
suffice.v: VERB_PLI(<vc-suffice>);
suffices.v: VERB_S_I(<vc-suffice>);
sufficed.v-d: VERB_SPPP_I(<vc-suffice>);
sufficing.v: (<vc-suffice> & <verb-pg,ge>) or <verb-ge-d>;

<vc-insist>: ({@MV+} & {TH+ or Zs- or TS+ or <embed-verb>}) or (SI*j+ & I*j+);
insist.v: VERB_PLI(<vc-insist>);
insists.v: VERB_S_I(<vc-insist>);
insisted.v-d: VERB_SPPP_I(<vc-insist>);
insisting.v: (<vc-insist> & <verb-pg,ge>) or <verb-ge-d>;

<vc-wonder>: {@MV+} & {<QI+pref>};
wonder.v inquire.v: VERB_PLI(<vc-wonder>);
wonders.v inquires.v: VERB_S_I(<vc-wonder>);
wondered.v-d inquired.v-d: VERB_SPPP_I(<vc-wonder>);
wondering.v inquiring.v: (<vc-wonder> & <verb-pg,ge>) or <verb-ge-d>;

% Imperative go: "go play ball", "go take a walk", "go swimming"
% Similar pattern to "please.r" -- "please play ball" etc.
% "You and Rover go play with the ball." requires an S- link.
% Hmm ... changes to go.v seem to have obsoleted the need for this ...
% go.w: {E-} & (Wi- or S-) & I+;

% B-: "which way did it go?"
% QN+: "you will go when?"
% K+ & Pg+: "she went out shopping"
% K+ & Pa+: "she went out happy"
%      The above two could also be handled with <vc-keep-on>
%      Alternately, we could set up Ko+ here, and Ko- on "out.r".
<vc-go>:
  ({K+ or (K+ & (Pg+ or Pa+)) or [Pg+] or [[{Xc+} & Pa+]] or I*g+ or <b-minus>} & <mv-coord>)
  or QN+;
go.v: VERB_PLI(<vc-go>);

% SFs-: "There goes the cutest guy ever!", needs O*t to survive PP.
% However, prefer Pg over O*t when possible...
% Wg-: "Went shopping"
goes.v:
  (<verb-y-s> & (<vc-go> or ({[[O*t+]]} & {@MV+} & <verb-wall>)))
  or ({@E-} & Wg- & <vc-go>)
  or (<verb-and-s-> & <vc-go>)
  or (<vc-go> & <verb-and-s+>);
went.v-d:
  (<verb-y-sp> & (<vc-go> or ({[[O*t+]]} & {@MV+} & <verb-wall>)))
  or ({@E-} & Wg- & <vc-go>)
  or (<verb-and-sp-i-> & <vc-go>)
  or (<vc-go> & <verb-and-sp-i+>)
  or <verb-si>;

% Wg-: "Gone fishin"
gone.v: VERB_PP(<vc-go>) or ({@E-} & Wg- & <vc-go>);

% The keys are gone.  The popcorn is all gone.
gone.a:
  ({@E-} & Pa-) or
  (dAJra- & {@MV+}) or
  ({@MV+} & dAJla+);

% XXX TODO maybe need VJ and-able links for going etc. ???
% <tof-verb>: "there is going to be a meeting"
going.v goin'.v:
  ((<tof-verb> or ({K+ or [[{Xc+} & Pa+]]} & <mv-coord>)) & <verb-x-pg,ge>)
  or ({@E-} & Wg- & <vc-go>)
  or <verb-adj>
  or <verb-ge-d>;

% like "going to", except can't have both the IV ant I*t links at the
% same time...
% "I'm gonna do it."
% Sp*i- : "I gonna go".
gonna.v:
  (I*t+ & <verb-x-pg>)
  or ({@E-} & Sp*i- & WV- & I*t+);

% transitive: "stay the prisoner's execution"
<vc-stay>: {({@MV+} & (Pa+ or AF-)) or ({K+} & <mv-coord>) or (O+ & <mv-coord>)};
stay.v: VERB_PLI(<vc-stay>);
stays.v: VERB_S_T(<vc-stay>) or <verb-si>;
stayed.v-d: VERB_SPPP_T(<vc-stay>) or <verb-si>;
staying.v: (<vc-stay> & <verb-pg,ge>) or <verb-ge-d>;

<vc-stand>: {({@MV+} & Pa+) or ({O+ or <b-minus>} & {K+} & <mv-coord>)};
stand.v sit.v: VERB_PLI(<vc-stand>);
stands.v sits.v: VERB_S_T(<vc-stand>) or <verb-si>;
stood.v-d sat.v-d: VERB_SPPP_T(<vc-stand>) or <verb-si>;
standing.v sitting.v: <verb-pg> & <vc-stand>;
standing.g sitting.g: (<vc-stand> & <verb-ge>) or <verb-ge-d> or <verb-adj>;

<vc-sound>: ({@MV+} & {LI+ or Pa+ or AF-}) or {O+ & K+ & <mv-coord>};
sound.v: VERB_PLI(<vc-sound>);
sounds.v: VERB_S_T(<vc-sound>);
sounded.v-d: VERB_SPPP_T(<vc-sound>);
sounding.v: (<vc-sound> & <verb-pg,ge>) or <verb-ge-d>;

% K: "He is acting up"
<vc-act>: {({@MV+} & (LI+ or Pa+)) or ({K+ or AF-} & <mv-coord>)};
act.v: VERB_PLI(<vc-act>);
acts.v: VERB_S_I(<vc-act>);
% "be acted upon quickly"
acted.v-d: VERB_SPPP_I(<vc-act>) or (<verb-pv-b> & {K+} & <mv-coord>);
acting.v: (<vc-act> & <verb-pg,ge>) or <verb-ge-d>;

% Pa: The team reigns undefeated
<vc-reign>: {@MV+} & {LI+ or Pa+};

% See also rule.v far below
reign.v rule.w: VERB_PLI(<vc-reign>);
reigns.v rules.w: VERB_S_I(<vc-reign>);
reigned.v-d ruled.w-d: VERB_SPPP_I(<vc-reign>);
reigning.v ruling.w: (<vc-reign> & <verb-pg,ge>) or <verb-ge-d>;

% O+ & K+: "She looked him over."
% O+ & MV+: "I'll look her in the eye"
%   Must have at least one copy of MV+, to block "she looked him"
% MVa+ connects to adverbs.
% Pa+ connects to common adjectives (predicative adjectives)
% K+ connects to particles.
% [Pa+]0.1: prefer MVa to Pa whenever possible: "She look right"
% [K+]0.2: prefer Pa+ to K+ whenever possible: "She looked up"
% <verb-focus>: "Just look at him smile!"
<vc-look>: {({@MV+} & (LI+ or [{Xc+} & Pa+]0.1))
  or ({[K+]0.2 or AF-} & <mv-coord>)
  or (O+ & K+ & <mv-coord>)
  or (O+ & MV+ & <mv-coord>)};
look.v: VERB_PLI(<vc-look>) or <verb-focus>;
looks.v: VERB_S_T(<vc-look>);
looked.v-d: VERB_SPPP_T(<vc-look>);
looking.v: (<vc-look> & <verb-pg,ge>) or <verb-ge-d>;

% O+ & K+: "She waved him over."
% vc-ditrans:  "She waved me goodbye"
% MV+: "who did you wave to?"
<vc-wave>:
  {@MV+}
  or ((O+ & K+) & <mv-coord>)
  or <vc-ditrans>;
wave.v: VERB_PLI(<vc-wave>);
waves.v: VERB_S_I(<vc-wave>);
waved.v-d: VERB_SPPP_I(<vc-wave>);
waving.v: (<vc-wave> & <verb-pg,ge>) or <verb-ge-d>;

<vc-repent>: {{@MV+ or ({Xc+} & @EB+)} & OF+};
repent.v disapprove.v: VERB_PLI(<vc-repent>);
repents.v disapproves.v: VERB_S_I(<vc-repent>);
repented.v-d disapproved.v-d: VERB_SPPP_I(<vc-repent>);
repenting.v disapproving.v: (<vc-repent> & <verb-pg,ge>) or <verb-ge-d>;

<vc-talk>: <vc-repent> & <vc-fill>;
talk.v: VERB_PLI(<vc-talk>);
talks.v: VERB_S_T(<vc-talk>);
talked.v-d: VERB_SPPP_T(<vc-talk>);
talking.v: (<vc-repent> & <verb-pg,ge> & <vc-fill>) or <verb-ge-d>;

<vc-consist>: {@MV+ or @EB+} & OF+;
consist.v: VERB_PLI(<vc-consist>);
consists.v: VERB_S_I(<vc-consist>);
consisted.v-d: VERB_SPPP_I(<vc-consist>);
consisting.v: (<vc-consist> & <verb-pg,ge>) or <verb-ge-d>;

<vc-die>: {K+ or OF+} & <mv-coord>;
die.v: VERB_PLI(<vc-die>);
dies.v: VERB_S_I(<vc-die>);
died.v-d: VERB_SPPP_I(<vc-die>);
dying.v: (<vc-die> & <verb-pg,ge>) or <verb-ge-d>;

% I- & Bt- & WV-: "How many yars did it last?"
% Negative cost to discourage bad linkage with last.a
<vc-last>: {({[[@MV+]]} & Ot+) or Bt-} & <mv-coord>;
last.v wait.v: VERB_PLI(<vc-last>) or [I- & Bt- & WV-]-0.1;
lasts.v waits.v: VERB_S_I(<vc-last>);
lasted.v-d waited.v-d: VERB_SPPP_I(<vc-last>);
lasting.v waiting.v: <verb-pg> & <vc-last>;
lasting.g waiting.g: (<vc-last> & <verb-ge>) or <verb-ge-d>;

% TRANSITIVE COMPLEX VERBS (Those that take O+)

<vc-attempt>: <vc-trans> or ({@MV+} & <to-verb>);

attempt.v undertake.v deserve.v manage.v plot.v prefer.v neglect.v
afford.v commit.v profess.v desire.v please.v:
  VERB_PLI(<vc-attempt>);

attempts.v undertakes.v manages.v plots.v prefers.v
neglects.v affords.v commits.v professes.v:
  VERB_S_T(<vc-attempt>);

attempted.v managed.v plotted.v preferred.v neglected.v
afforded.v committed.v professed.v-d:
  VERB_SPPP_T(<vc-attempt>) or
  <verb-pv> or
  <verb-adj> or
  <verb-phrase-opener>;

undertook.v-d: VERB_SP_T(<vc-attempt>);
undertaken.v: VERB_PP(<vc-attempt>) or <verb-pv>;

attempting.g undertaking.g deserving.g preferring.g
neglecting.g affording.g committing.g desiring.g
pleasing.g:
  (<vc-attempt> & <verb-ge>);

% <verb-ge-d>: "The constant plotting annoyed Jim"
plotting.g professing.g:
  (<vc-attempt> & <verb-ge>) or <verb-ge-d>;

managing.g:
  (<vc-attempt> & <verb-ge>) or <verb-ge-d> or <verb-adj>;

attempting.v undertaking.v deserving.v managing.v plotting.v
preferring.v neglecting.v affording.v committing.v professing.v
desiring.v pleasing.v:
  <verb-pg> & <vc-attempt>;

% <vc-please>: "he gets as he deserves."
deserves.v desires.v pleases.v:
  VERB_S_T(<vc-attempt>) or <vc-please>;

deserved.v-d desired.v-d pleased.v-d:
  VERB_SPPP_T(<vc-attempt>) or
  <verb-pv> or
  <verb-adj> or
  <vc-please> or
  <verb-phrase-opener>;

% like <vc-trans> but with particle
<vc-seek>:
  ({@MV+} & <to-verb>)
  or ((((O+ or <b-minus>) & {K+})
    or (K+ & {[[@MV+]]} & O*n+)
    or ([[@MV+ & O*n+]])) & <mv-coord>);

seek.v: VERB_PLI(<vc-seek>);
seeks.v: VERB_S_T(<vc-seek>);
sought.v-d: VERB_SPPP_T(<vc-seek>) or <verb-pv> or <verb-phrase-opener>;
seeking.g: (<vc-seek> & <verb-ge>) or <verb-ge-d>;
seeking.v: <verb-pg> & <vc-seek>;

% Naked @MV+: "She volunteered as a nurse."
<vc-decline>: {<vc-trans>} or ({@MV+} & <to-verb>) or <mv-coord>;

decline.v fail.v hasten.v volunteer.v aim.v:
  VERB_PLI(<vc-decline>);

declines.v fails.v hastens.v volunteers.v aims.v:
  VERB_S_T(<vc-decline>);

declined.v-d hastened.v-d volunteered.v-d aimed.v-d:
  VERB_SPPP_T(<vc-decline>) or
  <verb-pv> or
  <verb-phrase-opener>;

failed.v-d:
  VERB_SPPP_T(<vc-decline>) or
  <verb-pv> or
  <verb-adj>;

declining.g failing.g hastening.g volunteering.g:
  (<vc-decline> & <verb-ge>) or
  <verb-ge-d> or
  <verb-adj>;
declining.v failing.v hastening.v volunteering.v aiming.v:
  <verb-pg> & <vc-decline>;

% like <vc-trans> but with particle
<vc-fight>:
  ({@MV+} & <to-verb>) or
  ({({O+ or <b-minus>} & {K+}) or
    (K+ & {[[@MV+]]} & O*n+) or
    [[@MV+ & O*n+]]} & <mv-coord>);
fight.v: VERB_PLI(<vc-fight>);
fights.v: VERB_S_T(<vc-fight>);
fought.v-d: VERB_SPPP_T(<vc-fight>) or (<verb-pv-b> & {K+} & <mv-coord>) or
({K+} & <verb-phrase-opener>);
fighting.g: (<vc-fight> & <verb-ge>) or <verb-ge-d>;
fighting.v: <verb-pg> & <vc-fight>;

% TO+ & Xc+: allows null-infinitive: "I did not mean to".
<vc-threaten>:
  {<vc-trans>}
  or (<mv-coord> & <null-verb>)
  or ({@MV+} & (<to-verb> or TH+ or Zs- or <embed-verb>));

threaten.v mean.v arrange.v pledge.v:
  VERB_PLI(<vc-threaten>);
threatens.v means.v arranges.v pledges.v:
  VERB_S_T(<vc-threaten>) or <vc-please>;

threatened.v-d meant.v-d arranged.v-d pledged.v-d:
  VERB_SPPP_T(<vc-threaten>) or
  <vc-please> or
  <verb-pv> or
  <verb-adj> or
  <verb-phrase-opener>;

meaning.g arranging.g threatening.g pledging.g:
  (<vc-threaten> & <verb-ge>) or <verb-ge-d>;
meaning.v arranging.v threatening.v pledging.v:
  <verb-pg> & <vc-threaten>;

<vc-plan>:
  <vc-trans> or
  ({@MV+} & {<to-verb> or TH+ or Zs- or <embed-verb> or VC+});
plan.v confess.v: VERB_PLI(<vc-plan>);
plans.v confesses.v: VERB_S_T(<vc-plan>);
planned.v-d confessed.v-d:
  VERB_SPPP_T(<vc-plan>) or
  <verb-pv> or
  <verb-adj> or
  <verb-phrase-opener>;
planning.g confessing.g: (<vc-plan> & <verb-ge>) or <verb-ge-d>;
planning.v confessing.v: <verb-pg> & <vc-plan>;

% <vc-trans> & QN+: "you decided that when?"
<vc-decide>:
  (<vc-trans> & {{Xc+} & QN+}) or
  ({@MV+} & {TH+ or {<QN+pref>} or
    <to-verb> or <embed-verb> or RSe+ or Zs-});

decide.v resolve.v: VERB_PLI(<vc-decide>);
decides.v resolves.v: VERB_S_T(<vc-decide>);
decided.v-d resolved.v-d:
  VERB_SPPP_T(<vc-decide>) or
  (<verb-s-pv> & {THi+}) or
  <verb-phrase-opener>;

deciding.v resolving.v: <verb-pg> & <vc-decide>;
deciding.g resolving.g: (<vc-decide> & <verb-ge>) or <verb-ge-d>;

% TO+ & Xc+: allows null-infinitive: "Oh gosh, I forgot to"
<vc-forget>:
  {<vc-trans>}
  or (<mv-coord> & <null-verb>)
  or ({@MV+} & (<QN+pref> or TH+ or <to-verb> or <embed-verb> or RSe+ or Zs- or Pg+));


remember.v forget.v: VERB_PLI(<vc-forget>);
remembers.v forgets.v: VERB_S_T(<vc-forget>);
remembered.v-d: VERB_SPPP_T(<vc-forget>) or <verb-pv> or <verb-phrase-opener>;
forgot.v-d: VERB_SP_T(<vc-forget>);
forgotten.v:
  VERB_PP(<vc-forget>) or
  <verb-pv> or
  <verb-adj> or
  <verb-phrase-opener>;
remembering.g forgetting.g: (<vc-forget> & <verb-ge>) or <verb-ge-d>;
remembering.v forgetting.v: <verb-pg> & <vc-forget>;

% OF+ & QN+: "you learned of this when?"
<vc-learn>:
  {<vc-trans>}
  or ({@MV+} & (TH+ or <to-verb> or <embed-verb> or RSe+ or Zs-
     or <of-coord>))
  or QN+;

learn.v: VERB_PLI(<vc-learn>);
learns.v: VERB_S_T(<vc-learn>);
learned.v-d: VERB_SPPP_T(<vc-learn>) or (<verb-pv> & {THi+}) or <verb-phrase-opener>;
learning.g: (<vc-learn> & <verb-ge>) or <verb-ge-d>;
learning.v: <verb-pg> & <vc-learn>;

% TO+ & Xc+: allows null-infinitive: "I did not propose to"
% O+ & MV+ & QN+: "You proposed to her when?"
<vc-propose>:
  <vc-trans>
  or (<mv-coord> & <null-verb>)
  or ({@MV+} & (<to-verb>
    or TH+ or <embed-verb> or RSe+ or QN+
    or Z- or Pg+ or TS+ or (SI*j+ & I*j+)));
propose.v: VERB_PLI(<vc-propose>);
proposes.v: VERB_S_T(<vc-propose>);

% <vc-propose> & QN+: "You proposed this to her when?"
proposed.v-d:
  VERB_SPPP_T(<vc-propose> & {{Xc+} & QN+})
  or (<verb-s-pv> & {THi+ or TSi+ or Z-})
  or <verb-adj>
  or <verb-phrase-opener>;
proposing.g: (<vc-propose> & <verb-ge>) or <verb-ge-d>;
proposing.v: <verb-pg> & <vc-propose>;

% TO+ & Xc+: allows null-infinitive: "I did not demand to"
% <vc-trans> & QN+: "You demand this why?"
% O+ & <of-coord>: "You demanded this of him why?"
<vc-demand>:
  (<vc-trans>)
  or ({O+} & <of-coord>)
  or (<mv-coord> & <null-verb>)
  or ({@MV+} & ((<to-verb> or TH+ or Z- or TS+ or ((SI*j+ or SFI**j+) & I*j+))));
demand.v: VERB_PLI(<vc-demand>);
demands.v: VERB_S_T(<vc-demand>);

demanded.v-d:
  VERB_SPPP_T(<vc-demand>)
  or <verb-pv>
  or <verb-phrase-opener>;
demanding.v: <verb-pg> & <vc-demand>;
demanding.g: (<vc-demand> & <verb-ge>) or <verb-ge-d>;

<vc-beg>:
  {<vc-trans>}
  or <of-coord>
  or ({@MV+} & ((<to-verb> or TH+ or Z- or TS+ or ((SI*j+ or SFI**j+) & I*j+))));
beg.v plead.v: VERB_PLI(<vc-beg>);
begs.v pleads.v: VERB_S_T(<vc-beg>);
begged.v-d pleaded.v-d:
  VERB_SPPP_T(<vc-beg>)
  or <verb-pv>
  or <verb-phrase-opener>;
begging.v pleading.v: <verb-pg> & <vc-beg>;
begging.g pleading.g: (<vc-beg> & <verb-ge>) or <verb-ge-d>;

<vc-bear>: <vc-trans>;
bear.v:
  (<verb-i> & (<vc-bear> or ({@MV+} & (Pg+ or TH+ or <to-verb>)))) or
  (<verb-pl> & <vc-bear>);
bears.v: VERB_S_T(<vc-bear>);
bore.v-d: VERB_SPPP_T(<vc-bear>);
born.v:
  VERB_PP(<vc-bear>)
  or <verb-pv>
  or <verb-phrase-opener>;

bearing.g: (<vc-bear> & <verb-ge>) or <verb-ge-d>;
bearing.v: <verb-pg> & <vc-bear>;

% [TO+]: allows null-infinitive: "Yes, I'd love to."
<vc-love>:
  <vc-trans> or
  ({@MV+} & (<to-verb> or [TO+] or Pg+));

love.v dislike.v hate.v: VERB_PLI(<vc-love>);
loves.v dislikes.v hates.v: VERB_S_T(<vc-love>);

loved.v-d disliked.v-d hated.v-d:
  VERB_SPPP_T(<vc-love>) or
  <verb-pv> or
  <verb-phrase-opener>;

loving.g disliking.g hating.g: (<vc-love> & <verb-ge>) or <verb-ge-d>;
loving.v disliking.v hating.v: <verb-pg> & <vc-love>;

% "It begins here"
% Pa+: "They continued, undaunted"
<vc-begin>:
  {<vc-trans>}
  or ({@MV+} & ({<to-verb>} or Pg+ or ({Xc+} & Pa+)));

begin.v continue.v cease.v: VERB_PLI(<vc-begin>);
begins.v continues.v ceases.v: VERB_S_T(<vc-begin>);
ceased.v-d:
  VERB_SPPP_T(<vc-begin>) or
  <verb-pv> or
  <verb-phrase-opener>;
continued.v-d:
  VERB_SPPP_T(<vc-begin>) or
  <verb-pv> or
  <verb-phrase-opener> or
  <verb-adj>;
began.v-d: VERB_SP_T(<vc-begin>);

begun.v: VERB_PP(<vc-begin>) or <verb-pv> or <verb-phrase-opener>;
beginning.g ceasing.g:
  (<vc-begin> & <verb-ge>) or <verb-ge-d>;
continuing.g: (<vc-begin> & <verb-ge>) or <verb-ge-d> or <verb-adj>;
beginning.v continuing.v ceasing.v: <verb-pg> & <vc-begin>;

% <vc-trans> with particle
% <vc-trans> & QN+: "you started this when?"
<vc-start>:
  ((({O+ or <b-minus>} & {K+ or QN+}) or
    (K+ & {[[@MV+]]} & O*n+) or
    [[@MV+ & O*n+]]) & <mv-coord>) or
  ({@MV+} & ((<to-verb> & {{Xc+} & QN+}) or Pg+));

start.v stop.v try.v: VERB_PLI(<vc-start>);
starts.v stops.v tries.v: VERB_S_T(<vc-start>);

% <verb-sp> & Xc+ & Pa+: "he stopped, unable to continue"
started.v-d stopped.v-d tried.v-d:
  VERB_SPPP_T(<vc-start>) or
  (<verb-pv-b> & {K+} & <mv-coord>) or
  (<verb-sp> & Xc+ & Pa+) or
  ({K+} & <verb-phrase-opener>);

starting.g stopping.g trying.g
startin'.g startin.g stoppin'.g stoppin.g tryin'.g tryin.g:
  (<vc-start> & <verb-ge>) or <verb-ge-d>;
starting.v stopping.v trying.v
startin'.v startin.v stoppin'.v stoppin.v tryin'.v tryin.v:
  <verb-pg> & <vc-start>;

% The Pg+ doesn't really apply to all of these ...
<vc-dispute>: <vc-trans> or
  ({@MV+} & (TH+ or Zs-)) or
  ({@MV+} & Pg+);
recognize.v dispute.v accept.v calculate.v
record.v deduce.v envision.v recount.v signify.v clarify.v disclose.v
recollect.v adduce.v posit.v reiterate.v infer.v presuppose.v:
  VERB_PLI(<vc-dispute>);
recognizes.v disputes.v calculates.v records.v deduces.v
accepts.v envisions.v recounts.v signifies.v clarifies.v discloses.v recollects.v
adduces.v posits.v reiterates.v infers.v presupposes.v:
  VERB_S_T(<vc-dispute>);
recognized.v-d disputed.v-d accepted.v-d calculated.v-d
recorded.v-d deduced.v-d envisioned.v-d
recounted.v-d signified.v-d clarified.v-d disclosed.v-d
recollected.v-d adduced.v-d posited.v-d
reiterated.v-d inferred.v-d presupposed.v-d:
  VERB_SPPP_T(<vc-dispute>)
  or (<verb-s-pv> & {THi+})
  or <verb-adj>
  or <verb-phrase-opener>;
recognizing.g disputing.g accepting.g calculating.g deducing.g recording.g
envisioning.g recounting.g signifying.g clarifying.g disclosing.g
recollecting.g adducing.g positing.g reiterating.g inferring.g presupposing.g:
(<vc-dispute> & <verb-ge>) or <verb-ge-d>;
recognizing.v disputing.v accepting.v calculating.v deducing.v recording.v
envisioning.v recounting.v signifying.v clarifying.v disclosing.v
recollecting.v adducing.v positing.v reiterating.v inferring.v presupposing.v:
<verb-pg> & <vc-dispute>;

undisputed.v: (<verb-s-pv> & {THi+});

% MVp+: "it repeated for ..." "She provided for ..."
<vc-repeat>: {<vc-trans>} or ({@MV+} & TH+) or MVp+;
repeat.v reflect.v provide.v counter.v signal.v: VERB_PLI(<vc-repeat>);
repeats.v reflects.v provides.v counters.v signals.v: VERB_S_T(<vc-repeat>);
repeated.v-d reflected.v-d countered.v-d signaled.v-d signalled.v-d:
  VERB_SPPP_T(<vc-repeat>) or
  <verb-pv> or
  <verb-adj> or
  <verb-phrase-opener>;
provided.v-d:
  VERB_SPPP_T(<vc-repeat>) or
  <verb-pv> or
  <verb-phrase-opener> or
  <verb-adj> or
  ((TH+ or <embed-verb>) & (({{Xd-} & Xc+} & dCO+) or ({Xd- & Xc+} & MVs-)));
repeating.v reflecting.v providing.v countering.v signaling.v signalling.v:
  <verb-pg> & <vc-repeat>;
repeating.g reflecting.g countering.g signaling.g signalling.g:
  (<vc-repeat> & <verb-ge>) or <verb-ge-d>;
providing.g:
  (<vc-repeat> & <verb-ge>) or
  <verb-ge-d> or
  ((TH+ or <embed-verb>) & (({{Xd-} & Xc+} & dCO+) or ({Xd- & Xc+} & MVs-)));

% -----------------------------------------------------
<vc-sense>:
  <vc-trans>
  or ({@MV+} & (<embed-verb> or TH+ or RSe+ or Pg+));

sense.v doubt.v reaffirm.v reckon.v regret.v ascertain.v discern.v
stipulate.v affirm.v certify.v trust.v postulate.v ensure.v imply.v verify.v:
  VERB_PLI(<vc-sense>);
senses.v reaffirms.v doubts.v reckons.v regrets.v
stipulates.v ascertains.v discerns.v affirms.v certifies.v trusts.v
postulates.v ensures.v implies.v verifies.v:
  VERB_S_T(<vc-sense>);
doubted.v-d reaffirmed.v-d sensed.v-d reckoned.v-d regretted.v-d stipulated.v-d
ascertained.v-d discerned.v-d affirmed.v-d certified.v-d
trusted.v-d postulated.v-d ensured.v-d implied.v-d verified.v-d:
  VERB_SPPP_T(<vc-sense>)
  or (<verb-s-pv> & {THi+})
  or <verb-adj>
  or <verb-phrase-opener>;

sensing.v doubting.v reckoning.v reaffirming.v stipulating.v
regretting.v ascertaining.v discerning.v
affirming.v certifying.v trusting.v postulating.v ensuring.v
implying.v verifying.v:
  <verb-pg> & <vc-sense>;

reaffirming.g sensing.g doubting.g stipulating.g
reckoning.g regretting.g ascertaining.g
discerning.g affirming.g certifying.g trusting.g
postulating.g ensuring.g implying.g verifying.g:
  (<vc-sense> & <verb-ge>) or <verb-ge-d>;

% -----------------------------------------------------
% ditrans: "She proclaimed him a liar and a thief"
<vc-proclaim>:
  <vc-trans>
  or ({@MV+} & (<embed-verb> or TH+ or RSe+ or Pg+))
  or <vc-ditrans>;

proclaim.v:
  VERB_PLI(<vc-proclaim>);
proclaims.v:
  VERB_S_T(<vc-proclaim>);
proclaimed.v-d:
  VERB_SPPP_T(<vc-proclaim>)
  or (<verb-s-pv> & {THi+})
  or <verb-adj>
  or <verb-phrase-opener>;
proclaiming.v:
  <verb-pg> & <vc-proclaim>;
proclaiming.g:
  (<vc-proclaim> & <verb-ge>) or <verb-ge-d>;

% -----------------------------------------------------
% Pv+ link: "John imagines himself lost in the woods."
% Pg+ link: "John imagines himself singing from a mountaintop"
% AZ+ link: "John imagined Mary as innocent as a lamb"
% similar to vc-see
<vc-imagine>: <vc-trans> or
  ({@MV+} & (<embed-verb> or TH+ or RSe+ or Z-)) or
  ((O+ or <b-minus>) & {@MV+} & {Pg+ or AZ+ or Pv+}) or
  ({@MV+} & Pg+);
imagine.v: VERB_PLI(<vc-imagine>);
imagines.v:  VERB_S_T(<vc-imagine>);
imagined.v:
  VERB_SPPP_T(<vc-imagine>)
  or (<verb-s-pv> & {THi+})
  or <verb-adj>
  or <verb-phrase-opener>;
imagining.g: (<vc-imagine> & <verb-ge>) or <verb-ge-d>;
imagining.v: <verb-pg> & <vc-imagine>;

% Pa**j+: "The doctor declared him insane."
% MVa+: "he will suspect soon", "he suspects already"
<vc-declare>:
  <vc-trans>
  or ({@MV+} & (<embed-verb> or TH+ or RSe+ or Pg+ or Z-))
  or ((O+ or <b-minus>) & ({@MV+} & Pa**j+))
  or MVa+;

declare.v conclude.v suspect.v concede.v foresee.v
emphasize.v maintain.v acknowledge.v note.v confirm.v stress.v:
  VERB_PLI(<vc-declare>) or <verb-manner>;

declares.v concludes.v suspects.v concedes.v foresees.v
emphasizes.v maintains.v acknowledges.v notes.v
confirms.v stresses.v:
  VERB_S_T(<vc-declare>) or <verb-manner>;

declared.v concluded.v suspected.v conceded.v
emphasized.v maintained.v acknowledged.v noted.v
confirmed.v-d stressed.v-d:
  VERB_SPPP_T(<vc-declare>)
  or (<verb-s-pv> & {THi+ or ({@MV+} & Pa+) })
  or <verb-adj>
  or <verb-phrase-opener>;

foresaw.v-d: VERB_SP_T(<vc-declare>) or <verb-manner>;
foreseen.v:
  VERB_PP(<vc-declare>) or
  (<verb-s-pv> & {@MV+ or THi+}) or
  <verb-adj> or
  <verb-phrase-opener>;

declaring.g concluding.g suspecting.g conceding.g
foreseeing.g emphasizing.g maintaining.g acknowledging.g
noting.g confirming.g stressing.g:
  (<vc-declare> & <verb-ge>) or <verb-ge-d>;
declaring.v concluding.v suspecting.v conceding.v
foreseeing.v emphasizing.v maintaining.v acknowledging.v
noting.v confirming.v stressing.v:
  <verb-pg> & <vc-declare>;

% ------------------------------
% Almost like the above, except:
% TO+ & Xc+: allows null-infinitive: "I could not assume to"
<vc-assume>: <vc-declare> or (<mv-coord> & <null-verb>);

fear.v presume.v assume.v waver.v:
  VERB_PLI(<vc-assume>) or <verb-manner>;

fears.v presumes.v assumes.v wavers.v:
  VERB_S_T(<vc-assume>) or <verb-manner>;

feared.v presumed.v assumed.v-d wavered.v-d:
  VERB_SPPP_T(<vc-assume>)
  or (<verb-s-pv> & {THi+ or ({@MV+} & Pa+) })
  or <verb-adj>
  or <verb-phrase-opener>;

fearing.g presuming.g assuming.g wavering.g:
  (<vc-assume> & <verb-ge>) or <verb-ge-d>;
fearing.v presuming.v assuming.v wavering.v:
  <verb-pg> & <vc-assume>;

% ---------------------------------------------------------------

% O+ & Pa**j+: "I believed it possible."
<vc-believe>:
  {<vc-trans>}
  or (O+ & {@MV+} & (O*n+ or Pa**j+))
  or (<mv-coord> & (<embed-verb> or TH+ or RSe+));

believe.v answer.v worry.v: VERB_PLI(<vc-believe>);
believes.v answers.v worries.v: VERB_S_T(<vc-believe>);
believed.v-d answered.v-d worried.v-d:
  VERB_SPPP_T(<vc-believe>) or
  (<verb-s-pv> & {THi+ or <tof-verb>}) or
  <verb-phrase-opener>;
believing.g answering.g worrying.g:
  (<vc-believe> & <verb-ge>) or <verb-ge-d>;
believing.v answering.v worrying.v: <verb-pg> & <vc-believe>;


% resembles <vc-trans> with particle
<vc-rule>:
  ({@MV+} & (<embed-verb> or TH+ or RSe+)) or
  ((({O+ or <b-minus>} & {K+}) or
     (K+ & {[[@MV+]]} & O*n+) or
     [[@MV+ & O*n+]]) & <mv-coord>);
rule.v add.v: VERB_PLI(<vc-rule>);
rules.v adds.v: VERB_S_T(<vc-rule>);
ruled.v-d added.v-d:
  VERB_SPPP_T(<vc-rule>) or
  (<verb-s-pv-b> & {({@MV+} & (THi+ or <tof-verb>)) or ({K+} & <mv-coord>)}) or
  ({K+} & <verb-phrase-opener>) or
  <verb-adj>;
ruling.g adding.g: (<vc-rule> & <verb-ge>) or <verb-ge-d>;
ruling.v adding.v: <verb-pg> & <vc-rule>;

% <vc-trans> with particle
<vc-figure>:
  ({@MV+} & (TH+ or Zs- or <embed-verb>)) or
  ((((O+ or <b-minus>) & {K+}) or (K+ & {[[@MV+]]} & O*n+)) & <mv-coord>) or
  ([[@MV+ & O*n+]]);
figure.v: VERB_PLI(<vc-figure>);
figures.v: VERB_S_T(<vc-figure>);
figured.v-d:
  VERB_SPPP_T(<vc-figure>) or
  (<verb-pv-b> & {K+} & <mv-coord>) or
  ({K+} & <verb-phrase-opener>);
figuring.g: (<vc-figure> & <verb-ge>) or <verb-ge-d>;
figuring.v: <verb-pg> & <vc-figure>;

% (QI+ & {MV+}): "I did not say why until recently"
% <vc-trans> & QN+: "you said this to her where?"
%    "you indicate this why?"
% O+ & I*j+: "I would have said so, were he there"
%            Could also have solved this with <vc-trans> & {I*j+}
<vc-predict>:
  (<vc-trans> & {{Xc+} & QN+})
  or (O+ & <mv-coord> & I*j+)
  or ({@MV+} & (<embed-verb> or TH+ or RSe+ or Zs- or VC+))
  or ({@MV+} & (<QI+pref> & {MV+}));

% See also: words.v.10 for paraphrasing verbs
% I- & B- & <embed-verb>: "What did John say you should do?"
predict.v realize.v discover.v determine.v announce.v say.v mention.v admit.v
recall.v reveal.v divulge.v state.v observe.v indicate.v stammer.v bawl.v
analyse.v analyze.v assess.v establish.v evaluate.v examine.v question.v test.v
hypothesize.v hypothesise.v document.v envisage.v:
  VERB_PLI(<vc-predict>)
  or (I- & <b-minus> & <embed-verb>);

predicts.v realizes.v discovers.v determines.v announces.v says.v
mentions.v admits.v recalls.v reveals.v divulges.v states.v observes.v
indicates.v stammers.v bawls.v
analyses.v analyzes.v assesses.v establishes.v evaluates.v examines.v
questions.v tests.v hypothesizes.v hypothesises.v envisages.v
documents.v:
  VERB_S_T(<vc-predict>);

predicted.v-d realized.v-d discovered.v-d determined.v-d announced.v-d
mentioned.v-d admitted.v-d recalled.v-d revealed.v-d divulged.v-d
stated.v-d observed.v-d indicated.v-d stammered.v-d bawled.v-d
analysed.v-d analyzed.v-d
assessed.v-d established.v-d evaluated.v-d examined.v-d questioned.v-d
tested.v-d hypothesized.v-d hypothesised.v-d well-established.v-d
envisaged.v-d documented.v-d:
  VERB_SPPP_T(<vc-predict>)
  or (<verb-s-pv> & ({THi+} or QI+))
  or <verb-adj>
  or <verb-phrase-opener>;

% The second line is almost, not quite, <verb-s-pv>
said.v-d:
  VERB_SPPP_T(<vc-predict>) or
  ({@E-} & ((Pvf- & <verb-wall>) or [[Mv-]]) & {@MV+} & {THi+}) or
  <verb-adj> or
  [[<verb-phrase-opener>]];

predicting.g realizing.g discovering.g determining.g
announcing.g saying.g mentioning.g admitting.g recalling.g revealing.g
divulging.g stating.g observing.g indicating.g stammering.g bawling.g
analysing.g analyzing.g assessing.g establishing.g evaluating.g examining.g
questioning.g testing.g hypothesizing.g hypothesising.g documenting.g envisaging.g:
  (<vc-predict> & <verb-ge>) or <verb-ge-d>;
predicting.v realizing.v discovering.v determining.v
announcing.v saying.v mentioning.v admitting.v recalling.v revealing.v
divulging.v stating.v observing.v indicating.v stammering.v bawling.v
analysing.v analyzing.v assessing.v establishing.v evaluating.v examining.v
questioning.v testing.v
hypothesizing.v hypothesising.v documenting.v envisaging.v:
<verb-pg> & <vc-predict>;

<vc-guess>: {<vc-trans>} or
  ({@MV+} & (TH+ or <QI+pref> or <embed-verb> or RSe+ or Zs-));

% esplain: basilect of explain
guess.v estimate.v understand.v notice.v explain.v esplain.v demonstrate.v:
  VERB_PLI(<vc-guess>);
guesses.v estimates.v understands.v notices.v explains.v esplains.v
demonstrates.v:
  VERB_S_T(<vc-guess>);
guessed.v-d understood.v-d noticed.v-d explained.v-d esplained.v-d
demonstrated.v-d:
  VERB_SPPP_T(<vc-guess>)
  or (<verb-s-pv> & {THi+})
  or <verb-adj>
  or <verb-phrase-opener>;
estimated.v-d:
  VERB_SPPP_T(<vc-guess>) or
  (<verb-s-pv> & {THi+}) or
  <verb-adj> or
  <verb-phrase-opener>;
guessing.g estimating.g understanding.g noticing.g explaining.g
demonstrating.g: (<vc-guess> & <verb-ge>) or <verb-ge-d>;
guessing.v estimating.v understanding.v noticing.v explaining.v
demonstrating.v: <verb-pg> & <vc-guess>;

% (QI+ & {MV+}): "I did not know why until recently"
% MVa+: "He knows already"
% MVb+: "He should know better"
<vc-know>:
  {<vc-trans>}
  or ({@MV+} & (((OF+ or QI+) & <mv-coord>) or <embed-verb> or TH+ or RSe+ or Zs-))
  or ({Xc+} & (MVa+ or MVb+));
know.v: VERB_PLI(<vc-know> & {dCPu-}) or <verb-manner>;
knows.v: VERB_S_T(<vc-know>) or <verb-manner>;
knew.v-d: VERB_SP_T(<vc-know>);

% dCPu-: "... as is well known"
known.v well-known.v:
  VERB_PP(<vc-know>) or
  (<verb-s-pv> & {THi+ or <tof-verb> or QIi+} & {dCPu-}) or
  <verb-phrase-opener> or
  <verb-adj>;
knowing.g: (<vc-know> & <verb-ge>) or <verb-ge-d>;
knowing.v: <verb-pg> & <vc-know>;

% <vc-trans> & QN+: "You requested these favors why?"
<vc-request>: (<vc-trans> & {{Xc+} & QN+}) or
  ({@MV+} & (TH+ or <embed-verb> or RSe+ or Zs- or TS+ or ((SI*j+ or SFI**j+) & I*j+)));
request.v: VERB_PLI(<vc-request>);
requests.v: VERB_S_T(<vc-request>);
requested.v-d:
  VERB_SPPP_T(<vc-request>) or
  (<verb-s-pv> & {THi+ or TSi+}) or
  <verb-adj> or
  <verb-phrase-opener>;
requesting.g: (<vc-request> & <verb-ge>) or <verb-ge-d>;
requesting.v: <verb-pg> & <vc-request>;

% XXX why is there a cost on Pv ??
<vc-feel>: <vc-trans> or
  ({@MV+} &
    (Pa+ or TH+ or <embed-verb> or
    RSe+ or AF- or Vf+ or
    LI+ or <mv-coord> or [Pv+] or
    (O+ & (I*j+ or Pg+))));
feel.v: VERB_PLI(<vc-feel>);
feels.v: VERB_S_T(<vc-feel>);
felt.v-d:
  VERB_SPPP_T(<vc-feel>)
  or (<verb-s-pv> & {THi+})
  or <verb-phrase-opener>;
feeling.g: (<vc-feel> & <verb-ge>) or <verb-ge-d>;
feeling.v: <verb-pg> & <vc-feel>;

<vc-mind>: {<vc-trans>} or
  ({@MV+} & (QI+ or TH+ or Pg+));
mind.v: VERB_PLI(<vc-mind>);
minds.v: VERB_S_T(<vc-mind>);
minded.v-d: VERB_SPPP_T(<vc-mind>) or <verb-pv> or <verb-phrase-opener>;
minding.g: (<vc-mind> & <verb-ge>) or <verb-ge-d>;
minding.v: <verb-pg> & <vc-mind>;

% <verb-pv> & QN+: "Anesthesiology is studied where?"
<vc-study>: {<vc-trans>} or ({@MV+} & <QN+pref>);
study.v: VERB_PLI(<vc-study>);
studies.v: VERB_S_T(<vc-study>);
studied.v-d:
  VERB_SPPP_T(<vc-study>) or
  (<verb-pv> & {QN+}) or
  <verb-adj> or
  <verb-phrase-opener>;
studying.g: (<vc-study> & <verb-ge>) or <verb-ge-d>;
studying.v: <verb-pg> & <vc-study>;

% QI+ link: "I will discuss which vitamins I take"
% <verb-pv> & QN+: "It was discussed where?"
<vc-discuss>: <vc-trans> or ({@MV+} & (Pg+ or QI+));
discuss.v: VERB_PLI(<vc-discuss>);
discusses.v: VERB_S_T(<vc-discuss>);
discussed.v-d:
  VERB_SPPP_T(<vc-discuss>)
  or (<verb-pv> & {QN+})
  or <verb-adj>
  or <verb-phrase-opener>;
discussing.g: (<vc-discuss> & <verb-ge>) or <verb-ge-d>;
discussing.v:
<verb-pg> & <vc-discuss>;

<vc-oppose>: <vc-trans> or ({@MV+} & Pg+);
oppose.v enjoy.v advocate.v contemplate.v entail.v necessitate.v
justify.v risk.v avoid.v involve.v favor.v:
  VERB_PLI(<vc-oppose>);
opposes.v enjoys.v advocates.v contemplates.v entails.v necessitates.v
justifies.v risks.v avoids.v involves.v favors.v:
  VERB_S_T(<vc-oppose>);

opposed.v-d enjoyed.v-d advocated.v-d contemplated.v-d entailed.v-d
necessitated.v-d justified.v-d risked.v-d avoided.v-d involved.v-d favored.v-d:
  VERB_SPPP_T(<vc-oppose>) or
  (<verb-pv> & {QN+}) or
  <verb-adj> or
  <verb-phrase-opener>;

opposing.g enjoying.g advocating.g contemplating.g
entailing.g necessitating.g justifying.g risking.g avoiding.g
favoring.g involving.g:
  (<vc-oppose> & <verb-ge>) or <verb-ge-d>;
opposing.v enjoying.v advocating.v contemplating.v
entailing.v necessitating.v justifying.v risking.v avoiding.v involving.v
favoring.v:
  <verb-pg> & <vc-oppose>;

% MVp+: "he finished at last"
<vc-finish>: {<vc-trans>} or ({@MV+} & Pg+) or (MVp+ & {VC+});
finish.v practice.v resist.v: VERB_PLI(<vc-finish>);
finishes.v practices.v resists.v quits.v: VERB_S_T(<vc-finish>);

% <verb-pv>: "I want it finished"
finished.v-d practiced.v-d resisted.v-d quitted.v-d:
  VERB_SPPP_T(<vc-finish> or ({Xc+} & Pa+))
  or (<verb-pv> & {QN+})
  or <verb-adj>
  or <verb-phrase-opener>;
quit.v-d:
  VERB_PLI(<vc-finish>) or
  VERB_SPPP_T(<vc-finish> or ({Xc+} & Pa+))
  or <verb-pv>
  or <verb-adj>
  or <verb-phrase-opener>;

finishing.g practicing.g resisting.g quitting.g:
  (<vc-finish> & <verb-ge>) or <verb-ge-d>;

finishing.v practicing.v resisting.v quitting.v:
  <verb-pg> & <vc-finish>;

% Pv-: "I want it over with"
over_with: <verb-pv-b>;

% <vc-trans> with particle
% and also Pa**j for "The witch turned him green"
<vc-turn>:
  ((O+
    or (K+ & {[[@MV+]]} & O*n+)
    or ({O+ or <b-minus>} & {K+})
    or ((O+ or <b-minus>) & Pa**j+)
    or [[@MV+ & O*n+]]) & <mv-coord>)
  or ({@MV+} & (Pa+ or AF-));

turn.v: VERB_PLI(<vc-turn>);
turns.v: VERB_S_T(<vc-turn>);
turned.v-d:
  VERB_SPPP_T(<vc-turn>)
  or (<verb-pv-b> & {K+} & <mv-coord>)
  or ({K+} & <verb-phrase-opener>);
turning.v: <verb-pg> & <vc-turn>;
turning.g: (<vc-turn> & <verb-ge>) or <verb-ge-d>;

% <vc-trans> plus TI
<vc-become>:
  ((O+ or <b-minus> or TI+ or [[@MV+ & (O*n+ or TI+)]] or Pv+) & <mv-coord>)
   or ({@MV+} & (AF- or Pa+) & {QN+});
become.v: VERB_S_PLI(<vc-become>) or (<verb-s-pp> & <vc-become>) or <verb-pv>;
becomes.v: VERB_S_S(<vc-become>);
became.v-d: VERB_S_SP(<vc-become>);
becoming.g: (<vc-become> & <verb-ge>) or <verb-ge-d>;
becoming.v: <verb-pg> & <vc-become>;

% XXX Why is there a cost on Pv+ ???
% <vc-trans> plus TI
<vc-remain>:
  ({@MV+} & (AF- or Pa+))
  or ({O+ or <b-minus>
    or TI+
    or [[@MV+ & (O*n+ or TI+)]]
    or [Pv+]} & <mv-coord>);
remain.v: VERB_PLI(<vc-remain>);
remains.v: VERB_S_T(<vc-remain>);
remained.v-d: VERB_SPPP_T(<vc-remain>);
remaining.g: (<vc-remain> & <verb-ge>) or <verb-ge-d> or <verb-adj>;
remaining.v: <verb-pg> & <vc-remain>;

% <vc-trans> plus particle
<vc-grow>:
  ({@MV+} & (AF- or Pa+))
  or ((({O+ or <b-minus>} & {K+})
    or (K+ & {[[@MV+]]} & O*n+)
    or [[@MV+ & O*n+]]) & <mv-coord>);

% <verb-i> & QN+: "The crime rate began to grow when?"
grow.v: VERB_PLI(<vc-grow>) or <verb-sip> or (<verb-i> & QN+);
grows.v: VERB_S_T(<vc-grow>) or <verb-si>;
grew.v-d: VERB_SP_T(<vc-grow>) or <verb-si>;
grown.v:
  VERB_PP(<vc-grow>) or
  (<verb-pv-b> & {K+} & <mv-coord>) or
  <verb-adj> or
  ({K+} & <verb-phrase-opener>);
growing.g: (<vc-grow> & <verb-pg,ge>) or <verb-adj> or <verb-ge-d>;
growing.v: <verb-pg> & <vc-grow>;

% <vc-trans> plus OF
<vc-approve>: {O+ or <b-minus> or [[@MV+ & O*n+]] or ({@MV+} & OF+)} & <mv-coord>;
approve.v: VERB_PLI(<vc-approve>);
approves.v: VERB_S_T(<vc-approve>);
approved.v-d: VERB_SPPP_T(<vc-approve>) or <verb-pv> or <verb-adj>
or <verb-phrase-opener>;
approving.g: (<vc-approve> & <verb-ge>) or <verb-ge-d>;
approving.v: <verb-pg> & <vc-approve>;

% <vc-trans> plus OF
<vc-dispose>: (O+ or <b-minus> or [[@MV+ & O*n+]] or ({@MV+} & OF+)) & <mv-coord>;
dispose.v conceive.v: VERB_PLI(<vc-dispose>);
disposes.v conceives.v: VERB_S_T(<vc-dispose>);
disposed.v-d conceived.v-d: VERB_SPPP_T(<vc-dispose>) or <verb-pv> or
<verb-adj> or <verb-phrase-opener>;
disposing.g conceiving.g: (<vc-dispose> & <verb-ge>) or <verb-ge-d>;
disposing.v conceiving.v: <verb-ge> & <vc-dispose>;

% <vc-trans> plus particle
<vc-speak>:
  ((K+ & {[[@MV+]]} & O*n+)
    or ({O+ or <b-minus>} & {K+})
    or [[@MV+ & O*n+]]
    or OF+) & <mv-coord>;
speak.v: VERB_PLI(<vc-speak>);
speaks.v: VERB_S_T(<vc-speak>);
spoke.v-d spake.v-d: VERB_SP_T(<vc-speak>);
spoken.v:
  VERB_PP(<vc-speak>) or
  (<verb-pv-b> & {K+} & <mv-coord>) or
  ({K+} & <verb-phrase-opener>) or
  <verb-adj>;
speaking.v: <verb-pg> & <vc-speak>;
speaking.g: (<vc-speak> & <verb-ge>) or <verb-ge-d> or <verb-adj>;

% @MV+: "The coffee tastes (the same) as it did last year."
%      (do not want O for "the same")
<vc-taste>:
  <vc-trans>
  or ({@MV+} & ((LI+ & <mv-coord>) or AF- or Pa+ or OF+))
  or @MV+;
taste.v: VERB_PLI(<vc-taste>);
tastes.v: VERB_S_T(<vc-taste>);
tasted.v-d: VERB_SPPP_T(<vc-taste>) or <verb-pv> or <verb-phrase-opener>;
tasting.g: (<vc-taste> & <verb-ge>) or <verb-ge-d>;
tasting.v: <verb-pg> & <vc-taste>;

<vc-smell>:
  {<vc-trans>}
  or ({@MV+} & ((LI+ & <mv-coord>) or AF- or Pa+ or OF+))
  or @MV+;
reek.v smell.v: VERB_PLI(<vc-smell>);
reeks.v smells.v: VERB_S_T(<vc-smell>);
reeked.v-d smelled.v-d: VERB_SPPP_T(<vc-smell>) or <verb-pv> or <verb-phrase-opener>;
reeking.g smelling.g: (<vc-smell> & <verb-ge>) or <verb-ge-d>;
reeking.v smelling.v: <verb-pg> & <vc-smell>;

% <vc-trans> plus particle and Vt
% QN+: "you took which one?"
<vc-take>:
  (((K+ & {[[@MV+]]} & O*n+) or ((O+ or <b-minus>) & {K+ or Vt+}) or [[@MV+ & O*n+]]) & <mv-coord>)
  or ({O+} & Ot+ & {@MV+} & {<tot-verb> or <toi-verb>})
  or (OXii+ & Vtg+ & {@MV+} & TH+)
  or QN+
  or @MV+;

% If- & WV-: "How long did it take?"
take.v: VERB_S_PLI(<vc-take>) or (If- & <verb-wall>);
% conjoin: "He takes cookies and eats them."
takes.v: VERB_S_S(<vc-take>);
took.v-d: VERB_S_SP(<vc-take>);
taken.v:
  (<verb-s-pp> & <vc-take>)
  or (<verb-pv-b> & {K+} & <mv-coord>)
  or <verb-adj>
  or ({K+} & <verb-phrase-opener>)
  or (Pvf- & <verb-wall> & Vtg+ & THi+);

taking.v: <verb-s-pg> & <vc-take>;
taking.g: (<vc-take> & <verb-ge>) or <verb-ge-d>;
for_granted: Vtg-;

% VERBS TAKING [OBJ] + [OTHER COMPLEMENT]
% basically, all these are <vc-trans> plus mess
% I think the WR- here is dead and not used. See other WR- below
% O+ & QN+: "You put it where?"
<vc-put>:
  ((K+ & {[[@MV+]]} & O*n+) or
  ((O+ or <b-minus>) & (K+ or Pp+ or WR- or QN+)) or
  (Vp+ & (Zs- or MVa+))) & <mv-coord>;


% I- & WR- & <verb-wall> & O+: "where did you put it?"
% bare <verb-phrase-opener>: "The problem, put simply, is money."
put.v-d:
  VERB_SPPP_T(<vc-put>) or
  (<verb-ico> & <vc-put>) or
  ({@E-} & I- & WR- & <verb-wall> & O+) or
  (<verb-pv-b> & (K+ or Pp+ or WR-) & <mv-coord>) or
  ({K+ or Pp+} & <verb-phrase-opener>);
puts.v: VERB_S_T(<vc-put>);
putting.v: <verb-pg> & <vc-put>;
putting.g: (<vc-put> & <verb-ge>) or <verb-ge-d>;

% K+ & O*n+: "He costed out the plan"
<vc-cost>:
  ((<vc-opt-ditrans> or
    (K+ & O*n+) or
    (<b-minus> & {O+})) & <mv-coord> & {<toi-verb>}) or
  ([[@MV+ & O*n+]]);

cost.v-d: VERB_S_PLI(<vc-cost>) or VERB_S_SPPP(<vc-cost>);
costed.v-d: VERB_SPPP_T(<vc-cost>) or
  (<verb-pv-b> & (({K+} & <mv-coord>) or Pa+ or Pg+)) or
  ({K+ or Pa+ or Pg+} & <verb-phrase-opener>);
costs.v: VERB_S_S(<vc-cost>);
costing.v: <verb-s-pg> & <vc-cost>;
costing.g: (<vc-cost> & <verb-ge>) or <verb-ge-d>;

% ditransitive
<vc-find>:
  (<vc-ditrans> & <mv-coord>) or
  (K+ & {[[@MV+]]} & O*n+) or
  (<b-minus> & O+) or
  ((O+ or <b-minus>) & (({@MV+} & (Pa+ or AF- or Pg+)) or ({K+} & <mv-coord>))) or
  ([[@MV+ & O*n+]]) or
  ({@MV+} & (TH+ or <embed-verb> or RSe+));

find.v: VERB_PLI(<vc-find>);
finds.v: VERB_S_T(<vc-find>);
found.v-d: VERB_SPPP_T(<vc-find>) or
  (<verb-pv> & (({K+ or AF-} & <mv-coord>) or Pa+ or Pg+)) or
  ({K+ or Pa+ or Pg+} & <verb-phrase-opener>);
finding.v: <verb-pg> & <vc-find>;
finding.g: (<vc-find> & <verb-ge>) or <verb-ge-d>;

% ditransitive
<vc-get>:
  ((O+ or <b-minus>) & (({K+} & <mv-coord>) or ({@MV+} & (Pa+ or AF- or Pv+))))
  or ((<vc-ditrans>
    or (K+ & {[[@MV+]]} & O*n+)
    or K+
    or (<b-minus> & O+)
    ) & <mv-coord>)
  or ({@MV+} & (Pa+ or AF- or Pp+ or <to-verb> or VC+));

get.v: VERB_PLI(<vc-get>);
gets.v: VERB_S_T(<vc-get>);
got.v-d: VERB_SPPP_T(<vc-get>);

% basilect
% "I gotta go now"
% He said, "I gotta go now."
gotta.v-d:
  ({@E-} & Sp*i- & <verb-wall> & (I*t+ or Os+))
  or [I*t+ & <verb-x-pg>]0.05;

gotten.v:
  VERB_PP(<vc-get>) or
  (<verb-pv-b> & {K+ or Pp+} & <mv-coord>) or
  ({K+ or Pp+} & <verb-phrase-opener>);

getting.v gettin'.v gettin.v: <verb-pg> & <verb-wall> & <vc-get>;
getting.g: (<vc-get> & <verb-ge>) or <verb-ge-d>;

% Pa+: "He left unarmed"
<vc-leave>:
  ((O+ or <b-minus>) &
    (({K+} & <mv-coord>)
    or ({@MV+} & {Pa+ or AF- or Pv+ or Pg+})))
  or ({@MV+} & {Xc+} & (Pa+ or Pv+))
  or ({(K+ & {[[@MV+]]} & O*n+) or ([[@MV+ & O*n+]])} & <mv-coord>);

leave.v: VERB_PLI(<vc-leave>);
leaves.v: VERB_S_T(<vc-leave>);
left.v-d:
  VERB_SPPP_T(<vc-leave>)
  or (<verb-pv-b> & (({K+ or AF-} & <mv-coord>) or Pv+ or Pa+ or Pg+))
  or ({K+ or ({@MV+} & (Pv+ or Pa+ or Pg+))} & <verb-phrase-opener>);
leaving.v leavin'.v: <verb-pg> & <vc-leave>;
leaving.g leavin'.g: (<vc-leave> & <verb-ge>) or <verb-ge-d>;

<vc-keep>:
  ((O+ or (K+ & {[[@MV+]]} & O*n+) or [[@MV+ & O*n+]] or Vk+) & <mv-coord>)
  or ({O+ or <b-minus>} & (([K+] & <mv-coord>) or ({@MV+} & (Pa+ or AF- or Pg+ or Pv+))));

keep.v: VERB_PLI(<vc-keep>);
keeps.v: VERB_S_T(<vc-keep>);
kept.v-d: VERB_SPPP_T(<vc-keep>) or
(<verb-pv-b> & (({K+ or AF-} & <mv-coord>) or Pa+ or Pg+ or Pv+)) or
({K+ or ({@MV+} & (Pa+ or Pg+ or Pv+))} & <verb-phrase-opener>);
keeping.v: <verb-pg> & <vc-keep>;
keeping.g: (<vc-keep> & <verb-ge>) or <verb-ge-d>;

watch.i vigil.i pace.i: Vk-;
track.i: Vk- & {OF+};

<vc-set>:
  (
    (K+ & {[[@MV+]]} & O*n+) or
    ({O+ or <b-minus>} & {K+ or Vs+}) or
    [[@MV+ & O*n+]]
  ) & <mv-coord>;

set.v-d:
  VERB_SPPP_T(<vc-set>) or
  (<verb-ico> & <vc-set>) or
  (<verb-pv> & {K+ or Vs+} & <mv-coord>) or
  <verb-adj> or
  ({K+ or Vs+} & <verb-phrase-opener>);
sets.v: VERB_S_T(<vc-set>);
setting.v: <verb-pg> & <vc-set>;
setting.g: (<vc-set> & <verb-ge>) or <verb-ge-d> or <verb-adj>;

free.i straight.i loose.i: Vs- & {MV+};

% TH+: "The court held that ..."
<vc-hold>:
  ((K+ & {[[@MV+]]} & O*n+)
    or TH+
    or ({O+ or <b-minus>} & {K+ or Vh+})
    or [[@MV+ & O*n+]]) & <mv-coord>;

hold.v: VERB_PLI(<vc-hold>);
holds.v: VERB_S_T(<vc-hold>);
held.v-d: VERB_SPPP_T(<vc-hold>) or (<verb-pv-b> & {K+ or Vh+} & <mv-coord>) or
<verb-adj> or ({K+ or Vh+} & <verb-phrase-opener>);
holding.v: <verb-pg> & <vc-hold>;
holding.g: (<vc-hold> & <verb-ge>) or <verb-ge-d>;

hostage.i captive.i: Vh- or Vth-;

% TO+ & Xc+: allows null-infinitive: "I never expected to"
<vc-expect>:
  ({@MV+} & (<embed-verb> or TH+ or RSe+ or Z- or <to-verb>))
  or ((O+ or <b-minus> or OX+) & <mv-coord> & {<too-verb>})
  or (<mv-coord> & <null-verb>)
  or ([[@MV+ & O*n+]]);

expect.v claim.v: VERB_PLI(<vc-expect>);
expects.v claims.v: VERB_S_T(<vc-expect>);
expected.v-d claimed.v-d:
  VERB_SPPP_T(<vc-expect>)
  or (<verb-s-pv> & {<tof-verb> or THi+ or Z-})
  or <verb-adj>
  or ({@MV+} & {<to-verb>} & <verb-phrase-opener>);
expecting.g claiming.g: (<vc-expect> & <verb-ge>) or <verb-ge-d>;
expecting.v claiming.v: <verb-pg> & <vc-expect>;

<vc-intend>:
  ({@MV+} & (TH+ or Z- or <to-verb>)) or
  ((O+ or <b-minus> or OX+) & {@MV+} & <too-verb>);
intend.v: VERB_PLI(<vc-intend>);
intends.v: VERB_S_T(<vc-intend>);
intended.v-d:
  VERB_SPPP_T(<vc-intend>)
  or (<verb-pv> & {<to-verb> or Z- or @MV+})
  or <verb-adj>
  or (<mv-coord> & {<to-verb>} & <verb-phrase-opener>);
intending.g: (<vc-intend> & <verb-ge>) or <verb-ge-d>;
intending.v: <verb-pg> & <vc-intend>;

% O+ & TO+: "I dare you to!"
% TO+ & Xc+: "try it if you dare to!"
% I+: auxiliary: "no one dared say a word"
% N+ & TO: "I dare not to say the truth"
<vc-dare>:
  ({N+} & <mv-coord> & {<to-verb> or <null-verb>}) or
  ({N+} & I+) or
  ((O+ or <b-minus>) & <mv-coord> & {<too-verb> or <null-verb>});

% SI+ & <verb-rq-aux> & I+: "How dare you disobey orders"
% <verb-s> & N+ & I+: "He dare not lie to me!" (singular subject)
dare.v:
  VERB_PLI(<vc-dare>)
  or (<verb-s> & N+ & I+)
  or (SI+ & <verb-rq-aux> & {N+} & I+);
dares.v: VERB_S_T(<vc-dare>);
dared.v-d:
  VERB_SPPP_T(<vc-dare>)
  or (<verb-pv> & <to-verb>)
  or ({@MV+} & <to-verb> & <verb-phrase-opener>);
daring.g: (<vc-dare> & <verb-ge>) or <verb-ge-d>;
daring.v: <verb-pg> & <vc-dare>;

% [TO+]: allows null-infinitive: "Yes, I'd love to."
<vc-like>:
  ({@MV+} & (<to-verb> or [TO+] or Pg+))
  or ((O+ or <b-minus> or OX+) & <mv-coord> & {<too-verb>})
  or ([[@MV+ & O*n+]]);

like.v: VERB_PLI(<vc-like>);
likes.v: VERB_S_T(<vc-like>);
liked.v-d: VERB_SPPP_T(<vc-like>) or <verb-pv> or <verb-phrase-opener>;
liking.g: (<vc-like> & <verb-ge>) or <verb-ge-d>;
liking.v: <verb-pg> & <vc-like>;

% ditransitive
<vc-offer>:
  ((<vc-opt-ditrans> or
    (<b-minus> & {O+})) & <mv-coord>) or
  ({@MV+} & <to-verb>) or
  ([[@MV+ & O*n+]]);

offer.v: VERB_PLI(<vc-offer>);
offers.v: VERB_S_T(<vc-offer>);
offered.v-d: VERB_SPPP_T(<vc-offer>) or (<verb-pv-b> & {O+ or <b-minus>
or [[@MV+ & O*n+]]} & <mv-coord>) or ({O+ or [[@MV+ & O*n+]]} or <verb-phrase-opener>);
offering.g: (<vc-offer> & <verb-ge>) or <verb-ge-d>;
offering.v: <verb-pg> & <vc-offer>;

% ditransitive
% unlike vc-offer, "to" is optional.
<vc-refuse>:
  ((<vc-opt-ditrans> or
    (<b-minus> & {O+})) & <mv-coord>) or
  (<mv-coord> & {<to-verb>}) or
  ([[@MV+ & O*n+]]);

refuse.v: VERB_PLI(<vc-refuse>);
refuses.v: VERB_S_T(<vc-refuse>);
refused.v-d: VERB_SPPP_T(<vc-refuse>) or (<verb-pv-b> & {O+ or <b-minus> or
[[@MV+ & O*n+]]} & <mv-coord>) or ({O+ or [[@MV+ & O*n+]]} & <verb-phrase-opener>);
refusing.g: (<vc-refuse> & <verb-ge>) or <verb-ge-d>;
refusing.v: <verb-pg> & <vc-refuse>;

% Pa**j+: predicative adjective -- "I want it green", "I want it very shiny."
% TO+ & Xc+: allows null-infinitive: "Because I want to."
% intransitive: "Try it if you want"
% QN+: "you want which one?"
% O+ & QN+: "you want it when?"
<vc-want>:
  (<mv-coord> & ({<to-verb>} or <null-verb>))
  or ((O+ or <b-minus> or OX+) & <mv-coord> & {<too-verb> or Pv+ or Pa**j+})
  or ({O+} & QN+)
  or [[@MV+ & O*n+]]
  or [[CX- & <mv-coord>]];

want.v:
  VERB_PLI(<vc-want>);

% A touch of common Spanglish.
quiero.v:
  VERB_PLI(<vc-want>);

% SI+ & <verb-rq-aux> & I+: "Need you use so much flour?"
need.v:
  VERB_PLI(<vc-want>)
  or ((SI+ or SFI+) & <verb-rq-aux> & I+);
need.i need'st: {@E-} & ((S- & <verb-wall>) or (RS- & B-)) & (N+ & I+);
wants.v needs.v: VERB_S_T(<vc-want>) or <vc-please>;
wanted.v-d needed.v-d:
  VERB_SPPP_T(<vc-want>)
  or <verb-pv>
  or <verb-adj>
  or <vc-please>
  or <verb-phrase-opener>;
wanting.g needing.g: (<vc-want> & <verb-ge>) or <verb-ge-d>;
wanting.v needing.v: <verb-pg> & <vc-want>;

<vc-choose>:
  (<mv-coord> & {<to-verb>}) or
  ((O+ or <b-minus>) & <mv-coord> & {<too-verb>}) or
  ([[@MV+ & O*n+]]);

choose.v: VERB_PLI(<vc-choose>);
chooses.v: VERB_S_T(<vc-choose>);
chose.v-d: VERB_SP_T(<vc-choose>);
chosen.v:
  VERB_PP(<vc-choose>) or
  (<verb-pv> & {<to-verb>}) or
  <verb-adj> or
  (<mv-coord> & {<to-verb>} & <verb-phrase-opener>);
choosing.g: (<vc-choose> & <verb-ge>) or <verb-ge-d>;
choosing.v: <verb-pg> & <vc-choose>;

% <vc-prepare> is identical to <vc-choose>
<vc-prepare>:
  (<mv-coord> & {<to-verb>}) or
  ((O+ or <b-minus>) & <mv-coord> & {<too-verb>}) or
  ([[@MV+ & O*n+]]);

prepare.v press.v: VERB_PLI(<vc-prepare>);
prepares.v presses.v: VERB_S_T(<vc-prepare>);
prepared.v-d pressed.v-d:
  VERB_SPPP_T(<vc-prepare>)
  or <verb-pv>
  or <verb-adj>
  or <verb-phrase-opener>;
preparing.g pressing.g: (<vc-prepare> & <verb-ge>) or <verb-ge-d>;
preparing.v pressing.v: <verb-pg> & <vc-prepare>;

<vc-require>:
  ((O+ or <b-minus>) & <mv-coord> & {<too-verb>}) or
  ({@MV+} & (TH+ or <embed-verb> or TS+ or (SI*j+ & I*j+))) or
  Zs- or
  ([[@MV+ & O*n+]]);

require.v: VERB_PLI(<vc-require>);
requires.v: VERB_S_T(<vc-require>);
required.v-d:
  VERB_SPPP_T(<vc-require>)
  or (<verb-pv> & {<to-verb> or TSi+})
  or <verb-adj>
  or (<mv-coord> & {<to-verb>} & <verb-phrase-opener>);
requiring.g: (<vc-require> & <verb-ge>) or <verb-ge-d>;
requiring.v: <verb-pg> & <vc-require>;

<vc-command>:
  ({@MV+} & (TH+ or Zs- or TS+ or <embed-verb>)) or
  ((O+ or <b-minus>) & <mv-coord> & {<too-verb>}) or
  ([[@MV+ & {O*n+}]]);

command.v order.v urge.v: VERB_PLI(<vc-command>);
commands.v orders.v urges.v: VERB_S_T(<vc-command>);
<vc-commanded>: VERB_SPPP_T(<vc-command>) or (<verb-pv> & {<to-verb> or TH+ or TS+})
or ({@MV+} & {TH+ or <to-verb> or TS+} & <verb-phrase-opener>);
commanded.v-d urged.v-d: <vc-commanded>;
% An "ordered list"
ordered.v: <vc-commanded> or <verb-adj>;
commanding.g ordering.g urging.g: (<vc-command> & <verb-ge>) or <verb-ge-d>;
commanding.v ordering.v urging.v: <verb-pg> & <vc-command>;

% ditransitive
<vc-consider>:
  ({@MV+} & (TH+ or Pg+)) or
  ((O+ or <b-minus> or OX+) & <mv-coord> & {<too-verb> or Pa+}) or
  (((O+ & (B- or ({[[@MV+]]} & O*n+))) or ([[@MV+ & O*n+]])) & <mv-coord>);
consider.v: VERB_PLI(<vc-consider>);
considers.v: VERB_S_T(<vc-consider>);
considered.v-d: VERB_SPPP_T(<vc-consider>) or (<verb-s-pv-b> &
(({@MV+} & (<tof-verb> or Pa+)) or ({O+ or <b-minus> or [[@MV+ & O*n+]]} & <mv-coord>)))
or ((({@MV+} & (<tof-verb> or Pa+)) or ({O+ or [[@MV+ & O*n+]]}))
& <verb-phrase-opener>);
considering.g: (<vc-consider> & <verb-ge>) or <verb-ge-d>;
considering.v: <verb-pg> & <vc-consider>;

<vc-perceive>:
  ({@MV+} & (TH+ or <embed-verb>)) or
  ((O+ or <b-minus> or OX+) & <mv-coord> & {<too-verb>}) or
  ([[@MV+ & O*n+]]);

perceive.v: VERB_PLI(<vc-perceive>);
perceives.v: VERB_S_T(<vc-perceive>);
perceived.v-d: VERB_SPPP_T(<vc-perceive>) or (<verb-pv> & {<to-verb>}) or
<verb-adj> or (<mv-coord> & {<to-verb>} & <verb-phrase-opener>);
perceiving.g: (<vc-perceive> & <verb-ge>) or <verb-ge-d>;
perceiving.v: <verb-pg> & <vc-perceive>;

<vc-report>:
  (<mv-coord> & {TH+ or Z- or <embed-verb>}) or
  ((O+ or <b-minus>) & <mv-coord> & {<too-verb>}) or
  ([[@MV+ & O*n+]]);

report.v: VERB_PLI(<vc-report>);
reports.v: VERB_S_T(<vc-report>);
reported.v-d:
  VERB_SPPP_T(<vc-report>)
  or (<verb-s-pv> & {<tof-verb> or Z-})
  or <verb-adj>
  or (<mv-coord> & {<to-verb>} & <verb-phrase-opener>);
reporting.g: (<vc-report> & <verb-ge>) or <verb-ge-d>;
reporting.v: <verb-pg> & <vc-report>;

<vc-caution>:
  ((O+ or <b-minus>) & <mv-coord> & {TH+ or <embed-verb> or <too-verb>}) or
  (<mv-coord> & {TH+ or Zs-}) or
  ([[@MV+ & O*n+]]);

caution.v: VERB_PLI(<vc-caution>);
cautions.v: VERB_S_T(<vc-caution>);
cautioned.v-d:
  VERB_SPPP_T(<vc-caution>)
  or (<verb-pv> & ((O+ or <b-minus>) & <mv-coord> & {TH+ or <embed-verb> or Zs- or <to-verb>}))
  or (<mv-coord> & {TH+ or <embed-verb> or <to-verb>} & <verb-phrase-opener>);

cautioning.g: (<vc-caution> & <verb-ge>) or <verb-ge-d>;
cautioning.v: <verb-pg> & <vc-caution>;

<vc-warn>:
  ((O+ or <b-minus>) & <mv-coord> & {TH+ or <embed-verb> or <too-verb> or <of-coord>}) or
  (<mv-coord> & {TH+ or Zs- or <of-coord>}) or
  ([[@MV+ & O*n+]]);

warn.v advise.v: VERB_PLI(<vc-warn>);
warns.v advises.v: VERB_S_T(<vc-warn>);
warned.v-d advised.v-d:
  VERB_SPPP_T(<vc-warn>) or
  (<verb-pv> & {TH+ or <embed-verb> or Zs- or <to-verb> or <of-coord>}) or
  (<mv-coord> & {TH+ or <embed-verb> or <to-verb> or OF+} & <verb-phrase-opener>);
warning.g advising.g: (<vc-warn> & <verb-ge>) or <verb-ge-d>;
warning.v advising.v: <verb-pg> & <vc-warn>;

<vc-hear>:
  ((O+ or <b-minus>) & <mv-coord> & {I*j+ or Pg+}) or
  (<mv-coord> & {TH+ or Zs- or <embed-verb> or <of-coord>}) or
  ([[@MV+ & O*n+]]);

hear.v: VERB_PLI(<vc-hear>);
hears.v: VERB_S_T(<vc-hear>);
heard.v-d:
  VERB_SPPP_T(<vc-hear>) or
  (<verb-pv> & {Pg+}) or
  (<mv-coord> & {Pg+} & <verb-phrase-opener>);

hearing.g: (<vc-hear> & <verb-ge>) or <verb-ge-d>;
hearing.v: <verb-pg> & <vc-hear>;

% Xc+: poor comma choice: "The man that you saw laugh, jumped off a cliff".
% Cost on [I+]: "I saw her face" want Os to face.n not I to face.v
<vc-see>:
  ((<b-minus> or O+) & <mv-coord> & {[I*j+]0.05 or Pg+ or AZ+ or Pv+})
  or (<b-minus> & {@MV+} & [I*j+ & Xc+])
  or (<mv-coord> & {TH+ or Zs- or QI+ or <embed-verb>})
  or ([[@MV+ & O*n+]]);

see.v spot.v: VERB_PLI(<vc-see>);
sees.v spots.v: VERB_S_T(<vc-see>);
saw.v-d spotted.v-d: VERB_SP_T(<vc-see>);

seen.v:
  VERB_PP(<vc-see>) or
  (<verb-pv> & {Pg+ or AZ+}) or
  (<mv-coord> & {Pg+ or AZ+} & <verb-phrase-opener>);
seeing.g spotting.g: (<vc-see> & <verb-ge>) or <verb-ge-d>;
seeing.v spotting.v: <verb-pg> & <vc-see>;

% ditransitive verbs -- taking direct and indirect objects
<vc-owe>:
  (<vc-opt-ditrans>
    or (B- & {O+})
    or ([[@MV+ & O*n+]])
  ) & <mv-coord>;

owe.v deliver.v accord.v award.v term.v grant.v begrudge.v
assign.v rename.v repay.v dub.v entitle.v fine.v:
  VERB_PLI(<vc-owe>);

owes.v delivers.v accords.v awards.v terms.v grants.v begrudges.v
assigns.v renames.v repays.v dubs.v entitles.v fines.v:
  VERB_S_T(<vc-owe>);

owed.v-d delivered.v-d accorded.v-d awarded.v-d
granted.v-d begrudged.v-d assigned.v-d repaid.v-d fined.v-d:
  VERB_SPPP_T(<vc-owe>)
  or (<verb-pv-b> & {O+ or <b-minus> or [[@MV+ & O*n+]]} & <mv-coord>)
  or ({O+ or [[@MV+ & O*n+]]} & <verb-phrase-opener>);

owing.v delivering.v according.v awarding.v terming.v granting.v
begrudging.v assigning.v renaming.v repaying.v dubbing.v entitling.v fining.v:
  <verb-pg> & <vc-owe>;

owing.g delivering.g according.g awarding.g terming.g granting.g begrudging.g
assigning.g renaming.g repaying.g dubbing.g entitling.g fining.g:
  (<vc-owe> & <verb-ge>) or <verb-ge-d>;

% extended linking requirements based on the above
termed.v-d  dubbed.v-d entitled.v-d renamed.v-d:
  VERB_SPPP_T(<vc-owe>) or
  (<verb-pv-b> & {O+ or <b-minus> or [[@MV+ & O*n+]]} & <mv-coord>) or
  ({O+ or [[@MV+ & O*n+]]} & <verb-phrase-opener>) or
  <verb-adj>;

% intransitive: "we deliver!"
deliver.w:
  VERB_PLI(<vc-intrans>);
delivers.w:
  VERB_S_I(<vc-intrans>);

% <verb-pv> & Pa+: "It was delived broken"
delivered.w-d:
  VERB_SPPP_I(<vc-intrans>)
  or (<verb-pv-b> & ({Xc+} & Pa+));

% ditransitive
% 'Give' requires both direct *and* indirect object: X gave Y a Z.
% 'sent', 'poured': optional indirect object.
% XXX Some of these verbs don't belong here ...
% (): "I already gave", "I already gave at the office"
% K+: "She gave in to him"
<vc-give>:
  { K+
    or (B- & {O+ or K+})
    or <vc-opt-ditrans-costly>
    or (O+ & K+)
    or (K+ & {[[@MV+]]} & O*n+)
    or ([[@MV+ & O*n+]])
  } & <mv-coord>;

give.v send.v bring.v lend.v issue.v hand.v pour.v text.v:
  VERB_PLI(<vc-give>);

gives.v sends.v brings.v lends.v
issues.v hands.v pours.v texts.v:
  VERB_S_T(<vc-give>);

sent.v-d brought.v-d lent.v-d handed.v-d:
  VERB_SPPP_T(<vc-give>) or
  (<verb-pv-b> & {O+ or <b-minus> or K+ or [[@MV+ & O*n+]]} & <mv-coord>) or
  ({O+ or K+ or [[@MV+ & O*n+]]} & <verb-phrase-opener>);

issued.v-d poured.v-d texted.v-d:
  VERB_SPPP_T(<vc-give>) or
  <verb-adj> or
  (<verb-pv-b> & {O+ or <b-minus> or K+ or [[@MV+ & O*n+]]} & <mv-coord>) or
  ({O+ or K+ or [[@MV+ & O*n+]]} & <verb-phrase-opener>);

gave.v-d: VERB_SP_T(<vc-give>);
given.v:
  VERB_PP(<vc-give>) or
  <verb-adj> or
  (<verb-pv-b> & {O+ or <b-minus> or K+ or [[@MV+ & O*n+]]} & <mv-coord>) or
  ({O+ or K+ or [[@MV+ & O*n+]]} & <verb-phrase-opener>);

giving.g sending.g bringing.g
lending.g issuing.g handing.g pouring.g texting.g:
  (<vc-give> & <verb-ge>) or <verb-ge-d>;

giving.v sending.v bringing.v
lending.v issuing.v handing.v pouring.v texting.v:
  <verb-pg> & <vc-give>;

% ditransitive, but everything is optional. For example:
% If you bought after the crash, you were smart
% If you telegraphed after Sunday, I'd already left.
<vc-pass>:
  {(B- & {O+ or K+}) or
    <vc-opt-ditrans> or
    (O+ & K+) or
    (K+ & {{[[@MV+]]} & O*n+}) or
    ([[@MV+ & O*n+]])} & <mv-coord>;

pass.v buy.v pay.v sell.v deal.v telegraph.v wire.v: VERB_PLI(<vc-pass>);

passes.v buys.v pays.v sells.v deals.v telegraphs.v wires.v: VERB_S_T(<vc-pass>);

% (S- & B-) : allows WV-less attach to "The dog which Chris bought is ugly"
% <verb-adj>: "The telegraphed orders never arrived"
passed.v-d bought.v-d paid.v-d payed.v-d sold.v-d dealt.v-d
telegraphed.v-d wired.v-d:
  VERB_SPPP_T(<vc-pass>) or
  (S- & <b-minus>) or
  <verb-adj> or
  (<verb-pv-b> & {O+ or <b-minus> or K+ or [[@MV+ & O*n+]]} & <mv-coord>) or
  ({O+ or K+ or [[@MV+ & O*n+]]} & <verb-phrase-opener>);

passing.g buying.g paying.g selling.g dealing.g telegraphing.g wiring.g:
  (<vc-pass> & <verb-ge>) or <verb-ge-d>;

passing.v buying.v paying.v selling.v dealing.v telegraphing.v wiring.v:
  <verb-pg> & <vc-pass>;

% Xd- & O+ & Xc+ & Eq+: "risks, as one could call them, are stupid."
<vc-para-naming>:
  (Xd- & O+ & {@MV+} & Xc+ & Eq+);

% opt-ditransitive
<vc-call>:
  ({(B- & {O+ or Pa+ or K+})
    or <vc-opt-ditrans>
    or (O+ & (Pa+ or K+))
    or (K+ & {{[[@MV+]]} & O*n+})
    or ([[@MV+ & O*n+]])} & <mv-coord>)
  or <vc-para-naming>;

% This is not quite right:
% "she called him" but "*she shouted him"
call.v shout.v: VERB_PLI(<vc-call>);
calls.v shouts.v: VERB_S_T(<vc-call>);
called.v-d shouted.v-d:
  VERB_SPPP_T(<vc-call>)
  or (<verb-pv-b> & {O+ or <b-minus> or K+ or Pa+ or [[@MV+ & O*n+]]} & <mv-coord>)
  or ({O+ or K+ or Pa+ or [[@MV+ & O*n+]]} & <verb-phrase-opener>);

calling.g shouting.g: (<vc-call> & <verb-ge>) or <verb-ge-d>;
calling.v shouting.v: <verb-pg> & <vc-call>;

% Minimal ditransitive extension of words.v.6
% ditransitive: "Please paint it lime green"
% (O+ & Pa+): "Please paint it green"
<vc-color>:
  <vc-fill>
  or <vc-ditrans>
  or (O+ & Pa+);

% Pa+: "The house was painted yellow"
color.v colour.v paint.v: VERB_PLI(<vc-color>);
colors.v colours.v paints.v: VERB_S_T(<vc-color>);
colored.v-d coloured.v-d painted.v-d:
  VERB_SPPP_T(<vc-color>)
  or (<verb-pv-b> & {O+ or K+ or Pa+} & <mv-coord>)
  or ({K+} & <verb-phrase-opener>)
  or <verb-adj>;

coloring.v colouring.v painting.v:
  (<verb-pg> & <vc-color>) or
  <verb-and-pg-> or <verb-and-pg+>;

coloring.g colouring.g painting.g:
  (<vc-color> & <verb-ge>) or <verb-ge-d>;

% ditransitive
% Writing -- direct and indirect object are optional:
% 'he wrote' 'he wrote a letter' 'he wrote me a letter' 'he wrote me'
% 'he wrote me that blah happened' but '*he drew me that blah happened'
%
% <vc-opt-ditrans> & TH+: "he wrote her that he loved her"
<vc-write>:
  ({(<b-minus> & {O+ or K+})
    or (<vc-opt-ditrans> & {TH+})
    or (O+ & K+)
    or (K+ & {{[[@MV+]]} & O*n+})
    or ([[@MV+ & O*n+]])
  } & <mv-coord>)
  or ({@MV+} & (TH+ or <embed-verb>));

write.v charge.v draw.v: VERB_PLI(<vc-write>);
writes.v reads.v charges.v draws.v: VERB_S_T(<vc-write>);
wrote.v-d drew.v-d: VERB_SP_T(<vc-write>);

read.v-d:
  VERB_SPPP_T(<vc-write>) or
  (<verb-ico> & <vc-write>) or
  (<verb-pv-b> & {O+ or <b-minus> or K+ or [[@MV+ & O*n+]]} & <mv-coord>) or
  ({O+ or K+ or [[@MV+ & O*n+]]} & <verb-phrase-opener>);
charged.v-d:
  VERB_SPPP_T(<vc-write>) or
  (<verb-pv-b> & {O+ or <b-minus> or K+ or [[@MV+ & O*n+]]} & <mv-coord>) or
  ({O+ or K+ or [[@MV+ & O*n+]]} & <verb-phrase-opener>);
written.v-d drawn.v-d w/o.v-d:
  VERB_PP(<vc-write>) or
  (<verb-pv-b> & {O+ or <b-minus> or K+ or [[@MV+ & O*n+]]} & <mv-coord>) or
  ({O+ or K+ or [[@MV+ & O*n+]]} & <verb-phrase-opener>) or
  <verb-adj>;

writing.v reading.v charging.v drawing.v:
  <verb-pg> & <vc-write>;
writing.g reading.g charging.g drawing.g:
  (<vc-write> & <verb-ge>) or <verb-ge-d>;

% ditransitive
% Singing: if there's an indirect object, then a direct object is
% mandatory: '*she sang me'
% but then: 'she sang soprano'
<vc-sing>:
  ({(<b-minus> & {O+ or K+}) or
    <vc-opt-ditrans> or
    (O+ & K+) or
    <of-coord> or
    (K+ & {{[[@MV+]]} & O*n+}) or
    ([[@MV+ & O*n+]])} & <mv-coord>);
sing.v dance.v cry.v: VERB_PLI(<vc-sing>);
sings.v dances.v cries.v: VERB_S_T(<vc-sing>);
sang.v-d danced.v-d cried.v-d: VERB_SP_T(<vc-sing>);
sung.v-d: VERB_SPPP_T(<vc-sing>);
singing.g dancing.g crying.g: (<vc-sing> & <verb-ge>) or <verb-ge-d>;
singing.v dancing.v crying.v: <verb-pg> & <vc-sing>;

% <vc-shout>: <vc-sing>;
% shout.v: VERB_PLI(<vc-shout>);
% shouts.v: VERB_S_T(<vc-shout>);
% shouted.v: VERB_SP_T(<vc-shout>);
% shouting.g: (<vc-shout> & <verb-ge>) or <verb-ge-d>;
% shouting.v: <verb-pg> & <vc-shout>;

% ditransitive
<vc-allow>:
  ((<vc-ditrans> or
    ([[@MV+]] & O*n+)) & <mv-coord>) or
  ((O+ or <b-minus>) & <mv-coord> & {B- or <too-verb>});

allow.v: VERB_PLI(<vc-allow>);
allows.v: VERB_S_T(<vc-allow>);
allowed.v-d: VERB_SPPP_T(<vc-allow>) or
  (<verb-pv-b> & (({O+ or <b-minus> or [[@MV+ & O*n+]]} & <mv-coord>) or ({@MV+} & <to-verb>)))
  or ({O+ or [[@MV+ & O*n+]] or ({@MV+} & <to-verb>)} & <verb-phrase-opener>);
allowing.g: (<vc-allow> & <verb-ge>) or <verb-ge-d>;
allowing.v: <verb-pg> & <vc-allow>;

% ditransitive
<vc-promise>:
  ({O+ or <b-minus>} & <mv-coord> & {B- or <to-verb> or <embed-verb> or TH+ or RSe+ or Zs-}) or
  ((<vc-ditrans> or
    ([[@MV+ & O*n+]])) & <mv-coord>);

promise.v: VERB_PLI(<vc-promise>);
promises.v: VERB_S_T(<vc-promise>);
promised.v-d:
  VERB_SPPP_T(<vc-promise>)
  or (<verb-pv-b> & (({O+ or <b-minus> or [[@MV+ & O*n+]]} & <mv-coord>) or ({@MV+} & (<to-verb> or <embed-verb> or TH+ or RSe+ or Zs-))))
  or <verb-adj>
  or ({O+ or [[@MV+ & O*n+]] or ({{@MV+} & (<to-verb> or <embed-verb> or TH+)})} & <verb-phrase-opener>);
promising.g: (<vc-promise> & <verb-ge>) or <verb-ge-d>;
promising.v: <verb-pg> & <vc-promise>;

% ditransitive
<vc-show>:
  ({O+ or <b-minus>} & ({@MV+} & (QN+ or <embed-verb> or TH+ or RSe+ or Zs- or B-))) or
  ((
    <vc-opt-ditrans> or
    (O+ & K+) or
    (K+ & (B- or ({[[@MV+]]} & O*n+))) or
    ([[@MV+ & O*n+]]) or
    [[()]]) & <mv-coord>);

show.v: VERB_PLI(<vc-show>) or <verb-manner>;
shows.v: VERB_S_T(<vc-show>) or <verb-manner>;
showed.v-d: VERB_SP_T(<vc-show>);
shown.v:
  VERB_PP(<vc-show>) or
  <verb-manner> or
  (<verb-s-pv-b> &
    (({O+ or K+ or B- or [[@MV+ & O*n+]]} & <mv-coord>) or
    ({@MV+} & (QN+ or <embed-verb> or TH+ or RSe+ or Zs-)))) or
  ({O+ or K+ or [[@MV+ & O*n+]] or ({@MV+} & (QN+ or <embed-verb> or TH+))} & <verb-phrase-opener>);
showing.g: (<vc-show> & <verb-ge>) or <verb-ge-d>;
showing.v: <verb-pg> & <vc-show>;

% ditransitive
<vc-teach>:
  ((O+ or <b-minus>) & ({@MV+} & (QI+ or <embed-verb> or TH+ or RSe+ or Zs- or B- or <too-verb>)))
  or ({
    <vc-opt-ditrans>
    or (B- & {[[@MV+]]} & O*n+)
    or ([[@MV+ & O*n+]])} & <mv-coord>);

teach.v: VERB_PLI(<vc-teach>);
teaches.v: VERB_S_T(<vc-teach>);
% (<verb-sp,pp> & @MV+ & QN+): "You taught there when?"
taught.v-d:
  VERB_SPPP_T(<vc-teach>) or
  (<verb-pv-b> &
    (({O+ or <b-minus> or [[@MV+ & O*n+]]} & <mv-coord>) or
    ({@MV+} & (QI+ or <embed-verb> or TH+ or RSe+ or Zs- or <to-verb>)))) or
  (<verb-sp,pp> & @MV+ & QN+) or
  ({O+ or [[@MV+ & O*n+]] or ({@MV+} & (QI+ or <embed-verb> or TH+))} & <verb-phrase-opener>);
teaching.g: (<vc-teach> & <verb-ge>) or <verb-ge-d>;
teaching.v: <verb-pg> & <vc-teach>;

<vc-compel>:
  ((O+ or <b-minus>) & {@MV+} & <too-verb>);

compel.v: VERB_PLI(<vc-compel>);
compels.v: VERB_S_T(<vc-compel>);
compelled.v-d: VERB_SPPP_T(<vc-compel>) or (<verb-pv> & <to-verb>)
or ({@MV+} & <to-verb> & <verb-phrase-opener>);
compelling.v: <verb-pg> & <vc-compel>;
compelling.g: (<vc-compel> & <verb-ge>) or <verb-ge-d>;

<vc-force>:
  (((O+ or <b-minus>) & (({@MV+} & <too-verb>) or K+ or [()])) or
    (K+ & O*n+) or
    ([[{K+} & @MV+ & O*n+]])) & <mv-coord>;

force.v: VERB_PLI(<vc-force>);
forces.v: VERB_S_T(<vc-force>);
forced.v-d willed.v-d:
  VERB_SPPP_T(<vc-force>) or
  (<verb-pv-b> & ((K+ & <mv-coord>) or
  ({@MV+} & <to-verb>))) or
  ((K+ or ({@MV+} & <to-verb>)) & <verb-phrase-opener>) or
  <verb-adj>;
forcing.g: (<vc-force> & <verb-ge>) or <verb-ge-d>;
forcing.v: <verb-pg> & <vc-force>;

% -----------------------------------------

<vc-design>:
  (B- & <mv-coord> & {<too-verb>}) or
  (O+ & <mv-coord> & {<too-verb>}) or
  ([[@MV+ & O*n+ & <mv-coord>]]);

design.v permit.v authorize.v use.v cause.v enable.v
pressure.v train.v sentence.v prompt.v spur.v disincline.v
invite.v reelect.v encourage.v draft.v hire.v entice.v inspire.v
aid.v forbid.v employ.v educate.v tempt.v condemn.v commission.v
counsel.v induce.v instruct.v
license.v incite.v nominate.v destine.v provoke.v challenge.v exhort.v
implore.v motivate.v impel.v:
  VERB_PLI(<vc-design>)
  or (<verb-manner> & O+ & Xc+);

designs.v permits.v pressures.v trains.v sentences.v causes.v
enables.v authorizes.v uses.v prompts.v spurs.v disinclines.v
invites.v reelects.v encourages.v drafts.v hires.v entices.v
inspires.v aids.v forbids.v employs.v educates.v tempts.v
condemns.v commissions.v counsels.v induces.v
instructs.v licenses.v incites.v nominates.v destines.v provokes.v
challenges.v exhorts.v implores.v motivates.v impels.v:
  VERB_S_T(<vc-design>)
  or (<verb-manner> & O+ & Xc+);

designed.v-d permitted.v-d pressured.v-d trained.v-d
sentenced.v-d caused.v-d enabled.v-d
authorized.v-d prompted.v-d spurred.v-d invited.v-d disinclined.v-d
reelected.v-d encouraged.v-d drafted.v-d hired.v-d
enticed.v-d inspired.v-d aided.v-d employed.v-d
educated.v-d tempted.v-d condemned.v-d commissioned.v-d
counseled.v-d induced.v-d instructed.v-d
licensed.v-d incited.v-d nominated.v-d destined.v-d
provoked.v-d challenged.v-d exhorted.v-d
implored.v-d motivated.v-d impelled.v-d:
  VERB_SPPP_T(<vc-design>)
  or (<verb-pv> & {<to-verb>})
  or <verb-adj>
  or (<verb-manner> & O+ & Xc+)
  or ({{@MV+} & <to-verb>} & <verb-phrase-opener>);

forbade.v-d: VERB_SP_T(<vc-design>);
forbidden.v:
  VERB_PP(<vc-design>) or
  (<verb-pv> & {<to-verb>}) or
  <verb-adj> or
  ({{@MV+} & <to-verb>} & <verb-phrase-opener>);

designing.g permitting.g pressuring.g causing.g enabling.g
training.g sentencing.g authorizing.g prompting.g
spurring.g inviting.g disinclining.g
reelecting.g encouraging.g drafting.g hiring.g
enticing.g inspiring.g aiding.g employing.g educating.g tempting.g
condemning.g commissioning.g counseling.g inducing.g instructing.g
licensing.g inciting.g nominating.g destining.g provoking.g challenging.g
exhorting.g imploring.g motivating.g impelling.g:
  (<vc-design> & <verb-ge>) or <verb-ge-d>;

designing.v permitting.v pressuring.v causing.v enabling.v
training.v sentencing.v authorizing.v using.v prompting.v disinclining.v
spurring.v inviting.v reelecting.v encouraging.v drafting.v hiring.v
enticing.v inspiring.v aiding.v employing.v educating.v tempting.v
condemning.v commissioning.v counseling.v inducing.v instructing.v
licensing.v inciting.v nominating.v destining.v provoking.v challenging.v
exhorting.v imploring.v motivating.v impelling.v:
  <verb-pg> & <vc-design>;

used.v-d:
  VERB_SPPP_T(<vc-design>)
  or (<verb-pv> & {<too-verb>})
  or (<verb-sp> & <to-verb>)
  or <verb-adj>
  or (<verb-manner> & O+ & Xc+)
  or (<mv-coord> & {<too-verb>} & <verb-phrase-opener>);

using.g: (<vc-design> & (<verb-ge> or MVs-)) or <verb-ge-d>;

% --------------------------------------------------

<vc-elect>:
  ((O+ or <b-minus>) & ((<mv-coord> & {<too-verb>}) or
    ({[[@MV+]]} & (O*n+ or TI+)))) or
  ([[@MV+ & O*n+ & <mv-coord>]]);

elect.v appoint.v: VERB_PLI(<vc-elect>);
elects.v appoints.v: VERB_S_T(<vc-elect>);
elected.v-d appointed.v-d:
  VERB_SPPP_T(<vc-elect>)
  or (<verb-pv-b> & (({O+ or <b-minus> or TI+ or [[@MV+ & (O*n+ or TI+)]]} & <mv-coord>) or ({@MV+} & <to-verb>)))
  or (({O+ or TI+ or [[@MV+ & (O*n+ or TI+)]]} or ({@MV+} & <to-verb>)) & <verb-phrase-opener>)
  or <verb-adj>;

electing.g appointing.g: (<vc-elect> & <verb-ge>) or <verb-ge-d>;
electing.v appointing.v: <verb-pg> & <vc-elect>;

% vc-name is a ditransitive extension of vc-trans (with an extra TI+)
% <vc-para-naming>: "risks, as one might name them, are stupid."
<vc-name>:
  ((<vc-opt-ditrans>
    or (O+ & {[[@MV+]]} & TI+)
    or (B- & {O+ or TI+})
    or ([[@MV+ & O*n+]])) & <mv-coord>)
  or <vc-para-naming>;

name.v designate.v label.v: VERB_PLI(<vc-name>);
names.v designates.v labels.v: VERB_S_T(<vc-elect>);
named.v-d designated.v-d labelled.v-d labeled.v-d:
  VERB_SPPP_T(<vc-name>) or
  (<verb-pv-b> & ({O+ or <b-minus> or TI+ or [[@MV+ & (O*n+ or TI+)]]}) & <mv-coord>) or
  ({O+ or TI+ or [[@MV+ & (O*n+ or TI+)]]} & <verb-phrase-opener>) or
  <verb-adj>;
naming.g designating.g labelling.g labeling.g: (<vc-name> & <verb-ge>) or <verb-ge-d>;
naming.v designating.v labelling.v labeling.v: <verb-pg> & <vc-name>;

% optionally ditransitive, modeled on "name.v"
<vc-tag>: <vc-name> or <vc-intrans>;
tag.v: VERB_PLI(<vc-tag>);
tags.v: VERB_S_T(<vc-tag>);
tagged.v-d:
  VERB_SPPP_T(<vc-tag>) or
  ((<verb-pv-b> & ({O+ or <b-minus> or TI+ or [[@MV+ & (O*n+ or TI+)]]}) & <mv-coord>) or <verb-pv>) or
  ({O+ or TI+ or [[@MV+ & (O*n+ or TI+)]]} & <verb-phrase-opener>) or
  <verb-adj>;
tagging.g:
  (<vc-tag> & <verb-ge>) or
  <verb-adj> or
  <verb-ge-d>;
tagging.v: <verb-pg> & <vc-tag>;

<vc-program>:
  {((O+ or <b-minus>) & <mv-coord> & {<too-verb>}) or
  @MV+ or
  ([[@MV+ & O*n+ & <mv-coord>]])};

program.v oblige.v: VERB_PLI(<vc-program>);
programs.v obliges.v: VERB_S_T(<vc-program>);
programed.v-d programmed.v-d obliged.v-d:
  VERB_SPPP_T(<vc-program>) or
  (<verb-pv> & {<to-verb>}) or
  <verb-adj> or
  ({{@MV+} & <to-verb>} & <verb-phrase-opener>);
programing.g programming.g obliging.g: (<vc-program> & <verb-ge>) or <verb-ge-d>;
programing.v programming.v obliging.v: <verb-pg> & <vc-program>;

<vc-convince>:
  ((O+ or <b-minus>) & <mv-coord> & {<too-verb> or TH+ or <embed-verb>}) or
  ([[@MV+ & O*n+ & <mv-coord>]]);
convince.v persuade.v: VERB_PLI(<vc-convince>);
convinces.v persuades.v: VERB_S_T(<vc-convince>);
convinced.v-d persuaded.v-d:
  VERB_SPPP_T(<vc-convince>) or
  (<verb-pv> & {<to-verb> or TH+ or <embed-verb>}) or
  ({<mv-coord> & (<to-verb> or TH+ or <embed-verb>)} & <verb-phrase-opener>);
convincing.g persuading.g: (<vc-convince> & <verb-ge>) or <verb-ge-d>;
convincing.v persuading.v: <verb-pg> & <vc-convince>;

% K+ is for "tell him off"
% bare MVp+ for "Today, we will tell about ..."
% OF+ for "They have told of the soldiers' fear"
% (QN+ & {MV+}): "I did not tell why until recently"
% <embed-verb>: "He told me that Fred is dead."
% {O+} & <embed-verb>: "He told me Fred is dead."
% O+ & OF+: "tell me of that ingenious hero"
% Ox+ & Xc+ & SI*i+: "tell me, John."
% [()]: "only he can tell"
%
<vc-tell>:
  (((O+ & {O*n+ or K+}) or <b-minus>)
     & <mv-coord> & {TH+ or RSe+ or Zs- or <too-verb> or QI+ or BW-})
  or ({O+ & <mv-coord>} & <embed-verb>)
  or ({O+ & <mv-coord>} & OF+ & {Xc+ & SI*i+})
  or (Ox+ & {<mv-coord>} & Xc+ & SI*i+ & {Xc+ & OF+})
  or [()]0.3
  or (QI+ & {MV+})
  or ([[@MV+ & {O*n+} & <mv-coord>]]);

tell.v: VERB_PLI(<vc-tell>);
tell.w: {@E-} & I- & {@MV+} & (QI+ or TH+ or <embed-verb> or RSe+ or Zs-) & <verb-wall>;
tells.v: VERB_S_T(<vc-tell>);
told.v-d:
  VERB_SPPP_T(<vc-tell>)
  or (<verb-pv> & {TH+ or <embed-verb> or RSe+ or Zs- or <to-verb> or QI+ or BW-})
  or (<verb-pv-b> & O+ & <mv-coord>)
  or ({{@MV+} & (<embed-verb> or <to-verb> or QI+ or TH+)} & <verb-phrase-opener>);

telling.g: (<vc-tell> & <verb-ge>) or <verb-ge-d>;
telling.v: <verb-pg> & <vc-tell>;

% basilect telling
tellin': <verb-pg> & <vc-tell>;

% Many other paraphrasing question words are in words.v.10
% (QI+ & {MV+}): "I did not ask why until recently"
% OF+ & <mv-coord>: "Joseph asked of her : Who is this ?"
<vc-ask>:
  ({(O+ & {O*n+}) or <b-minus>} & <mv-coord>
    & {TS+ or <too-verb> or (QI+ & {MV+}) or BW-})
  or <of-coord>
  or ([[@MV+ & O*n+ & <mv-coord>]]);

ask.v: VERB_PLI(<vc-ask>);
asks.v: VERB_S_T(<vc-ask>);
asked.v-d:
  VERB_SPPP_T(<vc-ask>)
  or (<verb-pv> & {<to-verb> or QI+ or BW- or TH+ or TS+})
  or (<verb-pv-b> & O+ & <mv-coord>)
  or ({{@MV+} & (<to-verb> or QI+ or TH+ or TS+)} & <verb-phrase-opener>);

asking.g: (<vc-ask> & <verb-ge>) or <verb-ge-d>;
asking.v: <verb-pg> & <vc-ask>;

% TH+: "it helps that you know already"
<vc-help>:
  ({O+ or <b-minus>} & {@MV+} & {<to-verb> or I+ or QI+})
  or [[@MV+ & O*n+ & <mv-coord>]];

help.v: VERB_PLI(<vc-help>);
helps.v: VERB_S_T(<vc-help> or TH+);
helped.v-d:
  VERB_SPPP_T(<vc-help> or TH+)
  or (<verb-pv> & {<to-verb>})
  or ({{@MV+} & <to-verb>} & <verb-phrase-opener>);
helping.g: (<vc-help> & <verb-ge>) or <verb-ge-d>;
helping.v: <verb-pg> & <vc-help>;

<vc-remind>:
  ((O+ or <b-minus>) & {@MV+} &
    (<too-verb> or TH+ or <embed-verb> or <of-coord>)) or
  ([[@MV+ & O*n+ & <mv-coord>]]);

remind.v: VERB_PLI(<vc-remind>);
reminds.v: VERB_S_T(<vc-remind>);
reminded.v-d:
  VERB_SPPP_T(<vc-remind>) or
  (<verb-pv> & {<to-verb> or
     TH+ or
     <embed-verb> or
     <of-coord>}) or
  ({{@MV+} & (<embed-verb> or
    <to-verb> or TH+ or <of-coord>)} & <verb-phrase-opener>);
reminding.g: (<vc-remind> & <verb-ge>) or <verb-ge-d>;
reminding.v: <verb-pg> & <vc-remind>;

<vc-inform>:
  ((O+ or <b-minus>) & <mv-coord> & {<of-coord> or TH+ or Zs- or <embed-verb>}) or
  ([[@MV+ & O*n+ & <mv-coord>]]);

inform.v reassure.v alert.v guarantee.v notify.v forewarn.v:
  VERB_PLI(<vc-inform>);

informs.v reassures.v alerts.v guarantees.v notifies.v forewarns.v:
  VERB_S_T(<vc-inform>);

informed.v-d reassured.v-d alerted.v-d guaranteed.v-d
notified.v-d forewarned.v-d:
  VERB_SPPP_T(<vc-inform>)
  or (<verb-pv> & {<embed-verb> or TH+ or Zs- or <of-coord>})
  or ({{@MV+} & (<embed-verb> or TH+ or OF+)} & <verb-phrase-opener>)
  or <verb-adj>;

informing.g reassuring.g alerting.g guaranteeing.g notifying.g forewarning.g:
  (<vc-inform> & <verb-ge>) or <verb-ge-d>;

informing.v reassuring.v alerting.v guaranteeing.v notifying.v forewarning.v:
  <verb-pg> & <vc-inform>;

<vc-assure>:
  ((O+ or <b-minus>) & <mv-coord> & {<of-coord> or TH+ or Zs- or <embed-verb>}) or
  ([[@MV+ & O*n+ & <mv-coord>]]) or
  ({@MV+} & (TH+ or <embed-verb>));

assure.v: VERB_PLI(<vc-assure>);
assures.v: VERB_S_T(<vc-assure>);
assured.v-d:
  VERB_SPPP_T(<vc-assure>)
  or (<verb-pv> & {<of-coord> or <embed-verb> or TH+ or Zs-})
  or ({{@MV+} & (<embed-verb> or TH+ or OF+)} & <verb-phrase-opener>);

assuring.g: (<vc-assure> & <verb-ge>) or <verb-ge-d>;
assuring.v: <verb-pg> & <vc-assure>;

<vc-let>:
  ((O+ or <b-minus>) & <mv-coord> & {I+ or ((K+ or Pp+) & <mv-coord>)}) or
  ([[@MV+ & O*n+ & <mv-coord>]]);

let.v-d:
  VERB_SPPP_T(<vc-let>) or
  (<verb-ico> & <vc-let>) or
  (<verb-pv-b> & ((K+ or Pp+) & <mv-coord>)) or
  ((K+ or Pp+) & <verb-phrase-opener>);
lets.v: VERB_S_T(<vc-let>);
letting.g: (<vc-let> & <verb-ge>) or <verb-ge-d>;
letting.v: <verb-pg> & <vc-let>;

% Abbreviation for "let us"
% This seems to never be used, because 's.#us below expands
% EI- & Wi-: "Oh, let's!"
% S**i- & Wi-: "Oh Sue, let's!"
let's let’s: <verb-co> & {N+} & {I+};

% K+: "watch out!"
% K+ & MV+: "I watch out for you"
% XXX But this is too loose, as "out" should be the only allowed particle.
<vc-watch>:
  ((O+ or <b-minus>) & <mv-coord> & {I*j+ or Pg+}) or
  ([[@MV+ & O*n+ & <mv-coord>]]) or
  (K+ & <mv-coord>) or
  <mv-coord>;

watch.v: VERB_PLI(<vc-watch>);
watches.v: VERB_S_T(<vc-watch>);
watched.v-d:
  VERB_SPPP_T(<vc-watch>) or
  <verb-pv> or
  <verb-phrase-opener>;

watching.g: (<vc-watch> & <verb-ge>) or <verb-ge-d>;
watching.v: <verb-pg> & <vc-watch>;

<vc-appreciate>:
  ((O+ or <b-minus>) & <mv-coord> & {Pg+}) or
  ([[@MV+ & O*n+ & <mv-coord>]]);

appreciate.v spend.v: VERB_PLI(<vc-appreciate>);
appreciates.v spends.v: VERB_S_T(<vc-appreciate>);
appreciated.v-d spent.v-d:
  VERB_SPPP_T(<vc-appreciate>) or
  (<verb-pv> & {Pg+}) or
  ({{@MV+} & Pg+} & <verb-phrase-opener>);
appreciating.g spending.g: (<vc-appreciate> & <verb-ge>) or <verb-ge-d>;
appreciating.v spending.v: <verb-pg> & <vc-appreciate>;

% Pa**j is used for predicative adjectives: "make it nice and soft"
% ditransitive
<vc-make>:
  ((O+ or <b-minus> or OX+) & {
    (<mv-coord> & {I*j+ or Pa**j+ or B-})
    or ((K+ or AF-) & <mv-coord>)})
  or ((
    <vc-ditrans>
    or (K+ & {[[@MV+]]} & O*n+)
    or K+
    or Vm+
    or ([[{K+} & @MV+ & O*n+]])
  ) & <mv-coord>)
  or [[()]];

make.v: VERB_PLI(<vc-make>);
makes.v: VERB_S_T(<vc-make>);
made.v-d:
  VERB_SPPP_T(<vc-make>)
  or (<verb-s-pv-b> & ((<mv-coord> & Pa+) or ({O+ or K+} & <mv-coord>)))
  or ({({@MV+} & Pa+) or K+} & <verb-phrase-opener>);

built_of built_up_of composed_of constructed_of formed_of made_of
made_up_of:
  (<verb-pv-b> & (O+ or <b-minus>) & <mv-coord>) or (O+ & <verb-phrase-opener>);

making.g: (<vc-make> & <verb-ge>) or <verb-ge-d>;
making.v: <verb-pg> & <vc-make>;

<vc-render>:
  (((O+ or <b-minus>) & {({@MV+} & Pa+) or AF-}) or
    ([[@MV+ & O*n+]])) & <mv-coord>;

render.v deem.v: VERB_PLI(<vc-render>);
renders.v deems.v: VERB_S_T(<vc-render>);
rendered.v-d deemed.v-d: VERB_SPPP_T(<vc-render>) or (<verb-pv> & {Pa+ or AF-})
or ({{@MV+} & Pa+} & <verb-phrase-opener>);
rendering.g deeming.g: (<vc-render> & <verb-ge>) or <verb-ge-d>;
rendering.v deeming.v: <verb-pg> & <vc-render>;

<vc-deprive>:
  (((O+ or <b-minus>) & {{@MV+} & OF+}) or
    ([[@MV+ & O*n+]])) & <mv-coord>;

deprive.v accuse.v acquit.v purge.v disabuse.v exonerate.v absolve.v rob.v
convict.v: VERB_PLI(<vc-deprive>);
deprives.v accuses.v acquits.v purges.v disabuses.v
exonerates.v absolves.v robs.v convicts.v: VERB_S_T(<vc-deprive>);
deprived.v accused.v acquitted.v purged.v disabused.v exonerated.v absolved.v robbed.v
convicted.v-d:
  VERB_SPPP_T(<vc-deprive>) or
  (<verb-pv> & {OF+} & <mv-coord>) or
  <verb-adj> or
  ({{@MV+} & OF+} & <verb-phrase-opener>);
depriving.g accusing.g acquitting.g purging.g disabusing.g exonerating.g
absolving.g robbing.g convicting.g:
(<vc-deprive> & <verb-ge>) or <verb-ge-d>;
depriving.v accusing.v acquitting.v purging.v disabusing.v
exonerating.v absolving.v robbing.v convicting.v: <verb-pg> & <vc-deprive>;

<vc-clear>:
  (((O+ or <b-minus>) & {({@MV+} & OF+) or K+}) or
    ({K+} & O*n+) or
    K+ or
    ([[{K+} & @MV+ & O*n+]])) & <mv-coord>;

clear.v: VERB_PLI(<vc-clear>);
clears.v: VERB_S_T(<vc-clear>);
cleared.v-d:
  VERB_SPPP_T(<vc-clear>) or
  (<verb-pv-b> & {({@MV+} & OF+) or K+} & <mv-coord>) or
  ({K+ or ({@MV+} & OF+)} & <verb-phrase-opener>);
clearing.g: (<vc-clear> & <verb-ge>) or <verb-ge-d>;
clearing.v: <verb-pg> & <vc-clear>;

<vc-bet>:
  ({(O+ & {O*n+}) or (<b-minus> & {O+})} & <mv-coord> & {TH+ or <embed-verb> or RSe+}) or
  ([[@MV+ & O*n+ & <mv-coord>]]);

bet.v-d:
  VERB_SPPP_T(<vc-bet>) or
  (<verb-ico> & <vc-bet>) or
  <verb-manner> or
  (<verb-pv> & {O+ or <b-minus>} & <mv-coord> & {TH+ or <embed-verb> or RSe+ or @MV+});
bets.v: VERB_S_T(<vc-bet>);
betted.v-d:
  VERB_SPPP_T(<vc-bet>) or
  (<verb-pv-b> & {O+ or <b-minus>} & <mv-coord> & {TH+ or <embed-verb> or RSe+ or @MV+}) or
  ({O- or [[@MV+ & O*n+]] or TH+ or <embed-verb>} & <verb-phrase-opener>);
betting.g: (<vc-bet> & <verb-ge>) or <verb-ge-d>;
betting.v: <verb-pg> & <vc-bet>;

<vc-bother>:
  ({@MV+} & <to-verb>) or
  ((O+ or <b-minus>) & <mv-coord> & {THi+}) or
  ([[@MV+ & O*n+ & <mv-coord>]]);

bother.v: VERB_S_PLI(<vc-bother>) or <verb-why>;
bothers.v: VERB_S_S(<vc-bother>);
bothered.v-d: VERB_S_SPPP(<vc-bother>) or <verb-pv> or <verb-phrase-opener>;
bothering.v: <verb-s-pg> & <vc-bother>;
bothering.g: (<vc-bother> & <verb-ge>) or <verb-ge-d>;

<vc-surprise>:
  ((O+ or <b-minus>) & <mv-coord> & {THi+}) or
  ([[@MV+ & O*n+]]);

surprise.v alarm.v amaze.v amuse.v annoy.v
astonish.v astound.v depress.v disgust.v distress.v
dismay.v embarrass.v engage.v excite.v irritate.v:
  VERB_S_PLI(<vc-surprise>);

surprises.v alarms.v amazes.v amuses.v annoys.v
astonishes.v astounds.v depresses.v disgusts.v distresses.v
dismays.v embarrasses.v engages.v excites.v irritates.v:
  VERB_S_S(<vc-surprise>);

surprised.v-d alarmed.v-d amazed.v-d amused.v-d annoyed.v-d
astonished.v-d astounded.v-d depressed.v-d disgusted.v-d distressed.v-d
dismayed.v-d embarrassed.v-d engaged.v-d excited.v-d irritated.v-d:
  VERB_S_SPPP(<vc-surprise>) or <verb-pv> or <verb-phrase-opener>;

surprising.v alarming.v amazing.v amusing.v annoying.v
astonishing.v astounding.v depressing.v disgusting.v distressing.v
dismaying.v embarrassing.v engaging.v exciting.v irritating.v:
  <verb-s-pg> & <vc-surprise>;

surprising.g alarming.g amazing.g amusing.g annoying.g
astonishing.g astounding.g depressing.g disgusting.g distressing.g
dismaying.g embarrassing.g engaging.g exciting.g irritating.g:
  (<vc-surprise> & <verb-ge>) or <verb-ge-d>;

<vc-prove>:
  ((O+ or <b-minus> or [[@MV+ & O*n+]]) & <mv-coord>) or
  ((O+ or <b-minus> or OX+) & {@MV+} & (<too-verb> or [[{Xc+} & Pa+]])) or
  ({@MV+} & (<tof-verb> or TH+ or <embed-verb> or RSe+ or Zs- or (Pa+ & <verb-wall>)));

prove.v: VERB_Y_PLI(<vc-prove>);
proves.v: VERB_Y_S(<vc-prove>);
proved.v-d:
  VERB_Y_SPPP(<vc-prove>)
  or (<verb-s-pv> & {THi+ or <tof-verb>})
  or (<verb-adj> & {dCPu-} & {MV+})
  or ({{@MV+} & Pa+} & <verb-phrase-opener>);
proven.v:
  (<verb-x-pp> & <vc-prove>) or
  (<verb-s-pv> & {THi+ or <tof-verb> or Pa+ or dCPu-}) or
  <verb-adj> or
  ({{@MV+} & Pa+} & <verb-phrase-opener>);
proving.g: (<vc-prove> & <verb-ge>) or <verb-ge-d>;
proving.v: <verb-x-pg> &  <vc-prove>;

% TO+ & Xc+: allows null-infinitive: "Yes, I would strongly suggest to!"
<vc-suggest>:
  ((O+ or <b-minus> or [[@MV+ & O*n+]]) & <mv-coord>)
  or (<mv-coord> & <null-verb>)
  or ({@MV+} & (Pg+ or TH+ or <embed-verb> or RSe+ or Zs- or TS+ or ((SI*j+ or SFI**j+) & I*j+)));

suggest.v anticipate.v recommend.v: VERB_PLI(<vc-suggest>);
suggests.v anticipates.v recommends.v: VERB_S_T(<vc-suggest>);
suggested.v-d anticipated.v-d recommended.v-d:
  VERB_SPPP_T(<vc-suggest>)
  or (<verb-s-pv> & {THi+ or TSi+ or Z-})
  or <verb-adj>
  or <verb-phrase-opener>;
suggesting.g anticipating.g recommending.g: (<vc-suggest> & <verb-ge>) or <verb-ge-d>;
suggesting.v anticipating.v recommending.v: <verb-pg> & <vc-suggest>;

% ditransitive
<vc-deny>:
  ((<vc-opt-ditrans> or
    (B- & {O+}) or
    [[@MV+ & O*n+]]) & <mv-coord>) or
  ({@MV+} & (Pg+ or TH+ or <embed-verb> or RSe+));

deny.v: VERB_PLI(<vc-deny>);
denies.v: VERB_S_T(<vc-deny>);
denied.v-d:
  VERB_SPPP_T(<vc-deny>) or
  (<verb-pv-b> & {O+ or <b-minus> or [[@MV+ & O*n+]]} & <mv-coord>) or
  ({O+ or ([[@MV+ & O*n+]])} & <verb-phrase-opener>);
denying.g: (<vc-deny> & <verb-ge>) or <verb-ge-d>;
denying.v: <verb-pg> & <vc-deny>;

<vc-describe>:
  ((O+ or <b-minus>) & <mv-coord> & {AZ+}) or
  ({@MV+} & (QI+ or Z-)) or
  ([[@MV+ & O*n+ & <mv-coord>]]);

describe.v: VERB_PLI(<vc-describe>);
describes.v: VERB_S_T(<vc-describe>);
described.v-d:
  VERB_SPPP_T(<vc-describe>) or
  (<verb-pv> & {AZ+ or Z-}) or
  <verb-adj> or
  (<mv-coord> & {AZ+} & <verb-phrase-opener>);
describing.g: (<vc-describe> & <verb-ge>) or <verb-ge-d>;
describing.v: <verb-pg> & <vc-describe>;

<vc-portray>:
  ((O+ or <b-minus>) & <mv-coord> & {AZ+}) or
  ([[@MV+ & O*n+ & <mv-coord>]]);

portray.v depict.v regard.v view.v characterize.v: VERB_PLI(<vc-portray>);
portrays.v depicts.v regards.v views.v characterizes.v: VERB_S_T(<vc-portray>);
portrayed.v-d depicted.v-d regarded.v-d viewed.v-d characterized.v-d:
  VERB_SPPP_T(<vc-portray>) or
  (<verb-pv> & {AZ+}) or
  <verb-adj> or
  (<mv-coord> & {AZ+} & <verb-phrase-opener>);
portraying.g depicting.g regarding.g viewing.g characterizing.g:
  (<vc-portray> & <verb-ge>) or <verb-ge-d>;
portraying.v depicting.v regarding.v viewing.v characterizing.v:
<verb-pg> & <vc-portray>;

% -------------------------------------------------------------------------------
% IDIOMATIC VERBS
% Some of these might be better handled by using a K+ for the particle,
% but then limiting which particle is allowed.

do_so take_place show_up take_office do_battle give_way make_way
take_part catch_up catch_on file_suit pick_up take_off break_free
take_over jump_ship see_fit take_note:
  VERB_PLI(<vc-intrans>);
does_so takes_place shows_up pleads_guilty pleads_innocent
takes_office does_battle gives_way makes_way takes_part catches_up
catches_on files_suit picks_up takes_off breaks_free takes_over
jumps_ship sees_fit lets_go takes_note comes_true comes_clean
comes_of_age:
  VERB_S_T(<vc-intrans>);

showed_up pleaded_guilty pleaded_innocent made_way caught_up caught_on
filed_suit picked_up jumped_ship:
  VERB_SPPP_I(<vc-intrans>);

plead_guilty plead_innocent:
  VERB_SPPP_I(<vc-intrans>) or
  (<verb-ico> & <vc-intrans>);

let_go:
  VERB_SPPP_I(<vc-intrans>) or
  (<verb-ico> & <vc-intrans>) or
  <verb-pv>;

did_so took_place took_office did_battle gave_way took_part took_off
broke_free took_over saw_fit took_note came_true came_clean came_of_age:
  VERB_SP_I(<vc-intrans>);

done_so taken_place shown_up taken_office done_battle given_way
taken_part taken_off broken_free taken_over seen_fit taken_note:
  VERB_PP(<vc-intrans>);

come_true come_clean come_of_age:
  VERB_PLI(<vc-intrans>) or
  VERB_PP(<vc-intrans>);

doing_so taking_place showing_up pleading_guilty pleading_innocent
taking_office
doing_battle giving_way making_way taking_part catching_up catching_on
filing_suit picking_up taking_off breaking_free taking_over jumping_ship
seeing_fit letting_go taking_note coming_true coming_clean coming_of_age:
  (<vc-intrans> & <verb-pg,ge>) or <verb-ge-d>;

<vc-put-up-with>: (O+ or <b-minus> or [[@MV+ & O*n+]]) & <mv-coord>;
allow_for bring_about get_rid_of let_go_of take_note_of:
  VERB_PLI(<vc-trans>);
puts_up_with allows_for brings_about gets_rid_of lets_go_of
takes_note_of:
  VERB_S_T(<vc-trans>);
put_up_with let_go_of:
  VERB_SPPP_T(<vc-trans>) or
  (<verb-ico> & <vc-trans>);
allowed_for brought_about got_rid_of took_note_of: VERB_SPPP_T(<vc-trans>);
gotten_rid_of taken_note_of: VERB_PP(<vc-trans>);
putting_up_with allowing_for bringing_about getting_rid_of letting_go_of
taking_note_of:
  (<vc-trans> & (<verb-ge> or <verb-pg>)) or <verb-ge-d>;

<vc-take-it>: {[@MV+]} & TH+;
take_it make_out point_out give_notice serve_notice: VERB_PLI(<vc-take-it>);
takes_it makes_out points_out gives_notice serves_notice: VERB_S_I(<vc-take-it>);
 made_out pointed_out served_notice:
  VERB_SPPP_I(<vc-take-it>) or <verb-pv> or <verb-phrase-opener>;
took_it gave_notice: VERB_SP_I(<vc-take-it>);
taken_it given_notice: VERB_PP(<vc-take-it>);
taking_it making_out pointing_out giving_notice serving_notice:
  (<vc-take-it> & <verb-pg,ge>) or <verb-ge-d>;

<vc-turn-out>: {[@MV+]} & THi+;
turn_out: VERB_S_PLI(<vc-turn-out>);
turns_out: VERB_S_S(<vc-turn-out>);
turned_out: VERB_S_SPPP(<vc-turn-out>);
turning_out: <verb-s-pg> & <vc-turn-out>;

% (QI+ & {MV+}): "I did not figure out why until recently"
<vc-find-out>: {[@MV+]} & (TH+ or (QI+ & {MV+}) or <embed-verb>);
find_out figure_out: VERB_PLI(<vc-find-out>);
finds_out figures_out: VERB_S_I(<vc-find-out>);
found_out figured_out:
  VERB_SPPP_I(<vc-find-out>) or
  <verb-pv> or
  <verb-phrase-opener>;
finding_out figuring_out: (<vc-find-out> & <verb-pg,ge>) or
<verb-ge-d>;

<vc-keep-on>: {Pg+ or @MV+};
keep_on give_up go_around: VERB_S_PLI(<vc-keep-on>);
keeps_on gives_up goes_around: VERB_S_S(<vc-keep-on>);
kept_on: VERB_S_SPPP(<vc-keep-on>);
gave_up went_around: VERB_S_SP(<vc-keep-on>);
given_up gone_around: <verb-s-pp> & <vc-keep-on>;
keeping_on giving_up going_around: (<vc-keep-on> & <verb-pg,ge>) or <verb-ge-d>;

% An alternate solution to this, and the above, is to give each
% particle a unique subscript, e.g "up.r: Ku-" and then force the
% particle match by using Ku+ on "end". For example: "go_out" could
% be handled in this way, or as <vc-end-up>.
<vc-end-up>: Pg+ or Pa+ or ({AF-} & <mv-coord>);
end_up: VERB_S_PLI(<vc-end-up>);
ends_up: VERB_S_S(<vc-end-up>);
ended_up: VERB_S_SPPP(<vc-end-up>);
ending_up: (<vc-end-up> & <verb-pg,ge>) or <verb-ge-d>;

% two-word passives
% done_for accounted_for adhered_to arrived_at barked_at belched_at catered_to
/en/words/words.v.1.p:
  <verb-pv>
  or <verb-manner>
  or <vc-please>
  or <verb-phrase-opener>;

% -----------------------------------------------------------------
% wall connectors
% The naked Wa+: affirmative replies: "a red car"
% The naked Wb+, without a WV+, links to topic questions: "what did you think?".
% The naked Wi+, without a WV+, links to imperatives: "put it on the table".
% The naked Wl+: locative expreessions: "on the table"
% The naked Wn+, without a WV+, links to nominals: "what a shame!".
% The naked Wx+, without a WV+, links to opinions: "ruined!".
% The naked Qd+, without a WV+, links to subj-verb-inverts: "are you
%     insane?", "Are you the one?"
% The Qd+ with a WV+, for questions: "did you do that?"
% Wv+ with a wall: why-verb questions: "why even go there?"
% Wh+ & Wa+: "How much money?"
%
% XXX everywhere where Ws+ is used, should probably be <wi-wall>!?
<wo-wall>:
  hWa+ or hWb+ or hWg+ or hWi+ or hWn+ or hWw+ or hWx+ or hQd+
  or (hWh+ & hWa+);
<wi-wall>: (hWd+ or hWp+ or hWr+ or hWq+ or hWs+ or hWj+ or hWc+ or hWe+
   or hWt+ or hWo+ or hWv+ or hQd+) & <WALL>;

% Paraphrasing, quotational complements:
<paraph-null>: [()]0.1;

% An entire quoted sentence. Costly, if the quotes are missing.
<quote-sent>:
  ((Xc+ or Xe+ or [()]) & QUd+ & (<wi-wall> or <wo-wall> or hWl+) & {X+} & QUc+)
  or [(Xc+ or Xe+) & (<wi-wall> or <wo-wall>) & {X+}];

% ---------------------------------------------------------------
% Quote with or without quotation marks.
% "This is a test," she said.
% We should go, I agreed.
% Its confusing as to how to best link this. With the quotation marks,
% the comma must link under the quotes, and "she said" works like a new
% sentence. Without the quotes, the comma can link like the filler-it
% example below.
%
% QU+ & <embed-verb> & QU+: He said, "This is it."
% Xc+ or Xe+ or [[()]]: punctuation is commonly missing.
<vc-paraph>:
  (<verb-wall> & {@MV+} & (Xc+ or Xp+ or <paraph-null>) & CP-)
  or ({<verb-wall>} & {@MV+} & ((Xd- or Xq-) & (Xc+ or Xp+ or <paraph-null>)
      & (dCOq+ or CP- or Eq+ or <verb-wall>)))
  or [{@MV+} & (Xc+ or Xe+ or [[()]]) & <embed-verb>]
  or (<verb-wall> & {@MV+} & <quote-sent>);

% Xd- & Xc+: "If I'm right, he thought, this will work."
% CPa- & Xc+: "So thinks everyone"
% CPa- & <quote-sent>: "Spoke the king: Off with his head!"
% PF- & <quote-sent>: "And thus spoke the king: Off with his head!"
% PF- (by itself): "And thus spoke the king"
% [...]0.04: tie-breaker: avoid paraphrasing, if we at all can.
<vc-paraph-inv>:
  {@MV+} & [
    ((Xc+ or Xp+ or <paraph-null>) & CP-)
    or ((Xd- or Xq-) & (Xc+ or Xp+ or <paraph-null>)
      & (dCOq+ or CPx- or Eq+ or <verb-wall>))
    or ((PF- or CPa-) & {<quote-sent> & {RW+}})
    or [(Xc+ or Xe+) & <embed-verb>] ]0.04;

% filler-it: "The President is busy, it seems."
% The (Xd- or Xq-) links to the previous comma.
<vc-it-paraph>:
  {@MV+} & (Xd- or Xq-) & (Xc+ or Xp+ or <paraph-null>)
    & (dCOqi+ or CPi- or Eqi+ or <verb-wall>);

% ---------------------------------------------------------------
% Paraphrasing verbs like "say", "reply"
% acknowledge.q add.q admit.q affirm.q agree.q announce.q argue.q
/en/words/words.v.10.1:
  [[{@E-} & (((Sp- or I-) & <vc-paraph>) or (SIpj+ & <vc-paraph-inv>))]];

% Paraphrasing verbs that can take a particle.
/en/words/words.v.11.1:
  [[{@E-} & (((Sp- or I-) & {K+} & <vc-paraph>) or (SIpj+ & <vc-paraph-inv>))]];

/en/words/words.v.10.2:
  [[{@E-} & Ss- & <vc-paraph>]]
  or [[{@E-} & SIsj+ & <vc-paraph-inv>]];

/en/words/words.v.11.2:
  [[{@E-} & Ss- & {K+} & <vc-paraph>]]
  or [[{@E-} & SIsj+ & <vc-paraph-inv>]];

% XXX Why is there a cost on Pvf- ???
/en/words/words.v.10.3:
  {@E-} & (
    ((S- or PP-) & <vc-paraph>)
    or (SI*j+ & <vc-paraph-inv>)
    or [Pvf- & <vc-it-paraph>]
    or (Pv- & CV- & dCPu-));

/en/words/words.v.11.3:
  {@E-} & (
    ((S- or PP-) & {K+} & <vc-paraph>)
    or (SI*j+ & <vc-paraph-inv>)
    or [Pvf- & <vc-it-paraph>]
    or (Pv- & CV- & dCPu-));

% Why aren't the next two like the above?
read.q-d:
   {@E-} & (
    ((S- or I- or PP-) & {K+} & <vc-paraph>)
    or (SI*j+ & <vc-paraph-inv>));

wrote.q-d knew.q-d:
   {@E-} & (
    (S- & {K+} & <vc-paraph>)
    or (SI*j+ & <vc-paraph-inv>));

written.q known.q-d:
  {@E-} & ((PP- & <vc-paraph>) or (Pv- & CV- & dCPu-));

/en/words/words.v.10.4: [[{@E-} & Pg- & <vc-paraph>]];
/en/words/words.v.11.4: [[{@E-} & Pg- & {K+} & <vc-paraph>]];

seem.q appear.q: [[{@E-} & (SFp- or If-) & <vc-it-paraph>]];
seems.q appears.q:
  [[{@E-} & SFs- & <vc-it-paraph>]]
  or ({@E-} & Pv- & CV- & dCPu-);
seemed.q-d appeared.q-d: {@E-} & (SF- or PPf-) & <vc-it-paraph>;
seeming.q appearing.q: [[{@E-} & Pgf- & <vc-it-paraph>]];

say.q:
  {@E-} &
    (((Sp- or I- or (RS- & Bp-)) & <vc-paraph>)
    or (SIpj+ & <vc-paraph-inv>));

says.q:
  {@E-} & (((Ss- or (RS- & Bs-)) & <vc-paraph>) or (SIsj+ & <vc-paraph-inv>));

% Pv- & <vc-paraph>: "The play, it is said, is excellent"
said.q-d:
  {@E-} & (
    ((S- or PP- or (RS- & B-) or Pv-) & <vc-paraph>)
    or (SI*j+ & <vc-paraph-inv>)
    or (Pv- & CV- & dCPu-));

saying.q:
  {@E-} & Pg- & <vc-paraph>;

avow.q:
 [[{@E-} & (((Sp- or I-) & <vc-paraph>) or (SIpj+ & <vc-paraph-inv>))]];

tell.q:
  [[{@E-} & (Sp- or I- or SIpj+) & O+ & <vc-paraph>]];
tells.q:
  [[{@E-} & (Ss- or SIsj+) & O+ & <vc-paraph>]];
told.q-d:
  {@E-} & (((S- or PP- or SI*j+) & O+) or Pv-) & <vc-paraph>;
telling.q:
  [[{@E-} & Pg- & O+ & <vc-paraph>]];

ask.q:
  [[{@E-} & (((Sp- or I-) & {O+}) or SIpj+) & <vc-paraph>]];
asks.q:
  [[{@E-} & ((Ss- & {O+}) or SIsj+) & <vc-paraph>]];
asked.q-d:
  {@E-} & (((S- or PP-) & {O+}) or Pv- or SI*j+) & <vc-paraph>;
asking.q:
  {@E-} & Pg- & {O+} & <vc-paraph>;

% idiomatic "voted yes/no" expressions using the V link.
% "he answered yes", "say yes!", "Just say no!"
<vc-vote>: {Xc+} & Vv+ & <mv-coord>;
answer.w reply.w say.w vote.w: VERB_PLI(<vc-vote>);
answers.w replies.w says.w votes.w: VERB_S_I(<vc-vote>);
answered.w-d replied.w said.w-d voted.w-d: VERB_SPPP_I(<vc-vote>);
answering.w replying.w saying.w voting.w:
  (<vc-vote> & <verb-pg,ge>);

% ---------------------------------------------------------
% :.w  "My answer: yes"
is.w ":.w":
  <verb-x-s,u> & <vc-vote> & <verb-wall>;

was.w-d:
  <verb-x-s,u> & <vc-vote> & <verb-wall>;

been.w: {@E-} & PPf- & <vc-vote> & <verb-wall>;

be.w:
  {@E-} & (Ix- or (<verb-co> & <verb-wall>) or [S*x-]) & <vc-vote>;

% S- & Vv+ & Xc+ & <embed-verb>:  "The answer being yes, ..."
% S- & Xd- & MVg- & Vv+: "..., the answer being yes"
being.w:
   (S- & Vv+ & Xc+ & <embed-verb> & <verb-wall>)
   or (S- & Xd- & MVg- & Vv+ & <verb-wall>) ;

% E-: "The answer is surely yes"
% A- & Ds-: "His answer was an emphatic yes"
% Ds- & Jv-: "He replied with a yes"
yes.vote no.vote maybe.vote:
  (({E-} or (A- & Ds-)) & Vv-)
  or ({A-} & Ds- & Jv-);

double.v triple.v quadruple.v quintuple.v:
  {EN-} & VERB_PLI(<vc-fill>);
doubles.v triples.v quadruples.v quintuples.v:
  {EN-} & VERB_S_T(<vc-fill>);
doubled.v-d tripled.v-d quadrupled.v-d quintupled.v-d:
  {EN-} & (VERB_SPPP_T(<vc-fill>) or <verb-pv> or <verb-adj> or <verb-phrase-opener>);
doubling.v tripling.v quadrupling.v quintupling.v:
  {EN-} & (<verb-pg> & <vc-fill>);
doubling.g tripling.g quadrupling.g quintupling.g:
  {EN-} & ((<vc-fill> & <verb-ge>) or <verb-adj> or <verb-ge-d>);

% ===================================================================
% PREPOSITIONS

% conjoin preps: "prep and prep": "the coverage on TV and on the radio..."
<conjoin-preps>: dMJrp- or dMJlp+;

% alter-preps: "it is somewhere in or near the house"
% The "or" must take a prep object.
% XXX TODO: most preps below need this rule.
<alter-preps>: dMJrj- or dMJlj+;

% Statements beginning with a locative.
% "Under the tree." "On the right." "During the show."
<locative>: {EP-} & dWl- & J+;

% Mp- (which modifies nouns) has a cost, so that modifying verbs
% (using MVp-) is generally preferred.  The cost is small, though,
% to allow modifiers on conjoined nouns to work well.
% e.g. "...went to hell yesterday and heaven on Tuesday"
%
% [Mp- & MVp-]-0.61 prefers a connection to both the noun and the
%    verb, helping disambiguate.  The weight 0.61 plus 0.4 is greater
%    than one, to overcome cost on @MV+.
%
% The EA- & MX- needs to be tightened; it allows oddball constructions
% that prevent putting an EA+ on words.adv.2 (which should have EA+)
<prep-main-b>:
  <conjoin-preps>
  or [Mp-]0.4 or MVp- or [Mp- & MVp-]-0.61
  or ({EA-} & Pp-)
  or (Xc+ & {Xd-} & dCOw+)
  or [({Xc+ & {Xd-}} & (dCOa+ or dCOd+))]
  or ({EA-} & Xd- & {Xc+} & MVx-)
  or ({EA-} & Xd- & Xc+ & MX*x-);

% Wj- & Qp+: questions: By what means will you arrive?
<prep-main-a>:
  <prep-main-b>
  or (<subcl-verb> & (Mj- or (Xd- & Xc+ & MX*j-)))
  or (Wj- & Qp+)
  or <fronted>;

<prep-main-t>:
  <conjoin-preps> or
  [Mpn-] or Pp- or MVpn- or
  [({Xc+ & {Xd-}} & dCO*n+)] or
  (Xd- & Xc+ & (MX- or MVx-));

<prep-main-e>:
  <conjoin-preps> or
  [Mp-] or Pp- or <adv-as> or
  (Xc+ & {Xd-} & dCOw+) or
  [({Xc+ & {Xd-}} & (dCOa+ or dCOd+))] or
  (Xd- & Xc+ & (MX*x- or MVx-));

under beneath:
  ({Yd-} & {JQ+} & J+ & (<prep-main-a> or FM-))
  or <locative>
  or [MVp- & B-]
  or (Yd- & Pp-);
below above behind.p:
  ({Yd-} & {{JQ+} & J+} & (<prep-main-a> or FM-))
  or <locative>
  or [MVp- & B-];
within w/i:
  ({JQ+} & J+ & <prep-main-a>)
  or <locative>
  or [MVp- & B-];
during:
  ({JQ+} & J+ & (<prep-main-a> or UN-))
  or <locative>
  or [MVp- & B-];

% EW+: "From where did it come?"
% {EP-} & dWl-: "Just from down below"
% {hHM+}: "I think it's from uhh Mars"
from:
  ({hHM+} & {Yd-} & {JQ+} & (FM+ or J+ or Mgp+) & (<prep-main-a> or Mp-))
  or ({EP-} & dWl- & FM+)
  or [MVp- & B-]
  or EW+
  or NIr+;

at toward towards without w/o.p:
  ({JQ+} & (J+ or Mgp+) & <prep-main-a>)
  or <locative>
  or [MVp- & B-];

%
% XXX FIXME: MVp- & J+ is wrong: "*I saw John except Fred"
% XXX (The MVp- coming from prep-main-a)
% So give J+ a cost, to minimize this, for now ...
% Could this be fixable in postprocessing?
% {EBm+} & J+: "but not very much"
% [[EBm+]]: "but not very"
except but.misc-ex:
  ({JQ+} & (({EBm+} & [[J+]]) or [[EBm+]] or Mgp+) & <prep-main-a>)
  or [MVp- & B-]
  or ((MVp+ or <to-verb>) & <prep-main-a>);

against beyond beside:
  ({JQ+} & (J+ or Mgp+) & <prep-main-a>)
  or <locative>
  or [MVp- & B-];

between:
  ({JQ+} & (J+ or Mgp+) & <prep-main-a>)
  or <locative>
  or [MVp- & B-]
  or NIr+;

% w/ as a common abbreviation for with
with w/:
  ({JQ+} & (J+ or Mgp+) & (<prep-main-a> or dRJrv-))
  or (Jw+ & (dRJrj- or dRJlj+))
  or [MVp- & B-]
  or <locative>
  or (J+ & {EBm+} & ([P+] or [[O*n+]]) & (
    MVp-
    or [({Xc+ & {Xd-}} & dCO+)]
    or (Xd- & Xc+ & (MX*x- or MVx-))));

among amongst amidst astride:
  ({JQ+} & (J+ or Mgp+) & (<prep-main-a> or FM-))
  or <locative>
  or [MVp- & B-];

% (Wj- & JQ+ & J+): "By what means?"  kind of a hack, for null-verb question.
% In what way?  To what end?
<null-prep-qu>: Wj- & JQ+ & J+;

% FL+ "for long"
for.p:
  ({JQ+} & (J+ or Mgp+ or TI+) & (<prep-main-a> or Mp-))
  or (J+ & (dRJrj- or dRJlj+))
  or <locative>
  or [MVp- & B-]
  or (MG- & JG+)
  or (MVp- & FL+)
  or <null-prep-qu>;

into:
  ({JQ+} & (J+ or Mgp+ or QI+) & <prep-main-a>)
  or <locative>
  or [MVp- & B-];

% re: "with regard to"
% Xe+: optional colon
re:
  {Xe+} & J+ & <prep-main-a>;

% comparative-like links to prepositions.
%    "He did it largely in his spare time"
% EZ+: "Jane was soaked almost as thoroughly"
% EC+: "Jane replied even more quietly"
<comp-prep>:
  {Xd-} & MVl- & (EZ+ or EC+ or [MVp+ or MVa+ or MVs+]0.5);
<comp-pause-prep>:
  (Xd- or [()]) & MVl- & (EZ+ or EC+ or [MVp+ or MVa+ or MVs+]0.5);

% <comp-pause-prep>: really really want to follow a comma.
% "Jane was gratified, though in a quieter way"
although.c though.c but.c yet.c:
  <comp-pause-prep>;

% cost on MVa- to give preference to MVl-
% {EP-} & dWl-: "Just about now."
% cost on Wl- & MV+: "about five times"
about:
  ({JQ+} & (J+ or Mgp+ or QI+) & <prep-main-a>)
  or EN+
  or EW+
  or EZ+
  or [MVp- & B-]
  or (<tof-verb> & (Mp- or MVp- or Pp-))
  or [<adv-as>]
  or [{EP-} & dWl- & (MVp+ or MVa+ or MVs+)]0.5
  or <comp-prep>;


% Grep also for "just_about", used as syonym for "nearly", "almost"
% XXX is this really needed ?? Seems to duplicate other stuff ..
just_about nearly_about almost_about right_about:
  ({JQ+} & (J+ or Mgp+ or QI+) & <prep-main-a>)
  or EW+;

% EN- & Pp-: "you are half-way through"
% EN- & J-: "He stopped, about half-way through"
% K-: "it wicks the moisture through"
% right/straight through: right/straight needs to modify through, so
% so that conjunctions can work correctly.
through.r right_through straight_through:
  ({JQ+} & J+ & (<prep-main-a> or FM-))
  or (EN- & (Pp- or J-))
  or K-
  or <locative>
  or [MVp- & B-];

<prep-across>:
  ({JQ+} & J+ & (<prep-main-a> or FM-))
  or K-
  or [MVp- & B-];

across along: <prep-across> or <fronted> or <locative>;

% <fronted>: "off went the cavalry"
% EP+: "off to the side"
off:
  <prep-across>
  or (MVp+ & {Xc+ & {Xd-}} & dCOp+)
  or <fronted>
  or EP+;

past.p:
  ({Yd-} & {JQ+} & J+ & (<prep-main-a> or FM-))
  or K-
  or <locative>
  or [MVp- & B-];

% Pp-: preposition
around:
  <alter-preps>
  or ({JQ+} & (J+ or Mgp+) & (<prep-main-a> or FM-))
  or <locative>
  or K-
  or Pp-
  or <adv-as>
  or <fronted>
  or [MVp- & B-]
  or [EN+];

% K-: "They ran the motor flat out"
flat_out: K-;

% up, down behaving as prepositions.
% EN- & Pp-: "you are halfway out"
% EN- & J-: "We stopped, about halfway up"
% {J+} & <fronted>: "Down the stairs came the dog", "Down came the dog".
% Wl- & MVp+: "out to lunch"
% Wl- & Pg+: "out walking the dog"
out.r up.r down.r:
  ({Yd-} & {JQ+} & ([J+] or [[MVp+]]) & (({Xd- & Xc+} & <adv-as>) or FM-))
  or K-
  or ({Yd-} & Pp-)
  or (EN- & (Pp- or J-))
  or ({J+} & <fronted>)
  or <locative>
  or (Wl- & (MVp+ or Pg+))
  or [MVp- & B-];


by:
  <alter-preps>
  or ({JQ+} & (J+ or Mgp+ or JT+) & (<prep-main-a> or FM-))
  or K-
  or <locative>
  or [MVp- & B-]
  or <null-prep-qu>;

% EN- & Pp-: "you are halfway in"
% EN- & J-: "we stopped, about halfway in"
in.r:
  <alter-preps>
  or ({JQ+} & (J+ or Mgp+ or IN+) & (<prep-main-a> or FM-))
  or K-
  or (EN- & (Pp- or J-))
  or <locative>
  or [MVp- & B-]
  or (MG- & JG+)
  or <null-prep-qu>;

on upon:
  <alter-preps>
  or ({JQ+} & (J+ or Mgp+ or ON+ or [QI+]) & <prep-main-a>)
  or K-
  or <locative>
  or [MVp- & B-];

% o'er: poetic contraction
over o'er:
  ({Yd-} & {JQ+} & (J+ or Mgp+ or QI+ or [[MVp+]]) & (<prep-main-a> or FM-))
  or K-
  or EN+
  or <locative>
  or [MVp- & B-]
  or (Yd- & Pp-);

just_over just_under well_over: EN+;

% XXX original LG recommends using the LI link, however the
% <prep-main-b> sort of clobbers this. Should this be "fixed"?
like.p:
  ({[EA-]} & (({hHM+} & (J+ or Mgp+ or [[Mp+ or MVs+]]) & <prep-main-b>)
     or (Vf- & Mgp+)
     or (LI- & (J+ or <subcl-verb>))))
  or <locative>
  or [MVp- & B-];

unlike:
   J+ & (MVp-
     or Pp-
     or [({Xc+ & {Xd-}} & dCO+)]
     or (Xd- & Xc+ & (E+ or MVx-)));

% o': silent, un-pronounced f.
% (OFd- & Jd+): "I have a lot of cookies", forces "lot" to be determiner;
%      The Jd+ forces a link to the object as well.
% Wj- & JQ+ & J+ & Qd+: "Of which person were you speaking?"
% QI+ & CV+: "She kept an organized record of which employees took their vacations"
%      The QI makes it interrogative, the CV links head word.
% (Js+ or Jp+ or Ju+): we explicitly exclude Jw+ as that leads to bad parses.
%
% Mf-: allows "from the Abbey of Stratford Langthorne" so that "of"
%      links to "Abbey" instead of something more distant.
%      XXX The Mp- below should be removed, and all occurrences of
%      Mp+ elsewhere should be replaced by (Mp+ or Mf+)
% Mf- & MVp+: "She was a girl of about John's age"
of:
  ({JQ+}
    & (Js+ or Jp+ or Ju+ or Mgp+ or (QI+ & {CV+}))
    & (Mp-
      or Mf-
      or OFj-
      or OFw-
      or (Xd- & Xc+ & MX*x-)
      or (<subcl-verb> & (Mj- or (Xd- & Xc+ & MX*j-)))
      or [[({Xc+ & {Xd-}} & dCO+)]]))
  or (Mf- & MVp+)
  or (Wj- & JQ+ & J+ & Qd+)
  or (OFd- & Jd+)
  or ((OFj- or Mp-) & B-)
  or (MG- & JG+)
  or (NF- & NJ+)
  or (Mp- & TI+);

of_them: (ND- or MF-) & (J+ or Pa+) & Xd- & (MX*x- or MVx-) & Xc+;

% MX-PHRASE: The blah, to be blahed, will be blah.
% TO- & Xc+: "I'd like to, I want to." (null infinitive)
% give [J+] a cost, so that numeric intervals are preferred
% I*t+ & TO-: passes on the TO constraint down the line
% I+ & MVi-: allows "What is there to do?"
%            but also incorrectly allows: "He is going to do"
%            This is where landmark transitivity could fix things,
%            by placing a link between "what" and "do".
% I*t+ & Wo-: "To be continued"
to.r:
  ({@E-} & {N+} & I*t+ & (TO- or Wo-))
  or ({@E-} & {NT-} & I+ &
    (<MX-PHRASE>
    or (SFsx+ & <S-CLAUSE>)
    or [{Xd- & Xc+} & MVi-]0.9
    or [<OPENER>]
    or [[R-]] ))
  or ({NT-} & TO- & Xc+)
  or I*a+
  or ({JQ+} & ([J+] or Mgp+) & <prep-main-a>)
  or <locative>
  or [MVp- & B-]1.1
  or <null-prep-qu>;

so_as_to: I+ & {Xd- & Xc+} & MVi-;

% Mail addresses.
care_of c/o:
  MVp- & J+;

% --------------------------------------------------------
% Preps suggesting comparative relations, orderings

besides: {J+ or Mgp+} & ([({Xc+ & {Xd-}} & dCO+)] or MVp- or <fronted>);
throughout: {J+} & ([({Xc+ & {Xd-}} & dCO+)] or MVp- or <fronted>);

versus.r v..r v.r vs..r vs.r: (J+ & Mp-) or (G- & G+);

worth.p: (Mp- & (J+ or OF+)) or (Paf- & Mgp+) or (Pa- & (J+ or B-));
opposite.p: J+ & <prep-main-b>;
better_off worse_off: {EC-} & Pa- & {Pg+};

% J+ & <fronted>: "out of the tree fell the squirrel."
off_of out_of:
  ({JQ+} & J+ & <prep-main-b>)
  or [MVp- & B-]
  or (J+ & <fronted>);

despite notwithstanding
other_than apart_from aside_from:
  (J+ or Mgp+) & (MVp- or (Xd- & Xc+ & (MVx- or E+))
  or [({Xc+ & {Xd-}} & dCO+)]);

rather_than:
  (J+ or Mgp+ or Mp+ or I+) &
    ((Xd- & Xc+ & (E+ or MVx-)) or MVp- or [({Xc+ & {Xd-}} & dCO+)]);

instead_of because_of prior_to:
  (J+ or Mgp+)
  & (MVp- or Pp- or [({Xc+ & {Xd-}} & dCO+)] or (Xd- & Xc+ & (E+ or MVx-)));

as_well_as:
  (J+ or Mgp+) & (MG- or Mp- or MVp- or [({Xc+ & {Xd-}} & dCO+)] or (Xd- & Xc+ & (MX*x- or MVx-)));

according_to as_of in_case_of in_response_to unbeknownst_to thanks_to:
  J+ & (MVp- or Pp- or [({Xc+ & {Xd-}} & dCO+)] or (Xd- & Xc+ & (E+ or MVx-)));

due_to along_with en_route_to in_connection_with:
  J+ & <prep-main-b>;

regardless_of as_to irrespective_of:
  (J+ or QI+) & (MVp- or [({Xc+ & {Xd-}} & dCO+)] or (Xd- & Xc+ & (E+ or MVx-)));

as_yet to_date so_far thus_far as_usual on_average
in_general in_particular in_response in_reply in_turn:
  <prep-main-b>;

% Mp- & QI+: "decisions such as when to go are taken by the instructor."
such_as:
  (J+ & (
    MVa-
    or Mp-
    or (Xc+ & Xd- & (MVx- or MX*x-))))
  or (Mp- & QI+);

lest:
  (<subcl-verb> or Mgp+ or Mv+) & (
    ({Xc+ & {Xd-}} & dCO*s+)
    or ({Xd- & Xc+} & MVs-)
    or (Xd- & Xc+ & E+));

albeit:
  (<subcl-verb> & {Xc+ & {Xd-}} & dCO*s+)
  or ({Xd-} & <coord> & Wd+);

no_matter:
  QI+ & ((Xc+ & {Xd-} & dCO+) or ({Xd- & Xc+} & MVs-));

% --------------------------------------------------------
% Preps that specify time-like relations
%
% ({[Mp-]-0.09} & MVp-): If we can identify both the head verb,
% and the head noun, then do so. It is tempting to think that these
% should modify the verb, only, but conjunctions prove otherwise:
% "... went to hell recently, and heaven before that." shows that
% "recently" has to modify "hell", and not "went".

recently:
  {EE- or EF+} & (
    ({Xd- & Xc+} & {[Mp-]-0.09} & MVp-)
    or Pp-
    or E+
    or ({Xc+ & {Xd-}} & dCO+)
    or EB-
    or JT-
    or <advcl-verb>
    or Qe+
    or [[Mp-]]);

% Wc- & Qd+: "Now, am I right?"
% MJr-: "when, if not now, do you want to do it?"
now.r:
  ({Xd- & Xc+} & {[Mp-]-0.09} & MVp-)
  or Pp-
  or E+
  or ({Xc+ & {Xd-}} & dCO+)
  or (Wc- & (Xc+ or [()]) & Qd+)
  or EB-
  or dMJr-
  or [[Mp-]];

% Wc- & Qd+: "Then, am I right?"
% {Xd-} & MVs- & Xs- & <subcl-verb>:  "I eat, then I sleep"
% JT+ & CO+: "then last week, I changed my mind"
% JT+: "if not next Tuesday, then when do you want to do it?"
% EB- & EZ+: "... worked first as a teacher and then as a photographer."
then.r:
  ({Xd- & Xc+} & {[Mp-]-0.09} & MVp-)
  or Pp-
  or E+
  or ({JT+} & {Xc+ & {Xd-}} & dCO+)
  or JT+
  or (Wc- & (Xc+ or [()]) & Qd+)
  or (EB- & {[EZ+]-0.1})
  or (S+ & Xd- & Xc+ & MVs-)
  or ({Xd-} & MVs- & Xs- & <subcl-verb>)
  or [[Mp-]];

% Wt-: "Later."  (all by itself) but also: "Later, he left"
% [Mp-]0.4:  see notes above <prep-main-b>; prefer MVb- when possible.
later earlier:
  ({ECa- or Yt-} &
    (E+ or
    [Mp-]0.4 or
    Pp- or
    ({[Mp-]-0.09} & MVb-) or
    (Wt- & {Xc+}) or
    [({Xc+ & {Xd-}} & dCO+)] or
    (Xd- & Xc+ & (MX*x- or MVx-)) or
    ({[[@Ec-]]} & {Xc+} & A+) or
    dAJrc- or dAJlc+)) or
  (Yt- & (<advcl-verb> or Qe+));

% --------------------------------------------------------
% Preps that specify space-like relations
everywhere anywhere:
  {EL+} & (
    (<subcl-verb> & (({Xc+ & {Xd-}} & dCO+) or ({Xd- & Xc+} & MVs-)))
    or (MVp- or Pp- or FM- or (Xc+ & Xd- & MVx-)));

% Pp-: "We are finally getting somewhere."
% Pp- & {EL+}: "I want to be somewhere else"
% MVp-: "The record skips somewhere else, too."
% EE+: "somewhere near, a mouse scratched."
% MVp+ & dCOp+: "Somewhere, far away, a dog barked."
% almost like <prep-main-b> ...
somewhere someplace:
  ({EL+} & (
    FM-
    or Pp-
    or MVp-
    or [({Xc+ & {Xd-}} & dCO+)]
    or (Xc+ & Xd- & MVx-)
  ) & Mp+)
  or ({EL+} & (MVp- or Pp-))
  or ({EL+} & {Xc+ & {Xd-}} & MVp+ & {Xc+ & {Xd-}} & dCOp+)
  or ({EL+} & {Xc+ & {Xd-}} & [[dCO+]])
  or EA+
  or EE+
  or EP+;

nowhere:
  ({EL+} & (MVp- or Pp- or FM- or (Xc+ & Xd- & MVx-)))
  or EP+;

% EE- & COp+: "somewhere near, a mouse scratched."
% EE- & FM-: "The ticking came from somewhere near."
% The EE- above should be replaced by ... EP- ??? XXX TODO
% dWl-: "somewhere near"
near.p:
  ({EE- or EF+} & (
    <alter-preps>
    or (J+ & (<prep-main-b> or FM- or <fronted>))))
  or ({EP-} & dWl- & {J+})
  or (EE- & {Xc+} & dCOp+)
  or (EE- & FM-);

% SF*p+: "nearby is another temple" (using p for 'prep')
% SFpp+: "nearby are more ruins"
% {EE-} & {Xc+} & COp+: "{somewhere} nearby, a mouse scratched"
% {EE-} & FM-: "The ticking came from {somewhere} nearby."
nearby close_by handy.r:
  A+
  or MVp-
  or Pp-
  or (SF*p+ & <CLAUSE>)
  or (EP- & dWl- & {J+})
  or ({EE-} & FM-)
  or ({EE-} & {Xc+} & dCOp+);

% "It lies in the regions nether"
nether.r distant.r:
  MVp-;

% similar to <prep-main-b> but not quite ...
all_over all_around:
  {J+} & (Mp- or Pp- or MVp- or dWl- or [({Xc+ & {Xd-}} & dCO+)] or FM-);

% Consider "Here's the ball." We have two choices: SFst+ as a filler-it,
% or <fronted> with subject-verb inversion.  Both seem reasonable.
here:
  J-
  or <prep-main-b>
  or [dSFst+ & <CLAUSE>]0.15
  or <fronted>;

% Wi-:  [come] Over here!
over_here: <verb-co>;

% EN- & Pp-: "you are halfway there"
% EN- & J-: "we stopped about halway there"
% Wl-: "There!"
% Wp- & PFt+: "there lay the ball"; the PFt+ prevents connections to
%             the PFb- on <vc-be>.
%             "there the remains can be found"
there.r thither:
  J-
  or <prep-main-b>
  or [(dSFst+ or dSFp+ or dSFut+) & <CLAUSE>].15
  or [dSFIst-].15
  or [dSFIp-].15
  or <fronted>
  or OXt-
  or (EN- & (Pp- or J-))
  or dWl-;

% This seems to be the simplest way to make "Go home!" parse correctly...
home.r: MVp-;

away: ({Yd-} & (MVp- or Pp- or ({Xc+ & {Xd-}} & dCO+))) or K- or dWl-;
aboard: ((MVp- or Mp- or Pp-) & {J+}) or K- or dWl-;
apart: {Yd-} & K-;

inside.r outside.r underneath alongside:
  <alter-preps>
  or ({EP-} & dWl-)
  or ({J+} & (<prep-main-b> or FM-));

amid plus.p minus.p via onto on_to:
  J+ & (<prep-main-b> or <fronted>);

% Bare-naked MVp-: "I want it back"
back.r: ({Yd-} & K-) or (MVp+ & (MVp- or FM-)) or MVp-;
forth aside.p: K- or <adv-as>;

next_to in_back_of in_front_of close_to on_top_of outside_of
inside_of atop:
  <alter-preps> or
  <locative> or
  (J+ & (<prep-main-b> or FM- or <fronted>));

ahead_of by_way_of akin_to betwixt vis-a-vis vis-à-vis cf.
in_lieu_of on_account_of in_place_of in_search_of:
  <alter-preps> or
  (J+ & (<prep-main-b> or <fronted>));

% XXX FIXME, the below is nutty; reassign to the above.
subject_to status_post in_conjunction_with sensu
 in_relation_to neath across_from circa ca. c.
 previous_to together_with as_regards s/p aka unto
 apropos_of w.i W.i:
 ({JQ+} & (J+ or Mgp+) & <prep-main-a>) or (MVp- & B-);

% --------------------------------------------------------
% More complex space-like prepositional phrases

overhead.r midway in_public in_private en_route
a_la_mode a_la_carte side_by_side from_coast_to_coast: <prep-main-b>;

% {EP-} & dWl-: "Just upstairs"
abroad upstairs.r downstairs.r overseas.r next_door:
  <prep-main-b> or ({EP-} & dWl-) or FM-;

elsewhere:
  <prep-main-b> or FM- or [[J-]];

ahead at_hand in_store in_reverse in_place in_town
under_way in_office out_of_office out_of_reach
in_reach within_reach on_guard at_large in_hand on_hand for_free
in_line in_loco_parentis on_board en_route in_bed
out_of_bed on_strike on_top from_afar at_stake in_question
at_issue on_lease on_trial in_league in_cahoots in_front in_back
on_break on_camera in_command in_concert by_association in_association
on_deck on_disk on_file on_foot on_location on_line online.r:
  MVp- or Mp- or Pp- or (Xc+ & Xd- & (MX*x- or MVx-));

uptown downtown.r offshore.r underground.r out_of_town:
  MVp- or Mp- or Pp- or FM- or ({EP-} & dWl-) or (Xc+ & Xd- & MVx-);

<common-prep>: MVp- or Pp- or (Xc+ & Xd- & MVx-);

herewith hereinafter therefrom therein thereon thereupon: <common-prep>;

/en/words/words-medical.prep.1: <common-prep>;

backward backwards forwards.r
sideways ashore abreast aft
leftward leftwards rightward rightwards
downhill southward underfoot westward eastward northward:
  <common-prep> or <locative>;

half-way.r halfway.r
two-fold
overnight.r on_hold on_track in_situ in_toto off_balance
on_tiptoe
in_check on_course off_course under_oath at_end by_example on_holiday
by_invitation on_patrol on_stage in_step in_tempo on_schedule
behind_schedule ahead_of_schedule for_good for_keeps
out_of_step out_of_phase in_tune out_of_tune in_session out_of_session
in_phase neck_and_neck under_contract
no_place out-of-doors out_of_hospital:
  <common-prep>;

% K-: "put forward the argument..."
forward.r: <common-prep> or K- or <adv-as>;

% 5' 3' are DNA ends
upstream downstream 5' 3':
  A+ or
  NIfp+ or NItp- or
  ({Yd- or EZ- or EE- or ET-} & {MVp+ or OF+} &
    (({Xc+ & Xd-} & (Ma- or dMJra-)) or
    hMJra+ or
    <fronted> or
    MVp- or
    Pp- or
    FM- or
    (Xc+ & Xd- & (MVx- or MX-))));

%upstream downstream 3' 5':
%A+ or
%((EZ- or Y-) & (MVp+ or OF+) & (MV- or MV+)) or
%(ET- or EZ- or Y- & Ma- & (MVp+ or OF+)) or
%(EE- or Y- & (FM- or TO-) & MVp+ or OF+);

indoors outdoors underwater.r:
  MVp- or Pp- or FM- or ({EP-} & dWl-) or (Xc+ & Xd- & MVx-);

% --------------------------------------------------------
% Patronymics and misc french/spanish/german connectives
% Many of these are already in the adjectives list
à auf aus aux comte comtes
dans de de_la del della delle der des du duc
la las le.c los nach noch och os ou på por
sans te über un une vom von zum zur zu:
  {G-} & G+;

% Spanish/Italian: "Dolce y Gabbana"
y.and: G- & G+;

% ====================================================================
% TIME AND PLACE EXPRESSIONS

% (Xd- & {Xc+} & MV+ & MVx-):  "We are ready, this time for sure."
this_time this_one_time this_once that_time these_days:
  <prep-main-t>
  or (Xd- & {Xc+} & MV+ & MVx-)
  or [[E+]];

last_time next_time:
  <prep-main-t>
  or JT-
  or YS+
  or [[<noun-main-s>]]
  or Wa-;

% Js-: "show results from last week"
day.r week.r month.r year.r weekend.r morning.r afternoon.r evening.r
night.r semester.r term.r season.r session.r:
  ((DTn- or DTi-) & (<prep-main-t> or [[E+]]))
  or (DTi- & (JT- or YS+ or Js- or [[<noun-main-s>]]))
  or (DTa- & <prep-main-t>);

the_next the_previous the_following this_past:
  DTn+;

% Strange -- the JT- is a time-expression link .. .is that right here ??
% I'm not convince any of this is correct...
articulo_mortis intra_vitam in_articulo_mortis in_extremis
 post_cibum post_coitum:
 <prep-main-t> or JT- or [[E+]] or YS+ or [[<noun-main-s>]];

% Js- links "show results from today"
<relative-date>:
  <prep-main-t> or JT- or Js- or YS+ or [[<noun-main-s>]];

% Date inverted relative: "What a great day yesterday was!"
% "What a great day was yesterday!"
<date-inv-rel>: Ss+ & Rn-;

% {Xd-} & MX-: "What a great day today!"
today tonight:
  <relative-date>
  or <date-inv-rel>
  or ({Xd-} & MX-)
  or [[E+]];

yesterday:
  {TD+} & (<relative-date> or <date-inv-rel> or [[E+]]);

tomorrow:
  {TD+} & (<relative-date> or <date-inv-rel>);

Monday Tuesday Wednesday Thursday Friday Saturday Sunday.i:
  ((DTn- or DTie- or [()]) & {G-} & {TD+ or TW+}
    & (<relative-date> or <date-inv-rel> or ON-))
  or [[AN+]];

morning.i afternoon.i night.i evening.i:
  TD-;

% (ND- & TY+ & MVp-): "John (born 20 December 1975) is clever"
January.i February March April.i May.i June.i
July August.i September.i October November December:
  ((DTn- or DTie- or ({TA-} & {TY+})) &
    (JT-
    or Jp-
    or YS+
    or IN-
    or [<noun-and-s>]
    or [[{ND-} & <noun-main-s>]]))
  or ((DTn- or DTie-) & <prep-main-t>)
  or (TM+ & {TY+} &
    ((Xd- & Xc+ & TW-)
    or ON-
    or JT-
    or [[<noun-main-s> or MVp- or Mp- or AN+]]))
  or (ND- & TY+ & MVp-)
  or <date-inv-rel>
  or AN+
  or Wa-;

% The naked ND- can occur with time intervals:
% "I can't decide between 7:30AM and 9:30AM"
% AM.ti PM.ti am.ti pm.ti a.m. p.m. o'clock:
/en/words/units.5:
  ND- & {{@MX+} & <noun-main-s> & {TZ+}} ;

% Time-zone names
A.tz ACDT.tz ACST.tz ADT.tz AEDT.tz AEST.tz AKDT.tz AKST.tz AST.tz
AWDT.tz AWST.tz B.tz BST.tz C.tz CDT.tz CEDT.tz CEST.tz CET.tz CST.tz
CXT.tz D.tz E.tz EDT.tz EEDT.tz EEST.tz EET.tz EST.tz F.tz G.tz
GMT.tz H.tz HAA.tz HAC.tz HADT.tz HAE.tz HAP.tz HAR.tz HAST.tz HAT
HAY.tz HNA.tz HNC.tz HNE.tz HNP.tz HNR.tz HNT.tz HNY.tz I.tz IST.tz
K.tz L.tz M.tz MDT.tz MESZ.tz MEZ.tz MSD MSK.tz MST.tz N.tz NDT.tz
NFT.tz NST.tz O.tz P.tz PDT.tz PST.tz Q.tz R.tz S.tz T.tz U.tz UTC.tz
V.tz W.tz WDT.tz WEDT WEST.tz WET.tz WST.tz X.tz Y.tz Z.tz:
  {Xd-} & TZ-;

% Abbreviated month names.
Jan.x Feb.x Mar.x Apr.x May.x Jun.x Jul.x Aug.x Sep.x Sept.x Oct.x Nov.x Dec.x:
 {Xi+} & TM+ & {TY+} &
   ((Xd- & Xc+ & TW-) or
   ON- or
   JT- or
   [[<noun-main-s> or MVpn- or Mp- or AN+]]);

fall.i spring.i winter.i summer.i:
  ((DTn- or DTi-) & <prep-main-t>) or
  (DTi- & (JT- or YS+ or [[<noun-main-s>]]));

% Jd- & Dmc-: "Millions of years ago..."
weeks.i days.i hours.i minutes.i seconds.i months.i years.i decades.i
centuries.i semesters.i terms.i nights.i:
  ((ND- or (Jd- & Dmc-) or [[EN-]] or [()]) & (Yt+ or (Ot- & {Mp+})))
  or (ND- & Ye-);

week.i day.i hour.i minute.i second.i month.i year.i decade.i century.i
semester.i term.i night.u:
  (NS- & (({NJ-} & {EN-} & (Yt+ or Ot-)) or (EN- & J-)))
  or (NSa- & [[Mp- or Ys-]])
  or ({NR- or TT-} & DG- & ((<subcl-verb> & (({Xc+ & {Xd-}} & dCO+) or MVp- or (Xd- & Xc+ & MVx-))) or Yt+));

year_and_a_half: NSa- & {EN-} & (Yt+ or Ot-);
moment.u:
  (NS- & (({EN-} & (Yt+ or Ot-)) or (EN- & J-)))
  or ({NR- or TT-} & DG- & ((<subcl-verb> & (({Xc+ & {Xd-}} & dCO+) or MVp- or (Xd- & Xc+ & MVx-))) or Yt+));

a_while: J- or Yt+ or Ot- or <adv-as>;
now.i then.i: JT- or FM-;
now_on then_on there_on: FM-;
from_now: Yt- & <prep-main-t>;

a_long_time some_time a_few_moments moments.u:
  Yt+ or Ot-;

% I can't figure out what the Js- would be for... ??
% ago: Yt- & (<prep-main-e> or <advcl-verb> or Qe+ or JT- or Js-);
ago:
  Yt- & (<prep-main-e> or <advcl-verb> or Qe+ or JT-);

every.i: {EN-} & Ye+ & <prep-main-t>;

% cost on MVp-: "about five times".
% cost on Qe-: prefer R & B in "how many times did you do it?"
times.i x.i:
  (ND- & (({Xc+ & {Xd-}} & dCOa+)
       or MVp- or EC+ or EZ+ or <advcl-verb> or [Qe+]0.5))
  or (((({ND-} & DG-) & {<subcl-verb>}) or (ND- & Ys+)) &
       (({Xc+ & {Xd-}} & dCO+) or [MVp-]0.1 or (Xd- & Xc+ & MVx-)));

time.i:
  {TT- or NR-} & DG- & {<subcl-verb>} &
    (({Xc+ & {Xd-}} & dCO+) or MVp- or (Xd- & Xc+ & MVx-));

the_year: TY+ & <noun-main-s>;
every_time:
  {EN-} & (<subcl-verb> & (({Xc+ & {Xd-}} & dCO+) or MVp- or (Xd- & Xc+ & MVx-)));

week.n moment.n hour.n minute.n year.n instant.n period.n month.n
second.n decade.n century.n:
  {NM+} & ((<noun-modifiers> &
    ((Ds- & {@M+} & {WN+ or TH+ or [[<embed-verb>]] or (R+ & Bs+)} & {@MXs+} &
      (<noun-main-s> or
      <rel-clause-s> or
      <noun-and-s>)) or
    Us- or
    (YS+ & Ds-) or
    (GN+ & (DD- or [()])))) or
  AN+);

day.n night.n:
  {NM+} & ((<noun-modifiers> &
    (({D*u- or @M+} & {WN+ or TH+ or [[<embed-verb>]] or (R+ & Bs+)} & {@MXs+} &
      (<noun-main-m> or
      <rel-clause-s> or
      <noun-and-x>)) or
    Us- or
    (YS+ & {D*u-}) or
    (GN+ & (DD- or [()])))) or
  AN+);

% {Dmc- or @M+}: avoid used Dmc together with M when parsing sentences like
% "She is two years older than me"
% ND- & A- & D- & Jp-: "we walked for a further three hours"
% Dmc- & Wa-: "Many years!" "How many years?"
days.n weeks.n moments.n hours.n minutes.n years.n instants.n
periods.n months.n nights.n seconds.n decades.n centuries.n:
  ({NM+} & ((<noun-modifiers> &
      (({Dmc- or @M+} & {WN+ or TH+ or <embed-verb> or (R+ & Bp+)}  & {@MXp+} &
        (<noun-main-p> or
        % <rel-clause-p> or
        <rel-clause-t> or
        <noun-and-p>)) or
      Up- or
      (YP+ & {Dmc-}) or
      (GN+ & (DD- or [()])))) or
    [[AN+]]))
  or (ND- & A- & D- & Jp-)
  or (Dmc- & Wa-);

% XXX A major problem here is that the dict entries for miles.n, feet.n
% create a bunch of parses that are wrong & interfere with the below.
% Jp-: "we walked for three kilometers"
% ND- & A- & D- & Jp-: "we walked for a further three kilometers"
% ND- & Rw+ & Bpm+: "how many miles did you bike?"
% ND-: "you biked how many miles?"
<units-funky-plural>:
  ((ND- or [()] or [[EN-]]) & (Yd+ or Ya+ or EC+ or [[MVp-]] or OD-))
  or ((ND- or [()]) & Jp-)
  or (ND- & A- & D- & Jp-)
  or (ND- & Rw+ & Bpm+) % <rel-clause-p>
  or ND-
  or (ND- & (NIfu+ or NItu- or EQt+ or EQt-));

% AU is abbreviation for "astronomical units"
blocks.i feet.i miles.i yards.i inches.i
meters.i millimeters.i centimeters.i micrometers.i kilometers.i
microns.i Angstroms.i wavelengths.i
AU.i au.i astronomical_units light-years.i: <units-funky-plural>;

block.i foot.i mile.i yard.i inch.i
meter.i millimeter.i centimeter.i micrometer.i kilometer.i
micron.i Angstrom.i wavelength.i
astronomical_unit light-year.i:
  (NS- & {NJ-} & {EN-} & (Yd+ or EC+ or [[MVp-]] or Ya+ or OD-)) or Us-;

% make sure that mile.i always has precedence over mile.n
% XXX TODO: probably same for the other .i's above...
mile.n:
  <marker-common-entity> or [<common-noun>];

a_long_way: Yd+;
point.i percentage_point:
(NS- or NIe-) & {NJ-} & (Yd+ or OD-);
points.i percentage_points: ND- & (Yd+ or MVp-);

dollars.i cents.i:  NIn- & (EC+ or Yd+ or OD-);
1_dollar one_dollar a_dollar 1_cent
one_cent a_cent: {NJ-} & (EC+ or Yd+ or OD-);
share.i pound.i ounce.i gallon.i barrel.i head.x: NSa- & Mp-;

twofold threefold fourfold fivefold sixfold sevenfold eightfold ninefold
tenfold a_hundredfold a_thousandfold:
  {EN-} & (MVp- or Em+ or EC+ or [Pa-] or A+
     or (Xd- & Xc+ & MX-) or NIfn+ or NItn-);

% Add cost to Op-, try to use any other linkage before making
% a unit be a plain-old object.
% A- & ND-:  "200 square ft of plywood"
<units-suffix>:
  ({A-} & (ND- or NS- or NIe-) & (NIfu+ or NItu-)) or
  ({A-} & (ND- or NS- or NIe-) & (AN+ or EQt+ or EQt-)) or
  ({A-} & (ND- or NS- or NIe-) & {DD- or EN-} & {Wd-} & {Mp+} & Sp+) or
  ({A-} & (ND- or NS- or NIe-) & ([{DD-} & Op-] or Jp-) & {Mp+}) or
  ({A-} & (ND- or NS- or NIe-) & Xd- & MX- & Xc+) or
  ((ND- or NS-) & {NJ-} & (EC+ or Y+ or OD- or (Us- & {Mp+}))) or
  Us-;

% Abbreviations of scientific units that follow numbers
% km².u mi².u in².u ft².u m².u cm².u
/en/words/units.1: <units-suffix>;
/en/words/units.3: <units-suffix>;
<UNITS>: <units-suffix>;

% Allows "200 sq. ft. of plywood", "200 cu yds of concrete"
/en/words/units.a: A+;

% Units abbreviations that can be followed by a period:
% ft. tbsp. yds.
/en/words/units.1.dot: {Xi+} & <units-suffix>;

% Time unit abbreviations:
<time-units>: <units-suffix> or ((ND- or NS-) & {NJ-} & Ot-);
/en/words/units.4: <time-units>;
/en/words/units.4.dot: {Xi+} & <time-units>;

% money, similar to units, above.
% Ds-: "We are talking about the dollar"
/en/words/currency:
  ((NIm- or NIn- or NIe-) & AN+)
  or ((NIm- or NIn- or NIe- or Ds-) & (Op- or Jp-) & {Mp+})
  or ((NIm- or NIn-) & {NJ-} & (EC+ or Yd+ or OD-))
  or <noun-assert>
  or Ou-
  or Us-;

% {NI-} & Jp-: "a purseful of dollars"
% Dmc- & Wa-: "How many dollars?"
/en/words/currency.p:
  ((NIn- or NIe-) & AN+)
  or ({NIn- or NIe-} & (Op- or Jp-) & {Mp+})
  or ((NIn-) & {NJ-} & (EC+ or Yd+ or OD-))
  or <noun-assert>
  or Up-;

% number-and-unit combinations, such as "50-kDa". The linking requirements
% should largely follow those for units except not allowing a numeric
% determiner.
% TODO: the <noun-main-x> linking requirements are likely rarely used, and
% it might be beneficial to cost them. If this is done, make the same
% modification for unit ranges also.
% NUMBER-AND-UNIT:
% ((({D*u-} or {Dmc-}) & <noun-rel-x> &
% (<noun-main-x> or Bsm+)) or (({D*u-} or {Dmc-}) & Us- & {Mp+})) or A+;
% Above screw up the usual units processing.

% ======================================================================
% QUESTION WORDS

% Allow a question to be preceded by a clause opener.
% "By the way, how was it?"
<clause-q>: {hCO-} & Wq-;

% QI- & (): "I do not know who"
% Ws- & Bsw+ & Sp*w+: "Who have bought your flat from?"
% {EL+ & {N+}} & Wd-: "Who?" "Who else?" "Who else not?"
% Wq- & Qw+: "who are they?"
% Jw-: "For who were you mistaken?"
who:
  (R- & (({MVp+ or MVx+} & RS+) or <porcl-verb>))
  or [QI-]
  or QN-
  or dSJl+ or dSJr-
  or Jw-
  or ({EL+} & ((S**w+ & {Bsw+}) or (R+ & B*w+)) & {EW-} & (Ws- or Wq- or QI*d- or BIqd-))
  or ({EL+ & {N+}} & Wd-)
  or (<clause-q> & Qw+)
  or ({MVp+ or MVx+} & (S**w+ or (R+ & B*w+))
     & (Xd- & Xc+ & MX*r-));

% Sp+: "what are the answers?"
% Ww-: Dr. Who: "What!"
% {EL+} & Ww-: "What else?" "What the fuck?"
% Wn- & O+: "What a jerk!"
% QI-: "I'll tell you what", "Say what?"
% Jw-: "To what do you owe your success?"
what:
  ({EL+} &
      (D**w+
      or Ss*w+
      or Sp*w+
      or (R+ & (Bsw+ or BW+)))
    & {hCO-} & {EW-} & (Wb- or Wq- or Ws- or QI*d- or QN- or BIqd- or QJ+ or QJ-))
  or ({EL+} & Ww-)
  or (Wn- & O+)
  or ((Ss*d+ or (R+ & (Bsd+ or BW+)))
    & (<noun-main2-s-no-punc> or (Ss*t+ & <CLAUSE>) or SIs*t-))
  or (D+ & JQ-)
  or Jw-
  or [QI-]0.5
  or QN-
  or dSJl+ or dSJr-;

% [QI-]: "I do not know which"
% (R+ & B*w+ & (QJ+ or QJ-)): "... which to pick and which to leave behind."
% QN- & D+: "you ate which one?"
which:
  ((Jr- or R-) & (({MVp+ or MVx+} & RS+) or <porcl-verb>))
  or ((D**w+ or ({OF+} & (S**w+ or (R+ & B*w+)))) & {EW-} & (Wq- or Ws- or QI*d- or BIqd-))
  or (JQ- & D+)
  or ({MVp+ or MVx+} & (S**w+ or B*w+) & (Xc+ & Xd- & MX*r-))
  or [QI-]
  or (QN- & {D+})
  or (R+ & B*w+ & (QJ+ or QJ-))
  or Jw-;

% <directive-opener> or Wi-: "Which way, left or right?"
which_way:
  <directive-opener> or <verb-co>;

% Jw-: "From whom did you run?"
whom:
  (R- & <porcl-verb>)
  or (R+ & B*w+ & {EW-} & (Wq- or QI*d- or BIqd- or (Xc+ & Xd- & MX*r-)))
  or (Jr- & (RS+ or <porcl-verb>))
  or Jw-;

whose:
  (D**w+ & (
    Mr-
    or ({EW-} & Wq-)
    or Ws-
    or QI*d-
    or BIqd-
    or (Xc+ & Xd- & MX*d-)))
  or (JQ- & D+)
  or (U+ & Jr- & (RS+ or <porcl-verb>));

% Os-: "I'll hire whomever I can find" "I'll hire whomever"
% EL+ & SJr-: "Bring him and whomever else"
% ({EL+} & Os- & Bsd+): Bring whomever else you care to.
whomever:
  (R- & <porcl-verb>)
  or (B*w+ & (Wq- or QI*d- or BIqd- or (Xc+ & Xd- & MX*r-)))
  or ({EL+} & dSJr-)
  or ({EL+} & (Ss*d+ or Bsd+ or {[[]]}) & Os-)
  or (Jr- & (RS+ or <porcl-verb>))
  or Jw-;

% EL+ & SJr-: "Bring him and whomever else"
whoever: {EL+} &
  (((Ss*d+ or Bsd+ or [[CX+]] or {[[]]}) &
    (<noun-main-s> or (Xc+ & {Xd-} & dCO+) or ({Xd- & Xc+} & MVs-)))
  or ({EL+} & dSJr-)
  or [[(O- or J-) & CX+]]);

whatever.c:
  ({EL+} & (((Ss*d+ or Bsd+ or BW+ or D**w+)
      & (<noun-main-s> or (Xc+ & {Xd-} & dCO+) or ({Xd- & Xc+} & MVs-)))
    or [[(O- or J-) & CX+]]))
  or ((ALx+ & J+) & (({Xd-} & Xc+ & dCO+) or ({Xd- & Xc+} & MVs-)));

whenever wherever however.c:
  {EL+} & (<subcl-verb> & (({Xc+ & {Xd-}} & dCO+) or ({Xd- & Xc+} & MVs-)));

whyever:
  ({EL+} & (
     ({hCO-} & Ww- & Qw+)
     or (QI- & (<subcl-verb> or <ton-verb>))
     or (<subcl-verb> & ((SFsx+ & <S-CLAUSE>) or WY- or BIq-))))
  or [[{@hCO-} & Wc- & Wi+]];

whichever:
 ({EL+} & (((Ss*d+ or Bsd+ or BW+ or D**w+)
     & (<noun-main-s> or (Xc+ & {Xd-} & dCO+) or ({Xd- & Xc+} & MVs-)))
       or [[(O- or J-) & CX+]]))
  or ((ALx+ & J+) & (({Xd-} & Xc+ & dCO+) or ({Xd- & Xc+} & MVs-)));

% Ww- & Qw+: "whither did it come?"
% EW-: "From whither did it come?"
whence whither:
  {EL+} & ((<subcl-verb> & (({Xc+ & {Xd-}} & dCO+)
      or ({Xd- & Xc+} & MVs-)))
    or ({hCO-} & {EW-} & Ww- & Qw+));

% Comparative-opener: "although a good worker, he's not a very good manager"
<COMP-OPENER>: (O*c+ & {Xc+ & {Xd-}} & dCOc+);

although.e in_as_much_as whereas whereof wherein:
  (<subcl-verb> & (({Xc+ & {Xd-}} & dCO*s+) or ({Xd- & Xc+} & MVs-)))
  or ({Xd-} & <coord> & (Wd+ or Wp+ or Wr+))
  or <COMP-OPENER>;

% QI- & (): "I do not know when"
% (Mv- & Cs+): "an examination when it happened revealed chicanery"
% The above is yucky, since it allows broken parses, e.g.
% *The school when I lived in England was wonderful
% Perhaps a better solution might be some special case of WN+ on
% examination? (would require 'examination' and all other 'operation'
% nouns to be in their own class...
% SJ: "when, but not how, will be decided."
% MJ: "when, if not next Tuesday, do you want to do it?"
% JT- & MJr-: "if not next Tuesday, then when do you want to do it?"
when:
  ((WN- or BIh-) & <subcl-verb>)
  or ((<ton-verb> or <subcl-verb>) & (BIq- or QI- or (SFsx+ & <S-CLAUSE>)))
  or (Mv- & <subcl-verb>)
  or [QI-]0.5
  or QN-
  or [dSJl+ or dSJr-]0.5
  or [dMJl+]0.5
  or ({JT-} & dMJr- & Qw+)
  or ({EW-} & (QJ- or QJ+))
  or ({hCO-} & {EW-} & (Ww- or Wq-) & {Qw+})
  or ((<subcl-verb> or Mp+ or Mgp+ or Mv+) &
    (({Xd- & Xc+} & MVs-) or ({Xc+ & {Xd-}} & dCO*s+) or (Xd- & Xc+ & E+)));

% QI- & (): "I do not know why"
% COa+: "Why, of course it will!"
% N+: "why not?"  "Why the hell not?"
% Wv- & Qa+: "why go there?"
why:
  {EL+} & (
    ({hCO-} & {EW-} & (Ww- or Wq-) & {Qw+ or N+})
    or (Wv- & Qa+)
    or (QI- & (<subcl-verb> or <ton-verb> or [()]0.5))
    or QN-
    or (<subcl-verb> & ((SFsx+ & <S-CLAUSE>) or WY- or BIq- or QJ+ or QJ-))
    or dCOa+
    or [dSJl+ or dSJr-]0.5
    or ({EW-} & (QJ- or QJ+))
    );

% QI- & (): "I do not know where"
% R+ & Bsw+: "Where does it go to?"
% Cs+ & Bsw+ & QI-: "Can you tell us where those strange ideas came from?"
% {EW-}: "about where did you put it?"
% Jw-: "From where did you get it?"
where:
  {EL+}
    & (
      ({hCO-} & {EW-} & Wq- & ((Rw+ & WR+) or (R+ & Bsw+) or Qw+))
      or [QI-]0.5
      or QN-
      or [dSJl+ or dSJr-]0.5
      or ({EW-} & (QJ- or QJ+))
      or (<subcl-verb> & Bsw+ & QI-)
      or ((WR+ or <subcl-verb> or <ton-verb>) & (BIq- or QI- or (SFsx+ & <S-CLAUSE>)))
      or ((<subcl-verb> or WR+) & <prep-main-b>));

whether:
  ((QI- or BIq-) & (<subcl-verb> or <ton-verb>))
  or (<subcl-verb> & SFsx+ & <S-CLAUSE>)
  or dSJl+ or dSJr-
  or [[(<subcl-verb> or MV+) & (({Xd- & Xc+} & MVs-) or ({Xc+ & {Xd-}} & dCO*s+))]];

whether_or_not:
  ((QI- or BIq-) & (<subcl-verb> or <ton-verb>))
  or (<subcl-verb> & (({Xd- & Xc+} & MVs-) or ({Xc+ & {Xd-}} & dCO*s+)));

% QI- & (): "I do not know how"
% QI- & H+: "it happened how many times?"
% EL+: "How else would you say that?"
% (EAh+ or EEh+) & Wq-: "How big?" "How quickly?"
%    Unfortunately, this is blocked by "S-V inversion required7"
% Wh- & H+: "How much money?"
how:
  ((((EAh+ or EEh+) & {HA+}) or H+ or AFh+) &
    {EW-} & (BIqd- or QI*d- or Wq- or Ws-))
  or [[{EW-} & Ww-]]
  or ({EW-} & Wh- & H+)
  or ({EW-} & <clause-q> & (({EL+} & Qw+) or AF+))
  or [QI-]0.5
  or (QN- & {H+})
  or ({EW-} & (QJ- or QJ+))
  or [dSJl+ or dSJr-]0.5
  or ((<subcl-verb> or <ton-verb>) & (QI- or BIq- or (SFsx+ & <S-CLAUSE>)));

% ----------------------------------------------------
% CONJUNCTIONS  & ADVERBS

% "that" as subjunctive or paraphrasing
% EBx+: He told me that even his mother likes me
% (perhaps this should be changed to a different EB ??)
that.j-c:
  ({EBx+} & <that-verb> &
    ([SFsx+ & <S-CLAUSE>] or TH- or [[MVh-]] or RJ*t+ or RJ*t-))
  or (TS- & (SI*j+ or SFI**j+) & I*j+);

% "that" as determiner.
% D*u+: "Give me that thing"
that.j-d: [{AL-} & D*u+];

% "that" as adverb.
% Xd-: iffy punctuation, e.g "The man, that you saw laugh...
that.j-r:
  ({[Xd-]} & R- & (({MVp+ or MVx+} & RS+) or <porcl-verb>))
  or EE+
  or (EA+ & {HA+})
  or DTn+;

% "that" as noun. naked Osn-: "give me that"
% cost on [<noun-and-s>] to avoid comma as conjunction.
that.j-p: ({[[M+]]} & <noun-main-h>) or [<noun-and-s>]0.5;

% "that" as topic. Wt is a topic link to LEFT-WALL.  "That I did not know".
that.j-t: Wt- & {Xc+} & Rn+ & B+;

% "No one is sitting at that there table"
that_there: Ds+;

% (Rnx+ & <verb-wall> & <CLAUSE-E>): "Because I said so"
% Not using Rnx+ & B+ above, because B+ goes too far...
because b/c bc cuz cos coz cause.j 'cause b'cause b'cuz bec bcoz:
  (<subcl-verb> & (({Xc+ & {Xd-}} & dCO*s+) or BIh- or ({Xd- & Xc+} & MVs-)))
  or (OF+ & (({Xc+ & {Xd-}} & dCO+) or BIh- or ({Xd- & Xc+} & <adv-as>)))
  or (Rnx+ & <verb-wall> & <CLAUSE-E>);

now_that just_as if_only in_case whereby whereupon insofar_as
inasmuch_as ere on_the_grounds_that on_grounds_that in_that
in_the_event_that in_the_event:
  <subcl-verb> & (({Xc+ & {Xd-}} & dCO*s+) or ({Xd- & Xc+} & MVs-));

if_only:
  ((MVp+ or MVa+ or MVs+) & ({Xd- & Xc+} & MVp-)) or (Wd+ & Wc-);

o_that:
  Wd+ & Wc-;

on_condition:
  (TH+ or TS+) & (({Xc+ & {Xd-}} & dCO*s+) or ({Xd- & Xc+} & MVs-));

unless though.r even_though:
  ((<subcl-verb> or Mgp+ or Mv+)
    & (({Xc+ & {Xd-}} & dCO*s+)
      or ({Xd- & Xc+} & MVs-)
      or (Xd- & Xc+ & E+)))
  or <COMP-OPENER>;

as_if as_though:
  ((<subcl-verb> or Mgp+ or Mv+ or Mp+)
    & (({Xc+ & {Xd-}} & dCO*s+) or
       ({Xd- & Xc+} & MVs-) or
        (Xd- & Xc+ & E+)))
  or ((BIh- or LI-) & <subcl-verb>);

as_soon_as:
  (<subcl-verb> & {Xc+ & {Xd-}} & dCO*s+)
  or ({EP-} & dWl- & (J+ or Paf+));

% J+ & CO+: "Until yesterday, ..." XXX this is wrong, should be JT+???
changequote(\,/)dnl
until 'til ’til ‘til `til til till.r:
changequote dnl
  ((Mgp+ or J+ or JT+ or UN+)
    & (({Xc+ & {Xd-}} & dCO+) or ({Xd- & Xc+} & MVp-) or [Mp-]))
  or (<subcl-verb> & (({Xc+ & {Xd-}} & dCO*s+) or ({Xd- & Xc+} & MVs-)))
  or <locative>;

since:
  ((Mgp+ or J+ or JT+ or UN+)
    & (({Xc+ & {Xd-}} & dCO+) or ({Xd- & Xc+} & MVp-) or [Mp-]))
  or (<subcl-verb> & (({Xc+ & {Xd-}} & dCO*s+) or ({Xd- & Xc+} & MVs-)))
  or ({E-} & <locative>)
  or [[MVa-]] or [[E+]];

ever_since:
  (J+ or Mgp+ or <subcl-verb>)
     & (({Xc+ & {Xd-}} & dCO*s+) or ({Xd- & Xc+} & MVs-));

after:
  {ET- or Yt-}
    & (((Mgp+ or J+ or JT+) & (<prep-main-b> or UN- or <advcl-verb> or Qe+))
      or (J+ & <fronted>)
      or <locative>
      or (<subcl-verb> & (({Xc+ & {Xd-}} & dCO*s+) or ({Xd- & Xc+} & MVs-))));

before:
  ({ET- or Yt-}
    & (({Mgp+ or J+ or JT+} & (<prep-main-b> or UN-))
      or (J+ & <fronted>)
      or <locative>
      or (<subcl-verb> & (({Xc+ & {Xd-}} & dCO*s+) or ({Xd- & Xc+} & MVs-)))))
  or (Yt- & (<advcl-verb> or Qe+));

if.r only_if if_and_only_if iff:
  (<subcl-verb> & {Xc+ & {Xd-}} & (Wd- & (Qd+ or Ws+ or Wq+)))
  or ((<subcl-verb> or [Mgp+] or [Mv+])
    & (({Xd- & Xc+} & MVs-) or ({Xc+ & {Xd-}} & dCO*s+)))
  or (QI- & <subcl-verb>);

% Perhaps the below is a cop-out, but getting the links just
% right between commas, buts, nots and onlys is proving to be hard.
but_only_if but_not_if:
  <subcl-verb> & (Xd- or [[()]]) & Xc+ & MVs-;

if_possible if_necessary:
  MVp-
  or (Xd- & Xc+ & (MVx- or E+))
  or ({Xc+ & {Xd-}} & dCO+);

if_so:
  ({Xc+ & {Xd-}} & dCO+);

no_wonder: (Wd+ or Wp+ or Wr+) & Wc-;

% Pa+: "it can be pressed into shape while cold"
% dWl- & Mp+: "while in transit"
% (Xc+ or Xp+ or RW+): avoid cost when parsing
%            "He cheered, while lifting his glass".
while whilst:
  ((<subcl-verb> or Mgp+ or Mp+ or Pa+) &
    (({Xc+ & {Xd-}} & dCO*s+) or
       ({Xd- & (Xc+ or Xp+ or RW+)} & MVs-) or
       (Xd- & Xc+ & E+)))
  or ({EP-} & dWl- & (J+ or Mp+))
  or <COMP-OPENER>;

<adverb-join>: dRJlv+ or dRJrv-;

% once as adverb
once.e:
  ({<subcl-verb> or Mp+} &
    (({Xc+ & {Xd-}} & dCO*s+) or
    ({Xd- & Xc+} & MVs-) or
    (Xd- & Xc+ & E+))) or
  <adverb-join> or
  E+ or
  EB-;

% --------------------------------------------------------------------------
% Conjunctions

% Should these be treated as clause-openers (taking CO link)?
% e.g. "Also on this list is the Colossus of Rhodes."
% Another possibility: (Rnx+ & <CLAUSE-E>) "Because I say so"
% XXX should review these, they probably overlap the other conjunction
% usages below.
%

and/or: [(({Xd-} & <coord>) or Wc-) & (Wdc+ or Qd+ or Ws+ or Wq+)];

% and used as a conjunction in proper names:
% The Great Southern and Western Railroad
% Hmm, Maybe should use MG- & JG+ here, to be consistent with "of", "for":
% "The Society of Engineers", "The League for Abstinence"
% Add a tiny cost, so that other conjunctions get a shot, first.
and.j-g: [G- & G+]0.05;

% Conjoined adjectival modifiers.
% The black and white cat sleeps
% She ran hot and cold.
% The {EBb+} link handles "not", "yet" after the "and" -- "blah and not blah"
% See also <comma-adj-conjunction> for similar structures.
% The {Xd-} allows "blah blah, and blah"
% MVa is given a cost, so that Pa is used, if possible.
% (In general, we want to never use MVa if Pa is possible... )
% AJla- & AJr*+ allows "he is clever and funnier than Mike."
% <tot-verb>: "Mary is too boring and depressing to talk to"
%
% AJ*a: ordinary adjectives
% AJ*c: comparative adjectives
% AJ*s: superlative adjectives
%
and.j-a but.j-a yet.j-a and_yet:
  ({Xd-} & hAJla- & {Xd-} & {EBb+} & hAJr+) & (A+ or Pa- or [MVa-] or dAJra-) & {<tot-verb>};

% XJo-: either ... or ...
or.j-a:
  ({Xd-} & hAJla- & {Xd-} & {XJo-} & hAJra+) & (A+ or Pa- or [MVa-] or dAJra-);

% XJn-: neither ... nor ...
% Its marked optional only to solve the ugly case of
% "he is in neither the 105th nor the 106th battalion"
% At issue is that nouns really really want to get a determiner, so we give it one.
nor.j-a: hAJ- & {Xd-} & {XJn-} & hAJ+ & (A+ or Pa- or [MVa-]);

% comparatives:
% he is bigger, and badder, than the pope.
% If one comma is there, then the other must be also.
% SJl- & AJrc+: "She was John's height, or taller"
% Xc+ & Am+: "faster and stronger, Gracie is expected to win"
and.j-c or.j-c but.j-c yet.j-c:
  ((hAJlc- & hAJrc+) or
  (Xd- & hAJlc- & hAJrc+ & Xc+) or
  ({Xd-} & hSJl- & {EB+} & hAJrc+)) &
    (((Pam- or Mam- or AFm+) & {@MV+}) or
    ({[ECa-]} & MVb-) or
    ({Xc+} & Am+));

% [MVa-] : "he ran the fastest and the farthest"
and.j-s:
  (hAJls- & hAJrs+ & La-) or
  (hAJls- & hAJrs+ & DD- & [MVa-]) or
  (hAJle- & hAJre+ & [MVa-]) or
  (hAJld- & hAJrd+ & (D+ or DD+));

% conjoined post-nominal modifiers, to be used with conjunctions below.
<post-nom-a>: [Ma-] or (Xd- & Xc+ & MX*a-);
<post-nom-p>: Mp- or MVp- or (Xc+ & dCO+);
<post-nom-j>: J+ & ([Mp-] or Pp- or MVp-);
<post-nom-v>: Xd- & Mv-;

% [Ma-]: it  is more grammatically correct to have commas ...
% {EBb+}: "blah but not blah", "blah and not blah"
% vs.: "We examined the setting (urban vs rural)"
% cost on MJ*j: allow but.misc-ex take the first shot.
but.j-m and.j-m vs.j-m vs..j-m versus.j-m v.j-m v..j-m:
  ({Xd-} & hMJla- & {EBb+} & hMJra+ & <post-nom-a>) or
  ({Xd-} & hMJlp- & {EBb+} & hMJrp+ & <post-nom-p>) or
  [{Xd-} & hMJlj- & {EBb+} & hMJrj+ & {Xc+} & <post-nom-j>]0.5 or
  ({Xd-} & hMJlv- & {EBb+} & hMJrv+ & {Xc+} & <post-nom-v>);

% {XJo-}: Either .. or ...
% SJr+: "either by chemicals or hammering" as a stand-in for
%    "by chemicals or by hammering" because the second "by" is a zero.
%    "either by hammering or chemicals"
or.j-m:
  ({Xd-} & hMJla- & {XJo-} & {EBb+} & hMJra+ & <post-nom-a>) or
  ({Xd-} & hMJlp- & {XJo-} & {EBb+} & (hMJrp+ or hSJr+) & <post-nom-p>) or
  ({Xd-} & hMJlj- & {XJo-} & {EBb+} & hMJrj+ & {Xc+} & <post-nom-j>) or
  ({Xd-} & hMJlv- & {XJo-} & {EBb+} & hMJrv+ & {Xc+} & <post-nom-v>);

% XJn-: Neither .. nor ...
nor.j-m:
  ({Xd-} & hMJla- & {XJn-} & {EBb+} & hMJra+ & <post-nom-a>) or
  ({Xd-} & hMJlp- & {XJn-} & {EBb+} & hMJrp+ & <post-nom-p>) or
  ({Xd-} & hMJlj- & {XJn-} & {EBb+} & hMJrj+ & {Xc+} & <post-nom-j>) or
  ({Xd-} & hMJlv- & {XJn-} & {EBb+} & hMJrv+ & {Xc+} & <post-nom-v>);

% Conjoined question words.
% When and where is the party?
% How and why did you do that?
and.j-q: (QJ- & QJ+) & ((Wq- & Q+) or QI-);

% conjoined adverbs/prepositional phrases
% RJ*v: adverbs
% RJ*t: that "He said that ... and that ..."
% RJ*c: subordinate clauses: "Although he said ... and he did ..., ..."
% RJ*j: prep-object-relative (Mj): "the man for whom and with whom ..."
% RJ*r: "those" relative clauses: "...: those who do and those who don't"
and.j-r or.j-r:
  ((hRJlv- & hRJrv+) & MVr-) or
  ((hRJlt- & hRJrt+) & TH-) or
  ((hRJlc- & hRJrc+) & Cs-) or
  ((hRJlj- & hRJrj+) & Mj- & <subcl-verb>) or
  (({Xd-} & hRJlr- & hRJrr+) & J-);

% Conjoined nouns/noun phrases.
% "The cost and reliability were questioned" (Spx+)
% "He wrote for piano and flute."  (Ju-)
% "Where is the sickle and hammer?" (SIs-)
% Op- has a cost, so that "they verbed X and verbed Y" gets the VJ link
% at zero cost, and the SJ link at higher cost (since a "verbed Y" can be
% understood as a modified noun).  Actually, should probably have some
% post-processing rule to disallow this XXX to do fix above.  Example of
% bad SJ usage: "He bangs drums and played piano" i.e "he bangs a played piano"
%
% <noun-conjunction>: ({Xd-} & SJl- & SJr+) & etc.
% would allow "X , and Y" constructions, but these have tricky rules...
%
% noun-conj-dep-s & SI-: Are a dog and a cat here?
%
% XXX There should be a noun-sub-u but this requires a lot of work ...
<noun-conj-dep-s>: ({Xd-} & hSJls- & hSJrs+ & {[[Xc+]]});
<noun-conj-dep-p>: ({Xd-} & hSJlp- & hSJr+  & {[[Xc+]]}) or
                   ({Xd-} & hSJls- & hSJrp+ & {[[Xc+]]});
<noun-conj-dep-u>: ({Xd-} & hSJlu- & hSJr+  & {[[Xc+]]}) or
                   ({Xd-} & hSJlp- & hSJru+ & {[[Xc+]]}) or
                   ({Xd-} & hSJls- & hSJru+ & {[[Xc+]]});

% Give AN+ a cost, because in general, we don't want to conjoin nouns,
% and then use the resulting phrase to modify another noun ...
<noun-conj-head>: Ju- or dSJl+ or [[AN+]];

% XXX WTF? why does [O-] have a cost that the post-nominal doesn't?
% Having this cost messes up the following parse:
% "The Spirit, a liner carrying crew members and passengers, was attacked."
% I'm guessing the costly [O-] is needed to avoid some other screwup ..?
% {Dm-}: "I have a number of pennies and dimes"
% "I saw the (dog and pony) show"
% "I love cats and dogs" requires O- without cost.
%
% and_not ,_not: "I saw John, not Mary"
% We treat this here as an idiom, even though it's explicitly handled for
% AJ nd RJ conjunctions.  Kind-of wrong, it's just easier, for now.
%
% {Jd- & Dm-}: "A number of recommendations and suggestions were made"
%   with "number of" modifying the and.j-n
% [[<noun-conj-head>]] costs so that above is preferred: (huh????)
% "there was enough of the beer and sandwiches"
%
% XJa-: "Both June and Tom are coming"
%
% Xd- & SJl- & EBb+ & SJr+ & Xc+ & Wd- & Ssx+: EB+ forces a singular subject!
%
% and.j-n but_not but_just_not and_not ,_not just_not:
and.j-n 'n':
  (<noun-conj-dep-s> & <noun-rel-s> & {XJa-} & (
    <noun-conj-head>
    or (Spx+ & <CLAUSE>)
    or SIp-
    or ((SIp- or ({Dm-} & Op-) or Jp-) & Rw+ & B+)
    or Wa-
    or [{Ds-} & Os-]
    or <post-nominal-s>))
  or (<noun-conj-dep-p> & <noun-rel-p> & {XJa-} & (
    <noun-conj-head>
    or ({Jd- & Dm-} & Spx+ & <CLAUSE>)
    or SIp-
    or ((SIp- or ({Dm-} & Op-) or Jp-) & Rw+ & B+)
    or Wa-
    or ({{Jd-} & Dmc-} & Op-)
    or ({Dmc-} & Op- & Rw+ & B+)
    or <post-nominal-p>))
  or (<noun-conj-dep-u> & <noun-rel-x> & {XJa-} & (
    <noun-conj-head>
    or ({Jd- & Dm-} & Sux+ & <CLAUSE>)
    or SIu-
    or ((SIu- or Ou- or Jp-) & Rw+ & B+)
    or Wa-
    or [{{Jd-} & Dmu-} &  Ou-]
    or <post-nominal-u>))
  or ((Xd- & hSJl- & EB+ & hSJr+ & Xc+) & (Wd- & Ssx+))
  or (({Xd-} & hSJl- & EB+ & hSJr+ & {Xc+}) & O-);

% A zero-copula in a conjunction:
% "that is very nice, but not what I want"
but_not just_not: VJrsi- & O+;

% {XJo-}: "I can use either this or that".
% EB+: "the problem, or rather, one of the problems, ..."
%
or.j-n:
  (<noun-conj-dep-s> & <noun-rel-s> & {XJo-} &
    (<noun-conj-head>
    or (S*x+ & <CLAUSE>)
    or SIs-
    or ((SIs- or ({Ds-} & O-) or J-) & Rw+ & B+)
    or [Os-]
    or Wa-
    or <post-nominal-s>)) or
  (<noun-conj-dep-p> & <noun-rel-p> & {XJo-} &
    (<noun-conj-head>
    or (Spx+ & <CLAUSE>)
    or SIp-
    or ((SIp- or ({Ds-} & Op-) or Jp-) & Rw+ & B+)
    or [Op-]
    or Wa-
    or <post-nominal-p>))
  or ((Xd- & hSJl- & EB+ & hSJr+ & Xc+) & (Wd- & Ssx+))
  or (({Xd-} & hSJl- & EB+ & hSJr+ & {Xc+}) & O-);

% XJn- "neither this nor that"
% XJn- is optional: "I don't like dogs nor cats" but not having it is given
%     a cost only because it's a kind of a strange construction ...
% SJl- & SJr+ & SJl+: cascading nor's: "Neither snow nor rain nor heat..."
% SI- can be singular or plural: for example:
% "There is neither a dog nor a cat here"
% "Are neither John nor I invited?"
% Os- & Mp+: link the prep to the object, instead of the verb,
%       esp when the verb is the copula.
% XXX FIXME: there should be a (SJr- & Mp+) here, to allow this:
% "There is no ham in the hamburger, and neither pine nor apple in the pineapple."
% However, it needs to be configured so that it accepts the SJr- from the
% "and" only when there is no J link, as otherwise, i links at the wrong
% location. Ugh. what a big headache.
nor.j-n:
  hSJl- & (XJn- or [()]) & hSJr+
    & ((Wd- & S*x+) or SI- or (Os- & {Mp+}) or Wa- or dSJl+);

% Force use of commas: "Mary, but not Louise, is coming to the party"
% Not John, but Mary led the way.
% XJb-: "not only this but also that"
% XXX FIXME: the EBb and EBy below should probably be replaced by XJb
but.j-n yet.j-n:
  ((Xd- & hSJl- & EBb+ & hSJr+ & Xc+) & (Wd- & Ssx+)) or
  ((Xd- & hSJl- & EBy- & hSJr+) & (Wd- & Ssx+)) or
  (({Xd-} & hSJl- & (XJb- or EBy- or EBb+) & hSJr+) & Ou-);

but_also:
  {Xd-} & hSJl- & XJb- & hSJr+ & Ou-;

% SJ: "Who, if not Micheal, will provide for your care?"
% MJ: "when, if not tomorrow, do you want to do it?"
% MJr+ & MJr+: "if not next Tuesday, when do you want to do it?"
if.j-n:
  ((Xd- & hSJl- & EBb+ & hSJr+ & Xc+) & (Ws- & S**w+)) or
  ((Xd- & hSJl- & EBb+ & hSJr+ & Xc+) & (Ww- & Qw+)) or
  ((Xd- & hMJl- & EBb+ & hMJr+ & Xc+) & (Ww- & Qw+)) or
  ((EBb+ & hMJr+ & Xc+ & hMJr+) & Ww-);

% Conditional: if ... then ...
if.j-c: Wd- & <subcl-verb> & XJc+;
then.j-c: {Xd-} & XJc- & VJr+;

% --------------------------------------------------------------------------
% Conjoined verbs/verb phrases
% "Oscar Peterson played piano and wrote music."
% Pass through singular/plural agreement of subject.
% The weirdo (B- & {B+}) allows the following to parse:
% "This is a problem Moscow created and failed to solve."
% [I-]0.2, [<verb-ico>]0.2: avoid I links to conjoined non-infinitives.
% XXX This is hacky, we should just prevent such infinitive links from
% occurring at all.
% {TO+}: "I aim to do something and to help."
% {N+} & {TO+}: "I aim to do something and not to complain."
% {N+}: "John wants to pay for the beer, not steal it."
% Jj- & Jk+: "... to look at and listen to everything."
%    This is a link-crossing hack; see the link-crossing
%    discussion in the README; see section-J.html in the docs.
<verb-conjunction>:
  (({Xd-} & hVJlsi- & hVJrsi+) &
    (({@MV+} & {Xc+ & @MV+} & Ss- & <verb-wall>) or
    (RS- & Bs-) or
    ([I-]0.2 & {@MV+} & <verb-wall>) or
    ({Xd-} & dVJrsi-))) or
  (({Xd-} & hVJlpi- & {N+} & {TO+} & hVJrpi+) &
    (({@MV+} & Sp- & <verb-wall>) or
    (RS- & Bp-) or
    ([I-]0.2 & {@MV+} & <verb-wall>) or
    ({Xd-} & dVJrpi-))) or
  (({Xd-} & Jj- & hVJlpi- & hVJrpi+ & Jk+) &
    ([I-]0.2 & {@MV+} & <verb-wall>)) or
  (({Xd-} & hVJlst- & hVJrst+) &
    ((({@MV+} & Ss- & <verb-wall>) or
    ([I-]0.2 & {@MV+} & <verb-wall>)) &
      (O+ or (B- & {B+})))) or
  (({Xd-} & hVJlpt- & hVJrpt+) &
    ((({@MV+} & Sp- & <verb-wall>) or
    ([I-]0.2 & {@MV+} & <verb-wall>)) &
      (O+ or (B- & {B+})))) or
  (({Xd-} & hVJlh- & hVJrh+) & (PP- & {@MV+} & <verb-wall>)) or
  ((hVJlg- & hVJrg+) & (J-)) or
  ((hVJlp- & hVJrp+) & [<verb-ico>]0.2) or
  ((hVJls- & hVJrs+) & [<verb-ico>]0.2);

and.j-v or.j-v: <verb-conjunction>;
then.j-v: <verb-conjunction>;

% ditransitive conjunction: "I gave Bob a doll and Mary a gun"
% Actually, optionally ditransitive, to cover more cases.
% "I taught these mice to jump, and those mice to freeze"
% "I taught these mice to jump, and those to freeze"
<ditransitive-conjunction>:
  {Xd-} & VJd- & O+ & {O*n+};

and.j-o or.j-o: <ditransitive-conjunction>;
then.j-o: <ditransitive-conjunction>;

% Xj-: "on the table and just to the left of the lamp."
and.j-l or.j-l: Xj- & hWl+ & {Xj+};

% XJn-: neither ... nor ...
% I-: "I don't want that, nor do you"
nor.j-v:
  (hVJl*i- & XJn- & hVJr*i+ & ({@MV+} & S- & <verb-wall>)) or
  (hVJl*t- & XJn- & hVJr*t+ & ({@MV+} & S- & O+ & <verb-wall>)) or
  (Xd- & hVJl*i- & hVJr*i+ & ({@MV+} & S- & <verb-wall>)) or
  ({Xd-} & hVJl*i- & hVJr*i+ & ({@MV+} & I- & <verb-wall>));

% Similar to and, but allows optional comma before "but"
% "blah blah, but blah"
but.j-v:
  ((({Xd-} & hVJls-) & hVJrs+) & ((Ss- & <verb-wall>) or ({Xd-} & dVJrs-))) or
  ((({Xd-} & hVJlp-) & hVJrp+) & ((Sp- & <verb-wall>) or ({Xd-} & dVJrp-))) or
  ((hVJl- & hVJr+) & ((I- & <verb-wall>) or <verb-ico>));

% The XJb- guarantees that but.j-b is used with not_only
% "We not only X'ed but also Y'ed".
% This is the same pattern as the neither... nor... pattern above.
not_only: XJb+;
but_also:
  (hVJl*i- & XJb- & hVJr*i+ & ({@MV+} & S- & <verb-wall>));

% XJ: collocations with holes, i.e. "... blah blah X um um Y"
% where "blah blah" is always used with "um um".
% XJi: with infinitives
% ... not only X, but Y
% "you should not only ask for your money back, but demand it"
not_only: I- & I+ & XJi+;
but.j-r: {Xd-} & XJi- & I+;

% (Wa- & {OF+}): "Either of them."
% XJo+: "... either X or Y"
% [MVa] has a cost because we want XJo to get priority.
% MVa-: "He is either here or he is there."  which is fucked up...
% but hey ...  it would need a crossing-link to fix ...
%
% The costly [[<noun-main-x>]] is quite ugly and unappealing, but is
% needed to parse "he is either in the 105th nor the 106th battalion".
% The problem here is that "either in" seems to be order-reversed from
% "in either", and doing it right would require link-crossing.
either.r:
  Ds+
  or XJo+
  or E+
  or (OF+ & <noun-main-x>)
  or [[<noun-main-x>]]
  or [{Xd+ & Xc-} & <adv-as>]
  or (Wa- & {OF+});

% (Wa- & {OF+}): "Neither of them."
neither.r:
  Ds+
  or XJn+
  or E+
  or (OF+ & <noun-main-x>)
  or [[<noun-main-x>]]
  or (Wa- & {OF+});

nor.r: ((Xd- & <coord>) or Wd-) & Qd+;
for.r: [[(({Xd-} & <coord>) or Wc-) & (Wd+ or Wp+ or Wr+ or Qd+ or Ws+ or Wq+)]];
yet.r: ((({Xd-} & <coord>) or Wc-) & (Wd+ or Wp+ or Wr+)) or E+ or <adv-as> or ({Xd-} & Xc+ & dCO+);

% therefor is a common mis-spelling, unlikely the archaic therefor ...
% <fronted>: "thus it would seem"
thus therefore therefor:
  ((Xc+ & {Xd-}) & dCO+) or
  [dCO+]0.3 or
  ({Xd-} & <coord> & Wd+) or
  ({Xd- & Xc+} & (E+ or EB-)) or
  (Xd- & Xc+ & MVa-) or
  <fronted>;

% EBy+ link is for "verbed not X but Y" "I saw not Mary, but John"
%
% EB- & EA+: modify both the verb, via EB- and the adjective, via EA+
%   it is not sweet
%   *it tastes not sweet
%   it tastes bitter, not sweet   % "not" is modifying comma
%   it tastes bitter and not sweet
% EB- & EE+: "but not very much"
% {EA+}: "he is not a good programmer"
%     FIXME: it would be nice to have some + link for this case, also.
% N- & En+: "he does not want to steal it"
% O*x-: "I should imagine not!" The x blocks ditransitive-verb links.
not.e:
  [EBm-]0.1
  or (EBm- & (EA+ or EE+))
  or (EBb- & {EA+})
  or (EBx- & {EA+})
  or ({@E-} & N- & En+)
  or [{@E-} & N-]
  or NT+
  or EBy+
  or O*x-
  or <COMP-OPENER>
  or [[((Ma+ or Mg+ or Mv+ or Mp+) & dCO+)
    or (Mg- & Mgn+)
    or (Mv- & Mvn+)
    or (Mp- & Mp+)
    or (Ma- & Ma*n+)]];

% We include this, though it's not one of the strippable strings
n't n’t: N- or EB-;

% "Just" is tricky...
% COMP-OPENER: "just not a good swimmer, he fell behind"
just_not: <COMP-OPENER>;

% ---------------------------------------------------
% Question patterns.
% EAh- & Qe+: "How long did you say it was?"
% EAh-: "How long?" but this is blocked by "S-V inversion required7"
<how-q>: (EAh- & Qe+);

% ---------------------------------------------------
% ADJECTIVES
% Common disjuncts shared by virtually all adjectives.
%
% This one is used for openers and post-nominal modifiers.
<adj-opener>:
  [{@E-} & {@MV+} & <fronted> & {@MV+}]
  or ({@E-} & {@MV+} & ([[<OPENER>]] or (Xd- & Xc+ & MX*a-)));

% Conjoined adjectives
% Conjoined comparative adjectives, if EAm- is present.
<adj-conjoined>:
  ({[EA-]-0.1} & dAJra- & {@MV+})
  or ([EAm-]-0.1 & dAJrc- & {@MV+})
  or ({@MV+} & dAJla+)
  or ({@MV+} & [EAm-]-0.1 & dAJlc+);

<adj-op>: <adj-opener> or <adj-conjoined>;

% Ordinary adjectives
% abject.a abnormal.a abominable.a abortive.a abrasive.a abrupt.a
%
% [[{DD-} & <noun-and-p>]]:  "the rich and powerful":
%
% Lots and lots, but not all ordinary adjs can take <tot-verb>:
% Pa- & <tot-verb>: "... is too abrasive to talk to."
% {Xd-} & Pa-: "she stood there, naked"
% EAh-: "How big?"  "How tall?" --  blocked by "S-V inversion required7"
%     and if it was not blocked, the EAh would create bad parses for stuff
%     like "He is a decidedly jolly good player"
% EAh- & Qe+: "How big is it?"  "How tall is it?"
<ordinary-adj>:
  ({EA- or EF+} & (
    ({Xd-} & (Pa- or AF+ or Ma- or dMJra-) & {@MV+} & {<tot-verb>})
    or ({@MV+} & dMJla+)
    or AA+
    or <adj-opener>))
  or ({EF+} & <adj-conjoined>)
  or [[DD- & <noun-main-p>]]
  or [[{DD-} & <noun-and-p>]]
  or <how-q>;

% Macro, for all the common parts of an A+ connection, with
% the phonetic attachment as a variable.
define(`ADJ_PH',`'
  ({EA- or ({($1)} & {EF+ or MX*ta+})} & {[[@Ec-]]} & {Xc+} & A+))

% PH-: connect, phonetically, to a/an if it is there.
<adj-consn>: ADJ_PH(<wantPHc>);
<adj-vowel>: ADJ_PH(<wantPHv>);
<adj-phone>: ADJ_PH(<wantPH>);

/en/words/words.adj.1-vowel :
  <marker-common-entity> or
  <ordinary-adj> or <adj-vowel>;

/en/words/words.adj.1-const :
  <marker-common-entity> or
  <ordinary-adj> or <adj-consn>;

% Make the given name Frank be preferred to 'frank.a'
% e.g. "Frank felt vindicated when his long time rival Bill revealed that
% he was the winner of the competition."
frank.a:
  [<marker-common-entity> or <ordinary-adj> or <adj-consn>]0.2;

% Add a minuscule cost, so that the noun form is preferred...
% An older formulation of this used Ah- as the link, but I don't see
% why.  Generic adjective should be OK. Given a cost of 0.04, so
% as to give a slight preference for the noun-form, if possible.
<HYPHENATED-WORDS>.a:
  [<ordinary-adj> or <adj-phone>]0.04;

% Color names. Just like ordinary adjectives, except that the
% color names themselves can be modified by other nouns, gerunds,
% verbs and adjectives.
% This is done with the {(AN- or A-) & {Ds-}}
% I'm unclear about all the other adjective gobbledy-gook
%
% "She prefers fire-engine red"
% A-: "The house was painted burnt umber"
%     "The house was painted yellow lime"
% A- & Ds-: "The house was painted a fading yellow"
% AN-: "The house was painted fire-engine red"
%      "Her shoes are fire-engine red"
% [A-]0.2: "a big green apple" want "big" to modify "apple", not "green"
<color-adj>:
  ({EA- or EF+} & {(AN- or [A-]0.2) & {Ds-}} &
    (({[[@Ec-]]} & {Xc+} & A+)
    or ((Pa- or AF+ or Ma- or dMJra-) & {@MV+} & {<tot-verb>})
    or ({@MV+} & dMJla+)
    or AA+
    or [[DD- & <noun-main-p>]]
    or [[{DD-} & <noun-and-p>]]
    or <adj-opener>))
  or ({EF+} & {(AN- or [A-]0.2) & {Ds-}} & <adj-conjoined>);

% pinkish brownish not in this list as they can't take the modifiers.
auburn.a black.a blue.a blueberry.a
brown.a green.a gray.a grey.a lime.a
ochre.a orange.a pink.a purple.a
rasberry.a raspberry.a red.a
tawny.a ultramarine.a umber.a yellow.a:
  <color-adj>;


% Wx-: single-word opinion expression
<adj-stuff>:
  <marker-common-entity> or
  <how-q> or
  ({EA- or EF+} & (
    ((AF+ or Ma- or dMJra-) & {@MV+}) or
    ({@MV+} & dMJla+) or
    AA+ or
    Wx- or
    [[DD- & <noun-main-p>]] or
    [[{DD-} & <noun-and-p>]]));

% "We caught a through flight", "it's a done job" - adjective -- !?
% probably over-broad.
done.c gone.c through.c:
  <adj-stuff>
  or <adj-consn>;

responsible.a:
  ({EA- or EF+} & (
    ((Pa- or Vh-) & {@MV+}) or
    <adj-opener>))
  or ({EF+} & <adj-conjoined>)
  or <adj-stuff>
  or <adj-consn>;

accountable.a:
  ({EA- or EF+} & (
    ((Pa- or Vh-) & {@MV+}) or
    <adj-opener>))
  or ({EF+} & <adj-conjoined>)
  or <adj-stuff>
  or <adj-vowel>;

long.a:
  <ordinary-adj>
  or <adj-consn>
  or ((Ya- or Yt-) & (Pa- or Ma- or dMJra- or dMJla+))
  or (H- & Yt+);

% Hmm does distant really belong here?
% "The river is a mile wide here": Ya- & Pa- & MVp+
wide.a tall.a deep.a distant.a:
  <marker-common-entity> or
  <ordinary-adj> or <adj-consn>
  or (Ya- & (Pa- or Ma- or dMJra- or <adj-op>) & {@MV+})
  or (Ya- & {@MV+} & dMJla+);

old.a:
  <ordinary-adj> or <adj-vowel>
  or (Ytm- & (Pa- or Ma- or <adj-op> or dMJra- or dMJla+));

% ??? adj-op already has MX*a- in it, why do we need a bare MX- here ?
<aged>: NM+ & (Pa- or Max- or <adj-op> or (Xd- & MX- & Xc+) or dMJra- or dMJla+);
aged.i: <aged>;
% People aged 20-40 sometimes write "people ages 20-40..."  so make
% "ages" work like aged, but with a cost.
ages.i: [<aged>];


% The following all have a very regular pattern, with just one variable
% part, the TO TH type section.  This regularity should be exploited to
% simplify the expressions ...
easy.a expensive.a:
  ({EA- or EF+} &
    (((Paf- or AF+ or Ma- or dMJra-) & {@MV+} & {(<tot-verb> or <toi-verb>) & {LE+}})
    or ({@MV+} & {(<tot-verb> or <toi-verb>) & {LE+}} & dMJla+)
    or ({<tot-verb>} & <adj-opener>)))
  or ({EF+} & {<tot-verb>} & <adj-conjoined>)
  or <adj-stuff>
  or <adj-vowel>;

hard.a simple.a difficult.a fun.a:
  ({EA- or EF+} &
    (((Paf- or AF+ or Ma- or dMJra-) & {@MV+} & {(<tot-verb> or <toi-verb>) & {LE+}})
    or ({@MV+} & {(<tot-verb> or <toi-verb>) & {LE+}} & dMJla+)
    or ({<tot-verb>} & <adj-opener>)))
  or ({EF+} & {<tot-verb>} & <adj-conjoined>)
  or <adj-stuff>
  or <adj-consn>;

% M-: "Do you have an example ready?"
% Although we'd like to use Ma- for the above, post-processing prevents this.
ready.a:
  ({EA- or EF+} & (
    ((Pa- or AF+ or Ma- or dMJra-) & {@MV+} & {(<tot-verb> or <to-verb>) & {LE+}})
    or ({@MV+} & {(<tot-verb> or <to-verb>) & {LE+}} & dMJla+)
    or M-
    or ({<to-verb> or <tot-verb>} & <adj-opener>)))
  or ({EF+} & {<to-verb> or <tot-verb>} & <adj-conjoined>)
  or <adj-stuff>
  or <adj-consn>;

silly.a nasty.a pleasant.a dangerous.a cruel.a standard.a safe.a legal.a:
  ({EA- or EF+} &
    (((Paf- or AF+ or Ma- or dMJra-) & {@MV+} & {(<tot-verb> or <toi-verb>) & {LE+}})
    or ({@MV+} & {(<tot-verb> or <toi-verb>) & {LE+}} & dMJla+)
    or ({<tot-verb>} & <adj-opener>)))
  or ({EF+} & {<tot-verb>} & <adj-conjoined>)
  or <adj-stuff>
  or <adj-consn>;

% Identical to above, but starts with vowel
unpleasant.a illegal.a:
  ({EA- or EF+} &
    (((Paf- or AF+ or Ma- or dMJra-) & {@MV+} & {(<tot-verb> or <toi-verb>) & {LE+}})
    or ({@MV+} & {(<tot-verb> or <toi-verb>) & {LE+}} & dMJla+)
    or ({<tot-verb>} & <adj-opener>)))
  or ({EF+} & {<tot-verb>} & <adj-conjoined>)
  or <adj-stuff>
  or <adj-vowel>;

<adj-good>:
  ({EA- or EF+} &
    (((Paf- or AF+ or Ma- or dMJra-) & {@MV+} & {(<tot-verb> or THi+ or <toi-verb>) & {LE+}})
    or ({@MV+} & {(<tot-verb> or THi+ or <toi-verb>) & {LE+}} & dMJla+)
    or ({<tot-verb>} & <adj-opener>)
    or dAJr-))
  or ({EF+} & {<tot-verb>} & <adj-conjoined>)
  or <adj-stuff>;

good.a bad.a nice.a strange.a wonderful.a terrible.a possible.a fair.a
tough.a great.a:
  <adj-good> or <adj-consn>;

unusual.a useful.a impossible.a annoying.a unfair.a unuseful.a:
  <adj-good> or <adj-vowel>;

a_bitch :
  <adj-good>
  or ({EA- or EF+} & {[[@Ec-]]} & {Xc+} & A+);

% Surely this is incomplete...
one_and_only in_situ:
  A+;

% Identical to below, but starts with vowel.
important.a essential.a imperative.a:
  ({EA- or EF+} & (
    ((Paf- or AF+ or Ma- or dMJra-) & (({@MV+} & {(THi+ or <toi-verb> or TSi+) & {LE+}}) or <tot-verb>))
    or ((({@MV+} & {(THi+ or <toi-verb> or TSi+) & {LE+}}) or <tot-verb>) & dMJla+)
    or ({<tot-verb>} & <adj-opener>)))
  or ({EF+} & {<tot-verb>} & <adj-conjoined>)
  or <adj-stuff>
  or <adj-vowel>;

% Identical to above, but starts with consonant
crucial.a necessary.a vital.a:
  ({EA- or EF+} & (
    ((Paf- or AF+ or Ma- or dMJra-) & (({@MV+} & {(THi+ or <toi-verb> or TSi+) & {LE+}}) or <tot-verb>))
    or ((({@MV+} & {(THi+ or <toi-verb> or TSi+) & {LE+}}) or <tot-verb>) & dMJla+)
    or ({<tot-verb>} & <adj-opener>)))
  or ({EF+} & {<tot-verb>} & <adj-conjoined>)
  or <adj-stuff>
  or <adj-consn>;

common.a practical.a normal.a helpful.a striking.a
confusing.a frustrating.a disturbing.a logical.a just.a
natural.a deplorable.a detestable.a scary.a shocking.a
poetical.a:
  ({EA- or EF+} & (
    ((Paf- or AF+ or Ma- or dMJra-) & {@MV+} & {(THi+ or <toi-verb> or <tot-verb>) & {LE+}})
    or ({@MV+} & {(THi+ or <toi-verb>) & {LE+}} & dMJla+)
    or <adj-opener>))
  or ({EF+} & <adj-conjoined>)
  or <adj-stuff>
  or <adj-consn>;

original.a illogical.a elegant.a efficient.a awful.a unjust.a
absurd.a alarming.a acceptable.a unacceptable.a:
  ({EA- or EF+} & (
    ((Paf- or AF+ or Ma- or dMJra-) & {@MV+} & {(THi+ or <toi-verb> or <tot-verb>) & {LE+}})
    or ({@MV+} & {(THi+ or <toi-verb>) & {LE+}} & dMJla+)
    or <adj-opener>))
  or ({EF+} & <adj-conjoined>)
  or <adj-stuff>
  or <adj-vowel>;

% XXX FIXME Most of the below need to be sorted into vowel/consonant groups.
%
surprising.a interesting.a odd.a remarkable.a amazing.a exciting.a depressing.a
rare.a embarrassing.a:
  ({EA- or EF+} & (
    ((Paf- or AF+ or Ma- or dMJra-) & {@MV+} & {(THi+ or <fitcl-verb> or <toi-verb> or <tot-verb>) & {LE+}}) or
    ({@MV+} & {(THi+ or <fitcl-verb> or <toi-verb>) & {LE+}} & dMJla+) or
    <adj-opener>))
  or ({EF+} & <adj-conjoined>)
  or <adj-stuff>
  or <adj-phone>;

crazy.a sane.a insane.a stupid.a ridiculous.a wrong.a curious.a:
  ({EA- or EF+} & (
    ((Paf- or AF+ or Ma- or dMJra-) & {@MV+} & {(<to-verb> or <toi-verb> or THi+ or <tot-verb>) & {LE+}})
    or ({@MV+} & {(<to-verb> or <toi-verb> or THi+) & {LE+}} & dMJla+)
    or ({<to-verb>} & <adj-opener>)))
  or ({EF+} & {<to-verb>} & <adj-conjoined>)
  or <adj-stuff>
  or <adj-phone>;

wise.a unwise.a smart.a intelligent.a:
  ({EA- or EF+} & (
    ((Paf- or AF+ or Ma- or dMJra-) & {@MV+} & {(<to-verb> or <toi-verb> or <tot-verb>) & {LE+}}) or
    ({@MV+} & {(<to-verb> or <toi-verb>) & {LE+}} & dMJla+) or
    ({<to-verb>} & <adj-opener>)))
  or ({EF+} & {<to-verb>} & <adj-conjoined>)
  or <adj-stuff>
  or <adj-phone>;

unlikely.a:
  ({EA- or EF+} & (
    ((Paf- or AF+ or Ma- or dMJra-) & {@MV+} & {(<tof-verb> or THi+ or <fitcl-verb>) & {LE+}}) or
    ({@MV+} & {(<tof-verb> or THi+ or <fitcl-verb>) & {LE+}} & dMJla+) or
    ({<to-verb>} & <adj-opener>)))
  or ({EF+} & {<to-verb>} & <adj-conjoined>)
  or <adj-stuff>
  or <adj-vowel>;

likely.a:
  ({EA- or EF+} & (
    ((Paf- or AF+ or Ma- or dMJra-) & {@MV+} & {(<tof-verb> or THi+ or <fitcl-verb>) & {LE+}}) or
    ({@MV+} & {(<tof-verb> or THi+ or <fitcl-verb>) & {LE+}} & dMJla+) or
    ({<to-verb>} & <adj-opener>) or
    [E+]))
  or ({EF+} & {<to-verb>} & <adj-conjoined>)
  or <adj-stuff>
  or <adj-consn>;

% Just like below, but vowels
apparent.a official.a untrue.a urgent.a
evident.a impressive.a incredible.a inevitable.a admirable.a
insignificant.a undeniable.a understandable.a
improbable.a inexcusable.a outrageous.a
inconceivable.a unbelievable.a astonishing.a arguable.a:
  ({EA- or EF+} & (
    ((Paf- or AF+ or Ma- or dMJra-) & {@MV+} & {(THi+ or <fitcl-verb> or <tot-verb>) & {LE+}}) or
    ({@MV+} & {(THi+ or <fitcl-verb>) & {LE+}} & dMJla+) or
    <adj-opener>))
  or ({EF+} & <adj-conjoined>)
  or <adj-stuff>
  or <adj-vowel>;

% Just like above, but consonants
false.a strict.a significant.a funny.a notable.a tragic.a
plain.a definite.a mysterious.a pathetic.a probable.a
commendable.a conceivable.a miraculous.a self-evident.a
plausible.a demonstrable.a hilarious.a paradoxical.a shameful.a
disgraceful.a debatable.a lamentable.a regrettable.a well-known.a
a_drag a_bummer:
  ({EA- or EF+} & (
    ((Paf- or AF+ or Ma- or dMJra-) & {@MV+} & {(THi+ or <fitcl-verb> or <tot-verb>) & {LE+}}) or
    ({@MV+} & {(THi+ or <fitcl-verb>) & {LE+}} & dMJla+) or
    <adj-opener>))
  or ({EF+} & <adj-conjoined>)
  or <adj-stuff>
  or <adj-consn>;

clear.a unclear.a relevant.a irrelevant.a obvious.a immaterial.a:
  ({EA- or EF+} & (
    ((Paf- or AF+ or Ma- or dMJra-) & {@MV+} & {(THi+ or QIi+ or <tot-verb>) & {LE+}}) or
    ({@MV+} & {(THi+ or QIi+) & {LE+}} & dMJla+) or
    <adj-opener>))
  or ({EF+} & <adj-conjoined>)
  or <adj-stuff>
  or <adj-phone>;

% Implement a zero-that; viz.
%   "We have to make sure [that] everyone is here."
<phantom-that>: Cet+ & CV+;

clear.i:
  {EA- or EF+} & Vm- & (TH+ or <phantom-that>);

sure.i certain.i:
  {EA- or EF+} & Vm- & (TH+ or <phantom-that> or <to-verb> or (OF+ & {@MV+}));

% (AJrc- & {@MV+}): "It is lighter and less costly"
% Exactly the same as below, but consonant.
tactful.a conventional.a prudent.a sensible.a tactless.a polite.a
conceited.a valuable.a reasonable.a traditional.a tempting.a
lovely.a childish.a costly.a customary.a desirable.a
dumb.a fashionable.a foolish.a healthy.a hip.a
painful.a selfish.a sufficient.a boring.a
mandatory.a meaningless.a preferable.a senseless.a trivial.a
wrongheaded.a premature.a risky.a dishonest.a
hypocritical.a pointless.a
a_mistake:
  ({EA- or EF+} & (
    ((Paf- or AF+ or Ma- or dMJra-) & {@MV+} & {(<toi-verb> or <tot-verb>) & {LE+}})
    or ({@MV+} & {<toi-verb> & {LE+}} & dMJla+)
    or (dAJrc- & {@MV+})
    or <adj-opener>))
  or ({EF+} & <adj-conjoined>)
  or <adj-stuff>
  or <adj-consn>;

% Exactly the same as above, but vowel.
advisable.a impolite.a arrogant.a obnoxious.a
unreasonable.a unnecessary.a usual.a
inadvisable.a ethical.a unethical.a immoral.a
awkward.a appropriate.a effective.a energy-intensive.a
okay.a OK.a ok.a advantageous.a
inappropriate.a insufficient.a irrational.a irresponsible.a
enjoyable.a idiotic.a inconvenient.a unkind.a:
  ({EA- or EF+} & (
    ((Paf- or AF+ or Ma- or dMJra-) & {@MV+} & {(<toi-verb> or <tot-verb>) & {LE+}})
    or ({@MV+} & {<toi-verb> & {LE+}} & dMJla+)
    or (dAJrc- & {@MV+})
    or <adj-opener>))
  or ({EF+} & <adj-conjoined>)
  or <adj-stuff>
  or <adj-vowel>;

unknown.a questionable.a:
  ({EA- or EF+} & (
    ((Paf- or AF+ or Ma- or dMJra-) & {@MV+} & {QIi+}) or
    ({@MV+} & {QIi+} & dMJla+) or
    <adj-opener>))
  or ({EF+} & <adj-conjoined>)
  or <adj-stuff>
  or <adj-phone>;

certain.a:
  ({EA- or EF+} & (
    ((Paf- or AF+ or Ma- or dMJra-) & {@MV+} & {(<embed-verb> or <tof-verb> or TH+ or QI+ or (OF+ & {@MV+})) & {LE+}}) or
    ({@MV+} & {(<embed-verb> or <tof-verb> or TH+ or QI+ or (OF+ & {@MV+})) & {LE+}} & dMJla+) or
    ({<embed-verb> or <to-verb> or TH+ or QI+ or OF+} & <adj-opener>)))
  or ({EF+} & {<embed-verb> or <to-verb> or TH+ or QI+ or OF+} & <adj-conjoined>)
  or <adj-stuff>
  or <adj-consn>;

sure.a unsure.a uncertain.a careful.a:
  ({EA- or EF+} & (
    ((Pa- or AF+ or Ma- or dMJra-) & {@MV+} & {(<embed-verb> or <to-verb> or TH+ or QI+ or (OF+ & {@MV+})) & {LE+}}) or
    ({@MV+} & {(<embed-verb> or <to-verb> or TH+ or QI+ or (OF+ & {@MV+})) & {LE+}} & dMJla+) or
    ({<embed-verb> or <to-verb> or TH+ or QI+ or OF+} & <adj-opener>)))
  or ({EF+} & {<embed-verb> or <to-verb> or TH+ or QI+ or OF+} & <adj-conjoined>)
  or <adj-stuff>
  or <adj-phone>;

% XXX FIXME: many of the below should probably take <tot-verb> like the
% above...
% common adjectives, taking "to", "that" e.g. "was incorrect that"
correct.a incorrect.a right.a excited.a
disappointed.a upset.a sorry.a content.a determined.a
amused.a amazed.a astonished.a astounded.a pleased.a
disgusted.a distressed.a dismayed.a irritated.a embarrassed.a alarmed.a:
  ({EA- or EF+} & (
    ((Pa- or AF+ or Ma- or dMJra-) & {@MV+} & {(<to-verb> or TH+) & {LE+}}) or
    ({@MV+} & {(<to-verb> or TH+) & {LE+}} & dMJla+) or
    ({<to-verb> or TH+} & <adj-opener>)))
  or ({EF+} & {<to-verb> or TH+} & <adj-conjoined>)
  or <adj-stuff>
  or <adj-phone>;

glad.a fortunate.a unfortunate.a lucky.a unlucky.a happy.a sad.a
surprised.a delighted.a overjoyed.a:
  ({EA- or EF+} & (
    ((Pa- or AF+ or Ma- or dMJra-) & {@MV+} & {(<embed-verb> or <to-verb> or TH+) & {LE+}}) or
    ({@MV+} & {(<embed-verb> or <to-verb> or TH+) & {LE+}} & dMJla+) or
    ({<embed-verb> or <to-verb> or TH+} & <adj-opener>)))
  or ({EF+} & {<embed-verb> or <to-verb> or TH+} & <adj-conjoined>)
  or <adj-stuff>
  or <adj-phone>;

% common adjectives, taking "to", "of", "that" e.g. "proud that"
proud.a scared.a:
  ({EA- or EF+} & (
    ((Pa- or AF+ or Ma- or dMJra-) & {@MV+} & {(<to-verb> or TH+ or (OF+ & {@MV+})) & {LE+}}) or
    ({@MV+} & {(<to-verb> or TH+ or (OF+ & {@MV+})) & {LE+}} & dMJla+) or
    ({<to-verb> or TH+ or OF+} & <adj-opener>)))
  or ({EF+} & {<to-verb> or TH+ or OF+} & <adj-conjoined>)
  or <adj-stuff>
  or <adj-consn>;

% common adjectives, taking "of" e.g. "tired of", "sick of" etc.
tired.a pooped.a full.a sick.a critical.a guilty.a innocent.a
typical.a exemplary.a
capable.a contemptuous.a incapable.a reminiscent.a scornful.a mindful.a short.a
appreciative.a complimentary.a born.a worthy.a free.a terrified.a unworthy.a
prognostic.a dead.a:
  ({EA- or EF+} & (
    ((Pa- or AF+ or Ma- or dMJra-) & {@MV+} & {OF+ & {@MV+}}) or
    ({@MV+} & {OF+ & {@MV+}} & dMJla+) or
    ({OF+ & {@MV+}} & <adj-opener>)))
  or ({EF+} & {OF+ & {@MV+}} & <adj-conjoined>)
  or <adj-stuff>
  or <adj-phone>;

fond.a:
  ({EA- or EF+} & (
    ((Pa- or AF+ or Ma- or dMJra-) & OF+ & {@MV+}) or
    (OF+ & {@MV+} & dMJla+) or
    (OF+ & <adj-opener>)))
  or ({EF+} & OF+ & <adj-conjoined>)
  or <adj-consn>;


afraid.a ashamed.a unafraid.a unashamed.a:
  ({EA- or EF+} &
    (((Pa- or AF+ or Ma- or dMJra-) & {@MV+} & {((OF+ & {@MV+}) or <embed-verb> or TH+ or <to-verb>) & {LE+}}) or
    ({@MV+} & {((OF+ & {@MV+}) or <embed-verb> or TH+ or <to-verb>) & {LE+}} & dMJla+) or
    ({OF+ or <embed-verb> or TH+ or <to-verb>} & <adj-opener>)))
  or ({EF+} & {OF+ or <embed-verb> or TH+ or <to-verb>} & <adj-conjoined>)
  or <adj-vowel>;

apprehensive.a optimistic.a annoyed.a offended.a
insulted.a unhappy.a
secure.a pessimistic.a confused.a concerned.a depressed.a
doubtful.a grateful.a mad.a mistaken.a hopeful.a:
  ({EA- or EF+} & (
    ((Pa- or AF+ or Ma- or dMJra-) & {@MV+} & {TH+ & {LE+}}) or
    ({@MV+} & {TH+ & {LE+}} & dMJla+) or
    ({TH+} & <adj-opener>)))
  or ({EF+} & {TH+} & <adj-conjoined>)
  or <adj-stuff>
  or <adj-phone>;

aware.a unaware.a:
  ({EA- or EF+} &
    (((Pa- or AF+ or Ma- or dMJra-) & {@MV+} & {(TH+ or (OF+ & {@MV+})) & {LE+}}) or
    ({@MV+} & {(TH+ or (OF+ & {@MV+})) & {LE+}} & dMJla+) or
    ({TH+ or OF+} & <adj-opener>)))
  or ({EF+} & {TH+ or OF+} & <adj-conjoined>)
  or <adj-stuff>;

true.a conscious.a confident.a skeptical.a jealous.a
suspicious.a envious.a desirous.a
convinced.a unconvinced.a:
  ({EA- or EF+} & (
    ((Pa- or AF+ or Ma- or dMJra-) & {@MV+} & {(TH+ or (OF+ & {@MV+})) & {LE+}}) or
    ({@MV+} & {(TH+ or (OF+ & {@MV+})) & {LE+}} & dMJla+) or
    ({TH+ or OF+} & <adj-opener>)))
  or ({EF+} & {TH+ or OF+} & <adj-conjoined>)
  or <adj-stuff>
  or <adj-phone>;

eager.a reluctant.a able.a unable.a impatient.a
eligible.a brave.a anxious.a apt.a desperate
keen.a prepared.a willing.a hesitant.a:
  ({EA- or EF+} & (
    ((Pa- or AF+ or Ma- or dMJra-) & {@MV+} & {<to-verb> & {LE+}}) or
    ({@MV+} & {<to-verb> & {LE+}} & dMJla+) or
    ({<to-verb>} & <adj-opener>)))
  or ({EF+} & {<to-verb>} & <adj-conjoined>)
  or <adj-stuff>
  or <adj-phone>;

former.a: (<wantPHc> & A+) or G+ or (DG- & <noun-main-x>);
latter.a: DG- & <noun-main-x>;

overall.a onetime.a outboard.a pinstripe.a
goddam.a defacto de_facto de_jure erstwhile.a
foster.a outright.a online.a: <wantPH> & A+;

pro_forma ad_hoc bona_fide: (<wantPH> & A+) or Pa-;

a_priori a_posteriori apriori aposteriori:
  (<wantPHv> & A+) or <adv-as> or ({Xc+ & {Xd-}} & dCO+);

asleep.a awake.a alike.a alive.a ablaze.a adrift.a afire.a aflame.a
afloat.a afoot.a aghast.a aglow.a agog.a ajar.a amiss.a askew.a
astir.a awash.a awry.a de_rigeur rife.a fraught.a lacking.a:
  ((Ma- or Pa- or dMJra-) & {@MV+}) or
  ({@MV+} & dMJla+) or
  <adj-op>;

alone.a:
  ((Ma- or Pa- or dMJra-) & {@MV+}) or
  ({@MV+} & dMJla+) or
  <adj-op> or
  MVp- or
  E+;

% Like the above, but without the 'and' MJ links
% {Xd-}: "Y'all ain't gonna leave me here, all alone?"
all_alone:
  ({Xd-} & (Ma- or Pa-) & {@MV+}) or
  <adj-op> or
  MVp- or
  E+;

% Max-: "the only apartment available"
% Max passes through post-processing, whereas Ma- is disallowed...
outstanding.a available.a:
  ({EA- or EF+} & (
    ((Pa- or Max-) & {@MV+}) or
    <adj-opener>))
  or ({EF+} & <adj-conjoined>)
  or <adj-stuff>
  or <adj-vowel>;

due.i effective.i: (TY+ or JT+) & <prep-main-b>;

north.a south.a east.a west.a northeast.a
northwest.a southeast.a southwest.a:
  <marker-common-entity>
  or (<wantPH> & A+)
  or ({Yd-} & {OF+} & (Pp- or MVp- or Mp- or (Xc+ & Xd- & (MVx- or MX*x-))))
  or [[DD- & <noun-main-p>]]
  or [[{DD-} & <noun-and-p>]];

northern.a southern.a eastern.a western.a northeastern.a
northwestern.a southeastern.a southwestern.a:
  <marker-common-entity>
  or (<wantPHc> & A+)
  or G+;

% .. is this correct?
benumbed.a bespattered.a non_compos_mentis dead_on_arrival
 bedimmed.a null_and_void bedewed.a au_fait
 dead_of_other_causes right_sacrotransverse above_board
 K/O.a SGA.a TBA.a DOA.a asialo.a syntonic.a loco.a
 haywire.a:
  ((Ma- or Pa- or dMJra-) & {@MV+}) or
  ({@MV+} & dMJla+) or
   <adj-op>;

% -------------------------------------------------------------------------
%COMPARATIVES AND SUPERLATIVES

% Omm-: "I want more" -- the second m blocks O*t+ on <vc-be>
% Non-zero cost on Omm- so that EA+ is preferred.
% Cost of >1.0 on Omm- so that MVm- is preferred for "He runs more".
% naked EC-: "How much more?"
more:
  ({ECa-} & (EAm+ or EEm+ or [MVm-] or [EB*m-] or Qe+ or <advcl-verb> or dAJrc- or dAJlc+))
  or ({OF+} & (
    ({ECn-} & (Dmum+ or (Ss+ & <CLAUSE>) or Bsm+))
    or ({ECx- or ND-} & (Dmcm+ or (Sp+ & <CLAUSE>) or Bpm+))
    or ({ECn- or ECx- or ND-} & ([Omm-]1.1 or Jm- or (Xd- & MVt+ & Xc+ & MX*m-)))))
  or EC-
  or (DG- & (({MVa+} & <subcl-verb>) or B+ or Dm*w+ or EA+) & (ER- or (Wd- & Xc+ & ER+)));

more_of_a:  Ds*mc+ or (<PHc> & Ds*mx+);
more_of_an: Ds*mv+ or (<PHv> & Ds*mx+);

% XXX TODO: shouldn't less be a lot more like 'more', above?
% Cost of >1.0 on Om- so that MVm- is preferred for "He runs less".
less:
  ({ECa-} & (EAm+ or EEm+ or [MVm-] or [EB*m-] or dAJrc- or dAJlc+))
  or ({ECn-} & (Dmum+ or (Ss+ & <CLAUSE>) or Bsm+))
  or ({ECn-} & ([Om-]1.1 or Jm-))
  or (DG- & (({MVa+} & <subcl-verb>) or B+ or Dm*w+ or EA+) & (ER- or (Wd- & Xc+ & ER+)));

% ND- & Dmcm+ "I ran 10 fewer miles than Ben."
fewer:
  ({ECn- or ND-} & (Dmcm+ or Om- or Jm- or (Sp+ & <CLAUSE>) or dAJrc- or dAJlc+)) or
  (DG- & Dm*w+ & (ER- or (Wd- & Xc+ & ER+)));

farther:
  ({ECa-} & {K+} & (MVb- or Qe+ or <advcl-verb> or dAJrc- or dAJlc+)) or
  A+;

further.r:
  ({ECa-} & {K+} & (MVb- or Qe+ or <advcl-verb> or dAJrc- or dAJlc+)) or
  A+ or
  E+ or
  ({Xd-} & Xc+ & dCO+);

% links to adverbs on left..
% EEy+: "The price didn't drop nearly as fast."
% EAy+: "He is as smart"
% Hmm, probably want to give EAy a cost, to avoid its use in
% "William is described as smooth, yet thoughtful"
as.e-y: {EZ-} & ((EAy+ & {HA+}) or EEy+ or AM+);

% uses comparative links
% Cc+: "The coffee tastes the same as it did last year."
as.e-c:
  (MVz- & (((O*c+ or S**c+ or ({SFsic+} & Zc+)) & {Mp+}) or Mpc+ or <thncl-verb>))
  or (MVzo- & Ct+ & Bc+ & {U+})
  or (MVzc- & (CX+ or CQ+))
  or (MVzp- & Pa+)
  or (MVza- & Cta+ & ((AFd+ & {Pa+}) or PFc+));

% prepositional, mostly
% MVi- & TO+: "He said it in a voice so loud as to make everyone stare."
% VCz- & Sa*v+: "he left as agreed"
% VCz- & Sa*a+ & CV+: " ..., as shall be proven"
%         The punctuation is mandatory, here.
%         The CV is awkward, as it uses a null-subject.
%         XXX Try to get rid of this...
% Cz+ & CV+: "the accused, as it shall be shown, is innocent"
%      use Cz instead of <subcl-verb> because post-processing kills the
%      Cs link with a "Unbounded s domain78" error.
% {EZ-} & Cz+ & CV+: "Just as I suspected!"
%    cost on EZ-: prefer just.e over just.a
%
% AZ- & Mg+: "It described the treaty as marking a new stage"
% BIt+: "his statements, as candidate, contradict his actions"
% J+ & <conjoin-preps>: "Gallagher worked as a teacher and as a photographer."
as.e:
  ((J+ or Mp+ or TI+ or Zs+) &
    (({Xc+ & {Xd-}} & dCO+) or ({Xd- & Xc+} & MVp-) or (EZ- & MVp-)))
  or ((J+ or Mp+ or BIt+) & ([Mp-] or (Xd- & Xc+ & MX*x-)))
  or ({[EZ-]-0.1} & J+ & <conjoin-preps>)
  or (AZ- & Pa+)
  or (AZ- & Mg+)
  or (({[EZ-]-0.5} or {Xd-}) & {[hVCz-]-0.05} & Cz+ & CV+)
  % or ({Xd-} & hVCz- & Cz+ & CV+)
  or ((Sa*v+ or (Sa*a+ & CV+)) & {Xc+ & {Xd-}} & dCO+)
  or (Sa*v+ & {Xd- & {Xc+}} & VCz-)
  or [Sa*a+ & CV+ & {Xd- & {Xc+}} & VCz-]-0.05
  or (Sa*a+ & CV+ & {Xd- & {Xc+}}) % needed for MXsr constructions
  or (MVi- & TO+ & IV+)
  or [[(PFc+ or CQ+) & ({Xd- & Xc+} & MVs-)]];

% "as" as synonym for "while"
% <subcl-verb> & CO+: "As we set sail, a gale blew up"
% MVs- & Cs+ & CV+: "I slipped as I ran"
as.#while:
  [[<subcl-verb> & (({Xc+ & {Xd-}} & dCO+))]0.2
  or [MVs- & Cs+ & CV+]0.1]colloquial;


as_is: {Xd- & Xc+} & MVs-;

as_possible: MVz-;

% hVCz-: "It is just like I suspected."
% dWa-: "Like I said, she'll do it!"
like.e as.z:
  ({EZ-} or {Xd-}) & {[dWa- or hVCz-]-0.05} & Cz+ & CV+;

% Em+: "That is so not going to happen"
% EExk+: "That is so very beautiful"
% EBb- & EAxk+: "It tastes bitter and so good"
% MVp- & Xc+: "Hold the brush so"
% Pv-: "It seems so"
% MVs-: "she insisted, so we will do it"
% Cz+ & CV+: "the accused, so it shall be shown, is innocent"
so.e:
  ({EBb-} & EAxk+ & {HA+})
  or ({EZ-} & EExk+)
  or Em+
  or <fronted>
  or (Wq- & CQ+)
  or Wa-
  or (MVp- & Xc+)
  or ({Xd-} & {[hVCz-]-0.05} & Cz+ & CV+)
  or Pv-
  or O-
  or Js-;

and_so and_thus:
  <fronted>;

% Is <sent-start> ever needed here?
% Should we be using <coord> instead of MVs- ??
% Or maybe every use of MVs- should be converted to <coord> ???
so_that such_that:
  <subcl-verb> & {Xd- & Xc+} & MVs-;

% Cc+ & CV+: C links to the head-noun of the following clause, and CV+
%            links to the head verb. Must form a cycle.
%            Example: "I run more often than Ben climbs"
% MVp- & J+: "... other traditions than my own"
than.e:
  (MVt- & (((O*c+ or ({SFsic+} & Zc+) or U*c+) & {Mp+})
           or Mpc+ or S**c+ or MVat+ or MVpt+ or (Cc+ & CV+) or Pafc+))
  or ((MVta- or LE-) & Cta+ & ((AFd+ & {Pa+}) or PFc+))
  or ((MVti- or LEi-) & AFdi+ & {Pa+})
  or (((LE- & {AFd+}) or (LEi- & {AFdi+}))
    & (THc+ or (TOic+ & <inf-verb>) or (TOfc+ & <inf-verb>) or (TOtc+ & B+)))
  or (((MVto- & Ct+ & Bc+ & {U+}) or (MVtp- & (CX+ or CQ+))) & {Mp+})
  or (MVp- & J+);

% cost on MVa-: "we will arrive much sooner", want "much" to modify "sooner".
% ({OFd+} & Dmu+): "I drank much of the beer"
% cost on [[<noun-main-s>]] so that the above is preferred to an O- link
% EZ+: "Much as I expected!"
% H-: "How much?"
much:
  ({EE-} & ([[MVa-]] or (<wantPHc> & ECa+) or <advcl-verb> or Qe+))
  or ({EEx- or H-} & (
    ECn+
    or ({OFd+} & Dmu+)
    or (<noun-rel-s> & ([[<noun-main-s>]] or Bsm+))
    or [()]
  ))
  or (AM- & (Dmuy+ or MVy- or Oy- or Jy- or EB*y-))
  or EZ+;

mucho: Dm+;

slightly somewhat: EC+ or EA+ or MVa- or Em+;
far.c infinitely: EC+;
significantly substantially:
  ({EE- or EF+} & (EC+ or E+ or <adv-as> or ({Xc+ & {Xd-}} & dCO+)))
  or ({EE-} & EB-);

% ---------------------------------------------------------------
% comparative adjectives
% angrier.a balder.a balmier.a baser.a bawdier.a bigger.a blacker.a
%
% DD- & O- & Mp+: "Emma was the younger of two daughters"
%         (zero/ellipsis: "Emma was the younger [daughter] of two daughters"
% Wr- & MVt+ & Xc+ & PFb+: subject-verb inversion:
%       "cheaper than dirt, slime is better"

<comp-adj-misc>:
  ({[[@Ec-]]} & {Xc+} & Am+)
  or (DD- & O- & Mp+)
  or ({EA-} & dAJlc+)
  or (Wr- & {@MV+} & MVt+ & Xc+ & PFb+);

<comp-adj>:
  ({ECa-} & (
    ((Pam- or Mam- or AFm+ or ({EA-} & dAJrc-)) & {@MV+})
    or <comp-adj-misc>
  ))
  or (DG- & (TR+ or AF+) & {@MV+} & (ER- or (Wd- & Xc+ & ER+)));

/en/words/words.adj.2: <comp-adj>;

easier.a-c:
  ({ECa-} & (
    ((Pafm- or AFm+ or Mam- or ({EA-} & dAJrc-)) & {@MV+}
      & {(<tot-verb> or <toi-verb>) & {LE+}})
    or <comp-adj-misc>
  ))
  or (DG- & (TR+ or AF+) & {@MV+}
    & {<tot-verb> or <toi-verb>} & (ER- or (Wd- & Xc+ & ER+)));

harder.a-c:
  ({ECa-} & (
    ((Pafm- or AFm+ or Mam- or ({EA-} & dAJrc-))  & {@MV+}
      & {(<tot-verb> or <toi-verb>) & {LE+}})
    or <comp-adj-misc>
    or MVb-
  ))
  or (DG- & (TR+ or AF+) & {@MV+}
    & {<tot-verb> or <toi-verb>} & (ER- or (Wd- & Xc+ & ER+)));

higher.a-c deeper.a-c lower.a-c faster.a-c quicker.a-c slower.a-c:
  ({ECa-} & (
    ((Pam- or Mam- or AFm+ or ({EA-} & dAJrc-)) & {@MV+})
    or <comp-adj-misc>
    or MVb-
    or Qe+
    or <advcl-verb>
  ))
  or (DG- & (TR+ or AF+ or <subcl-verb>) & {@MV+} & (ER- or (Wd- & Xc+ & ER+)));

sooner.a-c:
  ({ECa- or Yt-} & (
    ((Pam- or Mam- or AFm+ or ({EA-} & dAJrc-)) & {@MV+})
    or <comp-adj-misc>
    or MVb-
    or Qe+
    or <advcl-verb>
  ))
  or (DG- & (TR+ or AF+ or <subcl-verb>) & {@MV+} & (ER- or (Wd- & Xc+ & ER+)));

longer.a-c:
  ({ECa- or Yt-} & (
    ((Pam- or Mam- or AFm+ or ({EA-} & dAJrc-)) & {@MV+})
    or <comp-adj-misc>
    or MVb-
    or Qe+
    or <advcl-verb>
    or Ot-
    or FL-
  ))
  or (DG- & (TR+ or AF+ or <subcl-verb>) & {@MV+} & (ER- or (Wd- & Xc+ & ER+)));

longer-term.a:
  ({ECa-} & (({[[@Ec-]]} & {Xc+} & Am+)
     or ((Pafm- or AFm+ or Mam-) & {@MV+} & {(<toi-verb> or THi+) & {LE+}})))
   or (DG- & (TR+ or AF+) & {@MV+} & {<toi-verb> or THi+} & (ER- or (Wd- & Xc+ & ER+)));

smarter.a-c nicer.a-c worse.a-c:
  ({ECa-} & (
    ((Pafm- or AFm+ or Mam- or ({EA-} & dAJrc-)) & {@MV+} & {(<toi-verb> or THi+) & {LE+}})
    or <comp-adj-misc>
  ))
  or (DG- & (TR+ or AF+) & {@MV+} & {<toi-verb> or THi+} & (ER- or (Wd- & Xc+ & ER+)));

better.a-c:
  ({ECa-} & (
    ((Pafm- or AFm+ or Mam- or ({EA-} & dAJrc-)) & {@MV+} & {(<toi-verb> or THi+) & {LE+}})
    or <comp-adj-misc>
    or MVb-
    or Qe+
    or <advcl-verb>
    or E+
  ))
  or (DG- & (TR+ or AF+) & {@MV+} & {<toi-verb> or THi+} & (ER- or (Wd- & Xc+ & ER+)));

different.a:
  ({ECa- or EA- or EF+} & (
    ((Pafm- or AFm+ or Mam- or ({EA-} & dAJrc-)) & {@MV+})
    or <comp-adj-misc>
    or AA+
    or [[DD- & <noun-main-p>]]
    or <adj-opener>
  ))
  or ({ECa- or EF+} & <adj-conjoined>);

than_expected than_imagined than_proposed than_suspected than_realized
than_intended than_supposed than_reported than_ever than_usual
than_normal than_suggested than_anticipated than_recommended: MVt-;

more_than no_more_than fewer_than no_fewer_than
less_than no_less_than greater_than no_greater_than
as_many_as an_estimated as_much_as
an_additional up_to down_to
upwards_of downwards_of:
  EN+;

% "up to where did it go?"
up_to down_to: EW+;

at_least: EN+ or dCO+ or [[{Xd- & Xc+} & MVa-]] or EB-;

% This is not quite right, since there may be other words in between
% "all ... but": "All was lost but for one tree."
% "Everything but one tree remained."
% nothing_but all_but: EN+ or E+;
all_but: EN+ or E+;
nothing_but: Vd- & I+;

% --------------------------------------------------------------------------
% superlative adjectives
% angriest.a baldest.a balmiest.a basest.a bawdiest.a biggest.a
<superlatives>:
  ({Xc+} & {NR-} & {[[@Ec-]]} & La-) or
  ({NR- or ND-} & DD- & <noun-rel-x> & {<ton-verb>} & <noun-main-x>) or
  dAJrs- or dAJls+;

/en/words/words.adj.3: <superlatives>;

favorite.a favourite.a:
  <superlatives>
  or ({Xc+} & {[[@Ec-]]} & [[Lf-]])
  or ([[Ds-]] & <noun-rel-x> & {<ton-verb>} & <noun-main-x>);

sole.a main.a: {Xc+} & {NR-} & {[[@Ec-]]} & L-;

% (DD- or [[()]]): allows optional but costly omission of "the"
% e.g. "Amen’s hair is (the) same as Ben’s" which is not terribly
% grammatical, but is not uncommon.
% [<noun-main-x>]0.1: prefer MVy for comparatives (see the_same, below).
%     "The coffee tastes the same as it did last year."
% D- & SJr-: "... her sisters' beauty and her own"
same.a own.a:
  ({Xc+} & {NR-} & {[[@Ec-]]} & La-)
  or ((DD- or [[()]]) & <noun-rel-x> & {<ton-verb>} & [<noun-main-x>]0.1)
  or (D- & dSJr-);

% [Oy-]0.1: see immediately above.
the_same:
  {EZ-} & (D**y+ or [Oy-]0.1 or Jy- or MVy-);

next.a:
  ({Xc+ & {Xd-}} & dCO+)
  or MVp- or DTi+ or NR+
  or ({Xc+} & {[[@Ec-]]} & L-)
  or (DD- & <noun-rel-x> & {<ton-verb>} & <noun-main-x>);

past.a previous.a: ({[[@Ec-]]} & {Xc+} & A+) or L- or (Pa- & {@MV+});

following.a remaining.a top.i: L-;

hardest.a-s easiest.a-s:
  ({Xc+} & {NR-} & {[[@Ec-]]} & La-) or
  ({NR- or ND-} & DD- & <noun-rel-x> & {<tot-verb>} & <noun-main-x>) or
  ({NR- or ND-} & DD- & (dAJre- or dAJle+)) or
  dAJrs- or dAJls+;

worst.a-s longest.a-s fastest.a-s furthest.a-s farthest.a-s slowest.a-s:
  ({Xc+} & {NR-} & {[[@Ec-]]} & La-) or
  ({NR- or ND-} & DD- & ((<noun-rel-x> & {<ton-verb>} & <noun-main-x>) or MVa-)) or
  ({NR- or ND-} & DD- & (dAJre- or dAJle+)) or
  dAJrs- or dAJls+;

% "he likes you best of all" has no determiner, just uses MVa-.
best.a-s personal_best:
  ({Xc+} & {NR-} & {[[@Ec-]]} & La-) or
  ({NR- or ND-} & DD- & ((<noun-rel-x> & {<ton-verb>} & <noun-main-x>) or (MVa- & {Mp+}))) or
  [[E+]] or
  [MVa- & {Mp+}] or
  ({NR- or ND-} & DD- & (dAJre- or dAJle+)) or
  dAJrs- or dAJls+;

% "despite best intentions"
best.a:  A+;

longest-term.a:
  ({Xc+} & {NR-} & {[[@Ec-]]} & La-) or ({NR- or ND-} & DD- & ((<noun-rel-x> & {<ton-verb>} & <noun-main-x>) or <adv-as>));

oftenest correctliest soonest disquietingliest:
 EA+;

% ===========================================================================
%ADVERBS

%ADVERBS WHOSE MAIN USE IS ADJECTIVAL
far_from: {EE-} & EA+;

a_bit a_little_bit the_least_bit:
  ({EE-} & EA+)
  or EE+
  or EC+
  or ({Xd- & Xc+} & MVa-)
  or [[<adv-of>]];

% OFd+ & Dm+: "I will wait a little while of time"
a_little_while:
  ({EE-} & EA+)
  or EE+
  or EC+
  or ({Xd- & Xc+} & MVa-)
  or [[<adv-of>]]
  or (OFd+ & Dm+);

% Basic adverbial adjectives, with phoneme markers.
% This is very simply just "({EE-} & EA+) or EE+" without the phonemes.
<adv-adj-econst>: ({EE- or <wantPHc>} & EA+) or (<wantPHc> & EE+);
<adv-adj-evowel>: ({EE- or <wantPHv>} & EA+) or (<wantPHv> & EE+);
<adv-adj-const>: <wantPHc> & (EA+ or EE+);
<adv-adj-vowel>: <wantPHv> & (EA+ or EE+);
%
%
pretty.e very_very very_very_very
jolly.e mighty.e damn.e bloody.e downright plumb
vitally chronically frightfully genuinely
humanly patently singularly supremely
decidedly demonstrably fashionably
frighteningly horrifyingly
laughably predominantly hugely:
  <adv-adj-econst>;

extremely exceptionally unbelievably incurably extraordinarily
exceedingly overly abundantly
unbearably unmistakably unspeakably
awfully indescribably intolerably
unalterably undisputedly unpardonably
unreasonably unusually infernally:
  <adv-adj-evowel>;

notoriously.e:
  <adv-adj-econst> or Em+;

% ---------------------------------------------------------
% Adverbs whose main use is adjectival, but can be used with
% adverbs such as "faster", "dumber", etc.
% "That one is marginally better". (thus EBm+)
% "It's an inherently better method" thus EC+ link
fabulously marginally moderately relatively ridiculously:
  <adv-adj-econst>
  or ({EE-} & EBm- & EC+)
  or [{EE- or <wantPHc>} & EC+]
  or [{EE-} & EBm-];

incomparably inherently unacceptably unarguably undeniably unimaginably:
  <adv-adj-evowel>
  or ({EE-} & EBm- & EC+)
  or [{EE- or <wantPHv>} & EC+]
  or [{EE-} & EBm-];

wide.e: <wantPHc> & EE+;

% {EE-} & EE+: "not very much"
% [[EE-]]: "It hurts, but not very"
% MVl- & MVl+: "we are going to arrive very nearly on time"
very.e way.e:
  <adv-adj-econst>
  or ({EE- or <wantPHc>} & EE+)
  or [[EE-]]
  or [[La-]]
  or (MVl- & MVl+);

not_very: <wantPHc> & EA+;

real.e: [[<adv-adj-const>]];
quite:
   <adv-adj-econst>
   or EZ+
   or [[Em+]];

amazingly incredibly:
  <adv-adj-evowel>
  or [EBm- & (EC+ or EA+)]-0.5
  or [EBm-]
  or ({Xd- & Xc+} & Em+)
  or ({Xc+ & {Xd-}} & dCO+)
  or (Xd- & Xc+ & MVa-);

% MVa-: "He is behaving very strangely"
strangely:
  <adv-adj-econst>
  or [EBm- & (EC+ or EA+)]-0.5
  or [EBm-]
  or ({Xd- & Xc+} & Em+)
  or ({Xc+ & {Xd-}} & dCO+)
  or ({Xd- & Xc+} & {EE-} & MVa-);

% EB-: "The problem, or rather, one of the problems, ..."
% Em+: "He had rather hoped that his wife's views..."
rather:
  <adv-adj-const>
  or Vw-
  or Em+
  or ({Xc+ & {Xd-}} & dCO+)
  or ({Xc+ & {Xd-}} & EB-);

particularly:
  <adv-adj-const>
  or Em+
  or [EB- & (EC+ or EA+)]-0.5
  or [EB-]
  or <comp-prep>
  or ({Xc+ & {Xd-}} & dCO+);

notably:
  <adv-adj-const>
  or [EB- & (EC+ or EA+)]-0.5
  or [EB-]
  or ({Xc+ & {Xd-}} & dCO+);

% Mp- & Ju+: "She was a girl nearly John's age"
% MVp- & Ju+: "She was a girl of nearly John's age"
% [EBm- & EA+]-0.1: prioritize above naked EA+
%
almost:
  <adv-adj-vowel>
  or EN+ or EZ+ or Em+
  or [EBm- & (EC+ or EA+)]-0.5
  or [EBm-]
  or <comp-prep>
  or ((Mp- or MVp-) & Ju+);

nearly:
  <adv-adj-econst>
  or EN+ or EZ+ or Em+
  or [EBm-]
  or [{EE-} & EBm- & (EC+ or EA+)]-0.5
  or <comp-prep>
  or ((Mp- or MVp-) & Ju+);

% The below is similar to "nearly" ...
just_about:
  Em+ or EN+ or EZ+
  or EA+;

entirely:
  <adv-adj-vowel>
  or ({EE- or EF+} & (({Xd- & Xc+} & MVa-) or Em+ or Qe+ or <advcl-verb>));

reasonably highly fairly totally completely terribly:
  <adv-adj-const>
  or ({EE- or EF+} & (({Xd- & Xc+} & MVa-) or Em+ or Qe+ or <advcl-verb>));

% EA+: "he is miserable, absolutely miserable."
% negative cost encourages forming a cycle, if possible.
absolutely:
  <adv-adj-vowel>
  or (EBm- & {[EA+]-0.1})
  or ({EE- or EF+} & (({Xd- & Xc+} & MVa-) or Em+ or Qe+ or <advcl-verb>));

% allowing as opener also
altogether equally:
  <adv-adj-vowel>
  or ({EE- or EF+} & (({Xd- & Xc+} & MVa-) or Em+ or Qe+ or <advcl-verb>))
  or ({Xc+ & {Xd-}} & dCO+);

really.e:
  <adv-adj-const> or Em+ or EBm-;

surprisingly:
  <adv-adj-const>
  or ({Xc+ & {Xd-}} & dCO+)
  or ({Xd- & Xc+} & E+)
  or (Xd- & Xc+ & MVa-);

especially:
  <adv-adj-vowel>
  or EB- or Em+
  or <comp-prep>
  or ({Xc+ & {Xd-}} & dCO+);

virtually: <adv-adj-const> or EN+ or EZ+ or Em+;

wholly fully critically greatly grossly duly unduly:
  (<wantPHc> & EA+)
  or ({EE- or EF+} & (({Xd- & Xc+} & MVa-) or Em+ or Qe+ or <advcl-verb>));

seemingly: (<wantPHc> & EA+) or Em+;
utterly: (<wantPHv> & EA+) or Em+;

barely just_barely scarcely hardly merely truly practically:
  Em+ or EBm- or (<wantPHc> & EA+) or Wa- or ({EBm+} & <COMP-OPENER>);

partly.e largely.e mostly.e chiefly.e simply.e purely.e solely.e:
  Em+ or (<wantPHc> & EA+) or EB- or Wa-
  or <comp-prep>;

% Em+: "It sure is great"
sure.ee: Em+;
very_much: Em+;

% Em+: "It sure the fuck is great to see you, man!"
% "It sure the hell is!"
the_fuck the_hell: [Em+] or Wa-;

more_and_more less_and_less more_or_less: Em+ or Wa-;

% Adverbs like "biochemically". These tend to answer the question "how?"
% with a noun-form (as opposed to being verb-derived) A lot of these are
% of the "-ically" form
%
/en/words/words.adv.3-const:
  (<wantPHc> & EA+)
  or ({Xd- & Xc+} & (E+ or <adv-as>))
  or EBm-
  or ({{Xd-} & Xc+} & dCO+);

% academically administratively aesthetically
/en/words/words.adv.3-vowel:
  (<wantPHv> & EA+)
  or ({Xd- & Xc+} & (E+ or <adv-as>))
  or EBm-
  or ({{Xd-} & Xc+} & dCO+);

in_part: EB- or <comp-prep>;

% ---------------------------------------------------------
%ADVERBS WHOSE ONLY (MAIN) USE IS POST_VERBAL
barefoot.e willy-nilly quarterly.e madly.e outright.e
staccato.e legato.e all_the_way all_the_time all_along
anymore.e aloud.e upwards.e downwards.e upward.e downward.e
inward.e outward.e inwards.e outwards.e anytime.e live.e
wholesale.e anew.e forever.e awhile.e aback.e afoul.e afresh.e aloft.e
amok.e amuck.e onstage.e
apiece.e askance.e astern.e asunder.e
inter_alia mutatis_mutandis par_excellence
upside-down.e ab_initio ad_infinitum ad_lib
ad_libitum ad_nauseum aground.e astray.e into_account into_effect
to_market to_bid from_scratch to_office for_office for_good
at_once to_normal to_bed to_town into_office in_advance to_trial by_lot
in_stride by_credit_card by_ear by_foot in_kind en_masse to_mind in_mind
in_pencil in_pen to_scale for_trial all_right full_time part_time by_proxy:
  {Xd- & Xc+} & <adv-as>;

% Adjectives that appear post-verbally e.g. "she wiped the table dry"
% "we will arrive exhausted"
% comparative link *must* have EE- to "more", "less"
% These are more or less adverbs ...
dry.e flat.e blind.e tired.e refreshed.e fresh.e exhausted.e rejuvenated.e:
  ({EE- or EF+ } & (<adv-as> or dAJra- or dAJla+)) or
  (EE- & (dAJrc- or dAJlc+)) or Wa-;

wild.e rampant.e shut.e tight.e
open.e closed.e loud.e hot.e cold.e free.e:
  {EE- or EF+} & (({Xd- & Xc+} & <adv-as>) or <advcl-verb>);

hard.e wrong.e: {EE- or EF+} & (({Xd- & Xc+} & <adv-as>) or Qe+ or <advcl-verb>);
early.e late.e: {EE- or EF+} & (({Xd- & Xc+} & <adv-as>) or TA+ or Qe+ or <advcl-verb>);
far.e: {EE- or EF+} & (({Xd- & Xc+} & <adv-as>) or <advcl-verb> or Qe+ or Yd+);
yet.e: ({Xd- & Xc+} & <adv-as>) or EBm-;

high.e deep.e low.e:
  {EE- or EF+} & (
    ({Xd- & Xc+} & <adv-as>) or
    ({Xc+ & {Xd-}} & dCO+) or
    <advcl-verb> or
    Qe+);

% up, down behaving as adverbs: "She looked down"
left.e right.e straight.e up.e down.e:
  ({EE- or EF+} &
    (({Xd- & Xc+} & <adv-as>)
    or ({Xc+ & {Xd-}} & dCO+)
    or Qe+
    or <advcl-verb>))
  or (Kx- & Ky+)
  or (Pp- & Pp+)
  or Wa-;

short.e: {Yd- or EE- or EF+} & {OF+} & ({Xd- & Xc+} & <adv-as>);

% ---------------------------------------------------------
%ADVERBS USABLE POST-VERBALLY OR PRE-VERBALLY
properly.e: ({Xd- & Xc+} & <adv-as>) or Em+;

% XXX??? This is a proper subset of the connectors in <ordinary-adv>
% but why?  Why narrow it like this?
finely specially literally heavily alternately severely dearly
voluntarily flatly purposely jointly universally thickly widely:
  {EE- or EF+} &
    (({Xd- & Xc+} & <adv-as>)
    or Em+
    or Qe+
    or <advcl-verb>
    or [[EA+]]);

respectively: ({Xd- & Xc+} & <adv-as>) or ({Xd- & Xc+} & E+) or ({Xd- & Xc+} & EB-);
long.e: E+ or ({EE- or EF+} & (({Xd- & Xc+} & <adv-as>) or Ot- or FL- or Yt+));
daily.e nightly.e weekly.e monthly.e yearly.e hourly.e
partially: ({Xd- & Xc+} & <adv-as>) or E+ or EB-;

% AJr-: "That is fine and well enough!"
well.e:
  ({EE- or EF+} & (
    ({Xd- & Xc+} & <adv-as>)
    or Qe+
    or <advcl-verb>
    or dAJr-
    or [E+]))
  or [{EA- or EF+} & (Pa- or AF+)]
  or Yd+;

% EE- & EB-: "..., or more exactly, ..."
exactly.e:
  E+
  or ({EE-} & {Xc+ & {Xd-}} & EB-)
  or EN+
  or EW+
  or EZ+
  or [{Xd- & Xc+} & MVa-]
  or <comp-prep>;

roughly approximately:
  EA+
  or ({EE- or EF+} & (EN+ or EW+ or EZ+ or ({Xd- & Xc+} & <adv-as>) or E+))
  or ({Xc+ & {Xd-}} & dCO+);

together: ({Xd- & Xc+} & <adv-as>) or E+ or K- or [Mp-] or ({Xc+ & {Xd-}} & dCO+);
definitely: {EE-} & (E+ or EB- or (Xd- & Xc+ & <adv-as>));
by_far: EB- or E+ or <adv-as>;
hereby thereby reputedly: E+ or ({Xd- & Xc+} & EB-) or ({Xc+ & {Xd-}} & dCO+);

% ---------------------------------------------------------
%ADVERBS USABLE POST-VERBALLY, PRE-VERBALLY, OR AS OPENERS
initially already somehow again
once_again nowadays sometimes nevertheless nonetheless at_first
at_best at_present of_late indeed:
  ({Xd- & Xc+} & <adv-as>) or E+ or ({Xc+ & {Xd-}} & dCO+) or EBm-;

twice.e:
  ({Xd- & Xc+} & <adv-as>)
  or E+
  or ({Xc+ & {Xd-}} & dCO+)
  or EBm-
  or <adverb-join>
  or ({EN-} & EZ+);

hence: (Yt- & ({Xd- & Xc+} & <adv-as>)) or E+ or ({Xc+ & {Xd-}} & dCO+) or EBm-;

otherwise formerly lately:
  ({Xd- & Xc+} & <adv-as>)
  or E+
  or ({Xc+ & {Xd-}} & dCO+)
  or ({Xc+ & {Xd-}} & EB-);

also.e:
  ({Xd- & Xc+} & (E+ or <adv-as>))
  or ({Xc+ & {Xd-}} & dCO+)
  or ({Xc+ & {Xd-}} & EB-);

gradually.e sadly.e broadly.e clearly.e
annually.e characteristically.e comparatively.e
confidentially.e currently.e fundamentally.e hypothetically.e
ironically.e justifiably.e momentarily.e mercifully.e
nominally.e ominously.e periodically.e realistically.e
simultaneously.e subsequently.e superficially.e thankfully.e
unofficially.e effectively.e traditionally.e briefly.e
eventually.e ultimately.e
mysteriously.e naturally.e oddly.e plainly.e truthfully.e
appropriately.e simply.ee:
  {EE- or EF+} & (
    ({Xd- & Xc+} & (<adv-as> or E+))
    or ({Xc+ & {Xd-}} & dCO+)
    or ({Xc+ & {Xd-}} & EB-)
    or Qe+
    or <directive-adverb>
    or <advcl-verb>
    or [[EA+]]);

% ({Xc+ & {Xd-}} & EB-)): "..., or more precisely, ..."
precisely.e specifically.e generally.e:
  {EE- or EF+} & (
    ({Xd- & Xc+} & (<adv-as> or E+))
    or ({Xc+ & {Xd-}} & dCO+)
    or ({Xc+ & {Xd-}} & EB-)
    or Qe+
    or <advcl-verb>
    or EW+);

occasionally.e often.e originally.e:
  {EE- or EF+} & (
    ({Xd- & Xc+} & (<adv-as> or E+))
    or ({Xc+ & {Xd-}} & dCO+)
    or ({Xc+ & {Xd-}} & EB-)
    or Qe+
    or <advcl-verb>);

% ---------------------------------------------------------
% ordinary manner adverbs
% abjectly ably abnormally abortively abruptly absent-mindedly absently
% COa+ prevents linking of these to relative causes (via COd- on CLAUSE)
% (EEh- & {Qe+}): "How quickly?"
% <fronted>: "Onward came the cavalry"
% ECa+: "It is vastly cheaper"
% XXX TODO the PH- should be split into PHc- and PHv- versions.
<ordinary-adv>:
  ({EE- or EF+} &
    (({Xd- & {Xc+}} & <adv-as>)
    or Em+
    or ({PH-} & ECa+)
    or ({Xc+ & {Xd-}} & dCOa+)
    or Qe+
    or <advcl-verb>
    or <adverb-join>
    or <fronted>
    or [[{PH-} & EA+]]))
  or (EEh- & {Qe+});

% XXX FIXME: there are things in there, like "tall.e" that seem not to
% belong??
% XXX FIXME: the Qe construction is weird:
%    "How slickly did he talk?"
%    "*How slickly did you say it was?"
/en/words/words.adv.1: <ordinary-adv>;
/en/words/words-medical.adv.1: <ordinary-adv>;

% EN+: "you are halfway there"
% EN- & EN+: "you are about halfway there"
halfway.e partway.e half-way.e part-way.e:
  [<ordinary-adv>]
  or ({EN-} & EN+);

% ---------------------------------------------------------
% words.adv.4 contains "prepositional" adverbs, e.g. lingually
% meidally subdermally ... Right now we treat these as ordinary
% adverbs, and add the Pp- link .. but is this link actually used
% anywhere?
/en/words/words.adv.4: <ordinary-adv> or Pp-;

differently:
{EE- or EF+} & (({MVp+} & {Xd- & Xc+} & <adv-as>) or Em+ or
({MVp+} & {Xc+ & {Xd-}} & dCO+) or Qe+ or <advcl-verb> or [[EA+]]);

independently:
{EE- or EF+} & (({(MVp+ or OF+)} & {Xd- & Xc+} & <adv-as>) or Em+ or
({(MVp+ or OF+)} & {Xc+ & {Xd-}} & dCO+) or Qe+ or <advcl-verb> or [[EA+]]);


shortly: {EE- or EF+} & (({Xd- & Xc+} & <adv-as>) or E+ or ET+ or ({Xc+ & {Xd-}}
 & dCO+) or Qe+ or <advcl-verb>);
immediately stat.e: ({Xd- & Xc+} & <adv-as>) or E+ or ET+ or ({Xc+ & {Xd-}} & dCO+) or EB-;
soon: ({EE- or EF+} & (({Xd- & Xc+} & <adv-as>) or E+ or ET+ or ({Xc+ & {Xd-}}
 & dCO+) or EB- or Qe+ or <advcl-verb>)) or ({EA- or EF+} & (Pa- or AF+));

certainly possibly probably importantly remarkably interestingly:
  {EE-} & (E+
    or (Xd- & Xc+ & (E+ or <adv-as>))
    or ({Xc+ & {Xd-}} & dCO+)
    or ({Xc+ & {Xd-}} & EB-));

% ---------------------------------------------------------
% ordinary clausal adverbs
% absurdly actually additionally admittedly allegedly alternatively
/en/words/words.adv.2:
  E+
  or (Xd- & Xc+ & E+)
  or ({Xd- & Xc+} & <adv-as>)
  or ({Xc+ & {Xd-}} & dCO+)
  or <directive-adverb>
  or EBm-;

% These are taken from words.adv.2 and allowed EB- when separated by
% commas.
% Wc- & Qd+: "however, am I right?"
however.e consequently.e moreover.e potentially.e conversely.e
finally.e actually.e thusly.e:
  E+
  or (Xd- & Xc+ & (E+ or <adv-as>))
  or ({Xc+ & {Xd-}} & dCO+)
  or (Wc- & (Xc+ or [()]) & Qd+)
  or ({Xc+ & {Xd-}} & EBm-);

% TODO: "similarly" and "differently" get very different linking requirements.
% see if these should be made the same.
similarly.e:
  ({MVp+} & {Xd- & Xc+} & (E+ or <adv-as>))
  or ({MVp+} & {Xc+ & {Xd-}} & dCO+)
  or ({Xc+ & {Xd-}} & EBm-);

not_surprisingly if_nothing_else:
  E+
  or (Xd- & Xc+ & (E+ or <adv-as>))
  or ({Xc+ & {Xd-}} & dCO+)
  or EBm-;

though.e:
  (Xd- & Xc+ & (E+ or <adv-as>))
  or ({Xc+ & {Xd-}} & dCO+);

% Nearly identical to words.adv.2, but do not force the EBm-
% Wt-: single-word sentence: "Evidently"
% Wt- & Pv+: "Evidently so"
% EB- & EA+: "... or perhaps taller"
still.e presumably undoubtedly evidently apparently
usually typically perhaps:
  E+
  or (Xd- & Xc+ & (E+ or <adv-as>))
  or (Wt- & ({Xc+} or Pv+ or N+))
  or ({Xc+ & {Xd-}} & dCO+)
  or (EB- & {[EA+]-0.1});

% scilicet
% "it [sc. gouache] was also popular in France"
% MX*tn- & O+: scilicet nouns
% MX*ta- & P+: scilicet adjectives
% MVx- & P+: to verbs "While walking [scilicet in the evening] ..."
% Some of these sc. constructions should forcefully require a comma.
in_fact of_course in_effect for_example for_instance
e.g. eg. i.e. ie. id_est
sc sc. scilicet viz.r viz..r videlicet
that_is that_is_to_say to_wit namely.r in_other_words:
  E+
  or (Xd- & Xc+ & (E+ or MVa-))
  or ({Xc+ & {Xd-}} & dCO+)
  or ({Xd-} & <coord> & (Wd+ or Wp+ or Wr+))
  or [EB- & {Xc+}]
  or [Xd- & EB- & Xc+]
  or (Xd- &
     ((MX*tn- & {Xc+} & O+) or
      (MX*ta- & {Xc+} & P+) or
      (MVx- & {Xc+} & P+))
    & Xc+);


% -----------------------------------------------------------
% ADVERBS USABLE POST_VERBALLY OR AS OPENERS
% Note that similar interjective openers will be given COp+ links
% by the UNKNOWN-WORD.a rule -- "Umm, I think he did it."
no.e nope.e nah.e no_way
yes.e yeah.e yep.e yup.e
ok.e okay.e OK.e fine.e sure.e whatever.e:
  ({Xc+ & {Xd-}} & dCO+);

% EB-: "Would they have accepted this or, instead, would they have...?"
thereafter.e overall.e lengthwise.e
instead.e anyhow.e anyway.e:
  <directive-opener>
  or <directive-adverb>
  or ({Xd- & Xc+} & (MVp- or E+ or EB-));

% Wa-: Single-word responses to questions.
someday.e sometime.e maybe.e
afterwards.e afterward.e worldwide.e nationwide.e
statewide.e world-wide.e nation-wide.e state-wide.e industrywide.e
the_world_over:
  <directive-opener>
  or <directive-adverb>
  or ({Xd- & Xc+} & (MVp- or E+))
  or (Wa- & {Wa+});

% Comparative form of maybe, similar to "perhaps"
% EB- & EA+: "She was John's age or maybe older"
maybe.c:
  EB- & EA+;

% EP+: "Maybe under the bed"
maybe.r:
  EP+;

% Argumentatives (children gain-saying).
not.intj is_too is_not is_so unh_unh: Wa-;

% Openers to directives, commands (Xc+ & EI+ connection to infinitives)
% or single-word interjections, exclamations.
% These are semantically important, so they've got to parse!
% Wa- & Wa+: "Oh my God"
no.ij nope.ij nah.ij no_way never.ij not_possible
yes.ij yeah.ij yep.ij yup.ij
ok.ij okay.ij OK.ij fine.ij exactly.ij sure.ij
good.ij good_enough fair_enough whatever.ij
hah.ij hey.ij well.ij wtf.ij hell_yes hell_no of_course
really.ij say.ij seriously.ij
just_asking wondering.ij
gesundheit
oh_no oh_my oh_my_days
ohmigod OMG oh_em_gee omigosh
oh_dear dear_me dearie_me deary_me dear.ij
dear_Lord dear_Lordy Lord_be_praised
Lordy my_Lord
fuck_me fucking_hell
uh_huh uh uhh uuh unh
my.ij my_oh_my my_my my_my_my
tsk tsk_tsk tsk_tsk_tsk:
  <directive-opener>
  or <directive-adverb>
  or OH+
  or [Wa- & {Wa+}]0.1;

% Like above, but also used as plain-old interjections.
% XXX FIXME, this used to be done by treating as adjectives; but no more!?
% A- & Wa-: "Holy Mother of God"
howdy phew psst pssst ahem
ah ahh a_ha aha eh ehh hmm hmmm huh
oops eep egad egads ermahgerd ouch alas whoa whoah
oh.ij ohh doh dohh woo_hoo
boo.ij booo boooo phooey pooh
wow.ij wowie wowee wowzers whoopee hurray hurrah rah rahh hooray
ooo oo hoo hoowee hoo-wee who-wee hubba whammo wammo
yikes yowza yowzah zoiks zoinks zounds zowie
goody.ij
amen.ij alrighty
jeepers Jee-sus
Kee-reist Christ Christ_almighty JHC
Jesu
Jesus_Christ Jesus_fucking_Christ Jesus_H_Christ
Christ_alive crikey cripes Jiminy_Cricket
Jaysus Jeebus Jehoshaphat sweet_Jesus
jeez jeez_Louise jimminy geez geez_Louise geeminy
God_Almighty God_almighty God_in_heaven
drat.ij shoot.ij blast.ij doggone doggone_it rats.ij
dammit god_dammit goddammit damn_it damn_it_all
dang dagnabit dagnabbit dag-nabbit dag_nabbit dagnabbit_all darn.ij
shucks.ij golly golly_gee golly_gee_willikers good_golly
golly_gosh gosh gee gee_whiz gee_willikers my_gosh blimey
gadzooks ye_gods
gracious_sakes good_gracious goodness_gracious goodness_gracious_me
goodness_me great_googly_moogly great_Ceasar great_Scott
good_heavens good_gravy land_sakes sakes_alive snakes_alive
heavens_to_Betsy heavens_above
by_God by_golly by_Jove by_jingo
dear_God
sacre_bleu
ay caramba carumba ay_caramba ay_carumba
kamoley kamoly moley moly
holy_Moses mother_of_God Mother_of_God
mama_mia mamma_mia
sonuvabitch son_of_a_bitch
heck sodding_hell
aw aww awww oh_great oh_wow
er err.ij errr um.ij umm
anyways honey.ij man.ij baby.ij hush.ij:
  <directive-opener>
  or <directive-adverb>
  or OH+
  or ({{Xd-} & Xc+} & EI+)
  or (({A-} or {E-} or {EE-}) & Wa-)
  or [<ordinary-adj> or <adj-phone>];

% A single plain hello all by itself.  Costly, because these days,
% it's not normally a sentence opener.
% Vv-:  "I said, hello!"
% Ds- & Jv-: "He greeted me with a loud hello"
% Perhaps these should also appear as nouns? hello.n does ...
hello.ij hallo halloo hollo hullo hillo hi
hello_there hey_there hi_there
ahoy ahoy_there ship_ahoy land_ahoy shh shhh:
  [<directive-opener> or <directive-adverb>]
  or Vv-
  or ({A-} & Ds- & Jv-)
  or OH+
  or (Wa- & {Wa+});

% Single plain word by itself.  "OK, Bye!"
bye.ij goodbye.ij:
  OH+ or (Wa- & {Wa+});

% Openers to directives, commands
% EI+: connection to infinitive imperatives: "on arrival, do it!"
% E+: split infinitives, e.g. "you should instead go home"
%     "It will, more often than not, go by train."
prima_facie before_long
by_hand by_car by_plane by_boat by_bus by_train by_phone
by_telephone in_person at_long_last on_cue
on_arrival by_request in_total in_turn
later_on for_now more_often_than_not
on_second_thought
again_and_again time_and_again over_and_over
day_by_day day_after_day step_by_step one_by_one
even_so all_of_a_sudden on_occasion upon_occasion:
  <directive-opener>
  or <directive-adverb>
  or ({Xd- & Xc+} & (<adv-as> or E+));

for_sure for_certain for_real:
  <directive-opener>
  or <directive-adverb>
  or ({Xd- & {MV+} & Xc+} & <adv-as>);

% sort-of-like given names ...
% Xd- & SIsi-: "thank ou, sir!"; the comma is mandatory.
stop.misc-inf sir.misc-inf madam.misc-inf ma'am:
  <directive-opener> or <directive-adverb> or Wa- or OH+ or (Xd- & SIsi-);

% Exclamations, vocatives
oh.voc O: OH+;

% Fillers, planners, hesitation markers
% "I told him that um he's wrong"
uh.h uhh.h er.h err.h errr.h um.h umm.h ummm.h erm.h
oh.h ooh.h oooh.h o.h oo.h ooo.h ah.h ahh.h aah.h
....h yeah.h sure.h like.h you_know I_mean okay.h so.h
actually.h basically.h right.h uh_huh well.h jeez.h:
  {Xd-} & dHM- & {Xc+} & {hHM+};

% -----------------------------------------------------------
%ADVERBS USABLE ONLY PRE-VERBALLY (OR PRE-/OPENER)
newly: E+;
rightly: {EE-} & E+;
necessarily no_longer: E+ or EBm-;
ever: E+ or EBm- or EC+ or <adv-as> or <COMP-OPENER>;

never.e always: ({EN-} & (E+ or EB-)) or <COMP-OPENER>;
seldom rarely.e: ({EE-} & (E+ or EB-)) or <COMP-OPENER>;

% MVa-: "He did just what you asked."
% EC+: "I ate just some of the cookies"
% MVl- & MVl+: " we are going to arrive just about on time"
just.e:
  E+
  or (EB- & EA+)
  or [EB-]
  or EC+
  or EN+
  or EP+
  or EW+
  or EZ+
  or (MVl- & MVl+)
  or <comp-prep>
  or <adv-as>;

meantime.e secondly thirdly
in_brief in_short in_sum in_essence:
  ({Xd- & Xc+} & E+) or
  ({Xc+ & {Xd-}} & dCO+) or
  <directive-adverb>;
furthermore:
  ({Xd- & Xc+} & E+) or
  ({Xc+ & {Xd-}} & dCO+) or
  <directive-adverb> or
  EB-;
and_furthermore: <directive-adverb>;
mainly primarily:
  E+
  or ({Xc+ & {Xd-}} & dCO+)
  or <directive-adverb>
  or EB-
  or <comp-prep>;

% The MV- links seem to really confuse the issue... kill them.
% Except they are really needed:
% MVl- & MVp+: "I am here only for a few days"
% MVa-: "This can saw only wood"
only:
  La-
  or E+
  or EN+
  or EB-
  or (MVl- & MVp+)
% or <comp-prep>
  or <adv-as>
  or (Rnx+ & <CLAUSE-E>)
  or (MVp+ & Wq- & Q+);

never.i at_no_time not_once rarely.i since_when:
  {MVp+} & Wq- & Q+;

not_since:
  (J+ or <subcl-verb>) & Wq- & Q+;

% cost on [EC+]: try to use <comp-prep> instead.
even.e:
  E+
  or EB-
  or <comp-prep>
  or [EC+]0.5
  or ((MVp+ or MVa+ or MVs+) & ({Xc+ & {Xd-}} & dCO+))
  or (Rnx+ & <CLAUSE-E>);

not_even: (Rnx+ & <CLAUSE-E>) or <COMP-OPENER>;

% {EE-} & EE+: "but not too much"
too:
  {ECa-} & (
    EA+
    or ({EE-} & EE+)
    or ({Xd- & Xc+} & MVa-)
    or (Xd- & Xc+ & E+));

% original
% sufficiently: {EE-} & (EAxk+ or EExk+ or MVak-);
% modified
sufficiently: {EE-} & (EAxk+ or EExk+ or ({Xd- & Xc+} & <adv-as>) or E+);

% much like an ordinary adverb, except even more commas allowed
% please.e: <ordinary-adv>;
please.e:
  {EE- or EF+} & (({Xd- & Xc+} & <adv-as>)
  or ({Xc+ & {Xd-}} & ([Em+]0.1 or dCO+))
  or Qe+
  or <advcl-verb> or [[EA+]]);

% polite command verb
please.w thank_you: <verb-co> & {{Xc+} & Vv+} & <verb-wall>;


% ==========================================================
% MISCELLANEOUS WORDS AND PUNCTUATION

etc etc.: {Xi-} & Xd- & Xc+ & (MX- or MVa-);
so_on the_like vice_versa v.v.:
  (<noun-rel-x> & <noun-main-x>) or
  <noun-and-x> or
  ((<verb-i> or <verb-sp,pp> or <verb-pg,ge> or <verb-pv>) & {@MV+}) or
  M- or MV-;

% Emoticons ... at start or end of sentences ...
<EMOTICON> :
  dCO+
  or (Wd- & NIa+)
  or Wa-
  or ((Xp- or ({@Xca-} & [[Xc-]])) & RW+)
  or Xi-;

% The WV+ after the Xx+ allows the root verb after the punct to attach
% to the wall.  e.g. "A woman lives next door, who is a nurse."
% The naked WV+ without any W+ allows "that I did not know."
% Xp+ is for new sentences. "Who is Obama? Where was he born?"
% Xs+ is for dependent clauses starting with "so".
%       "I stayed so I could see you."
% Xx+ has mild cost so that other uses of commas can take precedence.
% XXX TODO: after all WV's work, the WV link should no longer be optional...
% XXX that is, change <WALL> to just WV+.
%
<sent-start>:
  (<wo-wall> or <wi-wall>) & {hCPx+ or hCPi+ or hCPu+} & {([Xx+]0.06 or Xp+ or Xs+) & {hWV+}} & {RW+ or Xp+};

% <sent-start>: "So, don't do it!"
%    The cost on sent-start is to force preference for CV over WV,
%    whenever possible.
so.ij:
  ((({Xd-} & ([MVs-]0.5 or <coord>) & Xs-) or ({Xc+} & Wc-))
     & (<subcl-verb> or [<sent-start>]0.5))
  or <directive-adverb>;

% QU+ links to quoted phrases.
% ZZZ+ is a "temporary" addition for randomly-quoted crap, and
% virtual CAPs morphemes. (??)
% (Xc+ or [()]): allow missing comma, but at a price.
% hWl+ is here, not elsewhere, to avoid use together with Xx+
LEFT-WALL:
  <sent-start>
  or (QUd+ & <sent-start> & (Xc+ or [()]) & QUc+)
  or (hWl+ & {Xj+} & (RW+ or Xp+))
  or (QUd+ & hWl+ & {Xj+} & (Xc+ or [()]) & QUc+)
  or hCPa+
  or [[ZZZ+ & <sent-start>]];

% Cost on Xc- because Xc is intended for commas, not sentence-ends.
% Without this cost, the right wall gets used incorrectly with MX links.
RIGHT-WALL: RW- or ({@Xca-} & [[Xc-]]);

% mid-text period, question mark. Splits into two sentences.
<sent-split>: Xp- & <sent-start>;

% Quotation marks.
% TODO: Add ' as quotation mark.
% For a list see:
% http://en.wikipedia.org/wiki/Quotation_mark_glyphs#Quotation_marks_in_Unicode

% After a closing quote, the sentence may continue...
% You were asking, "How might this work?" so I answered.
% should these be a <sent-start> ??? why not?
<post-quote>:
  QUc- & {<wo-wall> or <wi-wall> or CP+};

« 《 【 『 „ “:
  QUd-;
» 》 】 』 ”:
  <post-quote>;

% For now, using ".x and ".y in the above definitions multiplies the number
% of linkages by 2^(number of "). So it is separated below.

% [[ZZZ-]]: link to "random" quotation marks that show up "for no reason".
% Cannot use a blanket W+ here to pick up all W connectors, because ... ??
""": QUd- or <post-quote> or [[ZZZ-]];

% Using backtic.x and backtic.y in the above definitions multiplies the
% number of linkages by 2^(number of backtics). So it is treated as a
% single item, below.
changequote(\,/)dnl
`: QUd- or <post-quote>;
changequote dnl

% Cost on Xc- because Xc is intended for commas, not periods.
% Without this cost, lists, such as "Paris, London and Berlin."
% get linked incorrectly with the MX link.
".":
  ((Xp- or ({@Xca-} & [[Xc-]])) & RW+)
  or (Xca- & Xc-)
  or Xi-
  or <sent-split>;

% Optional RW: "Is this a test?" she asked.
"!" "?" ‽ ؟ ？！:
   (Xp- & RW+)
   or ({@Xca-} & Xc- & {[RW+]})
   or ({@Xca-} & Xq+)
   or <sent-split>;

% Almost any W+ link appears here, except Wa, which clearly is wrong after
% a semicolon. -- looks a lot like <sent-start>.  We do need Wa for comma's
<semicol>:
  {@Xca-} & Xx- & (<wo-wall> or <wi-wall>) & {Xx+};

";" ；: <semicol> or ({EBx+} & Xd+);

% comma, as a conjunction
% AJ: "They taste bitter, not sweet"
% Give MVa a cost, so that Pa is used preferentially, if possible.
<comma-adj-conjunction>:
  ((hAJla- & (EBx+ or EBm+) & hAJra+) & (Pa- or [[MVa-]])) or
  (hAJla- & hAJra+ & dAJla+);

<comma-adv-conjunction>:
  (hRJlv- & hRJrv+ & dRJlv+);

% sometimes comma is used as if it were a semicolon
% Allow post-comma adverbial modifiers, but discourage these
% because the modifier may belong to a following phrase.
% e.g. "The blah, soon to be blah, will be blah." should not
% get an EBx link to "soon".
% XXX the correct solution to this is to add a new domain rule ! XXX

",":
  ({[@Xca-]-0.05 or [[[@Xc-]]]} & (({[EBx+]} & Xd+) or Xc-))
  or [<semicol>];

% ,.j
% Comma as a form of a conjunction (thus the .j subscript).
% This is given a very mild cost, so that other uses of comma can take
% precedence, if possible. XXX FIXME: Disallow use of comma as a
% conjunction unless there's and/or occuring later. For now, hack
% around this by upping the cost.
%
% Comma can conjoin nouns only if used in a list of 3 or more items:
% "This, that and the other thing"
% However, this is given a cost, so that geographic names are preferred:
% "He went to Gaeta, Italy, and to Paris, France."
%
% SJ: "I saw John, not Mary" is handled via idiomatic ,_not construction
% cost on [<verb-conjunction>]: allow Pa links with commas, e.g.
% "he paced, worried" but lower cost than Xx links
%
% SJl- & J- & SJr+: "by the A, the B..."
%   This is a tiny part of and.j-n, but we do not want the full
%   complexity of and.j-n here, since the commas force this.
%
%  SJl- & EB+ & SJr+ & Ou-: "Make love, not war"

",.j":
  [<comma-adj-conjunction>
  or <comma-adv-conjunction>
  or [<verb-conjunction>]0.5
  or (hSJl- & EBb+ & hSJr+ & Ou-)
  or [hSJl- & J- & hSJr+]0.5
  or (hSJl- & hSJr+ & dSJl+)
  or (Xj- & hWl+ & {Xj+})]0.05;

% zero-verbs. The dict uses the comma to be the zero-verb.
% For example: "I stood there, [resting] my hand on the knob"
% The verb "resting" is implicit, but missing. The simplest solution
% seems to be to use the comma as if it were a verb.  High cost, because
% commas are very common, while this usage is rare. Also: MVp+ instead
% of @MV+ because we don't want chaining of MVg to other commas.
% This is squonky, kind of, but I can't think of a better fix.
",.v":
  [[MVg- & O+ & {MVp+} & Xc+]];

% :.j
% <sent-start> is needed for a wall-connection to the subsequent verb.
% Should we also use a VC link here, similar to "so" ??
<colon>:
  {@Xca-} &
    ((Xx- & (<sent-start> or J+ or Qd+ or TH+ or <ton-verb>) & {Xx+}) or Xe-);

% Put a cost on this, because  we want to find other uses first ...
":.j":
  [<colon>]
  or (Wd- & W+)
  or (NI- & WV- & W+);

% :.p is a synonym for "that"
% Ce- & Ss*b+: "He said: 1 + 1 = 2"
% TH- & <that-verb>: "He said: the sky is blue"
":.p":
  (Ce- & Ss*b+)
  or (TH- & <that-verb>);

% Coordinating conjunctions that behave like punctuation.  These
% connect whole clauses.  Should we use <sent-start> here? Why or why not?
%
% then.ij is often used as a time-ordered conjunction: "I eat then I sleep"
% not.ij seems to result in bad parses quite often, do we need it?
% Xx-: provides coordination to the wall.
%      The cost on [<coord>] is to use the Xx when possible, because
%      the VC link often does not go leftwards far enough.
%      (e.g. "John screamed when I arrived but Sue left")
% Wc-: "But my efforts to win his heart have failed"
% EB+: "would they have accepted this or, instead, would they have ..."
but.ij and.ij or.ij not.ij also.ij then.ij but_not and_not and_yet:
  <directive-opener> or
  [{Xd-} & (Xx- or Wc-) & {Xc+} & {EB+}
    & (Wdc+ or Qd+ or Ws+ or Wq+ or Ww+) & <WALL>]1.1;

% (NI- & WV- & W+): Optionally numbered, bulleted lists
..y *.j "•" ⁂ ❧ ☞ ◊ ※  "….j" ○  。 ゜ ✿ ☆ ＊ ◕ ● ∇ □ ◇ ＠ ◎:
  (Wd- & W+)
  or (NI- & WV- & W+);

% 、 is the "enumeration-comma" used like a dash ...
% ‧ is the "middle dot"
% The four dashes are e28092 e28093 e28094 e28095
‒ – — ― ━ ー --.r -.r 、 ～.r ~.r ‧.r :
  [[<colon>]]
  or ({@Xca-} & (({EBx+} & Xd+) or Xc-))
  or (Wd- & W+)
  or (NI- & WV- & W+);

% ellipsis ... at the end, trailing off ...
% D- & O+: "He is such a ..."
....y ….y:
  (hCO- & Wd-)
  or ({D-} & O-)
  or Xx-;

% ellipsis ... at the start
% We- is used only for connecting ellipsis to the left wall.
% We- & J+: "... a ballroom polished like a skull."
% S+ has a cost so that the infinitive I+ is preferred.
% BI+, QI+: "... how those two should work together"
% TH+: "... that it rained."
....x ….x:
  We- & (J+ or [S+] or I+ or M+ or MV+ or BI+ or TH+ or QI+ or (R+ & B+ & {S+}));

% Ellipsis as verb: "Lud, son of Shem, ..."
% Qp-: "In what way...?
% O+: "I did not ... what you said"
....v ….v:
  (S- or I- or Qp-) & {O+} & <verb-wall>;

% ellipsis in the middle
% Cr- & S+: "... chances that ... could be ..."
....xy ….xy:
  Cr- & S+;

% Relative clause: "I need to find someone who ..."
....wh ….wh:
  RS- & B-;

% SJ: Conjoined: "either ... or  ..."
....n ….n:
  ({D+} & dSJl+) or
  ({D+} & dSJr-);

% The percent sign following a number (also basis pt, per mil)
% Also -- see above, for handling of 12ft. 12in. not just 12%
% AN- & Jp-: "... the concentration in v/v %"
"%" ‰ ‱ :
  (ND- & {DD-} & <noun-rel-x> & <noun-main-x>)
  or (ND- & (OD- or AN+))
  or ({E- or EA-} & A+)
  or (AN- & Jp-);

% See also /en/words/currency for currency names that follow a number.
$ USD.c US$.c C$.c AUD.c AUD$.c HK.c HK$.c
£ ₤ € ¤ ₳ ฿ ¢ ₵ ₡ ₢ ₠ ₫ ৳ ƒ ₣ ₲ ₴ ₭ ₺  ℳ  ₥ ₦ ₧ ₱ ₰ ₹ ₨ ₪ ₸ ₮ ₩ ¥ ៛ 호점
† †† ‡ § ¶ © ® ℗ № "#":
  NM*x+ & (AN+ or NM*y- or [[G+]] or (NIfu+ or NItu-) or
    ({EN- or NIc- or [[A- & NSa-]]} & {@MX+} &
      (OD- or ({DD-} & {[[@M+]]} &
        (<noun-main-p> or <noun-and-p> or [[(Ss+ & <CLAUSE>) or SIs-]])))));

% service mark, trademark.
% ℠ ™ :

% Espagnol stuff
% ¿ ¡:

"&": G- & {Xd- & G-} & G+;

% Possessive
"’":
  YP- & (({AL-} & {@L+} & (D+ or DD+)) or [[<noun-main-x>]] or DP+);

% Possessive or quote
"'":
  (YP- & (({AL-} & {@L+} & (D+ or DD+)) or [[<noun-main-x>]] or DP+))
  or QUc- or QUd+;

% Possessives
"'s.p" "’s.p":
  YS- & (({AL-} & {@L+} & (D+ or DD+)) or [[<noun-main-x>]] or DP+);

% Wd-: allows "(1 + 1) = 2"
"(" "[": {Wd-} & {EBx+} & dXdp+;

")" "]": {@Xca-} & dXcp-;

% foo: F+;

% -------------------------------------------------------------------------
% Common typographical (spelling) errors
%
% Assign a cost of 1.65 for no very good reason. Feel free to change this.
% .. well, some of this is dangerous. For example, setting the cost too
% low causes correct "than" usages with Z link (and missing wall) to be
% priced higher then "typos" with "then".
%
% These perform a kind-of poor-man's rewriting. A more principled,
% elevated design would define and use an "operator grammar" type
% approach. See https://en.wikipedia.org/wiki/Operator_grammar

teh.#the: [[the]0.05]bad-spelling;
u.#you: [[you]0.05]bad-spelling;

then.#than: [[than.e]0.65]bad-spelling;
than.#then-i: [[then.i]1.65]bad-spelling;
than.#then-ij: [[then.ij]0.65]bad-spelling;

% This breaks too many sentences in the basic corpus
% than.#then-r: [[then.r]1.65]bad-spelling;

rather_then.#rather_than: [rather_than]bad-spelling;

there.#their: [[their.p]0.65]bad-spelling;
% theres.#theirs: [[theirs.p]0.65]bad-spelling;

% The apostrophes prevent the dictionary from being read in.
% XXX FIXME, these are common typos.
%
% there.#they're: [[they're]0.65]bad-spelling;
% all.#all_of: [[all_of]0.65]bad-spelling;
% its.#it's: [[it's]0.65]bad-spelling;
% lets.#let's: [[let's]0.65]bad-spelling;
% your.#you're: [[you're]0.65]bad-spelling;
% your.#you're: [[you_are]0.65]bad-spelling;

% Using "or" instead of "nor" is a common mistake.
% "Neither this or that one will do"
or.#nor-j-n: [[nor.j-n]]bad-spelling;

% Hmm. "there lie the books" smells like present-tense: the books are
% there right now. "there lay the books" suggest past-continuous: they
% are not just there now, but always have been.  So is it really a typo
% to say "lie" instead of "lay"?
lie.#lay-v-d: [[lay.v-d]0.5]bad-spelling;

% i before e unless the weighty neighbor is a German loan word.
beleive.#believe-v: [[believe.v]0.1]bad-spelling;

% Common shorts
thru.#through-r: [[through.r]0.05]colloquial;
nite.#night: [[night.r or night.u or night.i or night.n]0.05]colloquial;
tonite.#tonight: [[tonight]0.05]colloquial;

% Emulated American cowboy accents: "You better git to goin'."
% High cost to avoid confusion with British insults: "you stupid git".
git.#get: [[get.v]0.45]colloquial;

% Y'gotta, Y'gonna
%   "keep y'mouth shut"
y'.#you y’.#you: [[you]0.05]colloquial;
y'.#your y’.#your: [[your]0.05]colloquial;

yor.#your: [[your]0.05]colloquial;

e.#he: [[he]0.05]colloquial;

be.#by: [[by]0.85]colloquial;
de.#the: [[the]0.25]colloquial;

% Slurred speech.
% XXX FIXME, this should probably be handled by a regex...
carousin.#carousing-v carousin'.#carousing-v carousin’.#carousing-v: [[carousing.v]0.05]colloquial;
drinkin.#drinking-v drinkin'.#drinking-v drinkin’.#drinking-v: [[drinking.v]0.05]colloquial;
fishin.#fishing-v fishin'.#fishing-v fishin’.#fishing-v: [[fishing.v]0.05]colloquial;
givin.#giving-v givin'.#giving-v givin’.#giving-v: [[giving.v]0.05]colloquial;
goin.#going-v goin'.#going-v goin’.#going-v: [[going.v]0.05]colloquial;
kidnappin.#kidnapping-v kidnappin'.#kidnapping-v kidnappin’.#kidnapping-v: [[kidnapping.v]0.05]colloquial;
runnin.#running-v runnin'.#running-v runnin’.#running-v: [[running.v]0.05]colloquial;
shoppin.#shopping-v shoppin'.#shopping-v shoppin’.#shopping-v: [[shopping.v]0.05]colloquial;

% Like above, but gerund
hangin.#hanging-g hangin'.#hanging-g hangin’.#hanging-g: [[hanging.g]0.05]colloquial;
singin.#singing-g singin'.#singing-g singin’.#singing-g: [[singing.g]0.05]colloquial;

S'pose.#suppose s'pose.#suppose S’pose.#suppose s’pose.#suppose: [suppose.v]colloquial;

an'.#and-j-n an’.#and-j-n: [[and.j-n]0.05]colloquial;
an'.#and-j-v an’.#and-j-v: [[and.j-v]0.05]colloquial;

% gimme.#give-me: [[give_me]0.05]colloquial;
gimme.#give-me: <verb-co> & O+ & {@MV+};

'em.#them ’em.#them 'm.#them ’m.#them : [them]colloquial;

% A bit too loose. These should only become enabled if the context
% is correct.
% an.#and-j-n: [[and.j-n]]colloquial;
% an.#and-j-v: [[and.j-v]]colloquial;

dat.#that-jp: [[that.j-p]0.05]colloquial;
dat.#that-jd: [[that.j-d]0.05]colloquial;

% Bad German accent
vas.#was-v-d: [[was.v-d]0.05]bad-spelling;
vas.#what: [[what]0.05]colloquial;
das.#this-p: [[this.p]0.05]colloquial;
das.#this-d: [[this.d]0.05]colloquial;

% Colloquial usages of "as":
% "I don't know as I can answer your question."
as.#that: [[that.j-c]0.05]colloquial;

% Desirable multi-word substitutions:
% as.#same-as: [[the_same_as]0.05]colloquial;
% because of this: "The coffee tastes [the same] as it did last week"

% Multi-word punctuation error: writing "its" when "it's" was meant.
% its.#it-is: [[it_is]0.2]colloquial;

% Initial unstressed syllable.
% Unfortunately, there's a lot of these:
% (a)'ccount (a)'greed (a)'noint (a)'pothecary (a)'rray (a)'rrest (at)'tend
% (be)'gin (be)'havior (be)'long (con)'cern (e)'scape (e)'stablish.
%
% Need to include capitalized versions because automatic
% downcasing does not work
'Cause.#because ’Cause.#because 'cause.#because ’cause.#because cause.#because: [because]colloquial;
'Fore.#before ’Fore.#before 'fore.#before ’fore.#before fore.#before: [before]colloquial;
'Fraid.#afraid ’Fraid.#afraid 'fraid.#afraid ’fraid.#afraid fraid.#afraid: [afraid.a]colloquial;
'Gainst.#against ’Gainst.#against 'gainst.#against ’gainst.#against gainst.#against: [against]colloquial;
'Midst.#amidst ’Midst.#amidst 'midst.#amidst ’midst.#amidst midst.#amidst: [amidst]colloquial;
'Mongst.#amongst ’Mongst.#amongst 'mongst.#amongst ’mongst.#amongst mongst.#amongst: [amongst]colloquial;
'Nother.#another ’Nother.#another 'nother.#another ’nother.#another nother.#another: [another]colloquial;
'Twixt.#betwixt ’Twixt.#betwixt 'twixt.#betwixt ’twixt.#betwixt twixt.#betwixt: [betwixt]colloquial;

% Poetic contractions; Shakespearean contractions
% The 's abbreviations are given a heavy cost to avoid conflict with possessives
% The 't abbreviations are given a heavy cost to evade bad contractions like "don't".
'r.#our ’r.#our: [[our]0.5]colloquial;
% 's.#his ’s.#his: [[his]1.5]colloquial;
% 's.#shall ’s.#shall: [[shall.v]1.5]colloquial;
's.#us ’s.#us: [[us]1.5]colloquial;
% 't.#it ’t.#it: [[it]1.5]colloquial;
art.#are: [[are.v]0.2]colloquial;
count'nance.#countenance count’nance.#countenance:
 [countenance.v]colloquial or [countenance.s]colloquial;
e'en.#even: [even.e]colloquial;
e'er.#ever: [ever]colloquial;
ha'.#have ha’.#have: [have.v]colloquial;
Heav'n.#heaven Heav’n.#heaven heav'n.#heaven heav’n.#heaven: [heaven.s]colloquial;
i'.#in: [in.r]colloquial;
gi'.#give: [give.v]colloquial;
ne'er.#never: [never.e]colloquial or [never.i]colloquial;
o'.#of: [of]colloquial;
o'.#on: [on]colloquial;
o'er.#over: [over]colloquial;
oft.#often: [often.e]colloquial;
t'.#to: [to.r]colloquial;
ta'en.#taken: [taken.v]colloquial;
th'.#the th’.#the: [the]colloquial;
th'.#thou th’.#thou: [thou]colloquial;
wi'.#with: [with]colloquial;

% Archaic, poetic 'tis:  'Tis the season to be jolly
%      'Twas the night before Christmas
% 'tis.#it-is: [it_is]colloquial;
% 'twill.#it-will: [it_will]colloquial;

'tis tis 'Tis 'twas twas 'Twas ’tis ’Tis ’twas ’Twas:
  Wn- & O+ & {@MV+};

% 'Twill soon strike twelve
'twill 'Twill
’twill ’Twill:
  Wd- & I+;

% Shakespearean
ample.#amply: [[amply]0.5]colloquial;

% Scottish, English alternate spellings
% gell.#gale: [gale.n]colloquial;
% gell.#girl: [girl.n]colloquial;


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Morphology guessing for unknown words.
% Given a particular kind of ending to a word, try to guess
% its part-of-speech.
%
% All of these have a cost of 0.1, so that the CAPTIALIZED-WORDS
% regex gets priority. (CAPITALIZED-WORDS has a cost of 0.05)
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% ING-WORDS.g: (<verb-pg> & <vc-tr,intr>) or (<vc-tr,intr> & <verb-ge>)
% or <verb-adj> or <verb-ge>;

<ING-WORDS>.g:
  [[(<verb-pg> & <vc-tr,intr>)] or
  [(<vc-tr,intr> & <verb-ge>)] or
  [({@E- or EA-}  & A+)] or
  [<verb-ge>] or
  ((<noun-modifiers> &
    (({D*u-} & <noun-rel-s> & (<noun-main-m> or Bsm+)) or
    <noun-and-p,u> or
    (YS+ & {D*u-}) or
    (GN+ & (DD- or [()])) or
    Us-)) or
  AN+ or
  {AN-})]0.1;

<ED-WORDS>.v-d:
  [ VERB_SPPP_T(`<vc-tr,intr>')
    or <verb-pv> or <verb-adj>]0.1;

<S-WORDS>.v: [ VERB_S_T(`<vc-tr,intr>') ]0.1;

<S-WORDS>.n:
  [(<noun-modifiers> &
    (({NM+ or Dmc-} & <noun-rel-p> & (<noun-main-p> or Bpm+)) or
    ({NM+ or Dmc-} & <noun-and-p>) or
    (YP+ & {Dmc-}) or
    (GN+ & (DD- or [()])) or
    Up-)) or
  [[AN+]]]0.1;

<LY-WORDS>.e:
  [{EE- or EF+} & (
    ({Xd- & Xc+} & <adv-as>)
    or Em+
    or ({Xc+ & {Xd-}} & dCO+)
    or Qe+
    or <advcl-verb>
    or [[EA+]])]0.1;

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Morphology guessing extension rules from BioLG-1.1.12
% Words guessed based on morphology.
% These occur primarily in biomedical and chemistry texts.
% These are processed via regular-expression matching, in 4.0.regex
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% guessed nouns that can be mass or countable
% (-in, -ine, -ion, -yl, -ose, -ol, -ide, -ity)
<MC-NOUN-WORDS>.n:
  [<noun-mass-count>]0.1;

% guessed nouns that are singular countable (-on, -or)
<C-NOUN-WORDS>.n:
  [<common-noun>]0.1;

% guessed adjectives (-ous, -ar, -ic)
<ADJ-WORDS>.a:
  [<ordinary-adj> or <adj-phone>]0.1;

% guessed adjectives/adverbs suffixed by "fold" with or without hyphen
<FOLD-WORDS>:
  [({EN-} & (MVp- or EC+ or A+)) or Em+]0.1;

% latin (postposed) adjectives. Mostly, we want these to take Pa-
% so that they actually work like post-posed adjectives. But also
% Pa+ so that they can act as nouns taking post-poed modifiers.
<LATIN-ADJ-WORDS>.a:
  [<ordinary-adj>]0.1 or
  [<noun-mass-count> & {Pa+}]0.2;

% latin (postposed) adjectives or latin plural noun
% always considered as nouns in the current version
% XXX maybe should be same as words.n.2.x instead of <generic-plural-id> ???
% e.g. blahblahgenesis is not plural.
<LATIN-ADJ-P-NOUN-WORDS>:
  [<ordinary-adj>]0.1 or
  [<noun-mass-count> & {Pa+}]0.2 or
  [<generic-plural-id>]0.2;

% latin (postposed) adjectives or latin singular noun
% always considered as nouns in the current version
% XXX this is <common-noun> with weird plural-like stuff ?? is this right?
<LATIN-ADJ-S-NOUN-WORDS>:
  [<ordinary-adj>]0.1 or
  [<noun-modifiers> &
   (AN+
   or ({NM+ or D*u-} & <noun-rel-s> &
      ((<noun-main-m> & {Pa+}) or <rel-clause-s>))
   or ({NM+ or D*u-} & <noun-and-p,u>)
   or (YS+ & {D*u-})
   or (GN+ & (DD- or [()]))
   or Us-)]0.2;

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% Guessing of unknown words, if none of the above rules applied.
<UNKNOWN-WORD>.n <QUOTED-WORD>.n:
  <noun-modifiers> &
    (AN+
    or ({NM+ or ({Jd-} & D*u-)} & <noun-rel-s> & (<noun-main-m> or <rel-clause-x>))
    or ({NM+ or ({Jd-} & D*u-)} & <noun-and-p,u>)
    or dSJrp-
    or (YS+ & {D*u-})
    or (GN+ & (DD- or [()]))
    or U-);

<UNKNOWN-WORD>.v <QUOTED-WORD>.v:
  {@E-} & (
    (Sp- & <verb-wall>) or
    (RS- & Bp-) or
    (I- & <verb-wall>) or
    ({EI-} & Wa- & <verb-wall>))
  & {O+ or <b-minus>} & <mv-coord>;

% Add a minuscule cost, so that the noun-form is preferred, when
% available.
<UNKNOWN-WORD>.a <QUOTED-WORD>.a:
  [<ordinary-adj> or <adj-phone>]0.04;

% These are the link-types that are not subject to the length limit.
% Always use "+" for these.  Some of these are obvious. Some deserve
% an explanation.  So:
%
% O+ is unlimited because some sentences put in long intervening
%    phrases. For example: "He puts forward, as one argument among
%    many others, the object of this sentence" requires a long O link
%    from "puts" to "object".
%
% VJ+ is unlimited because some sentences have very long clauses:
%    "He obtained the lease of the manor of Great Burstead Grange (near
%    East Horndon) from the Abbey of Stratford Langthorne, and purchased
%    the manor of Bayhouse in West Thurrock."
%
UNLIMITED-CONNECTORS:
      S+ & O+ & CO+ & C+ & Xc+ & MV+ & TH+ & W+ &
      RW+ & Xp+ & Xx+ & CP+ & SFsx+ & WV+ & CV+ &
      VJ+ & SJ+;

% The YS, YP, PH and ZZZ connectors can never be longer than one.
LENGTH-LIMIT-1: YS+ & YP+ & PH+ & ZZZ+;

% Handy test
% grrr: (A- & B- & C+ & D+) or [(E- & @F+ & @G+ & H+)] or [[(I- & J- & @K- & @L+)]];
