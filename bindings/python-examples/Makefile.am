#
# Makefile for python examples
#
# Created by Linas Vepstas February 2014
#

# Run tests.py if the user says 'make check'
TESTS = tests.py
AM_TESTS_ENVIRONMENT = python=$(PYTHON) \
   PYTHONPATH=$(srcdir)/../python:../$${python\#\#*/}:../$${python\#\#*/}/.libs \
   LINK_GRAMMAR_DATA=$(srcdir)/../../data; \
   export PYTHONPATH LINK_GRAMMAR_DATA;

TEST_EXTENSIONS = .py
PY_LOG_COMPILER = $(PYTHON)
#AM_PY_LOG_FLAGS = -v

EXTRA_DIST =         \
   AUTHORS           \
   LICENSE           \
   README.md         \
   example.py        \
   parses-en.txt     \
   parses-lt.txt     \
   parses-quotes-en.txt \
   parses-sat-en.txt \
   tests.py
