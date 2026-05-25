# Metric-Ordered Linkage Extraction and PP Blockers

This document records the implemented design of metric-ordered linkage
extraction.  The parser uses **Viterbi-style** ranking to fetch linkages in
ascending raw metric order for linkage-limit-truncated non-generation parses.
Here, Viterbi-style means dynamic programming over the `Parse_set` DAG: each
subproblem stream is ranked by local choice cost plus child ranks, and heaps
lazily merge the next cheapest candidates.  That makes low limits such as
`!limit=1` useful for batch testing and lets the parser inspect the best
region of very large parse sets without relying on random extraction.

Ordered extraction also changes the postprocessing (PP) failure problem.  A
random extractor can jump over a long prefix of PP-invalid raw linkages; a
metric extractor can get stuck spending its request budget on that prefix.
For a PP rule that can reject many low-metric candidates, the rule must either
be moved out of classic PP or mitigated before later candidates are requested.
The implemented English solution is:

- migrate most classic PP checks into dictionary grammar;
- handle the remaining exact blockers in the extractor;
- keep bounded `s` rule 78 authoritative in classic PP, while teaching the
  extractor bounded-domain feedback from PP failures so later same-shape
  violations can be skipped before linkage materialization.

The validation described below is best-effort corpus and challenge testing,
not a mathematical proof of equivalence.

## Motivation

The old linkage-limit-truncated extraction path could sample raw linkages
randomly.  Random sampling can jump over large regions of PP-invalid raw
linkages, so it can find a displayed linkage even when many lower-metric raw
derivations are doomed by postprocessing.

Metric-ordered extraction changes that behavior intentionally: it asks for the
next raw linkage in ascending metric order, postprocesses batches of such
linkages, and keeps the PP-valid ones.  This exposes a real problem: if the
lowest-metric region is dominated by raw linkages rejected by a PP rule, the
ordered extractor can spend its request budget there before reaching valid
linkages.

The target behavior is:

- For linkage-limit-truncated non-generation parses, fetch raw candidates in
  metric order and keep the best PP-valid linkages found within the configured
  request cap.
- Keep ordinary, non-truncated extraction behavior unchanged.

The extraction metric deliberately excludes values that are not stable at raw
extraction time.  `N_violations` is assigned by postprocessing, not extraction.
`unused_word_cost` is constant inside one `Parse_set` extraction phase.  Token
count and final display ordering are not part of the current Viterbi metric.
For small finite sentences with optional tokenizer words, metric extraction
therefore uses a bounded post-morphism lookahead: it extracts the finite raw
candidate set into one PP batch, lets `sane_linkage_morphism()` and
postprocessing assign the final scores, sorts that batch, and then keeps the
requested number of displayed linkages.  This preserves ordinary first-linkage
ordering for cases such as abbreviation alternatives whose final link length
changes only after empty words are removed.

## Current Extraction Shape

Metric extraction is enabled for non-generation parses when the requested
linkage limit is smaller than the number of linkages found.  This document
calls those cases linkage-limit-truncated parses; in code, they are the cases
where `opts->linkage_limit < sent->num_linkages_found`.  Metric extraction
ranks `Parse_set` candidates lazily through metric rankers and heaps, then
extracts postprocessing batches.  If too few candidates in a batch survive PP,
the code fetches additional blocks until either enough linkages are kept,
extraction is exhausted, or the request cap is reached.  The request cap is the
maximum number of raw extraction requests the linkage-limit-truncated path will
make while trying to fill the requested displayed linkage limit.

Metric extraction keeps the batch-shaped PP interface.  Morph-sort lookahead
and validation/tripwire modes still intentionally use batches.  In the normal
production metric path, however, exact extractor-handled PP rows are
suppressed and rule 78 is the remaining classic PP authority.  Large PP
batches are therefore not required for correctness in that path, and the
classic PP first-pass link-name scan no longer gives an important
`CONTAINS_ONE`/`CONTAINS_NONE` pruning benefit.  A high-limit benchmark that
used batch size 1 for the normal rule-78 feedback path produced identical
`corpus-failures.batch` output but was measurably slower, so the current
batched implementation remains preferable.  Exact parser-side constraints
that do not need PP feedback are preferred when they can reproduce the PP rule
or accepted rule family.

The current implementation separates metric extraction from ordinary
unrestricted extraction:

