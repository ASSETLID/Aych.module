(* camlp4r *)
(***********************************************************************)
(*                                                                     *)
(*                             Camlp4                                  *)
(*                                                                     *)
(*        Daniel de Rauglaudre, projet Cristal, INRIA Rocquencourt     *)
(*                                                                     *)
(*  Copyright 2002 Institut National de Recherche en Informatique et   *)
(*  Automatique.  Distributed only by permission.                      *)
(*                                                                     *)
(***********************************************************************)

(* $Id$ *)

(** A lexical analyzer. *)

value gmake : unit -> Token.glexer Token.t;
   (** Some lexer provided. See the module [Token]. The tokens returned
       follow the Objective Caml and the Revised syntax lexing rules.

       The meaning of the tokens are:
-      * [("", s)] is the keyword [s].
-      * [("LIDENT", s)] is the ident [s] starting with a lowercase letter.
-      * [("UIDENT", s)] is the ident [s] starting with an uppercase letter.
-      * [("INT", s)] (resp. ["INT32"], ["INT64"] and ["NATIVEINT"])
         is an integer constant whose string source is [s].
-      * [("FLOAT", s)] is a float constant whose string source is [s].
-      * [("STRING", s)] is the string constant [s].
-      * [("CHAR", s)] is the character constant [s].
-      * [("QUOTATION", "t:s")] is a quotation [t] holding the string [s].
-      * [("ANTIQUOT", "t:s")] is an antiquotation [t] holding the string [s].
-      * [("LOCATE", "i:s")] is a location directive at pos [i] holding [s].
-      * [("EOI", "")] is the end of input.

       The associated token patterns in the EXTEND statement hold the
       same names than the first string (constructor name) of the tokens
       expressions above.

       Warning: the string associated with the constructor [STRING] is
       the string found in the source without any interpretation. In
       particular, the backslashes are not interpreted. For example, if
       the input is ["\n"] the string is *not* a string with one
       element containing the character "return", but a string of two
       elements: the backslash and the character ["n"]. To interpret
       a string use the function [Token.eval_string]. Same thing for
       the constructor [CHAR]: to get the character, don't get the
       first character of the string, but use the function
       [Token.eval_char].

       The lexer do not use global (mutable) variables: instantiations
       of [Plexer.gmake ()] do not perturb each other.  *)

value make_lexer :
    unit -> (Token.glexer Token.t * (ref int * ref int * ref string));
   (** [make_lexer] builds a lexer as [gmake does], but returns also
       the triple [(bolpos, lnum, fname)] where
- [bolpos] contains the character number of the beginning of the current line,
- [lnum] contains the current line number and
- [fname] contains the name of the file being parsed. *)

value dollar_for_antiquotation : ref bool;
   (** When True (default), the next call to [Plexer.make ()] returns a
       lexer where the dollar sign is used for antiquotations. If False,
       the dollar sign can be used as token. *)

value specific_space_dot : ref bool;
   (** When False (default), the next call to [Plexer.make ()] returns a
       lexer where the dots can be preceded by spaces. If True, dots
       preceded by spaces return the keyword " ." (space dot), otherwise
       return the keyword "." (dot). *)

value no_quotations : ref bool;
   (** When True, all lexers built by [Plexer.make ()] do not lex the
       quotation syntax any more. Default is False (quotations are
       lexed). *)

(**/**)

(* deprecated since version 3.05; use rather function gmake *)
value make : unit -> Token.lexer;
