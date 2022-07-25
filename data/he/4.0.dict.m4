dnl
dnl Macro version of the 4.0.dict file.
dnl
dnl The comment delimiter for link-grammar data files is %
changecom(`%')dnl
 %***************************************************************************%
 %                                                                           %
 % Experimental prototype Hebrew dictionary                                  %
 %       Copyright (C) 2014  Amir Plivatsky                                  %
 %                                                                           %
 % Based on en/tiny.dict,                                                    %
 %       Copyright (C) 1991-1998  Daniel Sleator and Davy Temperley          %
 %
 % and on en/4.0.dict.m4,
 %       Copyright (C) 1991-1998  Daniel Sleator and Davy Temperley          %
 %       Copyright (c) 2003  Peter Szolovits and MIT.                        %
 %       Copyright (c) 2008-2014  Linas Vepstas                              %
 %       Copyright (c) 2013  Lian Ruiting                                    %
 %                                                                           %
 %  See file "LICENSE" for information about commercial use of this system   %
 %                                                                           %
 %***************************************************************************%

% Demo Hebrew dictionary, for initial checks. [ap]
%
% Its origin is the English tiny.dict, in which I translated some words
% and added a few more, for the purpose of checking:
% - tokenizing, including multi-prefix split
% - the resolver behavior with multi-prefix linkages
% - result printing
% The old English definitions are commented out by %#.
%
% Some of the definitions have been replaced by the corresponding ones from
% the full English dictionary (en/4.0.dict.m4). They were partially
% converted to handle Hebrew.
%
% Much of the grammar here is still the English one,
% so it is incorrect for Hebrew.
%
% By now several changes have been done to handle Hebrew more correctly.
% Subject/verb/adjective, gender, and single/plural etc. agreements are only
% partially done.  Most of the Hebrew grammatical constructs are not supported
% here.
%
% Among numerous other things, changes to handle count/uncountable changes
% have not been done yet. The created infrastructure for that may still need changes.

#define dictionary-version-number 5.11.0;
#define dictionary-locale         he_IL.UTF-8;

% For now.
LEFT-WALL: {Wa+} or {Wd+} or ();

% The costly-null is introduced here for now, so expressions can be copied from
% the English dictionary as they are.
<costly-null>: [[[[()]]]];

% NOUNS
% Initially copied from the English dictionary, most probably along with unneeded
% and inappropriate links for Hebrew (to be changed later).

% Hebrew S links:
% S1234
% 1: s, p - for singular, plural
% 2: c, u - for count, uncountable
% 3: m, f - gender: male, female
% 4: 1, 2, 3 - for 1st, 2nd, 3rd person

% The RJ links connect to "and"; the l,r prevent cross-linking
<clause-conjoin>: RJrc- or RJlc+;

% {@COd-} : "That is the man who, in Joe's opinion, we should hire"
<CLAUSE>: {({@COd-} & (C- or <clause-conjoin>)) or ({@CO-} & (Wd- & {CC+})) or [Rn-]};
<S-CLAUSE>: {({@COd-} & (C- or <clause-conjoin>)) or ({@CO-} & (Wd- & {CC+}))};
<CLAUSE-E>: {({@COd-} & (C- or <clause-conjoin>)) or ({@CO-} & (Wd- or {CC+})) or Re-};

% Post-nominal qualifiers, complete with commas, etc.
<post-nominal-x>:
  ({[B*j+]} & Xd- & (Xc+ or <costly-null>) & MX-);

<post-nominal-s>:
  ({[Bsj+]} & Xd- & (Xc+ or <costly-null>) & MX-);

<post-nominal-p>:
  ({[Bpj+]} & Xd- & (Xc+ or <costly-null>) & MX-);

<post-nominal-u>:
  ({[Buj+]} & Xd- & (Xc+ or <costly-null>) & MX-);

define(`NOUN_MAIN',`'
  (((S$1$2$3$4+ or P+) & <CLAUSE>) or SIs- or Js- or O$1*$3-
  or <post-nominal-s>
  or <costly-null>))

