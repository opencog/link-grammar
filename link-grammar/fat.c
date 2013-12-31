
#ifndef USE_FAT_LINKAGES

#if defined(_WIN32) || __APPLE__
/* Arghhh. Both Mac OSX and also MSWin fail because these are specified
 * in link-grammar.def, but are absent in the library.  So hack around
 * this by creating stub routines.  Don't care about the signature;
 * they're just junk that won't ever be called, anyway.
 */

void analyze_fat_linkage (void);
void analyze_fat_linkage (void) {}

void build_deletable (void);
void build_deletable (void) {}

void build_effective_dist (void);
void build_effective_dist (void) {}

void count_set_effective_distance (void);
void count_set_effective_distance (void) {}

void extract_fat_linkage (void);
void extract_fat_linkage (void) {}

void parse_options_get_use_fat_links (void);
void parse_options_get_use_fat_links (void) {}

void parse_options_set_use_fat_links (void);
void parse_options_set_use_fat_links (void) {}

void set_has_fat_down (void);
void set_has_fat_down (void) {}
#endif /* defined(_WIN32) || __APPLE__ */

#endif /* USE_FAT_LINKAGES */

