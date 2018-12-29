#!/usr/bin/env python
# coding: utf8
"""Python link-grammar test script"""

from __future__ import print_function
import sys, os, re
import locale
import unittest

# assertRaisesRegexp and assertRegexpMatches have been renamed in
# unittest for python 3, but not in python 2 (at least yet).
if hasattr(unittest.TestCase, 'assertRaisesRegex'):
    unittest.TestCase.assertRaisesRegexp = unittest.TestCase.assertRaisesRegex
    unittest.TestCase.assertRegexpMatches = unittest.TestCase.assertRegex

import lg_testutils # Found in the same directory of this test script

# Show information on this program run
print('Running by:', sys.executable)
print('Running {} in:'.format(sys.argv[0]), os.getcwd())
for v in 'PYTHONPATH', 'srcdir', 'LINK_GRAMMAR_DATA':
    print('{}={}'.format(v, os.environ.get(v)))
#===

from linkgrammar import (Sentence, Linkage, ParseOptions, Link, Dictionary,
                         LG_Error, LG_DictionaryError, LG_TimerExhausted,
                         Clinkgrammar as clg)

print(clg.linkgrammar_get_configuration())

# Show the location and version of the bindings modules
for imported_module in 'linkgrammar$', 'clinkgrammar', '_clinkgrammar', 'lg_testutils':
    module_found = False
    for module in sys.modules:
        if re.search(r'^(linkgrammar\.)?'+imported_module, module):
            print("Using", sys.modules[module], end='')
            if hasattr(sys.modules[module], '__version__'):
                print(' version', sys.modules[module].__version__, end='')
            print()
            module_found = True
    if not module_found:
        print("Warning: Module", imported_module,  "not loaded.")

sys.stdout.flush()
#===

def setUpModule():
    datadir = os.getenv("LINK_GRAMMAR_DATA", "")
    if datadir:
        clg.dictionary_set_data_dir(datadir)

    clg.test_data_srcdir = os.getenv("srcdir", os.path.dirname(sys.argv[0]))
    if clg.test_data_srcdir:
        clg.test_data_srcdir += "/"

# The tests are run in alphabetical order....
#
# First test: test the test framework itself ...
class AAALinkTestCase(unittest.TestCase):
    def test_link_display_with_identical_link_type(self):
        self.assertEqual(str(Link(None, 0, 'Left','Link','Link','Right')),
                         u'Left-Link-Right')

    def test_link_display_with_identical_link_type2(self):
        self.assertEqual(str(Link(None, 0, 'Left','Link','Link*','Right')),
                         u'Left-Link-Link*-Right')

class AADictionaryTestCase(unittest.TestCase):
    def test_open_nonexistent_dictionary(self):
        dummy_lang = "No such language test "

        save_stderr = divert_start(2)
        self.assertRaises(LG_DictionaryError, Dictionary, dummy_lang + '1')
        self.assertIn(dummy_lang + '1', save_stderr.divert_end())

        save_stderr = divert_start(2)
        self.assertRaises(LG_Error, Dictionary, dummy_lang + '2')
        self.assertIn(dummy_lang + '2', save_stderr.divert_end())

# Check absolute and relative dictionary access.
# Check also that the dictionary language is set correctly.
#
# We suppose here that this test program is located somewhere in the source
# directory, 1 to 4 levels under it, that the data directory is named 'data',
# and that it has a parallel directory called 'link-grammar'.
DATA_DIR = 'data'
PARALLEL_DIR = 'link-grammar'
class ABDictionaryLocationTestCase(unittest.TestCase):
    abs_datadir = None

    @classmethod
    def setUpClass(cls):
        cls.po = ParseOptions(verbosity=0)
        cls.original_directory = os.getcwd()

        # Find the 'data' directory in the source directory.
        os.chdir(clg.test_data_srcdir)
        up = ''
        for _ in range(1, 4):
            up = '../' + up
            datadir = up + DATA_DIR
            if os.path.isdir(datadir):
                break
            datadir = ''

        if not datadir:
            assert False, 'Cannot find source directory dictionary data'
        cls.abs_datadir = os.path.abspath(datadir)

    @classmethod
    def tearDownClass(cls):
        del cls.po
        os.chdir(cls.original_directory)

    def test_open_absolute_path(self):
        d = Dictionary(self.abs_datadir + '/en')
        self.assertEqual(str(d), 'en')
        if os.name == 'nt':
            d = Dictionary(self.abs_datadir + r'\en')
            self.assertEqual(str(d), 'en')

    def test_open_relative_path_from_data_directory(self):
        os.chdir(self.abs_datadir)
        d = Dictionary('./en')
        self.assertEqual(str(d), 'en')
        if os.name == 'nt':
            d = Dictionary(r'.\en')
            self.assertEqual(str(d), 'en')

    def test_open_lang_from_data_directory(self):
        os.chdir(self.abs_datadir)
        d = Dictionary('en')
        self.assertEqual(str(d), 'en')

    # Test use of the internal '..' path
    def test_open_from_a_language_directory(self):
        os.chdir(self.abs_datadir + '/ru')
        d = Dictionary('en')
        self.assertEqual(str(d), 'en')

    def test_open_relative_path_from_data_parent_directory(self):
        os.chdir(self.abs_datadir + '/..')
        d = Dictionary('data/en')
        self.assertEqual(str(d), 'en')
        if os.name == 'nt':
            d = Dictionary(r'data\en')
            self.assertEqual(str(d), 'en')

    # Test use of the internal './data' path.
    def test_open_from_data_parent_directory(self):
        os.chdir(self.abs_datadir + '/..')
        d = Dictionary('en')
        self.assertEqual(str(d), 'en')

    # Test use of the internal '../data' path.
    def test_open_from_a_parallel_directory(self):
        os.chdir(self.abs_datadir + '/../' + PARALLEL_DIR)
        d = Dictionary('en')
        self.assertEqual(str(d), 'en')

