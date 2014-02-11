#!/usr/bin/python

from distutils.core import Extension
from distutils.core import setup

# Assuming you have link-grammar 4 headers in your default include path
link_grammar_ext = Extension('pylinkgrammar/_clinkgrammar',
    sources=['pylinkgrammar/link_grammar.i'],
    libraries=['link-grammar'],
)

import pylinkgrammar

setup(name='pylinkgrammar', version=pylinkgrammar.__version__,
    description='Python bindings for Link Grammar system',
    long_description=open('README').read(),
    author='MetaMetrics, Inc.',
    author_email='engineering@lexile.com',
    ext_modules=[link_grammar_ext],
    packages=['pylinkgrammar'],
    url='https://www.bitbucket.org/metametrics/pylinkgrammar/',
    classifiers = [
        'Development Status :: 4 - Beta',
        'Intended Audience :: Developers',
        'License :: OSI Approved :: BSD License',
        'Operating System :: POSIX :: Linux',
        'Operating System :: MacOS :: MacOS X',
        'Programming Language :: Python',
        'Programming Language :: Python :: 2.6',
        'Programming Language :: Python :: 2.7',
        'Topic :: Software Development :: Libraries :: Python Modules',
    ],
) 