<noun-main-s,c,m,3>: NOUN_MAIN(s,c,m,3);
<noun-main-s,u,m,3>: NOUN_MAIN(s,u,m,3);
<noun-main-s,c,f,3>: NOUN_MAIN(s,c,f,3);
<noun-main-s,u,f,3>: NOUN_MAIN(s,u,f,3);
<noun-main-p,c,m,3>: NOUN_MAIN(p,c,m,3);
<noun-main-p,u,m,3>: NOUN_MAIN(p,u,m,3);
<noun-main-p,c,f,3>: NOUN_MAIN(p,c,f,3);
<noun-main-p,u,f,3>: NOUN_MAIN(p,u,f,3);

% NOUN_MAIN_H() arguments:
% 1: s, p - for singular, plural
% 2: m, f - gender: male, female

% Dmu -> D*u ??? for now
define(`NOUN_MAIN_H',
  ((Jd- & D*u- & O$1-)
  or (Jd- & D*u- & {Wd-} & S$1*$2*b+)
  or ((S$1*$2*b+ or O$1*$2+) & <CLAUSE>) or SI$1*$2*b- or [[J$1*$2-]] or [O$1*$2-]
  or <post-nominal-x>
  or <costly-null>))

<noun-sub-x>: {@M+} & {((R+ & B+) or (Ds- & Rb+)) & {[[@M+]]}} & {@MX+};
<noun-sub-s>: {@M+} & {((R+ & Bs+) or (Ds- & Rb+)) & {[[@M+]]}} & {@MXs+};
<noun-sub-p>: {@M+} & {((R+ & Bp+) or (Ds- & Rb+)) & {[[@M+]]}} & {@MXp+};

% Ds- here disallows *הכלב שחור רץ
<noun-modifiers>:
  ((@A+ or Ds-) & {[[@AN-]]})
  or [@AN-]0.1
  or ([[@AN-].1 & @A+] & {[[@AN-]]})
  or ();

<rel-clause-x>: {Rw+} & B*m+;
<rel-clause-s>: {Rw+} & Bsm+;
<rel-clause-p>: {Rw+} & Bpm+;

<noun-and-s>: ({@M+} & SJls+) or ({[@M+]} & SJrs-);
<noun-and-p>: ({[@M+]} & SJlp+) or ({[[@M+]]} & SJrp-);
<noun-and-u>: ({[@M+]} & SJlu+) or ({[[@M+]]} & SJru-);
<noun-and-x>: ({[@M+]} & SJl+) or ({[[@M+]]} & SJr-);

define(`COMMON_NOUN',
  (<noun-modifiers> &
    (({NMa+} & AN+)
    or ((NM+ or ({[NM+]1.5}    )) % & Ds- moved to noun-modifiers
      & ((<noun-sub-$1> & (NOUN_MAIN($1,$2,$3,$4) or <rel-clause-$1>))
        or <noun-and-$1>))
    or SJrs-
    or (YS+ & Ds-)
    or (GN+ & (DD- or [()]))
    or Us-
    or ({Ds-} & Wa-))))

%#dog cat woman man park yard bone neighbor store street bird hammer nose
%#party friend house movie brother sister diner student exam:
%# {@A-} & Ds- & {@M+ or (R+ & Bs+)} &
%# (J- or Os- or (Ss+ & (({@CO-} & {C-}) or R-)) or SIs-);
כלב חתול איש פארק שכן רחוב פטיש אף
חבר בית סרט אח סטודנט מבחן ניסוי לב ורד שולחן:
%% ({@A+} or {Ds-}) & {@M+ or (R+ & Bs+) or (Ds- & Rb+)} &
%% (J- or Os- or ((Ss+ or P+) & (({@CO-} & {C-}) or R-)) or SIs-);
COMMON_NOUN(s,c,m,3);

כלבה חתולה אישה חצר עצם שכנה חנות ציפור
מסיבה חברה אחות ארוחה סטודנטית:
COMMON_NOUN(s,c,f,3);

%#dogs cats women men
%#parks yards bones neighbors stores streets birds hammers noses
%#parties friends houses movies brothers sisters diners students exams
%#wars winters actions laws successes:
%#{@A+} & {Dmc-} & {@M+ or (R+ & Bp+)} &
%# (J- or Op- or (Sp+ & (({@CO-} & {C-}) or R-)) or SIp-);
כלבים חתולים גברים פארקים שכנים רחובות פטישים אפים
חברים בתים סרטים אחים סטודנטים מבחנים שולחנות:
%%{@A+} & {Dmc-} & {@M+ or (R+ & Bp+)} &
%% (J- or Op- or (Sp+ & (({@CO-} & Wd- & {C-}) or R-)) or SIp-);
COMMON_NOUN(p,c,m,3);

