#########################################################################
#                                                                       #
#                            Objective Caml                             #
#                                                                       #
#            Xavier Leroy, projet Cristal, INRIA Rocquencourt           #
#                                                                       #
#   Copyright 1999 Institut National de Recherche en Informatique et    #
#   en Automatique.  All rights reserved.  This file is distributed     #
#   under the terms of the Q Public License version 1.0.                #
#                                                                       #
#########################################################################

# $Id$

include ../../config/Makefile

CAMLC=../../boot/ocamlrun ../../ocamlc -I ../../stdlib
CAMLOPT=../../boot/ocamlrun ../../ocamlopt -I ../../stdlib
OPTFLAGS=-S
CAMLDEP=../../boot/ocamlrun ../../tools/ocamldep
CAMLRUN=../../byterun/ocamlrun
CODERUNPARAMS=OCAMLRUNPARAM='o=100'

callback.byt: callback.cmo callbackprim.o
	$(CAMLC) -o callback.byt -custom callback.cmo callbackprim.o ../../otherlibs/unix/libunix.a
callback.out: callback.cmx callbackprim.o
	$(CAMLOPT) -o callback.out callback.cmx callbackprim.o ../../otherlibs/unix/libunix.a

manyargs.byt: manyargs.cmo manyargsprim.o
	$(CAMLC) -o manyargs.byt -custom manyargs.cmo manyargsprim.o
manyargs.out: manyargs.cmx manyargsprim.o
	$(CAMLOPT) -o manyargs.out manyargs.cmx manyargsprim.o

multdef.out: multdef.cmx usemultdef.cmx
	$(CAMLOPT) -o multdef.out multdef.cmx usemultdef.cmx

cm.byt: cmcaml.ml cmstub.c cmmain.c
	$(CAMLC) -custom -o cm.byt cmcaml.ml cmstub.c cmmain.c

cmlinked.out: cmcaml.ml cmstub.c cmmain.c
	$(CAMLC) -output-obj -o cm.o cmcaml.ml
	$(BYTECC) -g -o cmlinked.out cm.o -I../../byterun -DNO_BYTECODE_FILE cmstub.c cmmain.c ../../byterun/libcamlrun.a $(BYTECCLIBS)

cm.out: cmcaml.ml cmstub.c cmmain.c
	$(CAMLOPT) -output-obj -o cm.o cmcaml.ml
	$(NATIVECC) -g -o cm.out cm.o -I$(LIBDIR) -DNO_BYTECODE_FILE cmstub.c cmmain.c ../../asmrun/libasmrun.a $(NATIVECCLIBS)

bigarrays.byt: ../../otherlibs/bigarray/bigarray.cma \
               ../../otherlibs/bigarray/libbigarray.a bigarrays.ml
	$(CAMLC) -custom -o bigarrays.byt \
                -I ../../otherlibs/bigarray \
                -I ../../otherlibs/unix \
                unix.cma bigarray.cma bigarrays.ml

bigarrays.out: ../../otherlibs/bigarray/bigarray.cmxa \
               ../../otherlibs/bigarray/libbigarray.a bigarrays.ml
	$(CAMLOPT) $(OPTFLAGS) -o bigarrays.out \
                -I ../../otherlibs/bigarray \
                -I ../../otherlibs/unix \
                unix.cmxa bigarray.cmxa bigarrays.ml

bigarrf.byt: bigarrf.o bigarrfstub.o \
               ../../otherlibs/bigarray/bigarray.cma \
               ../../otherlibs/bigarray/libbigarray.a bigarrfml.ml
	$(CAMLC) -custom -o bigarrf.byt \
                -I ../../otherlibs/bigarray \
                -I ../../otherlibs/unix \
                unix.cma bigarray.cma bigarrf.ml \
                bigarrf.o bigarrfstub.o \
                ../../byterun/libcamlrun.a -cclib -lg2c

