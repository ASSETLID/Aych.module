/***********************************************************************/
/*                                                                     */
/*                           Objective Caml                            */
/*                                                                     */
/*         Xavier Leroy and Damien Doligez, INRIA Rocquencourt         */
/*                                                                     */
/*  Copyright 1996 Institut National de Recherche en Informatique et   */
/*  en Automatique.  All rights reserved.  This file is distributed    */
/*  under the terms of the GNU Library General Public License, with    */
/*  the special exception on linking described in file ../LICENSE.     */
/*                                                                     */
/***********************************************************************/

/* $Id$ */

#ifndef CAML_SIGNALS_H
#define CAML_SIGNALS_H

#ifndef CAML_NAME_SPACE
#include "compatibility.h"
#endif
#include "misc.h"
#include "mlvalues.h"

/* <private> */
extern value caml_signal_handlers;
CAMLextern int volatile caml_pending_signal;
CAMLextern int volatile caml_something_to_do;
extern int volatile caml_force_major_slice;
CAMLextern int volatile caml_async_signal_mode;
/* </private> */

CAMLextern void caml_enter_blocking_section (void);
CAMLextern void caml_leave_blocking_section (void);

/* <private> */
void caml_urge_major_slice (void);
CAMLextern int caml_convert_signal_number (int);
void caml_execute_signal(int signal_number, int in_signal_handler);
void caml_process_event(void);

CAMLextern void (*caml_enter_blocking_section_hook)(void);
CAMLextern void (*caml_leave_blocking_section_hook)(void);
CAMLextern void (* volatile caml_async_action_hook)(void);
/* </private> */

#endif /* CAML_SIGNALS_H */
