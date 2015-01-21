#!/usr/bin/env python
# coding: utf8
#
# Python link-grammar test script
#
from itertools import chain
import unittest

from linkgrammar import Parser, Linkage, ParseOptions, Link
import linkgrammar._clinkgrammar as clg

import os

def setUpModule():
    datadir = os.getenv("LINK_GRAMMAR_DATA", "");
    if datadir:
        # Make sure we use an absolute path, else "Error opening word file..."
        datadir = os.path.abspath(datadir)
        clg.dictionary_set_data_dir(datadir)

# The tests are run in alphabetical order....
#
# First test: test the test framework itself ...
class AAALinkTestCase(unittest.TestCase):
    def test_link_display_with_identical_link_type(self):
        self.assertEqual(unicode(Link('Left','Link','Link','Right')),
                         u'Left-Link-Right')

    def test_link_display_with_identical_link_type(self):
        self.assertEqual(unicode(Link('Left','Link','Link*','Right')),
                         u'Left-Link-Link*-Right')

class BParseOptionsTestCase(unittest.TestCase):
    def test_setting_verbosity(self):
        po = ParseOptions()
        po.verbosity = 2
        #Ensure that the PO object reports the value correctly
        self.assertEqual(po.verbosity, 2)
        #Ensure that it's actually setting it.
        self.assertEqual(clg.parse_options_get_verbosity(po._po), 2)

    def test_setting_verbosity_to_not_allow_value_raises_value_error(self):
        po = ParseOptions()
        self.assertRaises(ValueError, setattr, po, "verbosity", 14)

    def test_setting_verbosity_to_non_integer_raises_type_error(self):
        po = ParseOptions()
        self.assertRaises(TypeError, setattr, po, "verbosity", "a")

    def test_setting_linkage_limit(self):
        po = ParseOptions()
        po.linkage_limit = 3
        self.assertEqual(clg.parse_options_get_linkage_limit(po._po), 3)

    def test_setting_linkage_limit_to_non_integer_raises_type_error(self):
        po = ParseOptions()
        self.assertRaises(TypeError, setattr, po, "linkage_limit", "a")

    def test_setting_linkage_limit_to_negative_number_raises_value_error(self):
        po = ParseOptions()
        self.assertRaises(ValueError, setattr, po, "linkage_limit", -1)

    def test_setting_disjunct_cost(self):
        po = ParseOptions()
        po.disjunct_cost = 3.0
        self.assertEqual(clg.parse_options_get_disjunct_cost(po._po), 3.0)

    def test_setting_disjunct_cost_to_non_integer_raises_type_error(self):
        po = ParseOptions()
        self.assertRaises(TypeError, setattr, po, "disjunct_cost", "a")

    def test_setting_min_null_count(self):
        po = ParseOptions()
        po.min_null_count = 3
        self.assertEqual(clg.parse_options_get_min_null_count(po._po), 3)

    def test_setting_min_null_count_to_non_integer_raises_type_error(self):
        po = ParseOptions()
        self.assertRaises(TypeError, setattr, po, "min_null_count", "a")

    def test_setting_min_null_count_to_negative_number_raises_value_error(self):
        po = ParseOptions()
        self.assertRaises(ValueError, setattr, po, "min_null_count", -1)

    def test_setting_max_null_count(self):
        po = ParseOptions()
        po.max_null_count = 3
        self.assertEqual(clg.parse_options_get_max_null_count(po._po), 3)

    def test_setting_max_null_count_to_non_integer_raises_type_error(self):
        po = ParseOptions()
        self.assertRaises(TypeError, setattr, po, "max_null_count", "a")

    def test_setting_max_null_count_to_negative_number_raises_value_error(self):
        po = ParseOptions()
        self.assertRaises(ValueError, setattr, po, "max_null_count", -1)

    def test_setting_short_length(self):
        po = ParseOptions()
        po.short_length = 3
        self.assertEqual(clg.parse_options_get_short_length(po._po), 3)

    def test_setting_short_length_to_non_integer_raises_type_error(self):
        po = ParseOptions()
        self.assertRaises(TypeError, setattr, po, "short_length", "a")

    def test_setting_short_length_to_negative_number_raises_value_error(self):
        po = ParseOptions()
        self.assertRaises(ValueError, setattr, po, "short_length", -1)

    def test_setting_islands_ok(self):
        po = ParseOptions()
        po.islands_ok = True
        self.assertEqual(po.islands_ok, True)
        self.assertEqual(clg.parse_options_get_islands_ok(po._po), 1)
        po.islands_ok = False
        self.assertEqual(po.islands_ok, False)
        self.assertEqual(clg.parse_options_get_islands_ok(po._po), 0)

    def test_setting_islands_ok_to_non_boolean_raises_type_error(self):
        po = ParseOptions()
        self.assertRaises(TypeError, setattr, po, "islands_ok", "a")

    def test_setting_max_parse_time(self):
        po = ParseOptions()
        po.max_parse_time = 3
        self.assertEqual(clg.parse_options_get_max_parse_time(po._po), 3)

    def test_setting_max_parse_time_to_non_integer_raises_type_error(self):
        po = ParseOptions()
        self.assertRaises(TypeError, setattr, po, "max_parse_time", "a")

    def test_setting_display_morphology(self):
        po = ParseOptions()
        po.display_morphology = True
        self.assertEqual(po.display_morphology, True)
        self.assertEqual(clg.parse_options_get_display_morphology(po._po), 1)
        po.display_morphology = False
        self.assertEqual(po.display_morphology, False)
        self.assertEqual(clg.parse_options_get_display_morphology(po._po), 0)

    def test_setting_all_short_connectors(self):
        po = ParseOptions()
        po.all_short_connectors = True
        self.assertEqual(po.all_short_connectors, True)
        self.assertEqual(clg.parse_options_get_all_short_connectors(po._po), 1)
        po.all_short_connectors = False
        self.assertEqual(po.all_short_connectors, False)
        self.assertEqual(clg.parse_options_get_all_short_connectors(po._po), 0)

    def test_setting_all_short_connectors_to_non_boolean_raises_type_error(self):
        po = ParseOptions()
        self.assertRaises(TypeError, setattr, po, "all_short_connectors", "a")


