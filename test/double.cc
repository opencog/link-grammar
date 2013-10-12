
#include "link-includes.h"

int main() {

    Dictionary    dict, dict2;
    Parse_Options opts, opts2;

    opts  = parse_options_create();
    dict  = dictionary_create_lang("en");
    dictionary_delete(dict);
    parse_options_delete(opts);

    opts2  = parse_options_create();
    dict2  = dictionary_create_lang("en");
    dictionary_delete(dict2);
    parse_options_delete(opts2);

    return 0;
}
