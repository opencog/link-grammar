# -*- coding: utf8 -*-
#
# High-level Python bindings build on top of the low-level
# C API (clinkgrammar)
# See http://www.abisource.com/projects/link-grammar/api/index.html to get
# more information about C API

import _clinkgrammar as clg


__all__ = ['ParseOptions', 'Dictionary', 'Link', 'Linkage', 'Sentence']


class ParseOptions(object):
    def __init__(self, verbosity=0,
                       linkage_limit=100,
                       min_null_count=0,
                       max_null_count=0,
                       islands_ok=False,
                       short_length=6,
                       all_short_connectors=False,
                       display_morphology=False,
                       spell_guess=False,
                       max_parse_time=-1,
                       disjunct_cost=2.7):

        self._obj = clg.parse_options_create()
        self.verbosity = verbosity
        self.linkage_limit = linkage_limit
        self.min_null_count = min_null_count
        self.max_null_count = max_null_count
        self.islands_ok = islands_ok
        self.short_length = short_length
        self.all_short_connectors = all_short_connectors
        self.display_morphology = display_morphology
        self.spell_guess = spell_guess
        self.max_parse_time = max_parse_time
        self.disjunct_cost = disjunct_cost

    def __del__(self):
        if hasattr(self, '_obj'):
            clg.parse_options_delete(self._obj)
            del self._obj

    @property
    def version(self):
        return clg.linkgrammar_get_version()

    @property
    def verbosity(self):
        """
        This is the level of description printed to stderr/stdout about the
        parsing process.
        """
        return clg.parse_options_get_verbosity(self._obj)

    @verbosity.setter
    def verbosity(self, value):
        if not isinstance(value, int):
            raise TypeError("Verbosity must be set to an integer")
        if value not in (0, 1, 2, 3, 4, 5, 6):
            raise ValueError("Verbosity levels can be any integer between 0 and 6 inclusive")
        clg.parse_options_set_verbosity(self._obj, value)

    @property
    def linkage_limit(self):
        """
        This parameter determines the maximum number of linkages that are considered in post-processing. If more than linkage_limit linkages found, then a random sample of linkage_limit is chosen for post-processing. When this happen a warning is displayed at verbosity levels bigger than 1.
        """
        return clg.parse_options_get_linkage_limit(self._obj)

    @linkage_limit.setter
    def linkage_limit(self, value):
        if not isinstance(value, int):
            raise TypeError("Linkage Limit must be set to an integer")
        if value < 0:
            raise ValueError("Linkage Limit must be positive")
        clg.parse_options_set_linkage_limit(self._obj, value)

    @property
    def disjunct_cost(self):
        """
        Determines the maximum disjunct cost used during parsing, where the
        cost of a disjunct is equal to the maximum cost of all of its connectors.
        The default is that only disjuncts of cost 2.7 or less are considered.
        """
        return clg.parse_options_get_disjunct_cost(self._obj)

    @disjunct_cost.setter
    def disjunct_cost(self, value):
        if not isinstance(value, float):
            raise TypeError("Distjunct cost must be set to a float")
        clg.parse_options_set_disjunct_cost(self._obj, value)

    @property
    def min_null_count(self):
        """
         The minimum number of null links that a parse might have. A call to
         sentence_parse will find all linkages having the minimum number of
         null links within the range specified by this parameter.
        """
        return clg.parse_options_get_min_null_count(self._obj)

    @min_null_count.setter
    def min_null_count(self, value):
        if not isinstance(value, int):
            raise TypeError("min_null_count must be set to an integer")
        if value < 0:
            raise ValueError("min_null_count must be positive")
        clg.parse_options_set_min_null_count(self._obj, value)

    @property
    def max_null_count(self):
        """
         The maximum number of null links that a parse might have. A call to
         sentence_parse will find all linkages having the minimum number of
         null links within the range specified by this parameter.
        """
        return clg.parse_options_get_max_null_count(self._obj)

    @max_null_count.setter
    def max_null_count(self, value):
        if not isinstance(value, int):
            raise TypeError("max_null_count must be set to an integer")
        if value < 0:
            raise ValueError("max_null_count must be positive")
        clg.parse_options_set_max_null_count(self._obj, value)

    @property
    def short_length(self):
        """
         The short_length parameter determines how long the links are allowed
         to be. The intended use of this is to speed up parsing by not
         considering very long links for most connectors, since they are very
         rarely used in a correct parse. An entry for UNLIMITED-CONNECTORS in
         the dictionary will specify which connectors are exempt from the
         length limit.
        """
        return clg.parse_options_get_short_length(self._obj)

    @short_length.setter
    def short_length(self, value):
        if not isinstance(value, int):
            raise TypeError("short_length must be set to an integer")
        if value < 0:
            raise ValueError("short_length must be positive")
        clg.parse_options_set_short_length(self._obj, value)

    @property
    def islands_ok(self):
        """
        This option determines whether or not "islands" of links are allowed.
        For example, the following linkage has an island:
            +------Wd-----+
            |     +--Dsu--+---Ss--+-Paf-+      +--Dsu--+---Ss--+--Pa-+
            |     |       |       |     |      |       |       |     |
          ///// this sentence.n is.v false.a this sentence.n is.v true.a
        """
        return clg.parse_options_get_islands_ok(self._obj) == 1

    @islands_ok.setter
    def islands_ok(self, value):
        if not isinstance(value, bool):
            raise TypeError("islands_ok must be set to a bool")
        clg.parse_options_set_islands_ok(self._obj, 1 if value else 0)

    @property
    def max_parse_time(self):
        """
         Determines the approximate maximum time (in seconds) that parsing is
         allowed to take. After this time has expired, the parsing process is
         artificially forced to complete quickly by pretending that no further
         solutions can be constructed. The actual parsing time might be
         slightly longer.
        """
        return clg.parse_options_get_max_parse_time(self._obj)

    @max_parse_time.setter
    def max_parse_time(self, value):
        if not isinstance(value, int):
            raise TypeError("max_parse_time must be set to an integer")
        clg.parse_options_set_max_parse_time(self._obj, value)

    @property
    def display_morphology(self):
        """
        Whether or not to show word morphology when a linkage diagram is printed.
        """
        return clg.parse_options_get_display_morphology(self._obj) == 1

    @display_morphology.setter
    def display_morphology(self, value):
        if not isinstance(value, bool):
            raise TypeError("display_morphology must be set to a bool")
        clg.parse_options_set_display_morphology(self._obj, 1 if value else 0)

    @property
    def spell_guess(self):
        """
         Whether or not to running the spelling guesser on unknown words.
        """
        return clg.parse_options_get_spell_guess(self._obj) == 1

    @spell_guess.setter
    def spell_guess(self, value):
        if not isinstance(value, bool):
            raise TypeError("spell_guess must be set to a bool")
        clg.parse_options_set_spell_guess(self._obj, 1 if value else 0)

    @property
    def all_short_connectors(self):
        """
         If true, then all connectors have length restrictions imposed on
         them -- they can be no farther than short_length apart. This is
         used when parsing in \"panic\" mode, for example.
        """
        return clg.parse_options_get_all_short_connectors(self._obj) == 1

    @all_short_connectors.setter
    def all_short_connectors(self, value):
        if not isinstance(value, bool):
            raise TypeError("all_short_connectors must be set to a bool")
        clg.parse_options_set_all_short_connectors(self._obj, 1 if value else 0)