class CParserTestCase(unittest.TestCase):
    def test_specifying_options_when_instantiating_parser(self):
        p = Parser(linkage_limit=10)
        self.assertEqual(clg.parse_options_get_linkage_limit(p.parse_options._po), 10)

    def test_that_parser_can_be_destroyed_when_linkages_still_exist(self):
        """
        If the parser is deleted before the associated swig objects
        are, there will be bad pointer dereferences (as the swig
        objects will be pointing into freed memory).  This test ensures
        that parsers can be created and deleted without regard for
        the existence of PYTHON Linkage objects
        """
        p = Parser()
        linkages = p.parse_sent('This is a sentence.')
        del p


class DBasicParsingTestCase(unittest.TestCase):
    def setUp(self):
        self.p = Parser()

    def test_that_parse_sent_returns_list_of_linkage_objects_for_valid_sentence(self):
        result = self.p.parse_sent("This is a relatively simple sentence.")
        self.assertTrue(isinstance(result, list))
        self.assertTrue(isinstance(result[0], Linkage))
        self.assertTrue(isinstance(result[1], Linkage))

    def test_utf8_encoded_string(self):
        result = self.p.parse_sent("I love going to the café.")
        self.assertTrue(isinstance(result, list))
        self.assertTrue(1 < len(result))
        self.assertTrue(isinstance(result[0], Linkage))
        self.assertTrue(isinstance(result[1], Linkage))

        # def test_unicode_encoded_string(self):
        result = self.p.parse_sent(u"I love going to the caf\N{LATIN SMALL LETTER E WITH ACUTE}.".encode('utf8'))
        self.assertTrue(isinstance(result, list))
        self.assertTrue(1 < len(result))
        self.assertTrue(isinstance(result[0], Linkage))
        self.assertTrue(isinstance(result[1], Linkage))

        # def test_unknown_word(self):
        result = self.p.parse_sent("I love going to the qertfdwedadt.")
        self.assertTrue(isinstance(result, list))
        self.assertTrue(1 < len(result))
        self.assertTrue(isinstance(result[0], Linkage))
        self.assertTrue(isinstance(result[1], Linkage))

        # def test_unknown_euro_utf8_word(self):
        result = self.p.parse_sent("I love going to the qéáéğíóşúüñ.")
        self.assertTrue(isinstance(result, list))
        self.assertTrue(1 < len(result))
        self.assertTrue(isinstance(result[0], Linkage))
        self.assertTrue(isinstance(result[1], Linkage))

        # def test_unknown_cyrillic_utf8_word(self):
        result = self.p.parse_sent("I love going to the доктором.")
        self.assertTrue(isinstance(result, list))
        self.assertTrue(1 < len(result))
        self.assertTrue(isinstance(result[0], Linkage))
        self.assertTrue(isinstance(result[1], Linkage))

    def test_getting_link_distances(self):
        result = self.p.parse_sent("This is a sentence.")[0]
        self.assertEqual(result.link_distances, [5,2,1,1,2,1,1])
        result = self.p.parse_sent("This is a silly sentence.")[0]
        self.assertEqual(result.link_distances, [6,2,1,1,3,2,1,1,1])


