/********************************************************************************/
/* Copyright (c) 2004                                                           */
/* Daniel Sleator, David Temperley, and John Lafferty                           */
/* All rights reserved                                                          */
/*                                                                              */
/* Use of the link grammar parsing system is subject to the terms of the        */
/* license set forth in the LICENSE file included with this software,           */ 
/* and also available at http://www.link.cs.cmu.edu/link/license.html           */
/* This license allows free redistribution and use in source and binary         */
/* forms, with or without modification, subject to certain conditions.          */
/*                                                                              */
/********************************************************************************/
void       prune(Sentence sent);
int        power_prune(Sentence sent, int mode, Parse_Options opts);
void       pp_and_power_prune(Sentence sent, int mode, Parse_Options opts);
int        prune_match(Connector * a, Connector * b, int wa, int wb);
int        x_prune_match(Connector * a, Connector * b);
void       expression_prune(Sentence sent);
Disjunct * eliminate_duplicate_disjuncts(Disjunct * );
