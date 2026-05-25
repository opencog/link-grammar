#!/usr/bin/env python3
"""Run on-demand metric extraction validation checks.

The script builds a bounded zero-null validation corpus from the English
regression corpora, then runs maintainer checks:

* ``pp`` runs ``metric-pp-validate`` against classic PP on the bounded corpus.
* ``first`` checks that metric extraction with ``!limit=1`` returns the same
  first ``!links`` block as ordinary sorted extraction, or one of the first
  ordinary equal-cost alternatives when ordinary tie order differs.
* ``suppressions`` compares normal metric output against
  ``metric-classic-pp`` output, verifying that extractor-side PP suppressions
  do not change displayed linkages.
"""

import argparse
import difflib
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path


DEFAULT_MAX_LINKAGES = 2000

COUNT_BATCH_HEADER = [
    "!batch",
    "!echo",
    "!limit=1",
    "!short=254",
    "!null=0",
    "!spell=0",
    "!graphics=0",
    "!constituents=0",
    "!timeout=60",
]

COMMON_LINK_HEADER = [
    "!echo",
    "!short=254",
    "!null=0",
    "!spell=0",
    "!graphics=0",
    "!links=1",
    "!constituents=0",
    "!morphology=0",
    "!timeout=60",
]

NO_LINK_HEADER = [
    "!batch",
    "!echo",
    "!short=254",
    "!null=0",
    "!spell=0",
    "!graphics=0",
    "!links=0",
    "!constituents=0",
    "!morphology=0",
    "!timeout=60",
]

CHECKS = ("pp", "first", "suppressions")
MAX_CONSOLE_MISMATCHES = 20

FOUND_RE = re.compile(r"^Found ([0-9]+) linkages?\b")
ERROR_RE = re.compile(r"([0-9]+) errors?\.")
VALIDATE_SUMMARY_RE = re.compile(r"(false-positive|false-negative)=([0-9]+)")
COST_RE = re.compile(r"cost vector = \((.*)\)")


def parse_args():
    parser = argparse.ArgumentParser(
        description="Validate metric extraction on bounded zero-null corpora.")
    parser.add_argument(
        "--output-dir",
        type=Path,
        help="directory for generated batches, parser logs, and diffs")
    parser.add_argument(
        "--max-linkages",
        type=int,
        default=DEFAULT_MAX_LINKAGES,
        help="maximum finite zero-null linkage count to validate")
    parser.add_argument(
        "--checks",
        default=",".join(CHECKS),
        help="comma-separated checks to run: pp, first, suppressions, or all")
    return parser.parse_args()


def project_root():
    return Path(__file__).resolve().parents[1]


def find_parser():
    direct = os.environ.get("LINK_GRAMMAR_LINK_PARSER")
    if direct:
        parser = Path(direct)
    else:
        build_dir = os.environ.get("LINK_GRAMMAR_BUILD_DIR")
        if not build_dir:
            sys.exit(
                "Set LINK_GRAMMAR_LINK_PARSER or LINK_GRAMMAR_BUILD_DIR. "
                "For build-tree runs, use link-parser/link-parser so the "
                "wrapper finds the in-tree liblink-grammar.")
        parser = Path(build_dir) / "link-parser" / "link-parser"

    if not parser.exists():
        sys.exit(f"link-parser not found: {parser}")
    if not os.access(parser, os.X_OK):
        sys.exit(f"link-parser is not executable: {parser}")
    return parser


def parse_checks(text):
    if text == "all":
        return set(CHECKS)

    checks = {item.strip() for item in text.split(",") if item.strip()}
    unknown = checks - set(CHECKS)
    if unknown:
        sys.exit("unknown check(s): " + ", ".join(sorted(unknown)))
    if not checks:
        sys.exit("no validation checks requested")
    return checks


def make_output_dir(path):
    if path is None:
        return Path(tempfile.mkdtemp(prefix="metric-validate-"))
    path.mkdir(parents=True, exist_ok=True)
    return path