class BParseOptionsTestCase(unittest.TestCase):
    def test_setting_verbosity(self):
        po = ParseOptions()
        po.verbosity = 2
        #Ensure that the PO object reports the value correctly
        self.assertEqual(po.verbosity, 2)
        #Ensure that it's actually setting it.
        self.assertEqual(clg.parse_options_get_verbosity(po._obj), 2)

    def test_setting_verbosity_to_not_allow_value_raises_value_error(self):
        po = ParseOptions()
        self.assertRaises(ValueError, setattr, po, "verbosity", 121)

    def test_setting_verbosity_to_non_integer_raises_type_error(self):
        po = ParseOptions()
        self.assertRaises(TypeError, setattr, po, "verbosity", "a")

    def test_setting_linkage_limit(self):
        po = ParseOptions()
        po.linkage_limit = 3
        self.assertEqual(clg.parse_options_get_linkage_limit(po._obj), 3)

    def test_setting_linkage_limit_to_non_integer_raises_type_error(self):
        po = ParseOptions()
        self.assertRaises(TypeError, setattr, po, "linkage_limit", "a")

    def test_setting_linkage_limit_to_negative_number_raises_value_error(self):
        po = ParseOptions()
        self.assertRaises(ValueError, setattr, po, "linkage_limit", -1)

    def test_setting_disjunct_cost(self):
        po = ParseOptions()
        po.disjunct_cost = 3.0
        self.assertEqual(clg.parse_options_get_disjunct_cost(po._obj), 3.0)

    def test_setting_disjunct_cost_to_non_integer_raises_type_error(self):
        po = ParseOptions()
        self.assertRaises(TypeError, setattr, po, "disjunct_cost", "a")

    def test_setting_min_null_count(self):
        po = ParseOptions()
        po.min_null_count = 3
        self.assertEqual(clg.parse_options_get_min_null_count(po._obj), 3)

    def test_setting_min_null_count_to_non_integer_raises_type_error(self):
        po = ParseOptions()
        self.assertRaises(TypeError, setattr, po, "min_null_count", "a")

    def test_setting_min_null_count_to_negative_number_raises_value_error(self):
        po = ParseOptions()
        self.assertRaises(ValueError, setattr, po, "min_null_count", -1)

    def test_setting_max_null_count(self):
        po = ParseOptions()
        po.max_null_count = 3
        self.assertEqual(clg.parse_options_get_max_null_count(po._obj), 3)

    def test_setting_max_null_count_to_non_integer_raises_type_error(self):
        po = ParseOptions()
        self.assertRaises(TypeError, setattr, po, "max_null_count", "a")

    def test_setting_max_null_count_to_negative_number_raises_value_error(self):
        po = ParseOptions()
        self.assertRaises(ValueError, setattr, po, "max_null_count", -1)

    def test_setting_short_length(self):
        po = ParseOptions()
        po.short_length = 3
        self.assertEqual(clg.parse_options_get_short_length(po._obj), 3)

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
        self.assertEqual(clg.parse_options_get_islands_ok(po._obj), 1)
        po.islands_ok = False
        self.assertEqual(po.islands_ok, False)
        self.assertEqual(clg.parse_options_get_islands_ok(po._obj), 0)

    def test_setting_islands_ok_to_non_boolean_raises_type_error(self):
        po = ParseOptions()
        self.assertRaises(TypeError, setattr, po, "islands_ok", "a")

    def test_setting_max_parse_time(self):
        po = ParseOptions()
        po.max_parse_time = 3
        self.assertEqual(clg.parse_options_get_max_parse_time(po._obj), 3)

    def test_setting_max_parse_time_to_non_integer_raises_type_error(self):
        po = ParseOptions()
        self.assertRaises(TypeError, setattr, po, "max_parse_time", "a")

    def test_setting_spell_guess_to_non_integer_raises_type_error(self):
        po = ParseOptions()
        self.assertRaises(TypeError, setattr, po, "spell_guess", "a")

    def test_setting_display_morphology(self):
        po = ParseOptions()
        po.display_morphology = True
        self.assertEqual(po.display_morphology, True)
        self.assertEqual(clg.parse_options_get_display_morphology(po._obj), 1)
        po.display_morphology = False
        self.assertEqual(po.display_morphology, False)
        self.assertEqual(clg.parse_options_get_display_morphology(po._obj), 0)

    def test_setting_all_short_connectors(self):
        po = ParseOptions()
        po.all_short_connectors = True
        self.assertEqual(po.all_short_connectors, True)
        self.assertEqual(clg.parse_options_get_all_short_connectors(po._obj), 1)
        po.all_short_connectors = False
        self.assertEqual(po.all_short_connectors, False)
        self.assertEqual(clg.parse_options_get_all_short_connectors(po._obj), 0)

    def test_setting_all_short_connectors_to_non_boolean_raises_type_error(self):
        po = ParseOptions()
        self.assertRaises(TypeError, setattr, po, "all_short_connectors", "a")

    def test_setting_spell_guess(self):
        po = ParseOptions(spell_guess=True)
        if po.spell_guess == 0:
            raise unittest.SkipTest("Library is not configured with spell guess")
        self.assertEqual(po.spell_guess, 7)
        po = ParseOptions(spell_guess=5)
        self.assertEqual(po.spell_guess, 5)
        po = ParseOptions(spell_guess=False)
        self.assertEqual(po.spell_guess, 0)

    def test_specifying_parse_options(self):
        po = ParseOptions(linkage_limit=99)
        self.assertEqual(clg.parse_options_get_linkage_limit(po._obj), 99)

class CParseOptionsTestCase(unittest.TestCase):

    def test_that_sentence_can_be_destroyed_when_linkages_still_exist(self):
        """
        If the parser is deleted before the associated swig objects
        are, there will be bad pointer dereferences (as the swig
        objects will be pointing into freed memory).  This test ensures
        that parsers can be created and deleted without regard for
        the existence of PYTHON Linkage objects
        """
        #pylint: disable=unused-variable
        s = Sentence('This is a sentence.', Dictionary(), ParseOptions())
        linkages = s.parse()
        del s

    def test_that_invalid_options_are_disallowed(self):
        self.assertRaisesRegexp(TypeError, "unexpected keyword argument",
                                ParseOptions, invalid_option=1)

    def test_that_invalid_option_properties_cannot_be_used(self):
        po = ParseOptions()
        self.assertRaisesRegexp(TypeError, "Unknown parse option",
                                setattr, po, "invalid_option", 1)

    def test_that_ParseOptions_cannot_get_positional_arguments(self):
        self.assertRaisesRegexp(TypeError, "Positional arguments are not allowed",
                                ParseOptions, 1)

