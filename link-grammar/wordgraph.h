#ifdef USE_WORDGRAPH_DISPLAY
/* Wordgraph display representation modes. */
#define WGR_REG            0x01  /* Ordered sentence words + unsplit link */
#define WGR_SUB            0x02  /* Unsplit words as subgraphs */
#define WGR_NOPREV         0x04  /* No prev links */
#define WGR_NOUNSPLIT      0x08  /* No unsplit_word links */
#define WGR_NODBGLABEL     0x10  /* No debug label addition */
#define WGR_MAX            0x10 /* Maximum value of WGR_*, for mode[] length */
#define WGR_MAXDIGITS      2    /* Number of hex digits in WGR_MAX */
void wordgraph_show(Sentence, size_t, const char *);
#define wordgraph_show(sent, mode) wordgraph_show(sent, mode, #mode)
#else
#define wordgraph_show(sent, mode)
#endif /* USE_WORDGRAPH_DISPLAY */

Gword *gword_new(Sentence, const char *);
Gword **wordlist_resize(Gword **, size_t);
size_t wordlist_len(const Gword **);
const char *gword_status(Sentence, const Gword *);
const char *gword_morpheme(Sentence sent, const Gword *w);
void wordlist_append(Gword ***, Gword *);
Gword *empty_word(void);