class EEnglishLinkageTestCase(unittest.TestCase):
    def setUp(self):
        self.p = Parser()

    def test_a_getting_words(self):
        self.assertEqual(self.p.parse_sent('This is a sentence.')[0].words,
             ['LEFT-WALL', 'this.p', 'is.v', 'a', 'sentence.n', '.', 'RIGHT-WALL'])

    def test_b_getting_num_of_words(self):
        #Words include punctuation and a 'LEFT-WALL' and 'RIGHT_WALL'
        self.assertEqual(self.p.parse_sent('This is a sentence.')[0].num_of_words, 7)

    def test_c_getting_links(self):
        sent = 'This is a sentence.'
        linkage = self.p.parse_sent(sent)[0]
        self.assertEqual(linkage.links[0],
                         Link('LEFT-WALL','Xp','Xp','.'))
        self.assertEqual(linkage.links[1],
                         Link('LEFT-WALL','hWV','dWV','is.v'))
        self.assertEqual(linkage.links[2],
                         Link('LEFT-WALL','Wd','Wd','this.p'))
        self.assertEqual(linkage.links[3],
                         Link('this.p','Ss*b','Ss','is.v'))
        self.assertEqual(linkage.links[4],
                         Link('is.v','O*m','Os','sentence.n'))
        self.assertEqual(linkage.links[5],
                         Link('a','Ds**c','Ds**c','sentence.n'))
        self.assertEqual(linkage.links[6],
                         Link('.','RW','RW','RIGHT-WALL'))

    def test_d_spell_guessing_on(self):
        self.p = Parser(spell_guess = True)
        result = self.p.parse_sent("I love going to shoop.")
        resultx = result[0] if result else []
        for resultx in result:
            if resultx.words[5] == 'shop[~].v':
                break;
        self.assertEqual(resultx.words if resultx else [],
             ['LEFT-WALL', 'I.p', 'love.v', 'going.v', 'to.r', 'shop[~].v', '.', 'RIGHT-WALL'])

    def test_e_spell_guessing_off(self):
        self.p = Parser(spell_guess = False)
        result = self.p.parse_sent("I love going to shoop.")
        self.assertEqual(result[0].words,
             ['LEFT-WALL', 'I.p', 'love.v', 'going.v', 'to.r', 'shoop[?].v', '.', 'RIGHT-WALL'])

    # Stress-test first-word-capitalized in various different ways.
    # Roughly, the test matrix is this:
    # -- word is/isn't in dict as lower-case word
    # -- word is/isn't in dict as upper-case word
    # -- word is/isn't matched with CAPITALIZED_WORDS regex
    # -- word is/isn't split by suffix splitter
    # -- the one that is in the dict is not the grammatically appropriate word.
    #
    # Let's is NOT split into two! Its in the dict as one word, lower-case only.
    def test_f_captilization(self):
        self.assertEqual(self.p.parse_sent('Let\'s eat.')[0].words,
             ['LEFT-WALL', 'let\'s', 'eat.v', '.', 'RIGHT-WALL'])

        # He's is split into two words, he is in dict, lower-case only.
        self.assertEqual(self.p.parse_sent('He\'s going.')[0].words,
             ['LEFT-WALL', 'he', '\'s.v', 'going.v', '.', 'RIGHT-WALL'])

        self.assertEqual(self.p.parse_sent('You\'re going?')[0].words,
             ['LEFT-WALL', 'you', '\'re', 'going.v', '?', 'RIGHT-WALL'])

        # Jumbo only in dict as adjective, lower-case, but not noun.
        self.assertEqual(self.p.parse_sent('Jumbo\'s going?')[0].words,
             ['LEFT-WALL', 'Jumbo[!]', '\'s.v', 'going.v', '?', 'RIGHT-WALL'])

        self.assertEqual(self.p.parse_sent('Jumbo\'s shoe fell off.')[0].words,
             ['LEFT-WALL', 'Jumbo[!]',
              '\'s.p', 'shoe.n', 'fell.v-d', 'off', '.', 'RIGHT-WALL'])

        self.assertEqual(self.p.parse_sent('Jumbo sat down.')[0].words,
             ['LEFT-WALL', 'Jumbo[!]', 'sat.v-d', 'down.r', '.', 'RIGHT-WALL'])

        # Red is in dict, lower-case, as noun, too.
        # There's no way to really know, syntactically, that Red
        # should be taken as a proper noun (given name).
        #self.assertEqual(self.p.parse_sent('Red\'s going?')[0].words,
        #     ['LEFT-WALL', 'Red[!]', '\'s.v', 'going.v', '?', 'RIGHT-WALL'])
        #
        #self.assertEqual(self.p.parse_sent('Red\'s shoe fell off.')[0].words,
        #     ['LEFT-WALL', 'Red[!]',
        #      '\'s.p', 'shoe.n', 'fell.v-d', 'off', '.', 'RIGHT-WALL'])
        #
        #self.assertEqual(self.p.parse_sent('Red sat down.')[1].words,
        #     ['LEFT-WALL', 'Red[!]', 'sat.v-d', 'down.r', '.', 'RIGHT-WALL'])

        # May in dict as noun, capitalized, and as lower-case verb.
        self.assertEqual(self.p.parse_sent('May\'s going?')[0].words,
             ['LEFT-WALL', 'May.f', '\'s.v', 'going.v', '?', 'RIGHT-WALL'])

        self.assertEqual(self.p.parse_sent('May sat down.')[0].words,
             ['LEFT-WALL', 'May.f', 'sat.v-d', 'down.r', '.', 'RIGHT-WALL'])

        # McGyver is not in the dict, but is regex-matched.
        self.assertEqual(self.p.parse_sent('McGyver\'s going?')[0].words,
             ['LEFT-WALL', 'McGyver[!]', '\'s.v', 'going.v', '?', 'RIGHT-WALL'])

        self.assertEqual(self.p.parse_sent('McGyver\'s shoe fell off.')[0].words,
             ['LEFT-WALL', 'McGyver[!]',
              '\'s.p', 'shoe.n', 'fell.v-d', 'off', '.', 'RIGHT-WALL'])

        self.assertEqual(self.p.parse_sent('McGyver sat down.')[0].words,
             ['LEFT-WALL', 'McGyver[!]', 'sat.v-d', 'down.r', '.', 'RIGHT-WALL'])

        self.assertEqual(self.p.parse_sent('McGyver Industries stock declined.')[0].words,
             ['LEFT-WALL', 'McGyver[!]', 'Industries[!]',
              'stock.n-u', 'declined.v-d', '.', 'RIGHT-WALL'])

        # King in dict as both upper and lower case.
        self.assertEqual(self.p.parse_sent('King Industries stock declined.')[0].words,
             ['LEFT-WALL', 'King.b', 'Industries[!]',
              'stock.n-u', 'declined.v-d', '.', 'RIGHT-WALL'])

        # Jumbo in dict only lower-case, as adjective
        self.assertEqual(self.p.parse_sent('Jumbo Industries stock declined.')[0].words,
             ['LEFT-WALL', 'Jumbo[!]', 'Industries[!]',
              'stock.n-u', 'declined.v-d', '.', 'RIGHT-WALL'])

        # Thomas in dict only as upper case.
        self.assertEqual(self.p.parse_sent('Thomas Industries stock declined.')[0].words,
             ['LEFT-WALL', 'Thomas.b', 'Industries[!]',
              'stock.n-u', 'declined.v-d', '.', 'RIGHT-WALL'])

    # Some parses are fractionally preferred over others...
    def test_g_fractions(self):
        self.assertEqual(self.p.parse_sent('A player who is injured has to leave the field')[0].words,
             ['LEFT-WALL', 'a', 'player.n', 'who', 'is.v', 'injured.a', 'has.v', 'to.r', 'leave.v', 'the', 'field.n', 'RIGHT-WALL'])

        self.assertEqual(self.p.parse_sent('They ate a special curry which was recommended by the restaurant\'s owner')[0].words,
             ['LEFT-WALL', 'they', 'ate.v-d', 'a', 'special.a', 'curry.s',
              'which', 'was.v-d', 'recommended.v-d', 'by', 'the', 'restaurant.n',
              '\'s.p', 'owner.n', 'RIGHT-WALL'])

    # Verify that we are getting the linkages that we want
    # See below, remainder of parses are in text files
    def test_h_getting_links(self):
        sent = 'Scientists sometimes may repeat experiments or use groups.'
        linkage = self.p.parse_sent(sent)[0]
        self.assertEqual(linkage.diagram,
"\n    +---------------------------------------Xp--------------------------------------+"
"\n    +---------------------------->WV---------------------------->+                  |"
"\n    |           +-----------------------Sp-----------------------+                  |"
"\n    |           |                  +------------VJlpi------------+                  |"
"\n    +-----Wd----+          +---E---+---I---+----Op----+          +VJrpi+---Op--+    |"
"\n    |           |          |       |       |          |          |     |       |    |"
"\nLEFT-WALL scientists.n sometimes may.v repeat.v experiments.n or.j-v use.v groups.n . "
"\n\n")
        sent = 'I enjoy eating bass.'
        linkage = self.p.parse_sent(sent)[0]
        self.assertEqual(linkage.diagram,
"\n    +-----------------Xp----------------+"
"\n    +---->WV---->+                      |"
"\n    +--Wd--+-Sp*i+---Pg---+---Ou---+    |"
"\n    |      |     |        |        |    |"
"\nLEFT-WALL I.p enjoy.v eating.v bass.n-u . "
"\n\n")


        sent = 'We are from the planet Gorpon'
        linkage = self.p.parse_sent(sent)[0]
        self.assertEqual(linkage.diagram,
"\n    +--->WV--->+     +---------Js--------+"
"\n    +--Wd--+Spx+--Pp-+   +--DD--+---GN---+"
"\n    |      |   |     |   |      |        |"
"\nLEFT-WALL we are.v from the planet.n Gorpon[!] "
"\n\n")


