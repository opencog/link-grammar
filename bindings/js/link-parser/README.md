# link-parser

CLI for the CMU Link Grammar natural language parser, compiled for node with emscripten

**See the [main README for link-grammar project.](https://github.com/opencog/link-grammar#readme)**

## Installing

For simple use on your command-line console, install globally:

`npm install --global link-parser`

For use by a [portable] node process or package script, install as a package dependency:

`npm install --save link-parser`

## Usage

Same as the native-compiled program:

```
$ link-parser --help
Usage: link-parser.js [language|dictionary location]
                   [-<special "!" command>]
                   [--version]

Special commands are:
 Variable     Controls                                          Value
 --------     --------                                          -----
 bad          Display of bad linkages                           0 (Off)
 batch        Batch mode                                        0 (Off)
 constituents Generate constituent output                       0
 cost-max     Largest cost to be considered                  2.70
 disjuncts    Display of disjuncts used                         0 (Off)
 echo         Echoing of input sentence                         0 (Off)
 graphics     Graphical display of linkage                      1 (On)
 islands-ok   Use of null-linked islands                        0 (Off)
 limit        The maximum linkages processed                 1000
 links        Display of complete link data                     0 (Off)
 morphology   Display word morphology                           0 (Off)
 null         Allow null links                                  1 (On)
 panic        Use of "panic mode"                               1 (On)
 postscript   Generate postscript output                        0 (Off)
 ps-header    Generate postscript header                        0 (Off)
 rand         Use repeatable random numbers                     1 (On)
 short        Max length of short links                        16
 timeout      Abort parsing after this many seconds            30
 verbosity    Level of detail in output                         1
 debug        Comma-separated function names to debug
 test         Comma-separated test features
 walls        Display wall words                                0 (Off)
 width        The width of the display                      16381
 wordgraph    Display sentence word-graph                       0

Toggle a Boolean variable as in "!batch"; Set a variable as in "!width=100".
Get a more detailed help on a variable as in "!help var".

Overview:            https://en.wikipedia.org/wiki/Link_grammar
Home page:           https://www.abisource.com/projects/link-grammar
Documentation:       https://www.abisource.com/projects/link-grammar/dict/
Discussion group:    https://groups.google.com/d/forum/link-grammar
Report bugs to:      https://github.com/opencog/link-grammar/issues
```