כלבות חתולות נשים חצרות עצמות שכנות חנויות ציפורים
מסיבות חברות אחיות ארוחות סטודנטיות:
COMMON_NOUN(p,c,f,3);

%#water anger money politics trouble:
%#{@A+} & {Dmu-} & {@M+ or (R+ & Bs+)} &
%#(J- or Os- or (Ss+ & (({@CO-} & {C-}) or R-)) or SIs-);
מים כעס כסף פוליטיקה:
{@A+} & {Dmu-} & {@M+ or (R+ & Bs+)} &
(J- or Os- or (Ss+ & (({@CO-} & {C-}) or R-)) or SIs-);

%#law winter action war success:
%#{@A+} & {D*u-} & {@M+ or (R+ & Bs+)} &
%#(J- or Os- or (Ss+ & (({@CO-} & {C-}) or R-)) or SIs-);
חוק חורף פעולה מלחמה הצלחה:
{@A+} & {D*u-} & {@M+ or (R+ & Bs+)} &
(J- or Os- or (Ss+ & (({@CO-} & {C-}) or R-)) or SIs-);

%PRONOUNS

% 1: s, p - for singular, plural
% 2: m, f - gender: male, female
% 3: 1, 2, 3 - for 1st, 2nd, 3rd person
define(`PERSONAL_PRONOUN_S',
 ({{[[R+ & B$1*$2$3+]]} &
  (J- or O$1*$2$3- or ({S$1*$2$3+} & <CLAUSE>) or SI$1*$2$3- or SJl$1*$2$3+)}
% Need to update
 & {(({S$1*$2$3-} or (RS- & B$1*$2$3-) or ({Q-} & SI$1*$2$3+))
 & (((O$1*$2$3+ or B-) & {@MV+}) or P+ or AF-))})
)
%#she he: (Ss+ & (({@CO-} & {C-}) or R-)) or SIs-;
%#me him them us: J- or O-;
אותי אותו אותה אותם אותנו: J- or O-;
%#her: D+ or J- or O-;
%#its my your their our: D+;
%#his: D+;
שלה שלו שלי שלך שלהם שלנו: Mp-;


% May need a J- variant in order to invalidate "של הוא", etc.
%#you they we I: J- or O- or (Sp+ & (({@CO-} & {C-}) or R-)) or SIp-;
%% אתם אתן הם הן אנחנו אני: J- or O- or (Sp+ & (({@CO-} & Wd- & {C-}) or R-)) or SIp-;
%% היא הוא: {J- or O- or ({Ss+} & (({@CO-} & {Wd-} & {C-}) or R-)) or SIs-}
%%% היא הוא:  {{[[R+ & Bs+]]} & (({Ss+} & <CLAUSE>) or SIs- or SJls+)}
% From "is" - need to update them from the English dict.
%%% & {(({Ss-} or (RS- & Bs-) or ({Q-} & SIs+))
%%%  & (((O+ or B-) & {@MV+}) or P+ or AF-))};

אני: PERSONAL_PRONOUN_S(s,*,1);
אתה: PERSONAL_PRONOUN_S(s,m,2);
את: PERSONAL_PRONOUN_S(s,f,2);
הוא: PERSONAL_PRONOUN_S(s,m,3);
היא: PERSONAL_PRONOUN_S(s,f,3);
אנחנו אנו: PERSONAL_PRONOUN_S(p,*,3);
אתם: PERSONAL_PRONOUN_S(p,m,2);
אתן: PERSONAL_PRONOUN_S(p,f,2);
הם: PERSONAL_PRONOUN_S(p,m,3);
הן: PERSONAL_PRONOUN_S(p,f,3);

%#this: (J- or O- or (Ss+ & (({@CO-} & {C-}) or R-)) or SIs-) or D*u+;
%%זה: (J- or O- or ((Ss+ or Os+) & (({@CO-} & Wd- & {C-}) or R-)) or SIs-) or D*u+;
זה:
  NOUN_MAIN_H(s,m)
  or EA-
  or <noun-and-s>;

זו:
  NOUN_MAIN_H(s,f)
  or EA-
  or <noun-and-s>;

%#these: (J- or O- or (Sp+ & (({@CO-} & {C-}) or R-)) or SIp-) or Dmc+;
%#those: (Dmc+) or (({P+} or {{C+} & Bp+}) &
%#(J- or O- or (Sp+ & (({@CO-} & {C-}) or R-)) or SIp- or Xb-));
%%אלו: (J- or O- or (Sp+ & (({@CO-} & Wd- & {C-}) or R-)) or SIp-) or Dmc+;
אלו אלה:
  NOUN_MAIN_H(p,*)
  or EA-
  or <noun-and-p>;

% Demonstrative determiner only - needs a total fix
%%הללו: (Dmc+) or (({P+} or {{C+} & Bp+}) &
%%(J- or O- or (Sp+ & (({@CO-} & {C-}) or R-)) or SIp- or Xb-));

%---

%#the: D+;
% Need both R and B to nouns (to allow the second ה=: הכלב השחור רץ)
% hence invented here Rb
ה=: D+ or (Rb- & P+);
%#a: Ds+;

%#did: ({Q-} & SI+ & I+) or ({@E-} & (S- or
%#(RS- & B-)) & (((B- or O+) & {@MV+}) or I+));
%#do: (SIp+ & I+) or ({@E-} & (Sp- or
%#(RS- & Bp-) or I-) &
%#(((B- or O+) & {@MV+}) or I+));
%#does: ({Q-} & SIs+ & I+) or ({@E-} & (Ss- or (RS- & Bs-)) &
%#(((B- or O+) & {@MV+}) or I+));
%#done: {@E-} & (Pv- or M- or (PP- & (B- or O+) & {@MV+}));
%#doing: {@E-} & (Pg- or Mg-) & (O+ or B-) & {@MV+};

%#has: ({Q-} & SIs+ & PP+) or ({@E-} & (Ss- or (RS- & B-)) &
%#(TO+ or ((B- or O+) & {@MV+}) or PP+));
%#have: ({Q-} & SIp+ & PP+) or ({@E-} & (Sp- or
%#(RS- & Bp-) or I-) &
%#(TO+ or ((B- or O+) & {@MV+})));
%#had: ({Q-} & SI+ & PP+) or ({@E-} & (S- or (RS- & B-) or PP-) &
%#(TO+ or ((B- or O+) & {@MV+}) or PP+));
%#having: {@E-} & (M- or Pg-) & (TO+ or ((B- or O+) & {@MV+}) or PP+);

%#is was: ((Ss- or (RS- & Bs-) or ({Q-} & SIs+))
%# & (((O+ or B-) & {@MV+}) or P+ or AF-));
הנו היה: ((Ss- or (RS- & Bs-) or ({Q-} & SIs+))
 & (((O+ or B-) & {@MV+}) or P+ or AF-));
%#are were am: ((Sp- or (RS- & Bp-) or ({Q-} &
%#SIp+)) & (((O+ or B-) & {@MV+}) or P+ or AF-));
הנם היו הנני: ((Sp- or (RS- & Bp-) or ({Q-} &
SIp+)) & (((O+ or B-) & {@MV+}) or P+ or AF-));
%#be: I- & (((O+ or B-) & {@MV+}) or P+ or AF-);
%#been: PP- & (((O+ or B-) & {@MV+}) or P+ or AF-);
%#being: {@E-} & (M- or Pg-) & (((O+ or B-) & {@MV+}) or P+ or AF-);

%#will can.v may must could should would might: (({Q-} &
%#SI+) or S- or (RS- & B-)) & I+;

%VERBS

<MX-PHRASE>: Xd- & (Xc+ or <costly-null>) & (MX*p- or MVg-);
<OPENER>: {Xd-} & Xc+ & COp+;

% These are the verb-form expressions for ordinary verbs.

% <verb-wall>: these connect to the head verb:
% WV connects the wall to the head-verb,
% CV connects the dominating clause to the head verb of the dependent clause.
% IV connects infinitives to the head-verb
%<verb-wall>: dWV- or dCV- or dIV- or [[()]];
<verb-wall>: (); % Not implemented for now for Hebrew

% VERB() arguments:
% 1: s, p - for singular, plural
% 2: m, f - gender: male, female
% 3: 1, 2, 3 - for 1st, 2nd, 3rd person

define(`VERB',
       ({@E-} & (((S$1*$2$3- & <verb-wall>) or (RS- & B$1*$2$3-)) & {@MV+})))

