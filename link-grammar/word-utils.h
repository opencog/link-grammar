
#ifndef _LINK_GRAMMAR_WORD_UTILS_H_
#define _LINK_GRAMMAR_WORD_UTILS_H_

int word_contains(const char * word, const char * macro, Dictionary dict);
int is_past_tense_form(const char * str, Dictionary dict);
int is_entity(const char * str, Dictionary dict);

#endif /* _LINK_GRAMMAR_WORD_UTILS_H_ */
