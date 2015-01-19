#
# High-level Python bindings build on top of the low-level
# C API (clinkgrammar)

import locale

import _clinkgrammar as clg


__all__ = ['Parser', 'ParseOptions', 'Dictionary', 'Link', 'Linkage']


class Parser(object):
    # A cost of 2.7 allows the usual cost-2 connectors, plus the
    # assorted fractional costs, without going to cost 3.0, which
    # is used only during panic-parsing.
    def __init__(self, lang = "en",
                       verbosity = 0,
                       linkage_limit = 100,
                       min_null_count = 0,
                       max_null_count = 0,
                       islands_ok = False,
                       short_length = 6,
                       all_short_connectors = False,
                       display_morphology = False,
                       spell_guess = False,
                       max_parse_time = 30,
                       disjunct_cost = 2.7):
        locale.setlocale(locale.LC_ALL, "en_US.UTF-8")
        self.dictionary = Dictionary(lang)
        self.parse_options = ParseOptions(verbosity = verbosity,
                                          linkage_limit = linkage_limit,
                                          min_null_count = min_null_count,
                                          max_null_count = max_null_count,
                                          islands_ok = islands_ok,
                                          short_length = short_length,
                                          display_morphology = display_morphology,
                                          spell_guess = spell_guess,
                                          max_parse_time = max_parse_time,
                                          disjunct_cost = disjunct_cost)


    def parse_sent(self, txt):
        sent = clg.sentence_create(txt,self.dictionary._dict)
        n = clg.sentence_parse(sent, self.parse_options._po)
        if n > 0:
            linkages = [Linkage.create_from_sentence(i,sent, self.parse_options._po) for i in range(n)]
        else:
            linkages = []
        clg.sentence_delete(sent)
        return linkages

    @property
    def version(self):
        return clg.linkgrammar_get_version()

class ParseOptions(object):
    def __init__(self, verbosity = 0,
                       linkage_limit = 100,
                       min_null_count = 0,
                       max_null_count = 0,
                       islands_ok = False,
                       short_length = 6,
                       all_short_connectors = False,
                       display_morphology = False,
                       spell_guess = False,
                       max_parse_time = -1,
                       disjunct_cost = 2.7):


        self._po = clg.parse_options_create()
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
        if self._po is not None and clg is not None:
            clg.parse_options_delete(self._po)
            self._po = None

    def verbosity():
        doc = "This sets/gets the level of description printed to stderr/stdout about the parsing process."
        def fget(self):
            return clg.parse_options_get_verbosity(self._po)
        def fset(self, value):
            if not isinstance(value, int):
                raise TypeError("Verbosity must be set to an integer")
            if value not in [0, 1, 2, 3, 4, 5, 6]:
                raise ValueError("Verbosity levels can be any integer between 0 and 6 inclusive")
            clg.parse_options_set_verbosity(self._po, value)
        return locals()
    verbosity = property(**verbosity())

    def linkage_limit():
        doc = "This parameter determines the maximum number of linkages that are considered in post-processing. If more than linkage_limit linkages found, then a random sample of linkage_limit is chosen for post-processing. When this happen a warning is displayed at verbosity levels bigger than 1."
        def fget(self):
            return clg.parse_options_get_linkage_limit(self._po)
        def fset(self, value):
            if not isinstance(value, int):
                raise TypeError("Linkage Limit must be set to an integer")
            if value < 0:
                raise ValueError("Linkage Limit must be positive")
            clg.parse_options_set_linkage_limit(self._po, value)
        return locals()
    linkage_limit = property(**linkage_limit())

    def disjunct_cost():
        doc = "Determines the maximum disjunct cost used during parsing, where the cost of a disjunct is equal to the maximum cost of all of its connectors. The default is that only disjuncts of cost 2.7 or less are considered."
        def fget(self):
            return clg.parse_options_get_disjunct_cost(self._po)
        def fset(self, value):
            if not isinstance(value, float):
                raise TypeError("Distjunct cost must be set to a float")
            clg.parse_options_set_disjunct_cost(self._po, value)
        return locals()
    disjunct_cost = property(**disjunct_cost())

    def min_null_count():
        doc = "The minimum number of null links that a parse might have. A call to sentence_parse will find all linkages having the minimum number of null links within the range specified by this parameter."
        def fget(self):
            return clg.parse_options_get_min_null_count(self._po)
        def fset(self, value):
            if not isinstance(value, int):
                raise TypeError("min_null_count must be set to an integer")
            if value < 0:
                raise ValueError("min_null_count must be positive")
            clg.parse_options_set_min_null_count(self._po, value)
        return locals()
    min_null_count = property(**min_null_count())

    def max_null_count():
        doc = "The maximum number of null links that a parse might have. A call to sentence_parse will find all linkages having the minimum number of null links within the range specified by this parameter."
        def fget(self):
            return clg.parse_options_get_max_null_count(self._po)
        def fset(self, value):
            if not isinstance(value, int):
                raise TypeError("max_null_count must be set to an integer")
            if value < 0:
                raise ValueError("max_null_count must be positive")
            clg.parse_options_set_max_null_count(self._po, value)
        return locals()
    max_null_count = property(**max_null_count())

    def short_length():
        doc = "The short_length parameter determines how long the links are allowed to be. The intended use of this is to speed up parsing by not considering very long links for most connectors, since they are very rarely used in a correct parse. An entry for UNLIMITED-CONNECTORS in the dictionary will specify which connectors are exempt from the length limit."
        def fget(self):
            return clg.parse_options_get_short_length(self._po)
        def fset(self, value):
            if not isinstance(value, int):
                raise TypeError("short_length must be set to an integer")
            if value < 0:
                raise ValueError("short_length must be positive")
            clg.parse_options_set_short_length(self._po, value)
        return locals()
    short_length = property(**short_length())

    def islands_ok():
        doc = """
        This option determines whether or not "islands" of links are allowed. For example, the following linkage has an island:
            +------Wd-----+
            |     +--Dsu--+---Ss--+-Paf-+      +--Dsu--+---Ss--+--Pa-+
            |     |       |       |     |      |       |       |     |
          ///// this sentence.n is.v false.a this sentence.n is.v true.a
        """
        def fget(self):
            return clg.parse_options_get_islands_ok(self._po) == 1
        def fset(self, value):
            if not isinstance(value, bool):
                raise TypeError("islands_ok must be set to a bool")
            clg.parse_options_set_islands_ok(self._po, 1 if value else 0)
        return locals()
    islands_ok = property(**islands_ok())

    def max_parse_time():
        doc = "Determines the approximate maximum time (in seconds) that parsing is allowed to take. After this time has expired, the parsing process is artificially forced to complete quickly by pretending that no further solutions can be constructed. The actual parsing time might be slightly longer."
        def fget(self):
            return clg.parse_options_get_max_parse_time(self._po)
        def fset(self, value):
            if not isinstance(value, int):
                raise TypeError("max_parse_time must be set to an integer")
            clg.parse_options_set_max_parse_time(self._po, value)
        return locals()
    max_parse_time = property(**max_parse_time())

    def display_morphology():
        doc = "Whether or not to show word morphology when a linkage diagram is printed."
        def fget(self):
            return clg.parse_options_get_display_morphology(self._po) == 1
        def fset(self, value):
            if not isinstance(value, bool):
                raise TypeError("display_morphology must be set to a bool")
            clg.parse_options_set_display_morphology(self._po, 1 if value else 0)
        return locals()
    display_morphology = property(**display_morphology())

    def spell_guess():
        doc = "Whether or not to running the spelling guesser on unknown words."
        def fget(self):
            return clg.parse_options_get_spell_guess(self._po) == 1
        def fset(self, value):
            if not isinstance(value, bool):
                raise TypeError("spell_guess must be set to a bool")
            clg.parse_options_set_spell_guess(self._po, 1 if value else 0)
        return locals()
    spell_guess = property(**spell_guess())

    def all_short_connectors():
        doc = "If true, then all connectors have length restrictions imposed on them -- they can be no farther than short_length apart. This is used when parsing in \"panic\" mode, for example."
        def fget(self):
            return clg.parse_options_get_all_short_connectors(self._po) == 1
        def fset(self, value):
            if not isinstance(value, bool):
                raise TypeError("all_short_connectors must be set to a bool")
            clg.parse_options_set_all_short_connectors(self._po, 1 if value else 0)
        return locals()
    all_short_connectors = property(**all_short_connectors())