%#run come: {@E-} & (Sp- or (RS- & Bp-) or I- or W- or PP-) & {@MV+};
%%רצים באים הולכים: {@E-} & (Sp- or (RS- & Bp-) or I- or W- or PP-) & {@MV+};
רצים באים הולכים זזים: VERB(p,m,3);
רצות באות הולכות זזות: VERB(p,f,3);
%#runs comes goes: {@E-} & (Ss- or (RS- & Bs-)) & {@MV+};
%%רץ רצה בא באה הולך: {@E-} & (Ss- or (RS- & Bs-)) & {@MV+};
רץ בא הולך זז: VERB(s,m,3);
רצה באה הולכת זזה: VERB(s,f,3);
%%זז: {@E-} & (Ss*m3- or (RS- & Bs*m3-)) & {@MV+};
%#ran came went: {@E-} & (S- or (RS- & B-)) & {@MV+};
%#go: {@E-} & (Sp- or (RS- & Bp-) or I-) & {@MV+};
%#gone: {@E-} & PP- & {@MV+};
%#going: {@E-} & (Pg- or M-) & {TO+} & {@MV+};
%#running coming: {@E-} & (Pg- or M-) & {@MV+};

%#talk arrive die:
%#   {@E-} & (Sp- or (RS- & Bp-) or I-) & {@MV+};
מדבר מגיע מת:
   {@E-} & (Sp- or (RS- & Bp-) or I-) & {@MV+};