- Ordinary extraction for unrestricted/non-truncated cases.
- Metric-ordered extraction for linkage-limit-truncated non-generation cases.

## Principles for PP Blocker Handling

- Prefer exact dictionary or parser grammar constraints when the condition can
  be expressed generally.  This is the best outcome because invalid parses are
  not generated.
- Prefer exact parser/ranker DP state only when dictionary grammar cannot
  express the condition.  The terminal-state MFC ranker belongs to this class.
- Use PP feedback only as an optimization while classic PP remains
  authoritative, unless the learned summary has been validated as an exact
  replacement.
- PP remains authoritative unless a replacement is exact.
- Do not silently drop valid candidates.
- When rejecting a metric heap candidate, first preserve the successor
  frontier if otherwise valid higher-rank child combinations could be hidden.
- Prefer dictionary migration only for whole PP rules, or for substantial parts
  of a credible whole-rule migration.  Sentence-specific dictionary patches are
  not acceptable replacements for PP rules.
- Minimize memory use by default.  Spend extra memory only when it gives a
  substantial speedup or avoids repeated expensive extraction or PP work.
  Production mechanisms must have predictable memory bounds, especially on
  long sentences and huge parse sets.

When a retained PP row duplicates an exact extractor rule, normal metric
extraction suppresses that classic row so it does not become a silent second
implementation.  `-test=metric-classic-pp` and `-test=metric-pp-validate`
turn those rows back into validation tripwires.

## Implemented Mechanisms

### Terminal-State MFC Ranking

`must_form_cycle` is the most visible ordered-extraction blocker.  The current
metric path carries a small terminal state through metric ranking to represent
whether a subcandidate connects its boundary and whether an unresolved cycle
obligation remains.  It keeps `mk_parse_set()` unchanged and lets the ranker
reject candidates that cannot satisfy cycle obligations.

This is enabled by default for metric extraction.  It can be disabled with
`-test=no-metric-mfc` for diagnostics and comparison.  Earlier
candidate-level MFC prefilters and feedback queues were removed because they
could spend the sentence timeout internally rejecting a very large low-metric
population before any batch reached PP feedback.

A future `do_count()` or `mk_parse_set()` DP pruning path could be faster, but
only if it preserves the same exact MFC semantics.

### Bounded-Domain Candidate Summaries

Bounded-domain feedback is enabled by default for metric extraction.  It is
the partial extractor-side handling for the remaining bounded `s` PP rule 78.
Classic PP remains authoritative: the first unlearned rule-78 violation is
still materialized and rejected by PP.  After a batch PP failure, the metric
path stores a compact reachability summary on the responsible metric
candidate.  Later candidates with the same bounded-domain shape can then be
skipped before linkage materialization.

The current English `BOUNDED_RULES` require that `s` domains do not
extend to links whose left word is before the domain root.  The metric summary
mirrors that domain reachability after learning the exact domain-start
`Parse_choice` provenance from PP.

Bounded-domain feedback currently keeps only regular-domain starts.  That
matches the current English bounded rule exercised so far.  If a future
bounded failure is taught by a URFL, URFL-only, or left-domain starter, the
metric path ignores the mark and traces the reason under
`-test=pp-parse-set-trace` instead of applying an approximate summary.

On the hard long-sentence stress case used during development, combining
terminal MFC state with bounded-domain state learned two bounded blockers,
examined three raw linkages, and kept one PP-valid linkage quickly.  Use
`-test=no-metric-bounded-domain` to disable this feedback for comparison.
The specific stress sentence is not important to the design; it is just an
example where PP blockers had previously dominated ordered extraction.

### Global Contains-One Exact State

`CONTAINS_ONE_GLOBAL` handling is enabled by default for metric extraction.
Rules are declared statically in `PARSE_CONTAINS_ONE_GLOBAL_CONSTRAINTS`.  The
first global rules are encoded as exact ranker state: one bit records that the
selector appeared, and one bit records that a required criterion appeared.
Root states where a selector appears without a criterion are never emitted.
With `-test=metric-pp-validate`, those root states are materialized instead
and tagged as would-reject candidates so batched PP can validate the
prediction.

