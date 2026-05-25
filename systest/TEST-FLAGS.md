# Maintainer Test Flags

This file documents `-test=...` / `!test=...` values used by developers and
regression tests. These flags are not stable user-facing parser options.

Use `./systest/lgtests` as the fast source inventory of active
`test_enabled()` strings and their source locations. Do not add source scans to
normal compilation just to maintain this document.

When adding, removing, renaming, or changing the semantics of a test flag,
update this file in the same commit. Active flags use headings of the form
`### <flag-name>` with the flag name in backticks, so an on-demand checker can
compare this file with
`./systest/lgtests`.

For validation-style flags, the flag should enable the check. Validation
failures should be reported even at ordinary verbosity; maintainer verbosity
should only add successful-check detail, counters, or trace output. If a flag
is mainly a report generator, document any required `-verbosity=N` threshold
explicitly.

## Parser Automation And Output Comparison

### `@`

Suppresses the `link-parser` warning that tests are enabled. This is useful
for timing comparisons where the warning itself would skew output or timing.

### `auto-next-linkage`

Automatically advances through displayed linkages. With `:N`, displays up to
`N` linkages instead of the default display cap.

### `batch-print-parse-statistics`

In batch mode, prints parse statistics instead of detailed linkage output.
Useful for corpus-counting helpers that need the number of found linkages.

### `one-step-parse`

Lets interactive parsing with null links use the first parse step instead of
requiring the normal retry path.

### `extract-order`

Prints extraction-order diagnostics, including linkage index and cost data.

### `linkage-dedup`

Controls post-extraction duplicate marking. `:0` disables deduplication and
`:1` forces it.

### `removeZZZ`

Suppresses quotation/capitalization helper links in displayed linkages so
batch outputs can be compared more easily.

## Tokenization And Wordgraph Debugging

### `dictcap`

Enables experimental dictionary-based handling of capitalized words.

### `is_entity`

When capitalized-word handling reaches an entity-like word, prints entity
diagnostics for the original sentence.

### `wg`

Provides wordgraph display flags for `!wordgraph=3`. The `:FLAGS` argument is
a string of one-letter display flags as described in tokenizer documentation.

### `gvfile`

Keeps the generated wordgraph Graphviz file instead of deleting it, for
debugging wordgraph display output.

## Dictionary And Generation Diagnostics

### `generate`

Requests generation-mode dictionary setup. The optional `:walls` argument
requests wall generation.

### `no-macro-tag`

Disables dictionary macro-tag storage.

### `disallow-dup-idioms`

Treats duplicate idiom entries as dictionary errors instead of allowing them.

### `disjunct-address`

Includes disjunct address information when printing dictionary disjuncts.

## Pruning, Counting, And Parse-Set Diagnostics

### `always-parse`

Disables a pruning shortcut and always proceeds through full parsing.

### `no-mlink`

Disables mlink-table pruning.

### `min-len-encoding`

Overrides the minimum sentence length for trailing-connector encoding. The
`:N` argument supplies the threshold.

### `len-multi-pruning`

Overrides the minimum sentence length for pruning separately per null count.
The `:N` argument supplies the threshold.

### `count-table-entries`

With the relevant debug build support, prints detailed count-table entry
statistics.

### `parse-set-count-check`

Recounts the root parse-set DAG and compares it with stored root count and
overflow state. This is a visible diagnostic, not a production behavior
change.

### `tracon-set-print`

Prints `tracon_set` diagnostics.

## SAT Parser Diagnostics

### `SAT-cost`

In debug builds, keeps SAT linkage construction from rejecting solutions only
because disjunct cost exceeds the cutoff. This helps inspect SAT cost issues.

### `linkage-disconnected`

Allows sane but disconnected SAT solutions to be displayed instead of ignored.

### `sat-stats`

Prints SAT bottleneck counters such as PP violations and disconnected
linkages.

### `no-pp_pruning_1`

Historical SAT result-comparison flag for an older partial PP-pruning path.
The referenced code is currently disabled.

## Postprocessing Diagnostics

### `noPP`

Suppresses postprocessing rules by their two-digit message IDs. The argument
form is `:AABBCC`, for example `noPP:00010207`.

### `pp-parse-set-trace`

Traces PP failures back to metric extraction `Parse_set` / `Parse_choice`
provenance.

### `pp-parse-set-trace-all-links`

Extends PP parse-set tracing to all links in the rejected linkage.

## Metric Extraction

### `no-metric-extraction`

Disables metric extraction for comparison with ordinary indexed extraction.

### `metric-extraction`

Forces metric extraction even when ordinary indexed extraction would normally
be used because the finite parse count fits inside `!limit`.

### `no-metric-mfc`

Disables the default must-form-cycle terminal-state ranker handling.

### `no-metric-bounded-domain`

Disables the default bounded-domain metric feedback.

### `no-metric-global-contains-one`

Disables the global contains-one metric mechanism.

### `no-metric-pp-constraints`

Explicitly disables knowledge-declared parser/extractor constraints.

### `metric-classic-pp`

During metric extraction, runs classic PP checks even for rule families the
metric extractor normally handles before postprocessing.

### `metric-pp-validate`

Runs parser/extractor PP-constraint predictions against batched classic PP.
Use it together with `metric-extraction` when finite parses would otherwise
use ordinary extraction.

## Removed Or Obsolete Flags

### `no-metric-global-contains-one-feedback`

Removed when learned global contains-one feedback was dropped. Static global
contains-one handling remains available through `no-metric-global-contains-one`.

### `no-metric-contains-one-domain`

Removed with the unused domain-start contains-one feedback path.

### `no-metric-contains-none-domain`

Removed with the unused domain-start contains-none feedback path.

### `metric-domain-target-verify`

Removed with the targeted domain-feedback replay verifier.

### `metric-domain-reject-signature`

Removed with the domain contains-one feedback signature diagnostic.