%#talks.v arrives dies:
%#   {@E-} & (Ss- or (RS- & Bs-)) & {@MV+};
%#talked arrived died:
%#   {@E-} & (S- or (RS- & B-) or PP-) & {@MV+};
%#talking arriving dying:
%#   {@E-} & (Pg- or M-) & {@MV+};

%#see meet chase invite arrest:
%#   {@E-} & (Sp- or (RS- & Bp-) or I-) & (O+ or B-) & {@MV+};
%#sees meets chases invites arrests:
%#   {@E-} & (Ss- or (RS- & Bs-)) & (O+ or B-) & {@MV+};
%#met chased invited arrested:
%#{@E-} & (M- or Pv- or ((S- or (RS- & B-) or PP-) & (B- or O+))) & {@MV+};
פגש רדף הזמין אסר:
{@E-} & (M- or Pv- or ((S- or (RS- & B-) or PP-) & (B- or O+))) & {@MV+};
%#saw: {@E-} & (S- or (RS- & B-)) & (B- or O+) & {@MV+};
%#seen: {@E-} & (Pv- or M- or (PP- & (B- or O+))) & {@MV+};
%#seeing meeting chasing inviting arresting:
%#{@E-} & (Pg- or M-) & (O+ or B-) & {@MV+};

%#tell: {@E-} & (Sp- or (RS- & Bp-) or I-) & ((O+ or B-) &
%#{TH+ or C+ or QI+ or @MV+});
מספרים: {@E-} & (Sp- or (RS- & Bp-) or I-) & ((O+ or B-) &
{TH+ or C+ or QI+ or @MV+});
%#tells: {@E-} & (Ss- or (RS- & Bs-)) & ((O+ or B-) & {TH+ or C+
%#or QI+ or @MV+});
מספר: {@E-} & (Ss- or (RS- & Bs-)) & ((O+ or B-) & {TH+ or C+
or QI+ or @MV+});
%#told: {@E-} & (M- or Pv- or ((S- or (RS- & B-) or PP-) & (O+ or B-))) &
%#{TH+ or C+ or QI+ or @MV+};
%#telling: {@E-} & (Pg- or M-) & ((O+ or B-) & {TH+ or C+ or QI+ or
%#@MV+});

% END OF VERBS

%#recently sometimes soon gradually specifically generally initially
%#ultimately already now sadly broadly:
%#E+ or MV-;
לאחרונה לפעמים בקרוב בהדרגה במיוחד באופן_כללי בתחילה
לבסוף כבר עכשיו בעצבות בהרחבה:
E+ or MV-;