def is_sentence_line(line):
    stripped = line.strip()
    return stripped and not stripped.startswith("!") and not stripped.startswith("%")


def strip_batch_label(sentence):
    if sentence.startswith("*") or sentence.startswith(":"):
        return sentence[1:].lstrip()
    return sentence


def read_corpus_sentences(root):
    sources = [
        root / "data" / "en" / "corpus-knowledge.batch",
        root / "data" / "en" / "corpus-basic.batch",
        root / "data" / "en" / "corpus-fixes.batch",
        root / "data" / "en" / "corpus-fix-long.batch",
    ]
    sentences = []
    for source in sources:
        with source.open(encoding="utf-8") as infile:
            for line in infile:
                line = line.rstrip()
                if is_sentence_line(line):
                    sentences.append(line)
    return sentences


def write_batch(path, header, sentences):
    with path.open("w", encoding="utf-8") as outfile:
        for line in header:
            outfile.write(line)
            outfile.write("\n")
        outfile.write("\n")
        for sentence in sentences:
            outfile.write(sentence)
            outfile.write("\n")


def link_header(limit):
    return COMMON_LINK_HEADER + [f"!limit={limit}"]


def no_link_header(limit):
    return NO_LINK_HEADER + [f"!limit={limit}"]


def run_parser(parser, root, batch, test, tag, output_dir):
    stdout_path = output_dir / f"{tag}.out"
    stderr_path = output_dir / f"{tag}.err"
    command = [str(parser), "en"]
    if test:
        command.append(f"-test={test}")

    with batch.open(encoding="utf-8") as stdin, \
            stdout_path.open("w", encoding="utf-8") as stdout, \
            stderr_path.open("w", encoding="utf-8") as stderr:
        result = subprocess.run(
            command,
            cwd=root,
            stdin=stdin,
            stdout=stdout,
            stderr=stderr,
            check=False,
            text=True)

    stdout = stdout_path.read_text(encoding="utf-8", errors="replace")
    stderr = stderr_path.read_text(encoding="utf-8", errors="replace")
    return result.returncode, stdout, stderr, stdout_path, stderr_path


def parse_linkage_counts(stdout, sentences):
    counts = []
    index = 0
    pending = None
    for raw_line in stdout.splitlines():
        line = raw_line.rstrip()
        if index < len(sentences) and line == sentences[index]:
            if pending is not None:
                counts.append((sentences[pending], 0))
            pending = index
            index += 1
            continue
        if pending is None:
            continue
        match = FOUND_RE.match(line)
        if match:
            counts.append((sentences[pending], int(match.group(1))))
            pending = None
            continue
        if line.startswith("No complete linkages found"):
            counts.append((sentences[pending], 0))
            pending = None
            continue
        if line.startswith("+++++ error"):
            counts.append((sentences[pending], 0))
            pending = None

    if pending is not None:
        counts.append((sentences[pending], 0))
    if len(counts) != len(sentences):
        raise RuntimeError(
            f"parsed {len(counts)} counts for {len(sentences)} sentences")
    return counts


def parse_error_count(stderr):
    matches = ERROR_RE.findall(stderr)
    if not matches:
        return None
    return int(matches[-1])


def unexpected_parser_failure(tag, returncode, stderr):
    if returncode != 0:
        return f"{tag}: link-parser exited with {returncode}"
    errors = parse_error_count(stderr)
    if errors is not None and errors != 0:
        return f"{tag}: expected 0 parser errors, got {errors}"
    if "Timer is expired!" in stderr:
        return f"{tag}: parser reported a timeout"
    return None


def check_validate(stderr):
    problems = []
    for kind, value in VALIDATE_SUMMARY_RE.findall(stderr):
        if int(value) != 0:
            problems.append(f"{kind}={value}")

    for line in stderr.splitlines():
        if "Error: metric-pp-validate mismatch" not in line:
            continue
        if "S-V inversion" in line or "s-v inversion" in line:
            continue
        problems.append(f"unexpected validation mismatch: {line}")
    return problems