class ZENLangTestCase(unittest.TestCase):
    def setUp(self):
        self.p = Parser(lang = 'en')

    # Reads linkages from a test-file.
    def test_getting_links(self):
        parses = open("parses-en.txt")
        diagram = None
        sent = None
        for line in parses :
            # Lines starting with I are the input sentences
            if 'I' == line[0] :
                sent = line[1:]
                diagram = ""

            # Lines starting with O are the parse diagrams
            if 'O' == line[0] :
                diagram += line[1:]

                # We have a complete diagram if it ends with an
                # empty line.
                if '\n' == line[1] and 1 < len(diagram) :
                    linkage = self.p.parse_sent(sent)[0]
                    self.assertEqual(linkage.diagram, diagram)

        parses.close()


# Tests are run in alphabetical order; do the language tests last.
class ZDELangTestCase(unittest.TestCase):
    def setUp(self):
        self.p = Parser(lang = 'de')

    def test_a_getting_num_of_words(self):
        #Words include punctuation and a 'LEFT-WALL' and 'RIGHT_WALL'
        self.assertEqual(self.p.parse_sent('Dies ist den Traum.')[0].num_of_words, 7)
        self.assertEqual(self.p.parse_sent('Der Hund jagte ihn durch den Park.')[0].num_of_words, 10)

    def test_b_getting_words(self):
        self.assertEqual(self.p.parse_sent('Der Hund jagte ihn durch den Park.')[0].words,
            ['LEFT-WALL', 'der.d', 'Hund.n', 'jagte.s', 'ihn', 'durch',
               'den.d', 'Park.n', '.', 'RIGHT-WALL'])

    def test_c_getting_links(self):
        sent = 'Dies ist den Traum.'
        linkage = self.p.parse_sent(sent)[0]
        self.assertEqual(linkage.links[0],
                         Link('LEFT-WALL','Xp','Xp','.'))
        self.assertEqual(linkage.links[1],
                         Link('LEFT-WALL','W','W','ist.v'))
        self.assertEqual(linkage.links[2],
                         Link('dies','Ss','Ss','ist.v'))
        self.assertEqual(linkage.links[3],
                         Link('ist.v','O','O','Traum.n'))
        self.assertEqual(linkage.links[4],
                         Link('den.d','Dam','Dam','Traum.n'))
        self.assertEqual(linkage.links[5],
                         Link('.','RW','RW','RIGHT-WALL'))

