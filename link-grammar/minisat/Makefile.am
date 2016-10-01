VERSION=2.2.0
MINISAT_VERSION_INFO=2:2:0

libminisat_la_LDFLAGS = -version-info $(MINISAT_VERSION_INFO) -no-undefined
libminisat_la_CPPFLAGS = $(ZLIB_CPPFLAGS)

lib_LTLIBRARIES = libminisat.la

libminisat_la_SOURCES = \
	minisat/core/Dimacs.h \
	minisat/core/Solver.cc \
	minisat/core/Solver.h \
	minisat/core/SolverTypes.h \
	minisat/mtl/Alg.h \
	minisat/mtl/Alloc.h \
	minisat/mtl/Heap.h \
	minisat/mtl/IntMap.h \
	minisat/mtl/IntTypes.h \
	minisat/mtl/Map.h \
	minisat/mtl/Queue.h \
	minisat/mtl/Rnd.h \
	minisat/mtl/Sort.h \
	minisat/mtl/Vec.h \
	minisat/mtl/XAlloc.h \
	minisat/utils/Options.cc \
	minisat/utils/Options.h \
	minisat/utils/ParseUtils.h \
	minisat/utils/System.cc \
	minisat/utils/System.h
