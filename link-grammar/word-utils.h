int exp_compare(Exp * e1, Exp * e2);
int exp_contains(Exp * super, Exp * sub);
int word_contains(const char * word, const char * macro, Dictionary dict);
int isPastTenseForm(const char * str, Dictionary dict);
int isEntity(const char * str, Dictionary dict);