def select_validation_sentences(parser, root, sentences, max_linkages,
                                output_dir):
    count_batch = output_dir / "count.batch"
    write_batch(count_batch, COUNT_BATCH_HEADER, sentences)
    code, stdout, stderr, _, _ = run_parser(
        parser,
        root,
        count_batch,
        "batch-print-parse-statistics",
        "count",
        output_dir)
    if code != 0:
        raise RuntimeError(f"count run exited with {code}")

    counts = parse_linkage_counts(stdout, sentences)
    selected = [
        sentence
        for sentence, count in counts
        if 0 < count <= max_linkages
    ]
    too_large = sum(1 for _, count in counts if count > max_linkages)
    zero = sum(1 for _, count in counts if count == 0)
    total_linkages = sum(count for _, count in counts
                         if 0 < count <= max_linkages)
    max_selected = max((count for _, count in counts
                        if 0 < count <= max_linkages), default=0)
    return selected, {
        "source": len(sentences),
        "selected": len(selected),
        "zero": zero,
        "too_large": too_large,
        "total_linkages": total_linkages,
        "max_selected": max_selected,
        "count_errors": parse_error_count(stderr),
    }


class Linkage_block:
    """Canonical text for one displayed linkage."""

    def __init__(self, cost, lines):
        self.cost = cost
        self.lines = tuple(lines)

    def __eq__(self, other):
        if not isinstance(other, Linkage_block):
            return NotImplemented
        return self.cost == other.cost and self.lines == other.lines

    def __hash__(self):
        return hash((self.cost, self.lines))

    def text(self):
        return "\n".join(("cost vector = " + self.cost, *self.lines))


def parse_linkage_occurrences(stdout, sentences):
    """Return displayed linkage blocks for each echoed sentence occurrence."""
    blocks = [[] for _ in sentences]
    current_index = None
    current_cost = None
    current_lines = []
    next_sentence = 0

    def finish_linkage():
        nonlocal current_cost, current_lines
        if current_index is not None and current_cost is not None:
            blocks[current_index].append(Linkage_block(current_cost,
                                                       current_lines))
        current_cost = None
        current_lines = []

    for raw_line in stdout.splitlines():
        line = raw_line.rstrip()

        if next_sentence < len(sentences) and line == sentences[next_sentence]:
            finish_linkage()
            current_index = next_sentence
            next_sentence += 1
            continue

        if current_index is None:
            continue

        if line == "Bye.":
            finish_linkage()
            current_index = None
            continue

        cost_match = COST_RE.search(line)
        if cost_match:
            finish_linkage()
            current_cost = cost_match.group(1)
            current_lines = []
            continue

        if current_cost is None:
            continue
        if not line:
            finish_linkage()
            continue
        current_lines.append(line)

    finish_linkage()
    return blocks


def write_first_mismatches(path, mismatches):
    with path.open("w", encoding="utf-8") as outfile:
        for mismatch in mismatches:
            outfile.write("Sentence: ")
            outfile.write(mismatch["sentence"])
            outfile.write("\n")
            outfile.write("Reason: ")
            outfile.write(mismatch["reason"])
            outfile.write("\n")
            if mismatch.get("metric"):
                outfile.write("\nMetric first linkage:\n")
                outfile.write(mismatch["metric"].text())
                outfile.write("\n")
            for index, block in enumerate(mismatch.get("ordinary", []), 1):
                outfile.write(f"\nOrdinary minimum linkage {index}:\n")
                outfile.write(block.text())
                outfile.write("\n")
            outfile.write("\n" + ("=" * 72) + "\n\n")


