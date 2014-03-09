#!/usr/bin/tclsh
##!/usr/local/ActiveTcl/bin/tclsh
##!/usr/local/bin/tclsh
##!/devel/soar83/tcl-tk/tcl8.4.1/unix/tclsh

## The above path for tcl is probably not correct.
## Type 'which tclsh' or ask a higher-up to find the correct path

## A Tcl script to convert Persian CP-1256 text to Romanized
## Written by Jon Dehdari, 2005
## Licensed under the General Public License v.2 or later  (www.fsf.org)

while {-1 != [gets stdin myinput]} {
  puts [ regsub -all {\WA|^A} [ string map { \xC7 A \xC8 b \x81 p \xC9 P \xCA t \xCB V \xCC j \x8D c \xCD H \xCE x \xCF d \xD0 L \xD1 r \xD2 z \x8E J \xD3 s \xD4 C \xD5 S \xD6 D \xD8 T \xD9 Z \xDA E \xDB G \xDD f \xDE q \xDF k \x98 k \x90 g \xE1 l \xE3 m \xE4 n \xE6 u \xE5 h \xED i \xEC y \xF3 a \xF5 o \xF6 e \xC1 M \xC2 \] \xF0 N \xC4 U \xC6 I \xC0 X \xA5 \% \xA1 \, \x9d - } $myinput ] " |" ] 
}