class Dictionary(object):
    def __init__(self, lang):
        self._dict = clg.dictionary_create_lang(lang)

    def __del__(self):
        if self._dict is not None and clg is not None:
            clg.dictionary_delete(self._dict)
            self._dict = None

class Link(object):
    def __init__(self, left_word, left_link, right_link, right_word):
        self.left_word = left_word
        self.right_word = right_word
        self.left_link = left_link
        self.right_link = right_link

    def __eq__(self, other):
        return self.left_word == other.left_word and self.left_link == other.left_link and \
               self.right_word == other.right_word and self.right_link == other.right_link

    def __unicode__(self):
        if self.left_link == self.right_link:
            return u"%s-%s-%s" % (self.left_word, self.left_link, self.right_word)
        else:
            return u"%s-%s-%s-%s" % (self.left_word, self.left_link, self.right_link, self.right_word)

    def __repr__(self):
        return "Link: %s" % unicode(self)


class Linkage(object):
    @classmethod
    def create_from_sentence(cls, idx, sentence, parse_options):
        linkage_swig_obj = clg.linkage_create(idx,sentence, parse_options)
        linkage_obj = Linkage(linkage_swig_obj, null_count=clg.sentence_null_count(sentence))
        clg.linkage_delete(linkage_swig_obj)
        return linkage_obj

    def __init__(self, linkage_swig_obj, null_count=0):

        # XXX FIXME Most of the initialization below just wastes CPU time,
        # especially if the user does not need this stuff.
        self.num_of_words = clg.linkage_get_num_words(linkage_swig_obj)
        self.num_of_links = clg.linkage_get_num_links(linkage_swig_obj)
        self.link_cost = clg.linkage_link_cost(linkage_swig_obj)

        self.words = [clg.linkage_get_word(linkage_swig_obj,i) for i in range(self.num_of_words)]
        self.links = [Link(self.words[clg.linkage_get_link_lword(linkage_swig_obj,i)],
                           clg.linkage_get_link_llabel(linkage_swig_obj,i),
                           clg.linkage_get_link_rlabel(linkage_swig_obj,i),
                           self.words[clg.linkage_get_link_rword(linkage_swig_obj,i)]) for i in range(self.num_of_links)]
        self.null_count = null_count
        self.link_distances = [clg.linkage_get_link_length(linkage_swig_obj, i) for i in range(self.num_of_links)]

        # XXX FIXME: doing the below, if not needed, is a bit of a cpu-waster!
        #
        # display_walls=0, screen_width=180
        self.diagram = clg.linkage_print_diagram(linkage_swig_obj, 0, 180)
        self.senses = clg.linkage_print_senses(linkage_swig_obj)
        self.links_and_domains = clg.linkage_print_links_and_domains(linkage_swig_obj)
        # Disable postscript. Lets be real, no one will use this.
        # self.postscript_snippet = clg.linkage_print_postscript(linkage_swig_obj, 1, 0)
        # self.postscript = clg.linkage_print_postscript(linkage_swig_obj, 1, 1)