bigarrf.out: bigarrf.o bigarrfstub.o \
               ../../otherlibs/bigarray/bigarray.cma \
               ../../otherlibs/bigarray/libbigarray.a bigarrfml.ml
	$(CAMLOPT) $(OPTFLAGS) -o bigarrf.out \
                -I ../../otherlibs/bigarray \
                -I ../../otherlibs/unix \
                unix.cma bigarray.cma bigarrf.ml \
                bigarrf.o bigarrfstub.o \
                ../../byterun/libcamlrun.a -cclib -lg2c

bigarrf.o: bigarrf.f
	g77 -c bigarrf.f

bigarrfstub.o: bigarrfstub.c
	$(NATIVECC) $(NATIVECCCOMPOPTS) -I../../byterun -I../../otherlibs/bigarray -c bigarrfstub.c

fftba.byt: fftba.ml
	$(CAMLC) -o fftba.byt -I ../../otherlibs/bigarray \
          bigarray.cma fftba.ml

fftba.out: fftba.ml
	$(CAMLOPT) $(OPTFLAGS) -o fftba.out -I ../../otherlibs/bigarray \
          bigarray.cmxa fftba.ml

globroots.byt: globroots.ml globrootsprim.o
	$(CAMLC) -custom -o globroots.byt globroots.ml globrootsprim.o

globroots.out: globroots.ml globrootsprim.o
	$(CAMLOPT) -o globroots.out globroots.ml globrootsprim.o

globrootsprim.o: globrootsprim.c
	$(BYTECC) $(BYTECCCOMPOPTS) -I../../byterun -c globrootsprim.c

float.byt: float.cmo
	${CAMLC} -o float.byt float.cmo
float.out: float.cmx
	${CAMLOPT} -o float.out float.cmx

intext.byt: intext.cmo intextaux.o
	${CAMLC} -o intext.byt -custom intext.cmo intextaux.o
intext.out: intext.cmx intextaux.o
	${CAMLOPT} -o intext.out intext.cmx intextaux.o

tscanf.byt: tscanf.cmo
	${CAMLC} -o tscanf.byt tscanf.cmo
tscanf.out: tscanf.cmx
	${CAMLOPT} -o tscanf.out tscanf.cmx

scanf: tscanf.byt tscanf.out
	./tscanf.byt
	./tscanf.out

regexp.byt: ../../otherlibs/str/str.cma regexp.ml
	$(CAMLC) -custom -I ../../otherlibs/str -o regexp.byt str.cma regexp.ml
regexp.opt: ../../otherlibs/str/str.cmxa regexp.ml
	$(CAMLOPT) -I ../../otherlibs/str -o regexp.opt str.cmxa regexp.ml

md5.out: md5.ml
	$(CAMLOPT) -unsafe -inline 100 -o md5.out md5.ml

# Common rules

.SUFFIXES:
.SUFFIXES: .mli .ml .cmi .cmo .cmx .byt .fast.byt .out .fast.out .c .o

.ml.byt:
	$(CAMLC) -o $*.byt $<

.ml.fast.byt:
	cp $*.ml $*_fast.ml
	$(CAMLC) -unsafe -o $*.fast.byt $*_fast.ml
	rm -f $*_fast.ml

.ml.out:
	$(CAMLOPT) $(OPTFLAGS) -o $*.out $<

.ml.fast.out:
	cp $*.ml $*_fast.ml
	$(CAMLOPT) $(OPTFLAGS) -unsafe -o $*.fast.out $*_fast.ml
	rm -f $*_fast.ml

.mli.cmi:
	$(CAMLC) -c $<

.ml.cmo:
	$(CAMLC) -c $<

.ml.cmx:
	$(CAMLOPT) $(OPTFLAGS) -c $<

.c.o:
	$(NATIVECC) $(NATIVECCCOMPOPTS) -I../../byterun -c $<

clean::
	rm -f *.byt *.out
	rm -f *.cm[iox] *.[os]
	rm -f *~
	rm -f intext.data

# Dependencies

depend:
	$(CAMLDEP) *.mli *.ml > .depend

include .depend