The exact state currently encodes up to two global rules.  This bound keeps
the combined exact-state space within the `Metric_state` root-state bitset
limit, especially when MFC terminal state is also active.  The exact slots are
selected per sentence from the static global rules: rules whose selector does
not occur in the root `Parse_set` do not consume an exact slot.  Active rules
outside the exact-state budget fall back to a conservative root-candidate mask
scan before full linkage construction.  If such a fallback rule rejects many
candidates, it is promoted into an exact slot and the ranker is rebuilt.

This replaces two earlier prototypes.  The first stored selector/criterion
summary bits on every metric candidate and could allocate too many skipped
candidates on long sentences.  The second scanned only root candidates; it
reduced memory but still spent too much CPU reaching those roots.  Exact DP
state is the current production path for this PP class.

Focused validation on the two current `corpus-fix-long` global contains-one
blockers keeps 10,000 requested linkages after 30,000 examined candidates:
about 2.6 seconds and 163 MB RSS for line 28, and about 1.7 seconds and
119 MB RSS for line 41.

Use `-test=no-metric-global-contains-one` to disable the whole global
contains-one metric mechanism.

### Knowledge-Declared Parse Constraints

Some PP rules can be retained in `4.0.knowledge` as parser/extractor
constraints instead of active PP checks.  The
`PARSE_CONTAINS_ONE_CONSTRAINTS` and `PARSE_CONTAINS_NONE_CONSTRAINTS` stanzas
reuse the existing knowledge-file row syntax and parser, but store the rules
separately from ordinary PP rules.  Metric extraction can read those parsed
rules through postprocessing-owned helper APIs and reject matching root
candidates before linkage materialization and batch PP.

This currently covers the remaining subject-inversion `CONTAINS_ONE` rows
that require the `SI`/`SFI`/`SXI` companion links, and the `Qd,MX`
`CONTAINS_NONE` backstop behind `Bad subject inversion`.  During metric
extraction this path is enabled by default and can be disabled with
`-test=no-metric-pp-constraints`.  When the constraints are active, their
duplicate classic PP rows are suppressed by default; use
`-test=metric-classic-pp` to run the classic PP rows anyway for comparison.
Parse-constraint rejections are counted against the extraction request cap and
reported in maintainer verbosity summaries.  A bounded `s` parse constraint
was investigated but deferred because pre-materialization metric candidates
can still contain punctuation/sentence-split links removed by linkage
normalization before PP.

`-test=metric-pp-validate` is the validation mode for these parser/extractor
constraints and for static global contains-one state.  It records the first
constraint that would reject each metric candidate, keeps the candidate alive,
postprocesses the batch with the classic PP rows active, and compares the
prediction against the PP failure by rule suffix.  This makes the old PP rows
a tripwire for false positives, false negatives, and mismatched rules.  Use it
together with `-test=metric-extraction` when validation must run even for
finite parses that fit inside `!limit`.

### Dictionary PP Rule Migrations

Most English PP checks have been moved into dictionary grammar.  The important
policy is not the individual examples, but the standard: only complete or
substantially complete PP-rule replacements should be kept.  A new active
blocking rule can still be added to `4.0.knowledge`, but it must not be left
as an unmitigated classic-PP blocker for metric extraction if it can dominate
the low-metric region of a large parse set.

Examples of successful or accepted migrations include:

- `to6`, by removing the problematic `I#a` fallback path.
- `Incorrect relative15/16`, by tying the relative/preposition path to the
  required postnominal relation.
- `pronoun66`, by splitting the second-object connector path.
- `predicate41`, which became redundant with the current dictionary.
- `adjective63`, by licensing postposed adjective complements in dictionary
  grammar.
- `adjective65`, which was removed because the PP check was overbroad and
  direct removal improved the fixes corpus without changing the basic or
  long-corpus error counts.
- Redundant `CONTAINS_NONE` rules 69, 70, 74, 75, 76, 77, and 79, whose
  individual suppression changed only diagnostic headers in the standard
  English corpus outputs.

The completed migrations, accepted ranking changes, and deferred migration
candidates are documented in `data/en/GRAMMAR-FIXES.md`.

## Validation and Diagnostic Test Flags

The following metric-extraction flags are intended as durable validation and
diagnostic tools.  The full maintainer flag inventory is maintained in
`systest/TEST-FLAGS.md`.

