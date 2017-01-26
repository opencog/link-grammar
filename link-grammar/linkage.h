#ifndef _LINKAGE_H
#define _LINKAGE_H
void remap_linkages(Linkage, const int *remap);
void compute_chosen_words(Sentence, Linkage, Parse_Options);

void partial_init_linkage(Sentence, Linkage, unsigned int N_words);
void check_link_size(Linkage);
void remove_empty_words(Linkage);
void free_linkage(Linkage);
#endif /* _LINKAGE_H */