class DBasicParsingTestCase(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.d, cls.po = Dictionary(), None

    @classmethod
    def tearDownClass(cls):
        del cls.d, cls.po

    def parse_sent(self, text, po=None):
        if po is None:
            po = ParseOptions()
        return list(Sentence(text, self.d, po).parse())

    def test_that_parse_returns_empty_iterator_on_no_linkage(self):
        """Parsing a bad sentence with no null-links shouldn't give any linkage."""
        result = self.parse_sent("This this doesn't parse")
        linkage_exists = False
        for _ in result:
            linkage_exists = True
            self.assertFalse(linkage_exists, "Unparsable sentence has linkages.")

    def test_that_parse_returns_empty_iterator_on_no_linkage_sat(self):
        """Parsing a bad sentence with no null-links shouldn't give any linkage (sat)"""
        self.po = ParseOptions(use_sat=True)
        if self.po.use_sat != True:
            raise unittest.SkipTest("Library not configured with SAT parser")
        result = self.parse_sent("This this doesn't parse", self.po)
        linkage_exists = False
        for _ in result:
            linkage_exists = True
            self.assertFalse(linkage_exists, "SAT: Unparsable sentence has linkages.")

    def test_that_parse_sent_returns_list_of_linkage_objects_for_valid_sentence(self):
        result = self.parse_sent("This is a relatively simple sentence.")
        self.assertTrue(isinstance(result[0], Linkage))
        self.assertTrue(isinstance(result[1], Linkage))

    def test_utf8_encoded_string(self):
        result = self.parse_sent("I love going to the café.")
        self.assertTrue(len(result) > 1)
        self.assertTrue(isinstance(result[0], Linkage))
        self.assertTrue(isinstance(result[1], Linkage))

        # def test_unicode_encoded_string(self):
        if is_python2():
            result = self.parse_sent(u"I love going to the caf\N{LATIN SMALL LETTER E WITH ACUTE}.".encode('utf8'))
        else:
            result = self.parse_sent(u"I love going to the caf\N{LATIN SMALL LETTER E WITH ACUTE}.")
        self.assertTrue(len(result) > 1)
        self.assertTrue(isinstance(result[0], Linkage))
        self.assertTrue(isinstance(result[1], Linkage))

        # def test_unknown_word(self):
        result = self.parse_sent("I love going to the qertfdwedadt.")
        self.assertTrue(len(result) > 1)
        self.assertTrue(isinstance(result[0], Linkage))
        self.assertTrue(isinstance(result[1], Linkage))

        # def test_unknown_euro_utf8_word(self):
        result = self.parse_sent("I love going to the qéáéğíóşúüñ.")
        self.assertTrue(len(result) > 1)
        self.assertTrue(isinstance(result[0], Linkage))
        self.assertTrue(isinstance(result[1], Linkage))

        # def test_unknown_cyrillic_utf8_word(self):
        result = self.parse_sent("I love going to the доктором.")
        self.assertTrue(len(result) > 1)
        self.assertTrue(isinstance(result[0], Linkage))
        self.assertTrue(isinstance(result[1], Linkage))

    def test_getting_link_distances(self):
        linkage = self.parse_sent("This is a sentence.")[0]
        self.assertEqual([len(l) for l in linkage.links()], [5,2,1,1,2,1,1])
        linkage = self.parse_sent("This is a silly sentence.")[0]
        self.assertEqual([len(l) for l in linkage.links()], [6,2,1,1,3,2,1,1,1])

    # If \w is supported, other \ shortcuts are hopefully supported too.
    def test_regex_class_shortcut_support(self):
        r"""Test that regexes support \w"""
        po = ParseOptions(display_morphology=False)
        linkage = self.parse_sent("This is a _regex_ive regex test", po)[0]
        self.assertEqual(linkage.word(4), '_regex_ive[!].a')

    def test_timer_exhausted_exception(self):
        self.assertRaises(LG_TimerExhausted,
                          self.parse_sent,
                          "This should take more than one second to parse! " * 20,
                          ParseOptions(max_parse_time=1))

# The tests here are numbered since their order is important.
# They depend on the result and state of the previous ones as follows:
# - set_handler() returned a value that depend on it previous invocation.
# - A class variable "handler" to record its previous results.
class EErrorFacilityTestCase(unittest.TestCase):
    # Initialize class variables to invalid (for the test) values.
    handler = {
        "default":  lambda x, y=None: None,
        "previous": lambda x, y=None: None
    }

    def setUp(self):
        self.testit = "testit"
        self.testleaks = 0  # A repeat count for validating no memory leaks
        self.numerr = 0
        self.errinfo = clg.lg_None

    @staticmethod
    def error_handler_test(errinfo, data):
        # A test error handler.  It assigns the errinfo struct as an attribute
        # of its data so it can be examined after the call. In addition, the
        # ability of the error handler to use its data argument is tested by
        # the "testit" attribute.
        if data is None:
            return
        data.errinfo = errinfo
        data.gotit = data.testit

    def test_10_set_error_handler(self):
        # Set the error handler and validate that it
        # gets the error info and the data.
        self.__class__.handler["default"] = \
            LG_Error.set_handler(self.error_handler_test, self)
        self.assertEqual(self.__class__.handler["default"].__name__,
                         "_default_handler")
        self.gotit = None
        self.assertRaises(LG_Error, Dictionary, "seh_dummy1")
        self.assertEqual((self.errinfo.severity, self.errinfo.severity_label), (clg.lg_Error, "Error"))
        self.assertEqual(self.gotit, "testit")
        self.assertRegexpMatches(self.errinfo.text, "Could not open dictionary.*seh_dummy1")

    def test_20_set_error_handler_None(self):
        # Set the error handler to None and validate that printall()
        # gets the error info and the data and returns the number of errors.
        self.__class__.handler["previous"] = LG_Error.set_handler(None)
        self.assertEqual(self.__class__.handler["previous"].__name__, "error_handler_test")
        self.assertRaises(LG_Error, Dictionary, "seh_dummy2")
        self.gotit = None
        for i in range(0, 2+self.testleaks):
            self.numerr = LG_Error.printall(self.error_handler_test, self)
            if i == 0:
                self.assertEqual(self.numerr, 1)
            if i == 1:
                self.assertEqual(self.numerr, 0)
        self.assertEqual((self.errinfo.severity, self.errinfo.severity_label), (clg.lg_Error, "Error"))
        self.assertEqual(self.gotit, "testit")
        self.assertRegexpMatches(self.errinfo.text, ".*seh_dummy2")

    def test_21_set_error_handler_None(self):
        # Further test of correct number of errors.
        self.numerr = 3
        for _ in range(0, self.numerr):
            self.assertRaises(LG_Error, Dictionary, "seh_dummy2")
        self.numerr = LG_Error.printall(self.error_handler_test, None)
        self.assertEqual(self.numerr, self.numerr)

    def test_22_defaut_handler_param(self):
        """Test bad data parameter to default error handler"""
        # (It should be an integer >=0 and <= lg_None.)
        # Here the error handler is still set to None.

        # This test doesn't work - TypeError is somehow raised inside
        # linkgrammar.py when _default_handler() is invoked as a callback.
        #
        #LG_Error.set_handler(self.__class__.handler["default"], "bad param")
        #with self.assertRaises(TypeError):
        #    try:
        #        Dictionary("a dummy dict name (bad param test)")
        #    except LG_Error:
        #        pass

        # So test it directly.

        dummy_lang = "a dummy dict name (bad param test)"
        self.assertRaises(LG_Error, Dictionary, dummy_lang)
        LG_Error.printall(self.error_handler_test, self) # grab a valid errinfo
        #self.assertIn(dummy_lang, save_stderr.divert_end())
        self.assertRaisesRegexp(TypeError, "must be an integer",
                                self.__class__.handler["default"],
                                self.errinfo, "bad param")
        self.assertRaisesRegexp(ValueError, "must be an integer",
                                self.__class__.handler["default"],
                                self.errinfo, clg.lg_None+1)
        self.assertRaises(ValueError, self.__class__.handler["default"],
                          self.errinfo, -1)

        try:
            self.param_ok = False
            save_stdout  = divert_start(1) # Note: Handler parameter is stdout
            self.__class__.handler["default"](self.errinfo, 1)
            self.assertIn(dummy_lang, save_stdout.divert_end())
            self.param_ok = True
        except (TypeError, ValueError):
            self.assertTrue(self.param_ok)

    def test_23_prt_error(self):
        LG_Error.message("Info: prt_error test\n")
        LG_Error.printall(self.error_handler_test, self)
        self.assertRegexpMatches(self.errinfo.text, "prt_error test\n")
        self.assertEqual((self.errinfo.severity, self.errinfo.severity_label), (clg.lg_Info, "Info"))

    def test_24_prt_error_in_parts(self):
        LG_Error.message("Trace: part one... ")
        LG_Error.message("part two\n")
        LG_Error.printall(self.error_handler_test, self)
        self.assertEqual(self.errinfo.text, "part one... part two\n")
        self.assertEqual((self.errinfo.severity, self.errinfo.severity_label), (clg.lg_Trace, "Trace"))

    def test_25_prt_error_in_parts_with_embedded_newline(self):
        LG_Error.message("Trace: part one...\n\\")
        LG_Error.message("part two\n")
        LG_Error.printall(self.error_handler_test, self)
        self.assertEqual(self.errinfo.text, "part one...\npart two\n")
        self.assertEqual((self.errinfo.severity, self.errinfo.severity_label), (clg.lg_Trace, "Trace"))

    def test_26_prt_error_plain_message(self):
        LG_Error.message("This is a regular output line.\n")
        LG_Error.printall(self.error_handler_test, self)
        self.assertEqual(self.errinfo.text, "This is a regular output line.\n")
        self.assertEqual((self.errinfo.severity, self.errinfo.severity_label), (clg.lg_None, ""))

    def test_30_formatmsg(self):
        # Here the error handler is still set to None.
        for _ in range (0, 1+self.testleaks):
            self.assertRaises(LG_Error, Dictionary, "formatmsg-test-dummy-dict")
            LG_Error.printall(self.error_handler_test, self)
            self.assertRegexpMatches(self.errinfo.formatmsg(), "link-grammar: Error: .*formatmsg-test-dummy-dict")

    def test_40_clearall(self):
        # Here the error handler is still set to None.
        # Call LG_Error.clearall() and validate it indeed clears the error.
        self.assertRaises(LG_Error, Dictionary, "clearall-test-dummy-dict")
        LG_Error.clearall()
        self.testit = "clearall"
        self.numerr = LG_Error.printall(self.error_handler_test, self)
        self.assertEqual(self.numerr, 0)
        self.assertFalse(hasattr(self, "gotit"))

    def test_41_flush(self):
        # Here the error handler is still set to None.
        # First validate that nothing gets flushed (no error is buffered at this point).
        self.flushed = LG_Error.flush()
        self.assertEqual(self.flushed, False)
        # Now generate a partial error message that is still buffered.
        LG_Error.message("This is a partial error message.")
        # Validate that it is still hidden.
        self.numerr = LG_Error.printall(self.error_handler_test, self)
        self.assertEqual(self.numerr, 0)
        self.assertFalse(hasattr(self, "gotit"))
        # Flush it.
        self.flushed = LG_Error.flush()
        self.assertEqual(self.flushed, True)
        self.numerr = LG_Error.printall(self.error_handler_test, self)
        self.assertEqual(self.numerr, 1)
        self.assertRegexpMatches(self.errinfo.text, "partial")

    def test_50_set_orig_error_handler(self):
        # Set the error handler back to the default handler.
        # The error message is now visible (but we cannot test that).
        self.__class__.handler["previous"] = LG_Error.set_handler(self.__class__.handler["default"])
        self.assertIsNone(self.__class__.handler["previous"])
        for _ in range(0, 1+self.testleaks):
            self.__class__.handler["previous"] = LG_Error.set_handler(self.__class__.handler["default"])
        self.assertEqual(self.__class__.handler["previous"].__name__, "_default_handler")

        self.errinfo = "dummy"
        dummy_lang = "a dummy dict name (default handler test)"
        save_stderr = divert_start(2)
        self.assertRaises(LG_Error, Dictionary, dummy_lang)
        self.assertIn(dummy_lang, save_stderr.divert_end())
        self.assertEqual(self.errinfo, "dummy")

class FSATsolverTestCase(unittest.TestCase):
    def setUp(self):
        self.d, self.po = Dictionary(lang='en'), ParseOptions()
        self.po = ParseOptions(use_sat=True)
        if self.po.use_sat != True:
            raise unittest.SkipTest("Library not configured with SAT parser")

    def test_SAT_getting_links(self):
        linkage_testfile(self, self.d, self.po, 'sat')

class HEnglishLinkageTestCase(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.d, cls.po = Dictionary(), ParseOptions(linkage_limit=300, display_morphology=False)

    @classmethod
    def tearDownClass(cls):
        del cls.d, cls.po

    def parse_sent(self, text):
        return list(Sentence(text, self.d, self.po).parse())

    def test_a_getting_words(self):
        self.assertEqual(list(self.parse_sent('This is a sentence.')[0].words()),
             ['LEFT-WALL', 'this.p', 'is.v', 'a', 'sentence.n', '.', 'RIGHT-WALL'])

    def test_b_getting_num_of_words(self):
        #Words include punctuation and a 'LEFT-WALL' and 'RIGHT_WALL'
        self.assertEqual(self.parse_sent('This is a sentence.')[0].num_of_words(), 7)

    def test_c_getting_links(self):
        sent = 'This is a sentence.'
        linkage = self.parse_sent(sent)[0]
        self.assertEqual(linkage.link(0),
                         Link(linkage, 0, 'LEFT-WALL','Xp','Xp','.'))
        self.assertEqual(linkage.link(1),
                         Link(linkage, 1, 'LEFT-WALL','hWV','dWV','is.v'))
        self.assertEqual(linkage.link(2),
                         Link(linkage, 2, 'LEFT-WALL','hWd','Wd','this.p'))
        self.assertEqual(linkage.link(3),
                         Link(linkage, 3, 'this.p','Ss*b','Ss','is.v'))
        self.assertEqual(linkage.link(4),
                         Link(linkage, 4, 'is.v','O*m','Os','sentence.n'))
        self.assertEqual(linkage.link(5),
                         Link(linkage, 5, 'a','Ds**c','Ds**c','sentence.n'))
        self.assertEqual(linkage.link(6),
                         Link(linkage, 6, '.','RW','RW','RIGHT-WALL'))

    def test_d_spell_guessing_on(self):
        self.po.spell_guess = 7
        if self.po.spell_guess == 0:
            raise unittest.SkipTest("Library is not configured with spell guess")
        result = self.parse_sent("I love going to shoop.")
        resultx = result[0] if result else []
        for resultx in result:
            if resultx.word(5) == 'shop[~].v':
                break
        self.assertEqual(list(resultx.words()) if resultx else [],
             ['LEFT-WALL', 'I.p', 'love.v', 'going.v', 'to.r', 'shop[~].v', '.', 'RIGHT-WALL'])

    def test_e_spell_guessing_off(self):
        self.po.spell_guess = 0
        result = self.parse_sent("I love going to shoop.")
        self.assertEqual(list(result[0].words()),
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
        self.assertEqual(list(self.parse_sent('Let\'s eat.')[0].words()),
             ['LEFT-WALL', 'let\'s', 'eat.v', '.', 'RIGHT-WALL'])

        # He's is split into two words, he is in dict, lower-case only.
        self.assertEqual(list(self.parse_sent('He\'s going.')[0].words()),
             ['LEFT-WALL', 'he', '\'s.v', 'going.v', '.', 'RIGHT-WALL'])

        self.assertEqual(list(self.parse_sent('You\'re going?')[0].words()),
             ['LEFT-WALL', 'you', '\'re', 'going.v', '?', 'RIGHT-WALL'])

        # Jumbo only in dict as adjective, lower-case, but not noun.
        self.assertEqual(list(self.parse_sent('Jumbo\'s going?')[0].words()),
             ['LEFT-WALL', 'Jumbo[!]', '\'s.v', 'going.v', '?', 'RIGHT-WALL'])

        self.assertEqual(list(self.parse_sent('Jumbo\'s shoe fell off.')[0].words()),
             ['LEFT-WALL', 'Jumbo[!]',
              '\'s.p', 'shoe.n', 'fell.v-d', 'off', '.', 'RIGHT-WALL'])

        self.assertEqual(list(self.parse_sent('Jumbo sat down.')[0].words()),
             ['LEFT-WALL', 'Jumbo[!]', 'sat.v-d', 'down.r', '.', 'RIGHT-WALL'])

        # Red is in dict, lower-case, as noun, too.
        # There's no way to really know, syntactically, that Red
        # should be taken as a proper noun (given name).
        #self.assertEqual(list(self.parse_sent('Red\'s going?')[0].words()),
        #     ['LEFT-WALL', 'Red[!]', '\'s.v', 'going.v', '?', 'RIGHT-WALL'])
        #
        #self.assertEqual(list(self.parse_sent('Red\'s shoe fell off.')[0].words()),
        #     ['LEFT-WALL', 'Red[!]',
        #      '\'s.p', 'shoe.n', 'fell.v-d', 'off', '.', 'RIGHT-WALL'])
        #
        #self.assertEqual(list(self.parse_sent('Red sat down.')[1].words()),
        #     ['LEFT-WALL', 'Red[!]', 'sat.v-d', 'down.r', '.', 'RIGHT-WALL'])

        # May in dict as noun, capitalized, and as lower-case verb.
        self.assertEqual(list(self.parse_sent('May\'s going?')[0].words()),
             ['LEFT-WALL', 'May.f', '\'s.v', 'going.v', '?', 'RIGHT-WALL'])

        self.assertEqual(list(self.parse_sent('May sat down.')[0].words()),
             ['LEFT-WALL', 'May.f', 'sat.v-d', 'down.r', '.', 'RIGHT-WALL'])

        # McGyver is not in the dict, but is regex-matched.
        self.assertEqual(list(self.parse_sent('McGyver\'s going?')[0].words()),
             ['LEFT-WALL', 'McGyver[!]', '\'s.v', 'going.v', '?', 'RIGHT-WALL'])

        self.assertEqual(list(self.parse_sent('McGyver\'s shoe fell off.')[0].words()),
             ['LEFT-WALL', 'McGyver[!]',
              '\'s.p', 'shoe.n', 'fell.v-d', 'off', '.', 'RIGHT-WALL'])

        self.assertEqual(list(self.parse_sent('McGyver sat down.')[0].words()),
             ['LEFT-WALL', 'McGyver[!]', 'sat.v-d', 'down.r', '.', 'RIGHT-WALL'])

        self.assertEqual(list(self.parse_sent('McGyver Industries stock declined.')[0].words()),
             ['LEFT-WALL', 'McGyver[!]', 'Industries[!]',
              'stock.n-u', 'declined.v-d', '.', 'RIGHT-WALL'])

        # King in dict as both upper and lower case.
        self.assertEqual(list(self.parse_sent('King Industries stock declined.')[0].words()),
             ['LEFT-WALL', 'King.b', 'Industries[!]',
              'stock.n-u', 'declined.v-d', '.', 'RIGHT-WALL'])

        # Jumbo in dict only lower-case, as adjective
        self.assertEqual(list(self.parse_sent('Jumbo Industries stock declined.')[0].words()),
             ['LEFT-WALL', 'Jumbo[!]', 'Industries[!]',
              'stock.n-u', 'declined.v-d', '.', 'RIGHT-WALL'])

        # Thomas in dict only as upper case.
        self.assertEqual(list(self.parse_sent('Thomas Industries stock declined.')[0].words()),
             ['LEFT-WALL', 'Thomas.b', 'Industries[!]',
              'stock.n-u', 'declined.v-d', '.', 'RIGHT-WALL'])

    # Some parses are fractionally preferred over others...
    def test_g_fractions(self):
        self.assertEqual(list(self.parse_sent('A player who is injured has to leave the field')[0].words()),
             ['LEFT-WALL', 'a', 'player.n', 'who', 'is.v', 'injured.v-d', 'has.v', 'to.r', 'leave.v', 'the', 'field.n', 'RIGHT-WALL'])

        self.assertEqual(list(self.parse_sent('They ate a special curry which was recommended by the restaurant\'s owner')[0].words()),
             ['LEFT-WALL', 'they', 'ate.v-d', 'a', 'special.a', 'curry.s',
              'which', 'was.v-d', 'recommended.v-d', 'by', 'the', 'restaurant.n',
              '\'s.p', 'owner.n', 'RIGHT-WALL'])

    # Verify that we are getting the linkages that we want
    # See below, remainder of parses are in text files
    def test_h_getting_links(self):
        sent = 'Scientists sometimes may repeat experiments or use groups.'
        linkage = self.parse_sent(sent)[0]
        self.assertEqual(linkage.diagram(),
"\n    +---------------------------------------Xp--------------------------------------+"
"\n    +---------------------------->WV---------------------------->+                  |"
"\n    |           +-----------------------Sp-----------------------+                  |"
"\n    |           |                  +------------VJlpi------------+                  |"
"\n    +---->Wd----+          +---E---+---I---+----Op----+          +VJrpi+---Op--+    |"
"\n    |           |          |       |       |          |          |     |       |    |"
"\nLEFT-WALL scientists.n sometimes may.v repeat.v experiments.n or.j-v use.v groups.n ."
"\n\n")
        sent = 'I enjoy eating bass.'
        linkage = self.parse_sent(sent)[0]
        self.assertEqual(linkage.diagram(),
"\n    +-----------------Xp----------------+"
"\n    +---->WV---->+                      |"
"\n    +->Wd--+-Sp*i+---Pg---+---Ou---+    |"
"\n    |      |     |        |        |    |"
"\nLEFT-WALL I.p enjoy.v eating.v bass.n-u ."
"\n\n")


        sent = 'We are from the planet Gorpon'
        linkage = self.parse_sent(sent)[0]
        self.assertEqual(linkage.diagram(),
"\n    +--->WV--->+     +---------Js--------+"
"\n    +->Wd--+Spx+--Pp-+   +--DD--+---GN---+"
"\n    |      |   |     |   |      |        |"
"\nLEFT-WALL we are.v from the planet.n Gorpon[!]"
"\n\n")

class GSQLDictTestCase(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if os.name == 'nt' and \
                -1 == clg.linkgrammar_get_configuration().lower().find('mingw'):
            raise unittest.SkipTest("No SQL dict support yet on the MSVC build")

        #clg.parse_options_set_verbosity(clg.parse_options_create(), 3)
        cls.d, cls.po = Dictionary(lang='demo-sql'), ParseOptions()

    @classmethod
    def tearDownClass(cls):
        del cls.d, cls.po

    def test_getting_links(self):
        linkage_testfile(self, self.d, self.po)

    def test_getting_links_sat(self):
        sat_po = ParseOptions(use_sat=True)
        if sat_po.use_sat != True:
            raise unittest.SkipTest("Library not configured with SAT parser")
        linkage_testfile(self, self.d, sat_po)

class IWordPositionTestCase(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.d_en = Dictionary(lang='en')

    @classmethod
    def tearDownClass(cls):
        del cls.d_en

    def test_en_word_positions(self):
        linkage_testfile(self, self.d_en, ParseOptions(), 'pos')

    def test_en_spell_word_positions(self):
        po = ParseOptions(spell_guess=1)
        if po.spell_guess == 0:
            raise unittest.SkipTest("Library is not configured with spell guess")
        linkage_testfile(self, self.d_en, po, 'pos-spell')

    def test_ru_word_positions(self):
        linkage_testfile(self, Dictionary(lang='ru'), ParseOptions(), 'pos')

    def test_he_word_positions(self):
        linkage_testfile(self, Dictionary(lang='he'), ParseOptions(), 'pos')

# Tests are run in alphabetical order; do the language tests last.

class ZENLangTestCase(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.d, cls.po = Dictionary(lang='en'), ParseOptions()

    @classmethod
    def tearDownClass(cls):
        del cls.d, cls.po

    def test_getting_links(self):
        linkage_testfile(self, self.d, self.po)

    def test_quotes(self):
        linkage_testfile(self, self.d, self.po, 'quotes')

    def test_null_link_range_starting_with_zero(self):
        """Test parsing with a minimal number of null-links, including 0."""
        # This sentence has no complete linkage. Validate that the library
        # doesn't mangle parsing with null-count>0 due to power_prune()'s
        # connector-discard optimization at null-count==0.  Without commit
        # "Allow calling classic_parse() with and w/o nulls", the number of
        # linkages here is 1 instead of 2 and the unused_word_cost is 5.
        self.po = ParseOptions(min_null_count=0, max_null_count=999)
        linkages = Sentence('about people attended', self.d, self.po).parse()
        self.assertEqual(len(linkages), 2)
        self.assertEqual(linkages.next().unused_word_cost(), 1)
        # Expected parses:
        # 1:
        #    +------------>WV------------>+
        #    +--------Wd-------+----Sp----+
        #    |                 |          |
        #LEFT-WALL [about] people.p attended.v-d
        # 2:
        #
        #            +----Sp----+
        #            |          |
        #[about] people.p attended.v-d

    def test_2_step_parsing_with_null_links(self):
        self.po = ParseOptions(min_null_count=0, max_null_count=0)

        sent = Sentence('about people attended', self.d, self.po)
        linkages = sent.parse()
        self.assertEqual(len(linkages), 0)
        self.po = ParseOptions(min_null_count=1, max_null_count=999)
        linkages = sent.parse(self.po)
        self.assertEqual(len(linkages), 2)
        self.assertEqual(linkages.next().unused_word_cost(), 1)

class JADictionaryLocaleTestCase(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if is_python2(): # Locale stuff seems to be broken
            raise unittest.SkipTest("Test not supported with Python2")

        # python2: Gets system locale (getlocale() is not better)
        cls.oldlocale = locale.setlocale(locale.LC_CTYPE, None)
        #print('Current locale:', oldlocale)
        #print('toupper hij:', 'hij'.upper())

        tr_locale = 'tr_TR.UTF-8' if os.name != 'nt' else 'Turkish'
        try:
            locale.setlocale(locale.LC_CTYPE, tr_locale)
        except locale.Error as e: # Most probably tr_TR.UTF-8 is not installed
            raise unittest.SkipTest("Locale {}: {}".format(tr_locale, e))

        #print('Turkish locale:', locale.setlocale(locale.LC_CTYPE, None))
        # python2: prints HiJ (lowercase small i in the middle)
        #print('toupper hij:', 'hij'.upper())

        cls.d, cls.po = Dictionary(lang='en'), ParseOptions()

    @classmethod
    def tearDownClass(cls):
        locale.setlocale(locale.LC_CTYPE, cls.oldlocale)
        #print("Restored locale:", locale.setlocale(locale.LC_CTYPE))
        #print('toupper hij:', 'hij'.upper())
        del cls.d, cls.po, cls.oldlocale

    def test_dictionary_locale_definition(self):
        linkage = Sentence('Is it fine?', self.d, self.po).parse().next()
        self.assertEqual(list(linkage.words())[1], 'is.v')

# FIXME: Use a special testing dictionary for checks like that.
class JBDictCostReadingTestCase(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if is_python2(): # Locale stuff seems to be broken
            raise unittest.SkipTest("Test not supported with Python2")

        cls.oldlocale = locale.setlocale(locale.LC_CTYPE, None)
        ru_locale = 'ru_RU.UTF-8' if os.name != 'nt' else 'Russian'
        try:
            locale.setlocale(locale.LC_NUMERIC, ru_locale)
        except locale.Error as e: # Most probably ru_RU.UTF-8 is not installed
            del cls.oldlocale
            raise unittest.SkipTest("Locale {}: {}".format(ru_locale, e))
        # The dict read must be performed after the locale change.
        cls.d, cls.po = Dictionary(lang='en'), ParseOptions()

    @classmethod
    def tearDownClass(cls):
        locale.setlocale(locale.LC_CTYPE, cls.oldlocale)
        del cls.d, cls.po, cls.oldlocale

    # When a comma-separator LC_NUMERIC affects the dict cost conversion,
    # the 4th word is 'white.v'.
    def test_cost_sensitive_parse(self):
        linkage = Sentence('Is the bed white?', self.d, self.po).parse().next()
        self.assertEqual(list(linkage.words())[4], 'white.a')

class ZENConstituentsCase(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.d, cls.po = Dictionary(lang='en'), ParseOptions()

    @classmethod
    def tearDownClass(cls):
        del cls.d, cls.po

    def test_a_constituents_after_parse_list(self):
        """
        Validate that the post-processing data of the first linkage is not
        getting clobbered by later linkages.
        """
        linkages = list(Sentence("This is a test.", self.d, self.po).parse())
        self.assertEqual(linkages[0].constituent_tree(),
                "(S (NP this.p)\n   (VP is.v\n       (NP a test.n))\n   .)\n")

class ZDELangTestCase(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.d, cls.po = Dictionary(lang='de'), ParseOptions()

    @classmethod
    def tearDownClass(cls):
        del cls.d, cls.po

    def parse_sent(self, text):
        return list(Sentence(text, self.d, self.po).parse())

    def test_a_getting_num_of_words(self):
        #Words include punctuation and a 'LEFT-WALL' and 'RIGHT_WALL'
        self.assertEqual(self.parse_sent('Dies ist den Traum.')[0].num_of_words(), 7)
        self.assertEqual(self.parse_sent('Der Hund jagte ihn durch den Park.')[0].num_of_words(), 10)

    def test_b_getting_words(self):
        self.assertEqual(list(self.parse_sent('Der Hund jagte ihn durch den Park.')[0].words()),
            ['LEFT-WALL', 'der.d', 'Hund.n', 'jagte.s', 'ihn', 'durch',
               'den.d', 'Park.n', '.', 'RIGHT-WALL'])

    def test_c_getting_links(self):
        sent = 'Dies ist den Traum.'
        linkage = self.parse_sent(sent)[0]
        self.assertEqual(linkage.link(0),
                         Link(linkage, 0, 'LEFT-WALL','Xp','Xp','.'))
        self.assertEqual(linkage.link(1),
                         Link(linkage, 1, 'LEFT-WALL','W','W','ist.v'))
        self.assertEqual(linkage.link(2),
                         Link(linkage, 2, 'dies','Ss','Ss','ist.v'))
        self.assertEqual(linkage.link(3),
                         Link(linkage, 3, 'ist.v','O','O','Traum.n'))
        self.assertEqual(linkage.link(4),
                         Link(linkage, 4, 'den.d','Dam','Dam','Traum.n'))
        self.assertEqual(linkage.link(5),
                         Link(linkage, 5, '.','RW','RW','RIGHT-WALL'))

class ZLTLangTestCase(unittest.TestCase):
    def setUp(self):
        self.d, self.po = Dictionary(lang='lt'), ParseOptions()

    # Reads linkages from a test-file.
    def test_getting_links(self):
        linkage_testfile(self, self.d, self.po)

# Tests are run in alphabetical order; do the language tests last.
class ZRULangTestCase(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.d, cls.po = Dictionary(lang='ru'), ParseOptions()

    @classmethod
    def tearDownClass(cls):
        del cls.d, cls.po

    def parse_sent(self, text):
        return list(Sentence(text, self.d, self.po).parse())

    def test_a_getting_num_of_words(self):
        self.po.display_morphology = False
        #Words include punctuation and a 'LEFT-WALL' and 'RIGHT_WALL'
        self.assertEqual(self.parse_sent('это тести.')[0].num_of_words(), 5)
        self.assertEqual(self.parse_sent('вверху плыли редкие облачка.')[0].num_of_words(), 7)

    def test_b_getting_words(self):
        self.po.display_morphology = False
        self.assertEqual(list(self.parse_sent('вверху плыли редкие облачка.')[0].words()),
            ['LEFT-WALL', 'вверху.e', 'плыли.vnndpp', 'редкие.api',
                'облачка.ndnpi', '.', 'RIGHT-WALL'])

    def test_c_getting_links(self):
        self.po.display_morphology = False
        sent = 'вверху плыли редкие облачка.'
        linkage = self.parse_sent(sent)[0]
        self.assertEqual(linkage.link(0),
                         Link(linkage, 0, 'LEFT-WALL','Xp','Xp','.'))
        self.assertEqual(linkage.link(1),
                         Link(linkage, 1, 'LEFT-WALL','W','Wd','плыли.vnndpp'))
        self.assertEqual(linkage.link(2),
                         Link(linkage, 2, 'вверху.e','EI','EI','плыли.vnndpp'))
        self.assertEqual(linkage.link(3),
                         Link(linkage, 3, 'плыли.vnndpp','SIp','SIp','облачка.ndnpi'))
        self.assertEqual(linkage.link(4),
                         Link(linkage, 4, 'редкие.api','Api','Api','облачка.ndnpi'))
        self.assertEqual(linkage.link(5),
                         Link(linkage, 5, '.','RW','RW','RIGHT-WALL'))

    # Expect morphological splitting to apply.
    def test_d_morphology(self):
        self.po.display_morphology = True
        self.assertEqual(list(self.parse_sent('вверху плыли редкие облачка.')[0].words()),
            ['LEFT-WALL',
             'вверху.e',
             'плы.=', '=ли.vnndpp',
             'ре.=', '=дкие.api',
             'облачк.=', '=а.ndnpi',
             '.', 'RIGHT-WALL'])

def linkage_testfile(self, lgdict, popt, desc = ''):
    """
    Reads sentences and their corresponding
    linkage diagrams / constituent printings.
    """
    self.__class__.longMessage = True
    self.maxDiff = None
    if desc != '':
        desc = desc + '-'
    testfile = clg.test_data_srcdir + "parses-" + desc + clg.dictionary_get_lang(lgdict._obj) + ".txt"
    parses = open(testfile, "rb")
    diagram = None
    constituents = None
    wordpos = None
    sent = None
    lineno = 0
    last_opcode = None

    def getwordpos(lkg):
        words_char = []
        words_byte = []
        for wi, w in enumerate(lkg.words()):
            words_char.append(w + str((linkage.word_char_start(wi), linkage.word_char_end(wi))))
            words_byte.append(w + str((linkage.word_byte_start(wi), linkage.word_byte_end(wi))))
        return ' '.join(words_char) + '\n' + ' '.join(words_byte) + '\n'

    # Function code and file format sanity check
    def validate_opcode(opcode):
        if opcode != ord('O'):
            self.assertFalse(diagram, "at {}:{}: Unfinished diagram entry".format(testfile, lineno))
        if opcode != ord('C'):
            self.assertFalse(constituents, "at {}:{}: Unfinished constituents entry".format(testfile, lineno))
        if opcode != ord('P'):
            self.assertFalse(wordpos, "at {}:{}: Unfinished word-position entry".format(testfile, lineno))

    for line in parses:
        lineno += 1
        if not is_python2():
            line = line.decode('utf-8')

        validate_opcode(ord(line[0])) # Use ord() for python2/3 compatibility
        prev_opcode = last_opcode
        if line[0] in 'INOCP':
            last_opcode = line[0]

        # Lines starting with I are the input sentences
        if line[0] == 'I':
            sent = line[1:].rstrip('\r\n') # Strip whitespace before RIGHT-WALL (for P)
            diagram = ""
            constituents = ""
            wordpos = ""
            linkages = Sentence(sent, lgdict, popt).parse()
            linkage = next(linkages, None)
            self.assertTrue(linkage, "at {}:{}: Sentence has no linkages".format(testfile, lineno))

        # Generate the next linkage of the last input sentence
        elif line[0] == 'N':
            diagram = ""
            constituents = ""
            wordpos = ""
            linkage = next(linkages, None)
            self.assertTrue(linkage, "at {}:{}: Sentence has too few linkages".format(testfile, lineno))

        # Lines starting with O are the parse diagram
        # It ends with an empty line
        elif line[0] == 'O':
            diagram += line[1:]
            if line[1] == '\n' and len(diagram) > 1:
                self.assertEqual(linkage.diagram(), diagram, "at {}:{}".format(testfile, lineno))
                diagram = None

        # Lines starting with C are the constituent output (type 1)
        # It ends with an empty line
        elif line[0] == 'C':
            if line[1] == '\n' and len(constituents) > 1:
                self.assertEqual(linkage.constituent_tree(), constituents, "at {}:{}".format(testfile, lineno))
                constituents = None
            else:
                constituents += line[1:]

        # Lines starting with P contain word positions "word(start, end) ... "
        # The first P line contains character positions
        # The second P line contains byte positions
        # It ends with an empty line
        elif line[0] == 'P':
            if line[1] == '\n' and len(wordpos) > 1:
                self.assertEqual(getwordpos(linkage), wordpos, "at {}:{}".format(testfile, lineno))
                wordpos = None
            else:
                wordpos += line[1:]

        # Lines starting with "-" contain a Parse Option
        elif line[0] == '-':
                exec('popt.' + line[1:]) in {}, locals()

        elif line[0] in '%\r\n':
            pass
        else:
            self.fail('\nTest file "{}": Invalid opcode "{}" (ord={})'.format(testfile, line[0], ord(line[0])))

    parses.close()

    self.assertIn(last_opcode , 'OCP', "Missing result comparison in " + testfile)

def warning(*msg):
    progname = os.path.basename(sys.argv[0])
    print("{}: Warning:".format(progname), *msg, file=sys.stderr)

def is_python2():
    return sys.version_info[:1] == (2,)


import tempfile

class divert_start(object):
    """ Output diversion. """
    def __init__(self, fd):
        """ Divert a file descriptor.
        The created object is used for restoring the original file descriptor.
        """
        self.fd = fd
        self.savedfd = os.dup(fd)
        (newfd, self.filename) = tempfile.mkstemp(text=False)
        os.dup2(newfd, fd)
        os.close(newfd)

    def divert_end(self):
        """ Restore a previous diversion and return its content. """
        if not self.filename:
            return ""
        os.lseek(self.fd, os.SEEK_SET, 0)
        content = os.read(self.fd, 1024) # 1024 is more than needed
        os.dup2(self.savedfd, self.fd)
        os.close(self.savedfd)
        os.unlink(self.filename)
        self.filename = None
        return str(content)

    __del__ = divert_end

# Decorate Sentence.parse with eqcost_sorted_parse.
lg_testutils.add_eqcost_linkage_order(Sentence)

unittest.main()