def write_sentence_list(path, sentences):
    with path.open("w", encoding="utf-8") as outfile:
        for sentence in sentences:
            outfile.write(sentence)
            outfile.write("\n")


def normalize_test_settings(stdout):
    """Drop run-configuration echo lines that differ by construction."""
    return "\n".join(
        line for line in stdout.splitlines()
        if not line.startswith("test set to ")
    ) + ("\n" if stdout.endswith("\n") else "")


def first_differing_sentence(left_stdout, right_stdout, sentences):
    sentence_set = set(sentences)
    left_lines = left_stdout.splitlines()
    right_lines = right_stdout.splitlines()
    current_sentence = None

    for index in range(max(len(left_lines), len(right_lines))):
        left = left_lines[index] if index < len(left_lines) else None
        right = right_lines[index] if index < len(right_lines) else None

        if left == right:
            if left in sentence_set:
                current_sentence = left
            continue

        if left in sentence_set:
            return left
        if right in sentence_set:
            return right
        return current_sentence

    return None


def write_unified_diff(path, from_path, from_text, to_path, to_text):
    diff = difflib.unified_diff(
        from_text.splitlines(keepends=True),
        to_text.splitlines(keepends=True),
        fromfile=str(from_path),
        tofile=str(to_path))
    path.write_text("".join(diff), encoding="utf-8")


def print_first_mismatches(mismatches, mismatch_path):
    if not mismatches:
        return

    print("First-linkage mismatches:", file=sys.stderr)
    for mismatch in mismatches[:MAX_CONSOLE_MISMATCHES]:
        print(f"  - {mismatch['sentence']}", file=sys.stderr)
    if len(mismatches) > MAX_CONSOLE_MISMATCHES:
        remaining = len(mismatches) - MAX_CONSOLE_MISMATCHES
        print(f"  ... {remaining} more; see {mismatch_path}", file=sys.stderr)


def first_linkage_candidates(ordinary_stdout, metric_stdout, selected):
    ordinary_blocks = parse_linkage_occurrences(ordinary_stdout, selected)
    metric_blocks = parse_linkage_occurrences(metric_stdout, selected)
    mismatches = []
    tie_candidates = []
    skipped = 0

    for index, sentence in enumerate(selected):
        ordinary = ordinary_blocks[index]
        metric = metric_blocks[index]
        if not ordinary:
            if not metric:
                skipped += 1
                continue
            mismatches.append({
                "sentence": sentence,
                "reason": "metric displayed a linkage but ordinary did not",
                "metric": metric[0],
                "ordinary": [],
            })
            continue
        if len(metric) != 1:
            mismatches.append({
                "sentence": sentence,
                "reason": f"metric extraction displayed {len(metric)} linkages",
                "metric": metric[0] if metric else None,
                "ordinary": ordinary[:1],
            })
            continue

        if metric[0] != ordinary[0]:
            tie_candidates.append({
                "sentence": sentence,
                "metric": metric[0],
                "ordinary": ordinary[0],
            })

    return mismatches, tie_candidates, skipped


def check_first_tie_candidates(ordinary_stdout, tie_candidates):
    sentences = [candidate["sentence"] for candidate in tie_candidates]
    ordinary_blocks = parse_linkage_occurrences(ordinary_stdout, sentences)
    mismatches = []
    tie_reorders = []

    for index, candidate in enumerate(tie_candidates):
        sentence = candidate["sentence"]
        ordinary = ordinary_blocks[index]
        if not ordinary:
            mismatches.append({
                "sentence": sentence,
                "reason": "ordinary tie-check run displayed no linkage",
                "metric": candidate["metric"],
                "ordinary": [],
            })
            continue

        min_cost = ordinary[0].cost
        equal_cost = [block for block in ordinary if block.cost == min_cost]
        if candidate["metric"] in equal_cost:
            tie_reorders.append(sentence)
        else:
            mismatches.append({
                "sentence": sentence,
                "reason": "metric first linkage is not in the ordinary "
                          "first equal-cost bucket",
                "metric": candidate["metric"],
                "ordinary": equal_cost,
            })

    return mismatches, tie_reorders


