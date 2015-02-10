#ifdef USE_WORDGRAPH_DISPLAY
/* Wordgraph display representation modes. */
#define lo(l) (l-'a')
#define WGR_SUB        1<<lo('s') /* Unsplit words as subgraphs */
#define WGR_PREV       1<<lo('p') /* Prev links */
#define WGR_UNSPLIT    1<<lo('u') /* Unsplit_word links */
#define WGR_DBGLABEL   1<<lo('d') /* Debug label addition */
#define WGR_DOTDEBUG   1<<lo('h') /* Hex node numbers, for dot commands debug */
#define WGR_LEGEND     1<<lo('l') /* Add a legend */
#define WGR_X11        1<<lo('x') /* Display using X11 even on Windows */

void wordgraph_show(Sentence, const char *);
#endif /* USE_WORDGRAPH_DISPLAY */

Gword *gword_new(Sentence, const char *);
Gword *empty_word(void);

size_t gwordlist_len(const Gword **);
void gwordlist_append(Gword ***, Gword *);

const Gword **wordgraph_hier_position(Gword *);
void print_hier_position(const Gword *);
bool in_same_alternative(Gword *, Gword *);
Gword *find_real_unsplit_word(Gword *, bool);

size_t wordgraph_pathpos_len(Wordgraph_pathpos *);
Wordgraph_pathpos *wordgraph_pathpos_resize(Wordgraph_pathpos *, size_t);
bool wordgraph_pathpos_append(Wordgraph_pathpos **, Gword *, bool, bool);

const char *gword_status(Sentence, const Gword *);
const char *gword_morpheme(Sentence sent, const Gword *w);
