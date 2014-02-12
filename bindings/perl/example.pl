#! /usr/bin/env perl
#
# Example usage of link-grammar perl bindings

use link_grammar_perl;

my $ver = link_grammar_perl::linkgrammar_get_version();

print "Version $ver\n";

# English, Russuan and Turkish dictionaries
# my $en_dict = link_grammar_perl::dictionary_create_lang("en");

my $ru_dict = link_grammar_perl::dictionary_create_lang("ru");
# my $tr_dict = link_grammar_perl::dictionary_create_lang("tr");

my $po = link_grammar_perl::parse_options_create();

my $sent = link_grammar_perl::sentence_create("This is a test", $ru_dict);
my $num_parses = link_grammar_perl::sentence_parse($sent, $po);
$num_parses = link_grammar_perl::sentence_num_valid_linkages($sent);
print "Fond $num_parses valid parses:\n";

for (my $i=0; $i<$num_parses; $i++) {
    my $linkage = link_grammar_perl::linkage_create($i, $sent, $po);
    my $diagram = link_grammar_perl::linkage_print_diagram($linkage);
    print "Parse $i:\n$diagram";
}