def finish_first_check(output_dir, mismatches):
    mismatch_path = output_dir / "first-linkage-mismatches.txt"
    write_first_mismatches(mismatch_path, mismatches)
    print_first_mismatches(mismatches, mismatch_path)
    return mismatch_path


def run_pp_checks(parser, root, selected, max_linkages, output_dir):
    auto_next = f"auto-next-linkage:{max_linkages}"
    no_links_batch = output_dir / "validation-no-links.batch"
    write_batch(no_links_batch, no_link_header(max_linkages), selected)

    validate = run_parser(
        parser,
        root,
        no_links_batch,
        f"metric-extraction,metric-pp-validate,{auto_next}",
        "validate",
        output_dir)

    failures = []
    for tag, result in [
            ("validate", validate)]:
        problem = unexpected_parser_failure(tag, result[0], result[2])
        if problem:
            failures.append(problem)

    failures.extend(check_validate(validate[2]))

    paths = {
        "validate stderr": validate[4],
    }
    return failures, paths


def run_first_check(parser, root, selected, max_linkages, output_dir):
    first_ref_batch = output_dir / "first-reference.batch"
    first_metric_batch = output_dir / "first-metric.batch"
    first_ties_batch = output_dir / "first-tie-reference.batch"
    first_sentences = [strip_batch_label(sentence) for sentence in selected]
    write_batch(first_ref_batch, link_header(max_linkages), first_sentences)
    write_batch(first_metric_batch, link_header(1), first_sentences)

    ordinary = run_parser(
        parser,
        root,
        first_ref_batch,
        "no-metric-extraction,auto-next-linkage:1",
        "first-ordinary",
        output_dir)
    metric = run_parser(
        parser,
        root,
        first_metric_batch,
        "metric-extraction,auto-next-linkage:1",
        "first-metric",
        output_dir)

    failures = []
    for tag, result in [
            ("first-ordinary", ordinary),
            ("first-metric", metric)]:
        problem = unexpected_parser_failure(tag, result[0], result[2])
        if problem:
            failures.append(problem)

    mismatches, tie_candidates, skipped = first_linkage_candidates(
        ordinary[1], metric[1], first_sentences)
    tie_reorders = []
    if tie_candidates:
        tie_sentences = [candidate["sentence"] for candidate in tie_candidates]
        write_batch(first_ties_batch, link_header(max_linkages), tie_sentences)
        ordinary_ties = run_parser(
            parser,
            root,
            first_ties_batch,
            f"no-metric-extraction,auto-next-linkage:{max_linkages}",
            "first-ordinary-ties",
            output_dir)
        problem = unexpected_parser_failure(
            "first-ordinary-ties", ordinary_ties[0], ordinary_ties[2])
        if problem:
            failures.append(problem)
        tie_mismatches, tie_reorders = check_first_tie_candidates(
            ordinary_ties[1], tie_candidates)
        mismatches.extend(tie_mismatches)

    mismatch_path = finish_first_check(output_dir, mismatches)
    tie_path = output_dir / "first-linkage-tie-reorders.txt"
    write_sentence_list(tie_path, tie_reorders)
    if mismatches:
        failures.append(
            f"first-linkage check found {len(mismatches)} mismatches; "
            f"see {mismatch_path}")

    paths = {
        "first-ordinary stderr": ordinary[4],
        "first-metric stderr": metric[4],
        "first tie reorder list": tie_path,
        "first mismatch report": mismatch_path,
    }
    print(
        "First-linkage check: "
        f"{len(tie_reorders)} equal-cost tie reorders accepted; "
        f"{skipped} sentences had no ordinary valid linkage.")
    return failures, paths