- `-verbosity=5 -debug=process_metric_linkages,extract_metric_links`: emit a
  concise metric extraction trace through the standard debug output stream.
  The `process_metric_linkages` part records each metric extraction pass,
  batch PP result, PP failure type/message/links, learned feedback counts, and
  final stop reason.  The `extract_metric_links` part records extractor/ranker
  progress before a linkage is materialized, including stream and root
  candidate counts, feedback-problem queue activity, and pre-linkage rejection
  counters.  It is intentionally maintainer-verbosity only; user verbosity
  levels below 5 do not emit the trace even if the debug option is present.
- `-test=no-metric-extraction`: disable metric extraction for comparison.
- `-test=metric-extraction`: force metric extraction even when the
  finite parse count fits inside `!limit`.  This is for bounded validation
  comparisons of metric extraction against ordinary indexed extraction.
- `-test=no-metric-mfc`: disable the default MFC terminal-state
  ranker.
- `-test=no-metric-global-contains-one`: disable the default global
  contains-one metric mechanism.
- `-test=no-metric-bounded-domain`: disable the default bounded-domain
  feedback.
- `-test=no-metric-pp-constraints`: explicitly disable knowledge-declared
  parser/extractor constraints.
- `-test=metric-classic-pp`: during metric extraction, run classic PP checks
  even for rule families the metric extractor normally handles before PP.
- `-test=metric-pp-validate`: validate parser/extractor PP-constraint
  predictions against batched PP without dropping the predicted-bad
  candidates.  Use with `-test=metric-extraction` when finite parses would
  otherwise use ordinary extraction.  The current candidate-level coverage is
  static global contains-one constraints, `PARSE_CONTAINS_ONE_CONSTRAINTS`,
  and `PARSE_CONTAINS_NONE_CONSTRAINTS`.
- `-test=parse-set-count-check`: visibly recount the root parse-set DAG and
  compare it with the stored root count and the older all-bucket overflow
  scan.  This diagnoses count/overflow disagreements without changing normal
  extraction behavior.
- `-test=pp-parse-set-trace`: trace PP failures back to metric
  `Parse_set`/`Parse_choice` provenance.
- `-test=pp-parse-set-trace-all-links`: extend the PP trace to all links in
  the rejected linkage.
- `-test=extract-order`: print extraction-order diagnostics.

## On-Demand Maintainer Validation

The on-demand maintainer script `tests/metric-validate.py` builds a bounded
zero-null validation corpus from the English regression corpora.  It is the
main repeatable best-effort check that extractor-side PP handling neither
materializes guarded PP violations nor skips good displayed linkages on that
bounded corpus.  Rule 78 is the exception: it remains classic-PP
authoritative, and the extractor can only skip same-shape bounded violations
after PP feedback has taught them.

The script has three checks:

- `pp` runs `metric-pp-validate` against classic PP.
- `first` verifies that metric `!limit=1` returns a linkage from the ordinary
  first equal-cost linkage bucket.
- `suppressions` compares normal metric extraction with `metric-classic-pp`
  output to verify that extractor-side PP suppressions do not change displayed
  linkages.

Run the script from the repository root after building the tree.  It needs the
build-tree `link-parser` wrapper, either through `LINK_GRAMMAR_BUILD_DIR`:

```sh
LINK_GRAMMAR_BUILD_DIR=<build-dir> ./tests/metric-validate.py
```

or through the exact parser wrapper:

```sh
LINK_GRAMMAR_LINK_PARSER=<build-dir>/link-parser/link-parser \
  ./tests/metric-validate.py
```

By default the script runs all checks, uses a maximum finite zero-null linkage
count of 2000, and creates a temporary output directory for generated batches,
parser logs, normalized outputs, and diffs.  To keep those files in a known
scratch directory:

```sh
LINK_GRAMMAR_BUILD_DIR=<build-dir> \
  ./tests/metric-validate.py \
    --checks all \
    --max-linkages 2000 \
    --output-dir <scratch-dir>
```

Individual checks can be run while debugging:

```sh
LINK_GRAMMAR_BUILD_DIR=<build-dir> \
  ./tests/metric-validate.py --checks pp --output-dir <scratch-dir>

LINK_GRAMMAR_BUILD_DIR=<build-dir> \
  ./tests/metric-validate.py --checks first --output-dir <scratch-dir>

LINK_GRAMMAR_BUILD_DIR=<build-dir> \
  ./tests/metric-validate.py --checks suppressions --output-dir <scratch-dir>
```

