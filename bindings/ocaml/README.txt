INTRODUCTION
------------

This package contains the source for Ocaml 
interface to LinkGrammar. It enables Ocaml 
applications to use LinkGrammar to parse 
English sentences.

For more information on LinkGrammar refer
http://www.link.cs.cmu.edu/link/. The Ocaml API
maps closely to LinkGrammar's C API

This package interfaces to the LinkGrammar C-API
using the OCaml-C communication. It requires LinkGrammar 4.1b  
to be installed.

WARNING

This library was only tested on a Win XP machine
using the mingw toolchain


PACKAGE
-------

Files are:
  - README.txt this file
  - the library:
      . linkgrammar.mli      Interface to LinkGrammar
      . linkgrammar.ml       Implementation
      . linkgrammar.c        OCaml API to LinkGrammar C API Interfacing
  - API application:
      . utBatch.ml       API unit tests. Parses sentences from file
      . utApi.ml         Unit Tests of API functions


INSTALLATION
------------

1. Download and build LinkGrammar software 
   say in directory ./link-4.1b/
2. Make an archive of the LinkGrammar objects
   ar r link.a obj/analyze-linkage.o obj/and.o obj/api.o ... etc
include all objects except parse.o in the archive
3. Download OcamlLinkgrammar files into say /OcamlLinkGrammar/
4. build the C file
gcc -I../link-4.1b/include -I/c/Program\ Files/Objective\ Caml/lib -c linkgrammar.c
4. build the ocaml library
   ocamlc -c linkgrammar.mli
   ocamlc -custom linkgrammar.o ../link-4.1b/link.a -a -o linkgrammar.cma linkgrammar.ml
5. build the applications
   ocamlc -o utBatch linkgrammar.cma utBatch.ml
   ocamlc -o utApi linkgrammar.cma utApi.ml	


DOCUMENTATION
-------------

This OCaml API closely follows the LinkGrammar C API. Please refer
http://www.link.cs.cmu.edu/link/api/index.html for details of the API usage


**** Special Note ****

The "dictCreate" API call requires the LinkGrammar dictionary file names.
Unfortunately, there seems to be no portable way of specifying these
files. Hence applications have to run in the directory /link-4.1b
so that the dictionary files may be found.

LinkGrammar did allow the environment variable DICTPATH to be set
to point to the LinkGrammar dictionary directory (./link-4.1b/data).
However, the function dictopen() in file utilities.c seems to have such
functionality commented out. Hence you may want to uncomment this
code to have the DICTPATH environment variable take effect


RELATED
-------

LinkGrammar APIs for other languages are available at
http://www.link.cs.cmu.edu/link/papers/index.html

AUTHOR
------

Ramu Ramamurthy - ramu_ramamurthy at yahoo dot com

LICENSE (BSD)
-------------

Copyright (c) 2006, Ramu Ramamurthy
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of the author nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