def run_suppressions_check(parser, root, selected, max_linkages, output_dir):
    auto_next = f"auto-next-linkage:{max_linkages}"
    suppression_batch = output_dir / "suppressions.batch"
    suppression_sentences = [strip_batch_label(sentence)
                             for sentence in selected]
    write_batch(suppression_batch,
                link_header(max_linkages),
                suppression_sentences)

    classic = run_parser(
        parser,
        root,
        suppression_batch,
        f"metric-extraction,metric-classic-pp,{auto_next}",
        "suppressions-classic",
        output_dir)
    normal = run_parser(
        parser,
        root,
        suppression_batch,
        f"metric-extraction,{auto_next}",
        "suppressions-normal",
        output_dir)

    failures = []
    for tag, result in [
            ("suppressions-classic", classic),
            ("suppressions-normal", normal)]:
        problem = unexpected_parser_failure(tag, result[0], result[2])
        if problem:
            failures.append(problem)

    classic_norm_path = output_dir / "suppressions-classic.norm"
    normal_norm_path = output_dir / "suppressions-normal.norm"
    diff_path = output_dir / "suppressions.diff"
    classic_norm = normalize_test_settings(classic[1])
    normal_norm = normalize_test_settings(normal[1])
    classic_norm_path.write_text(classic_norm, encoding="utf-8")
    normal_norm_path.write_text(normal_norm, encoding="utf-8")

    if classic_norm != normal_norm:
        write_unified_diff(diff_path,
                           classic_norm_path, classic_norm,
                           normal_norm_path, normal_norm)
        first_sentence = first_differing_sentence(
            classic_norm, normal_norm, suppression_sentences)
        failure = "suppressions check found output differences"
        if first_sentence is not None:
            failure += f"; first differing sentence: {first_sentence}"
        failures.append(f"{failure}; see {diff_path}")
    else:
        diff_path.write_text("", encoding="utf-8")

    paths = {
        "suppressions classic stderr": classic[4],
        "suppressions normal stderr": normal[4],
        "suppressions diff": diff_path,
    }
    if classic_norm == normal_norm:
        print("Suppressions check: normal metric output matches "
              "metric-classic-pp output.")
    return failures, paths


def print_paths(paths):
    for label, path in paths.items():
        print(f"{label}: {path}")


def main():
    args = parse_args()
    checks = parse_checks(args.checks)
    root = project_root()
    parser = find_parser()
    output_dir = make_output_dir(args.output_dir)

    sentences = read_corpus_sentences(root)
    selected, stats = select_validation_sentences(
        parser, root, sentences, args.max_linkages, output_dir)
    if not selected:
        sys.exit("no bounded zero-null validation sentences were selected")

    failures = []
    paths = {}
    if "pp" in checks:
        pp_failures, pp_paths = run_pp_checks(
            parser, root, selected, args.max_linkages, output_dir)
        failures.extend(pp_failures)
        paths.update(pp_paths)

    if "first" in checks:
        first_failures, first_paths = run_first_check(
            parser, root, selected, args.max_linkages, output_dir)
        failures.extend(first_failures)
        paths.update(first_paths)

    if "suppressions" in checks:
        suppressions_failures, suppressions_paths = run_suppressions_check(
            parser, root, selected, args.max_linkages, output_dir)
        failures.extend(suppressions_failures)
        paths.update(suppressions_paths)

    print(f"Output directory: {output_dir}")
    print(
        "Selected {selected} of {source} sentences; "
        "{total_linkages} total linkages, max {max_selected}; "
        "{zero} zero-linkage, {too_large} over {limit}.".format(
            limit=args.max_linkages,
            **stats))
    if stats["count_errors"] is not None:
        print(f"Count pass parser errors: {stats['count_errors']}")
    print_paths(paths)

    if failures:
        print("Failures:", file=sys.stderr)
        for failure in failures:
            print(f"  - {failure}", file=sys.stderr)
        return 1

    print("metric validation checks passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
