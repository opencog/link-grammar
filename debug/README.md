Debugging
=========

There link-grammar library has API calls to ease debugging and development.
The `link-parser` program has corresponding options for this API.

Only the `link-parser` options will be discussed here.
Options to `link-parser` at the command-line are preceded with a `-` sign.
You can use a unique prefix of an option name instead of its full name. At
the **linkparser>** prompt or in batch files, options are preceded with
a `!` character.

For info on common options, see the "Special ! options" of the `link-grammar`
manual. For a general help message use `link-parser -help`.


Debug options
-------------

### 1) -verbosity=N (-v=N)
Sets the verbosity level of the library to N (a small non-negative integer).

#### Verbosity levels
0: Certain informative messages are not printed by the
library. `link-parser` also doesn't print its usual **linkparser>**
prompt. This is the current default verbosity level for the Python
binding.

1: This is the library default. This is also the default for
`link-parser`.

2: Display parsing steps time. In case an error/warning gets issued by the
library, this may help finding out at which step it happened.

3: Display some more Info messages:
- Freeing dictionaries.
- Number of insane-morphism linkages.
- A warning when all the linkages have a PP violation.

4: Display data file search and locale setup. It can be used to debug
problems with the locale setup or in finding the dictionary.

5-9: Show trace and debug messages regarding sentence handling. Higher
levels include the messages of the lower ones.

10-99: Show also trace and debug messages regarding reading the
dictionary.  As with levels greater then 4, higher levels include the
messages of the lower ones.

* 10: Basic dictionary debug.

100-...: Show only messages exactly at the specified level.
* 101: Print all the connectors, along with their length limit.
       A length limit of 0 means the value of the `short_length` option is
       used.

* 102: Print all disjuncts before and after pruning.

* 103: Show unsubscripted dictionary words and subscripted ones which share
       the same base word.

* 104: Memory pool statistics.

### 2) -debug=LOCATIONS (-de=LOCATIONS)
Show only messages from these LOCATIONS. The LOCATIONS string is a
comma-separated list of source file names (without specifying their
directory) and function names (fully qualified for C++) from which to
show the messages.

For example, to only show messages from the `flatten_wordgraph()` function
or the print.c file:

`link-parser -v=6 -debug=flatten_wordgraph,print.c`

Note that since print.c is used to produce certain messages, it is
currently needed to add it to the debug LOCATIONS list unless you
explicitly specify also the function in print.c (to further restrict
the messages).

### 3) -test=FEATURES (-te=FEATURES)
Enable certain features. These can be debug aids, or new features that
are not yet official or fully-developed.

For example, to automatically show all linkages of a sentence, the
following can be done:

`link-parser -test=auto-next-linkage`

`link-parser` warns when tests are enabled. This way it is possible to see in
the linkage output which tests were enabled. This is particularly important
when examining output files. However, when doing benchmarks (with and w/o
tests) this is not desired because these warnings skew the timing.
If needed, suppress this warnings with the added special tests `@`, as in:
`-test=@,one-step-parse`.

Useful examples
---------------

### -debug=...

1) See the tokens after flattening into the word array used by the parser:

```
echo "Let's test it" | \
link-parser -v=6 -debug=flatten_wordgraph,print_sentence_word_alternatives
```

2) Trace the work of `sane_linkage_morphism()`:

`link-parser -v=8 -debug=sane_linkage_morphism`

3) Same as (2) above, but also see other messages from sane.c:

`link-parser -v=8 -debug=sane.c`

(`sane_linkage_morphism()` happens to be in `sane.c` so this includes its
messages.)

4) Debug the tokenizer:

`link-parser -v=7 -debug=tokenizer.c`

Or, in order to display the word array:

`link-parser -v=7 -debug=tokenize.c,print_sentence_word_alternatives`

5) Debug post-processing:

`link-parser -v=9 -debug=post-process.c`

6) Debug expression pruning:

`link-parser -v=9 -debug=expression_prune`

7) Debug reading the affix and knowledge files:

`link-parser -v=11`

### -test=...

1) Automatically show all linkages:

`link-parser -test=auto-next-linkage`
Try to type some sentences at the **linkparser>** prompt to see its action.

2) Print more that 1024 linkages in `link-parser` (this is the maximum
`link-parser` would print by default), e.g. 20000:

`link-parser -test=auto-next-linkage:20000`

3) To print detailed linkages of **data/en/corpus-basic.batch**:

```
sed '/^*/d;/^!const/d;/^!batch/d' data/en/corpus-basic.batch | \
link-parser -test=auto-next-linkage
```

(If you cut&paste it to a terminal, remember to escape each of the "**!**"
characters with a backslash.)

This, along with "diff", "grep" etc., can be used in order to validate
that a change didn't cause undesired effects. Special care should be taken
if sentences with more than 1024 linkages are to be verified too (use a
larger `-limit=N` and `-test=auto-next-linkage:M`, when N>>M).

Note that this technique is not very effective if the order to the
linkages got changed (or if SAT-parser linkages need to be compared to the
classic-parser linkages). In that case the detailed linkages results need
to be filtered through a script which sorts them according to some
"canonical order" and also removes duplicates.

4) Display the wordgraph using `-wordgraph=N`, optionally using additional
wordgraph-display flags with `-test=wg:FLAGS`.

For more examples of how to use the wordgraph-display, see
[link-grammar/tokenize/README.md]
(/link-grammar/tokenize/README.md#word-graph-display)
and [msvc/README.md](/msvc/README.md).

5) Test the "trailing connector" hashing for short sentences too (e.g. for
all sentences with more than 10 tokens):
`link-parser test=len-trailing-hash:10`
Or optionally (in order to see relevant debug messages from `preparation.c`):
`link-parser test=len-trailing-hash:10 -v=5 -debug=preparation.c`

6) -test=<values> for SAT parser debugging:
`linkage-disconnected` - Display also solutions which don't have a full linkage.
`sat-stats` - Display the number of PP-violations and disconnected linkages.
`no-pp_pruning_1` - Disable a partial CONTAINS_NONE_RULES pruning

7)  -test=<values> for the pruning subsystem:
`len-multi-pruning:N` - Prune per null_count for more than N-token sentences.
`always-parse` - Don't use a parse shortcut and always fully prune.
`no-mlink` - Don't prune using an mlink table.

Debugging and STDIO streams
---------------------------
Messages at severity Info and higher (i.e. also Warning, Error and
Fatal) are printed to `stderr`. The other severities
(at Debug and below, i.e also
Trace and None) are printed to `stdout`. The rational is that
debugging messages, in order to be useful, need to appear along with the
regular output of the program, while errors are exceptional and need to
stand out when `link-parser`s `stdout` is redirected to a file.

The C API includes the ability to set the severity level threshold above
which messages are printed to `stderr` (see
"Improved error notification facility"->"C API" in
[link-grammar/README.md](/link-grammar/README.md)).

Note that when debugging errors during a sentence batch run, it may be useful
to redirect also `stderr` to the same file (the error facility of the library
flushes `stdout` before printing in order to preserve output order).

Using debugger
--------------

### Configuring for debug

`configure --enable-debug`

Its sets the DEBUG definitions and removes the optimization flags of the
compiler. The DEBUG definition adds various validity checks, test
messages, and some debug functions (that can be invoked, for example, from
the debugger).


| `gdb` command | Description |
|---------------|-------------|
| <pre>call wordgraph_show(sent, "") | If something goes wrong, it is may be useful to display the wordgraph. The second argument can include wordgraph display options.|
| <pre>call print_all_disjuncts(sent) | Print the disjuncts. |


FIXME: Document more debug functions.

Compilation definitions
-----------------------
Some debug-related compilation flags can be set using `configure`, to as `make`
arguments:

| Definition | Description |
| ---------- |-------------|
| `NO_SAN_DICT` | Don't use ASAN/UBSAN for dict reading. This cause a vast startup speedup when using ASAN/UBSAN. It is optional because it shouldn't normally be used when debugging the dictionary code. |
|`POOL_ALLOCATOR=0` | Pool allocator debug facility: A fake pool allocator that uses `malloc()` for each allocation is defined, in order that ASAN or valgrind can be used to find memory usage bugs. |
|`TRACON_SET_DEBUG` | Print tracon_set stats. |
|`DEBUG_PP_PRUNE` | PP pruning debug printout. |
|`DEBUG_TABLE_STAT`| print count table stats. |
|`DO_COUNT_TRACE` | Detailed trace of do_count. |
|`DEBUG_X_TABLE` | Print x_table stats. |

### Specific SAT-parser debug
| Definition | Description |
| ---------- |-------------|
| `CONNECTIVITY_DEBUG` |  Debug SAT connectivity . |
| `SAT_DEBUG`, `VARS` | Debug variables. |