class Dictionary(object):
    def __init__(self, lang='en'):
        self._obj = clg.dictionary_create_lang(lang)

    def __del__(self):
        if hasattr(self, '_obj') and clg:
            clg.dictionary_delete(self._obj)
            del self._obj

    def get_max_cost(self):
        return clg.dictionary_get_max_cost(self._obj)


class Link(object):
    def __init__(self, linkage, index, left_word, left_label, right_label, right_word):
        self.linkage, self.index = linkage, index
        self.left_word, self.right_word, self.left_label, self.right_label = \
            left_word, right_word, left_label, right_label

    def __eq__(self, other):
        return self.left_word == other.left_word and self.left_label == other.left_label and \
               self.right_word == other.right_word and self.right_label == other.right_label

    def __unicode__(self):
        if self.left_label == self.right_label:
            return u"%s-%s-%s" % (self.left_word, self.left_label, self.right_word)
        else:
            return u"%s-%s-%s-%s" % (self.left_word, self.left_label, self.right_label, self.right_word)

    def __repr__(self):
        return u"Link: %s" % unicode(self)

    def __len__(self):
        return clg.linkage_get_link_length(self.linkage._obj, self.index)

    def num_domains(self):
        return clg.linkage_get_link_num_domains(self.linkage._obj, self.index)



class Linkage(object):

    def __init__(self, idx, sentence, parse_options):
        self.sentence, self.parse_options = sentence, parse_options  # keep all args passed into clg.* fn
        self._obj = clg.linkage_create(idx, sentence._obj, parse_options)

    def __del__(self):
        if hasattr(self, '_obj') and clg:
            clg.linkage_delete(self._obj)
            del self._obj

    def null_count(self):
        return clg.sentence_null_count(self._obj)

    def num_of_words(self):
        return clg.linkage_get_num_words(self._obj)

    def num_of_links(self):
        return clg.linkage_get_num_links(self._obj)

    def words(self):
        for i in xrange(self.num_of_words()):
            yield self.word(i)

    def word(self, i):
        return clg.linkage_get_word(self._obj, i)

    def unused_word_cost(self):
        return clg.linkage_unused_word_cost(self._obj)

    def link_cost(self):
        return clg.linkage_link_cost(self._obj)

    def link(self, i):
        return Link(self, i, self.word(clg.linkage_get_link_lword(self._obj, i)),
                    clg.linkage_get_link_llabel(self._obj, i),
                    clg.linkage_get_link_rlabel(self._obj, i),
                    self.word(clg.linkage_get_link_rword(self._obj, i)))

    def links(self):
        for i in xrange(self.num_of_links()):
            yield self.link(i)

    def violation_name(self):
        return clg.linkage_get_violation_name(self._obj)

    def diagram(self, display_walls=False, screen_width=180):
        return clg.linkage_print_diagram(self._obj, display_walls, screen_width)

    def postscript(self, display_walls=True, print_ps_header=0):
        return clg.linkage_print_postscript(self._obj, display_walls, print_ps_header)

    def senses(self):
        return clg.linkage_print_senses(self._obj)

    def constituent_tree(self, mode=1):
        return clg.linkage_print_constituent_tree(self._obj, mode)


class Sentence(object):
    text = None
    dict = None
    parse_options = None

    def __init__(self, text, dict, parse_options):
        """
        In python 2.x, txt should be unicode string like u'Bo-bo-bo'
        """
        self.text, self.dict, self.parse_options = text, dict, parse_options  # keep all args passed into clg.* fn
        self._obj = clg.sentence_create(self.text, self.dict._obj)

    def __del__(self):
        if hasattr(self, '_obj') and clg:
            clg.sentence_delete(self._obj)
            del self._obj

    def split(self):
        return clg.sentence_split(self._obj)

    def parse(self):
        n = clg.sentence_parse(self._obj, self.parse_options._obj)
        for i in xrange(n):
            yield Linkage(i, self, self.parse_options._obj)