%#from with at against behind between below above
%#without under for in across through
%#by out up down along like.p on over into about:
%#J+ & (Mp- or MV- or Pp-);
% This doesn't consider the difference between "ב=" with or without a
% definite article included in it. Trying to do so leads to extremely
% complex expression (the same for ל= elsewhere here).
מ= עם ב= מול מאחור בין מלמטה מעל
בלי מתחת בשביל בתוך עבור מעבר דרך ליד חוץ למעלה למטה לאורך כ= כמו על לתוך בערך:
J+ & (Mp- or MV- or Pp-);

%#of: J+ & Mp-;
של: J+ & Mp-;

%#here there: MV- or Mp- or Pp-;
כאן שם: J- or (MV- or Mp- or Pp-);

%#that: (C+ & TH-)
%#or Ds+ or (R- & (C+ or RS+)) or SIs- or (Ss+ &
%#{{@CO-} & {C-}}) or J- or O-;
ש=: (C+ & TH-)
or Ds+ or (R- & (C+ or RS+)) or SIs- or (Ss+ &
{{@CO-} & {C-}}) or J- or O-;

%#to: (I+ & TO-) or ((MV- or Mp- or Pp-) & J+);
ל=: (I+ & TO-) or ((MV- or Mp- or Pp-) & J+);

%#who: (R- & (C+ or RS+)) or S+ or B+;
מי: (R- & (C+ or RS+)) or S+ or B+;
%#what: S+ or B+;
מה: S+ or B+;
%#which: (R- & (C+ or RS+)) or S+ or B-;
איזה: (R- & (C+ or RS+)) or S+ or B-;

%#because unless though although: (C+ & (({Xc+} & CO+) or MV-));
מפני בגלל מפאת
אילולי למרות_ש= למרות: (C+ & (({Xc+} & CO+) or MV-));
%#after before since until: (C+ or J+) & (({Xc+} & CO+) or MV- or Mp-);
אחרי לפני מאז עד: (C+ or J+) & (({Xc+} & CO+) or MV- or Mp-);
%#if: C+ & (({Xc+} & CO+) or MV-);
אם: C+ & (({Xc+} & CO+) or MV-);

%#when: (QI- & C+) or Q+ or (C+ & (({Xc+} & CO+) or MV-));
כאשר כש=: (QI- & C+) or Q+ or (C+ & (({Xc+} & CO+) or MV-));
%#where:(QI- & C+) or Q+;
איפה:(QI- & C+) or Q+;
%#how:  (QI- & (C+ or EA+)) or Q+ or EA+;
איך:  (QI- & (C+ or EA+)) or Q+ or EA+;

%#fast slow short long black white big small beautiful ugly tired angry:
%#  {EA-} & (A- or ((Pa- or AF+) & {@MV+}));
מהיר אטי קצר ארוך שחור לבן גדול קטן יפה מכוער עייף כועס:
   {EA-} & (A- or ((Pa- or AF+) & {@MV+}));

%#glad afraid scared.a fortunate unfortunate lucky unlucky certain sure:
%#{EA-} & (A- or ((Pa- or AF+) & {@MV+} & {C+ or TO+ or TH+}));
שמח שמחה מפחד מפחדת מפוחד מפוחדת בר_מזל בת_מזל ביש_מזל בטוח בטוחה:
 {EA-} & (A- or ((Pa- or AF+) & {@MV+} & {C+ or TO+ or TH+}));

%#very: EA+;
מאד: EA+;

%#but and: MV- & C+;
אבל ו=: MV- & C+;

",": Xc-;

% No actual definition yet - defined here because they appear as possible
% prefixes in 4.0.affix.
לכש= ככ= מב= מל= מש=: ();

% prefix stripping tests
שכבה כבה בה: ();

% With the following line in the dictionary, the parser will simply
% skip over (null-link) unknown words. If you remove it, the parser
% will output an error for any unknown words.
<UNKNOWN-WORD>: XXX+;

% Punctuations that get strip but are yet unhandled.
 "." "–" "‐" ")" "".y" "....y" ":" ";" "?" "!" ₪: <UNKNOWN-WORD>;    % RPUNC
"(" „ “ "".x" ....x: <UNKNOWN-WORD>;                                 % LPUNC
