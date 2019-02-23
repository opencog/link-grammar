#ifndef _WORDGRAPH_H
#define _WORDGRAPH_H

#include "api-structures.h"
#include "vc_vector/vc_vector.h"
#include "utilities.h"

#ifdef USE_WORDGRAPH_DISPLAY
/* Wordgraph display representation modes. */
#define lo(l) (l-'a')
#define WGR_SUB      (1<<lo('s')) /* Unsplit words as subgraphs */
#define WGR_COMPACT  (1<<lo('c')) /* Compact. Just now applies only to 's'. */
#define WGR_PREV     (1<<lo('p')) /* Prev links */
#define WGR_UNSPLIT  (1<<lo('u')) /* Unsplit_word links */
#define WGR_DBGLABEL (1<<lo('d')) /* Debug label addition */
#define WGR_DOTDEBUG (1<<lo('h')) /* Hex node numbers, for dot commands debug */
#define WGR_LEGEND   (1<<lo('l')) /* Add a legend */
#define WGR_X11      (1<<lo('x')) /* Display using X11 even on Windows */
#endif /* USE_WORDGRAPH_DISPLAY */

/* Original sentence words have their unsplit_word set to the wordgraph
 * start. See issue_sentence_word. */
#define IS_SENTENCE_WORD(sent, gword) (gword->unsplit_word == sent->wordgraph)

Gword *gword_new(Sentence, const char *);
void gword_set_print(const gword_set *);
Gword *wg_get_sentence_word(const Sentence, Gword *);
#if 0
void gwordlist_append_list(const Gword ***, const Gword **);
#endif

const Gword **wordgraph_hier_position(Gword *);
void print_hier_position(const Gword *);
bool in_same_alternative(Gword *, Gword *);
Gword *find_real_unsplit_word(Gword *, bool);

size_t wordgraph_pathpos_len(Wordgraph_pathpos *);
Wordgraph_pathpos *wordgraph_pathpos_resize(Wordgraph_pathpos *, size_t);
bool wordgraph_pathpos_add(Wordgraph_pathpos **, Gword *, bool, bool, bool);

const char *gword_status(Sentence, const Gword *);
const char *gword_morpheme(Sentence sent, const Gword *w);

/* ======================= Gwordlist access methods ======================= */
STYPEDEF(vc_vector, Gwordlist);

void print_lwg_path(Gwordlist *, const char *);

static inline Gwordlist *gwordlist_new(size_t n)
{
	return (Gwordlist *)vc_vector_create(n, sizeof(Gword *), NULL);
}

static inline void gwordlist_delete(const Gwordlist *gl)
{
	vc_vector_release((vc_vector *)gl);
}

static inline size_t gwordlist_len(const Gwordlist *gl)
{
	return vc_vector_count((vc_vector *)gl);
}

static inline bool gwordlist_append(Gwordlist *gl, Gword **p)
{
	return vc_vector_push_back((vc_vector *)gl, p);
}

static inline bool gwordlist_replace(Gwordlist *gl, size_t index, Gword **p)
{
	return vc_vector_replace((vc_vector *)gl, index, p);
}

/** Return a pointer to the gword pointer at the given index.
 * If the index is is after the last element, return a pointer to
 * a null pointer, to keep the code expectation (it originally referred
 * directly to the gwordlist array elements, when the last one was NULL).
 */
static inline Gword **gwordlist_at(const Gwordlist *gl, size_t index)
{
	static Gword *null_gword = NULL;
	if (index >= gwordlist_len(gl)) return &null_gword;

	return (Gword **)vc_vector_at((vc_vector *)gl, index);
}

static inline Gwordlist *gwordlist_copy(const Gwordlist *gl)
{
	return (Gwordlist *)vc_vector_create_copy((vc_vector *)gl);
}

static inline bool gwordlist_empty(const Gwordlist *gl)
{
	return vc_vector_count((vc_vector *)gl) == 0;
}

/* ====== Gwordlist iteration ===== */
/*
 * for (Gword *gw = gwordlist_begin(gl);
 *             gw != gwordlist_end(gl);
 *             gw = gwordlist_next(gl, gw)) { ... }
 */

static inline Gword **gwordlist_begin(const Gwordlist *gl)
{
	return (Gword **)vc_vector_begin((vc_vector *)gl);
}

static inline Gword **gwordlist_end(const Gwordlist *gl)
{
	return (Gword **)vc_vector_end((vc_vector *)gl);
}

static inline Gword **gwordlist_next(const Gwordlist *gl, Gword **gp)
{
	return (Gword **)vc_vector_next((vc_vector *)gl, (void *)gp);
}
/* ======================================================================== */

#endif /* _WORDGRAPH_H */
