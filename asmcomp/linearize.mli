(***********************************************************************)
(*                                                                     *)
(*                           Objective Caml                            *)
(*                                                                     *)
(*            Xavier Leroy, projet Cristal, INRIA Rocquencourt         *)
(*                                                                     *)
(*  Copyright 1996 Institut National de Recherche en Informatique et   *)
(*  en Automatique.  All rights reserved.  This file is distributed    *)
(*  under the terms of the Q Public License version 1.0.               *)
(*                                                                     *)
(***********************************************************************)

(* $Id$ *)

(* Transformation of Mach code into a list of pseudo-instructions. *)

type label = int
val new_label: unit -> label

type instruction =
  { mutable desc: instruction_desc;
    mutable next: instruction;
    arg: Reg.t array;
    res: Reg.t array;
    live: Reg.Set.t }

and instruction_desc =
    Lend
  | Lop of Mach.operation
  | Lreloadretaddr
  | Lreturn
  | Llabel of label
  | Lbranch of label
  | Lcondbranch of Mach.test * label
  | Lcondbranch3 of label option * label option * label option
  | Lswitch of label array
  | Lsetuptrap of label
  | Lpushtrap
  | Lpoptrap
  | Lraise

val has_fallthrough :  instruction_desc -> bool
val end_instr: instruction
val instr_cons: 
  instruction_desc -> Reg.t array -> Reg.t array -> instruction -> instruction
val invert_test: Mach.test -> Mach.test

type fundecl =
  { fun_name: string;
    fun_body: instruction;
    fun_fast: bool }

val fundecl: Mach.fundecl -> fundecl