class ZLTLangTestCase(unittest.TestCase):
    def setUp(self):
        self.p = Parser(lang = 'lt')

    # Reads linkages from a test-file.
    def test_getting_links(self):
        parses = open("parses-lt.txt")
        diagram = None
        sent = None
        for line in parses :
            # Lines starting with I are the input sentences
            if 'I' == line[0] :
                sent = line[1:]
                diagram = ""

            # Lines starting with O are the parse diagrams
            if 'O' == line[0] :
                diagram += line[1:]

                # We have a complete diagram if it ends with an
                # empty line.
                if '\n' == line[1] and 1 < len(diagram) :
                    linkage = self.p.parse_sent(sent)[0]
                    self.assertEqual(linkage.diagram, diagram)

        parses.close()


# Tests are run in alphabetical order; do the language tests last.
class ZRULangTestCase(unittest.TestCase):
    def setUp(self):
        self.p = Parser(lang = 'ru')

    def test_a_getting_num_of_words(self):
        #Words include punctuation and a 'LEFT-WALL' and 'RIGHT_WALL'
        self.assertEqual(self.p.parse_sent('это тести.')[0].num_of_words, 5)
        self.assertEqual(self.p.parse_sent('вверху плыли редкие облачка.')[0].num_of_words, 7)

    def test_b_getting_words(self):
        self.assertEqual(self.p.parse_sent('вверху плыли редкие облачка.')[0].words,
            ['LEFT-WALL', 'вверху.e', 'плыли.vnndpp', 'редкие.api',
                'облачка.ndnpi', '.', 'RIGHT-WALL'])

    def test_c_getting_links(self):
        sent = 'вверху плыли редкие облачка.'
        linkage = self.p.parse_sent(sent)[0]
        self.assertEqual(linkage.links[0],
                         Link('LEFT-WALL','Xp','Xp','.'))
        self.assertEqual(linkage.links[1],
                         Link('LEFT-WALL','W','Wd','плыли.vnndpp'))
        self.assertEqual(linkage.links[2],
                         Link('вверху.e','EI','EI','плыли.vnndpp'))
        self.assertEqual(linkage.links[3],
                         Link('плыли.vnndpp','SIp','SIp','облачка.ndnpi'))
        self.assertEqual(linkage.links[4],
                         Link('редкие.api','Api','Api','облачка.ndnpi'))
        self.assertEqual(linkage.links[5],
                         Link('.','RW','RW','RIGHT-WALL'))


    # Expect morphological splitting to apply.
    def test_d_morphology(self):
        self.p = Parser(lang = 'ru', display_morphology = True)
        self.assertEqual(self.p.parse_sent('вверху плыли редкие облачка.')[0].words,
            ['LEFT-WALL',
             'вверху.e',
             'плы.=', '=ли.vnndpp',
             'ре.=', '=дкие.api',
             'облачк.=', '=а.ndnpi',
             '.', 'RIGHT-WALL'])


unittest.main()
