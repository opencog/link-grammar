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
void          extract_thin_linkage(Sentence sent, Parse_Options opts, Linkage linkage);
void          extract_fat_linkage (Sentence sent, Parse_Options opts, Linkage linkage);
Linkage_info  analyze_fat_linkage (Sentence sent, Parse_Options opts, int pass);
Linkage_info  analyze_thin_linkage(Sentence sent, Parse_Options opts, int pass);