The script exits with status 0 when the selected checks pass and nonzero on
setup errors, parser errors, validation mismatches, linkage-output diffs,
timeouts, or memory exhaustion.  It writes failure details under the output
directory so the reported sentence or diff can be rerun manually.

## Abandoned or Limited Approaches

These approaches should not be repeated without a new reason:

- **One-by-one PP in normal metric extraction.**  It avoids some plumbing but
  is too slow.  PP must be done in batches.
- **Eager global constrained-subproblem splitting.**  It can eliminate repeated
  blockers, but rebuilding too many rankers causes large runtime and memory
  costs.
- **Local cycle feedback with many state bits.**  It can learn exact repeated
  cycle blockers, but hard sentences can produce many independent edge groups,
  causing state explosion.
- **Exact per-stream candidate deduplication.**  It used a lot of memory and
  skipped no useful repeated candidates in the stress case.  Repeated rejected
  linkages came from distinct streams/subproblems.
- **Count-time or simple parse-set-side MFC pruning.**  The simple state was
  too coarse and could reject valid parses because cycle witnesses can depend
  on connector-list context.
- **Partial dictionary fixes for one sentence.**  A dictionary change is useful
  only if it replaces a whole PP rule or is a significant part of a credible
  whole-rule replacement.
- **Global contains-one as constrained problem splitting.**  This was replaced
  by exact DP state plus conservative root-candidate mask scans because the
  constrained-problem queue is too expensive for this PP class.
- **Domain contains-one owned-link root-stream skipping.**  Prototype shortcuts
  that treated nested domain starts as ownership barriers could skip exact
  `CONTAINS_ONE` rejections before root emission, including an owned-DP
  summary variant checked against targeted replay.  A later hidden-frontier
  variant and learned-shield candidate summary also failed to improve the
  stress sentence materially.  These approaches were removed because they
  moved the rejection earlier but still forced huge stream-frontier
  enumeration, and in some variants increased memory use.

## Current PP Blocker Coverage

In the integrated English grammar, metric extraction handles the remaining
active PP blocker rows as follows:

- `FORM_A_CYCLE_RULES`: terminal-state MFC ranking is the default
  metric-extraction handling.  During metric extraction the duplicate classic
  PP row is suppressed unless `-test=metric-classic-pp` or
  `-test=metric-pp-validate` is set.
- `CONTAINS_ONE_RULES`: S-V rules 1, 2, and 7 are declared again as
  `PARSE_CONTAINS_ONE_CONSTRAINTS`, and their sentence-wide companions are
  declared as `PARSE_CONTAINS_ONE_GLOBAL_CONSTRAINTS`.  Metric extraction can
  reject those candidates before linkage materialization.  During metric
  extraction their duplicate active PP rows are suppressed unless
  `-test=metric-classic-pp` or `-test=metric-pp-validate` is set.
- `CONTAINS_NONE_RULES`: the `Qd,MX` `Bad subject inversion` row is declared
  again as `PARSE_CONTAINS_NONE_CONSTRAINTS`, so metric extraction can reject
  those candidates before linkage materialization.  During metric extraction
  its duplicate active PP row is suppressed unless `-test=metric-classic-pp`
  or `-test=metric-pp-validate` is set.
- `BOUNDED_RULES`: bounded-domain feedback is the default metric-extraction
  mitigation for the remaining bounded `s` row.  Classic PP remains
  authoritative for this feedback-only rule.

## Adding More PP Rules

`4.0.knowledge` still supports active PP rows, but an active PP rule that can
reject many low-metric candidates must not be added as the only protection for
linkage-limit-truncated metric extraction.  The ordered extractor needs a way
to avoid spending its request budget on a long prefix of candidates that
classic PP will later reject.

For a new or restored PP blocker, choose one of these production paths:

1. Encode the condition in the dictionary or parser grammar when it can be
   expressed exactly and generally.
2. Add exact parser/ranker state or a knowledge-declared parse constraint when
   dictionary grammar cannot express the rule.
3. Add feedback only when the rule cannot be implemented solely in the
   extractor and classic PP remains authoritative, as with rule 78 for bounded
   `s`.

Any new rule migration should preserve inherited disjunct costs unless the
ranking change is explicit and documented.  Keep completed migrations,
accepted ranking changes, and deferred rule families documented in
`data/en/GRAMMAR-FIXES.md`.
