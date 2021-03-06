/***********************************************************************/
/*                                                                     */
/*                           Objective Caml                            */
/*                                                                     */
/*             Damien Doligez, projet Para, INRIA Rocquencourt         */
/*                                                                     */
/*  Copyright 1996 Institut National de Recherche en Informatique et   */
/*  en Automatique.  All rights reserved.  This file is distributed    */
/*  under the terms of the GNU Library General Public License, with    */
/*  the special exception on linking described in file ../LICENSE.     */
/*                                                                     */
/***********************************************************************/

/* $Id$ */

#include <limits.h>

#include "compact.h"
#include "custom.h"
#include "config.h"
#include "fail.h"
#include "finalise.h"
#include "freelist.h"
#include "gc.h"
#include "gc_ctrl.h"
#include "major_gc.h"
#include "misc.h"
#include "mlvalues.h"
#include "roots.h"
#include "weak.h"

unsigned long caml_percent_free;
long caml_major_heap_increment;
CAMLexport char *caml_heap_start, *caml_heap_end;
CAMLexport page_table_entry *caml_page_table;
asize_t caml_page_low, caml_page_high;
char *caml_gc_sweep_hp;
int caml_gc_phase;        /* always Phase_mark, Phase_sweep, or Phase_idle */
static value *gray_vals;
static value *gray_vals_cur, *gray_vals_end;
static asize_t gray_vals_size;
static int heap_is_pure;   /* The heap is pure if the only gray objects
                              below [markhp] are also in [gray_vals]. */
unsigned long caml_allocated_words;
unsigned long caml_dependent_size, caml_dependent_allocated;
double caml_extra_heap_resources;
unsigned long caml_fl_size_at_phase_change = 0;

extern char *caml_fl_merge;  /* Defined in freelist.c. */

static char *markhp, *chunk, *limit;

static int gc_subphase;     /* Subphase_main, Subphase_weak, Subphase_final */
#define Subphase_main 10
#define Subphase_weak 11
#define Subphase_final 12
static value *weak_prev;

static void realloc_gray_vals (void)
{
  value *new;

  Assert (gray_vals_cur == gray_vals_end);
  if (gray_vals_size < caml_stat_heap_size / 128){
    caml_gc_message (0x08, "Growing gray_vals to %luk bytes\n",
                     (long) gray_vals_size * sizeof (value) / 512);
    new = (value *) realloc ((char *) gray_vals,
                             2 * gray_vals_size * sizeof (value));
    if (new == NULL){
      caml_gc_message (0x08, "No room for growing gray_vals\n", 0);
      gray_vals_cur = gray_vals;
      heap_is_pure = 0;
    }else{
      gray_vals = new;
      gray_vals_cur = gray_vals + gray_vals_size;
      gray_vals_size *= 2;
      gray_vals_end = gray_vals + gray_vals_size;
    }
  }else{
    gray_vals_cur = gray_vals + gray_vals_size / 2;
    heap_is_pure = 0;
  }
}

void caml_darken (value v, value *p /* not used */)
{
  if (Is_block (v) && Is_in_heap (v)) {
    if (Tag_val(v) == Infix_tag) v -= Infix_offset_val(v);
    CAMLassert (!Is_blue_val (v));
    if (Is_white_val (v)){
      Hd_val (v) = Grayhd_hd (Hd_val (v));
      *gray_vals_cur++ = v;
      if (gray_vals_cur >= gray_vals_end) realloc_gray_vals ();
    }
  }
}

static void start_cycle (void)
{
  Assert (caml_gc_phase == Phase_idle);
  Assert (gray_vals_cur == gray_vals);
  caml_gc_message (0x01, "Starting new major GC cycle\n", 0);
  caml_darken_all_roots();
  caml_gc_phase = Phase_mark;
  gc_subphase = Subphase_main;
  markhp = NULL;
#ifdef DEBUG
  caml_heap_check ();
#endif
}

static void mark_slice (long work)
{
  value *gray_vals_ptr;  /* Local copy of gray_vals_cur */
  value v, child;
  header_t hd;
  mlsize_t size, i;

  caml_gc_message (0x40, "Marking %ld words\n", work);
  gray_vals_ptr = gray_vals_cur;
  while (work > 0){
    if (gray_vals_ptr > gray_vals){
      v = *--gray_vals_ptr;
      hd = Hd_val(v);
      Assert (Is_gray_hd (hd));
      Hd_val (v) = Blackhd_hd (hd);
      size = Wosize_hd (hd);
      if (Tag_hd (hd) < No_scan_tag){
        for (i = 0; i < size; i++){
          child = Field (v, i);
          if (Is_block (child) && Is_in_heap (child)) {
            hd = Hd_val (child);
            if (Tag_hd (hd) == Forward_tag){
              value f = Forward_val (child);
              if (Is_block (f) && (Is_young (f) || Is_in_heap (f))
                  && (Tag_val (f) == Forward_tag || Tag_val (f) == Lazy_tag
                      || Tag_val (f) == Double_tag)){
                /* Do not short-circuit the pointer. */
              }else{
                Field (v, i) = f;
              }
            }
            else if (Tag_hd(hd) == Infix_tag) {
              child -= Infix_offset_val(child);
              hd = Hd_val(child);
            }
            if (Is_white_hd (hd)){
              Hd_val (child) = Grayhd_hd (hd);
              *gray_vals_ptr++ = child;
              if (gray_vals_ptr >= gray_vals_end) {
                gray_vals_cur = gray_vals_ptr;
                realloc_gray_vals ();
                gray_vals_ptr = gray_vals_cur;
              }
            }
          }
        }
      }
      work -= Whsize_wosize(size);
    }else if (markhp != NULL){
      if (markhp == limit){
        chunk = Chunk_next (chunk);
        if (chunk == NULL){
          markhp = NULL;
        }else{
          markhp = chunk;
          limit = chunk + Chunk_size (chunk);
        }
      }else{
        if (Is_gray_val (Val_hp (markhp))){
          Assert (gray_vals_ptr == gray_vals);
          *gray_vals_ptr++ = Val_hp (markhp);
        }
        markhp += Bhsize_hp (markhp);
      }
    }else if (!heap_is_pure){
      heap_is_pure = 1;
      chunk = caml_heap_start;
      markhp = chunk;
      limit = chunk + Chunk_size (chunk);
    }else if (gc_subphase == Subphase_main){
      /* The main marking phase is over.  Start removing weak pointers to
         dead values. */
      gc_subphase = Subphase_weak;
      weak_prev = &caml_weak_list_head;
    }else if (gc_subphase == Subphase_weak){
      value cur, curfield;
      mlsize_t sz, i;
      header_t hd;

      cur = *weak_prev;
      if (cur != (value) NULL){
        hd = Hd_val (cur);
        if (Color_hd (hd) == Caml_white){
          /* The whole array is dead, remove it from the list. */
          *weak_prev = Field (cur, 0);
        }else{
          sz = Wosize_hd (hd);
          for (i = 1; i < sz; i++){
            curfield = Field (cur, i);
           weak_again:
            if (curfield != caml_weak_none
                && Is_block (curfield) && Is_in_heap (curfield)){
              if (Tag_val (curfield) == Forward_tag){
                value f = Forward_val (curfield);
                if (Is_block (f) && (Is_young (f) || Is_in_heap (f))){
                  if (Tag_val (f) == Forward_tag || Tag_val (f) == Lazy_tag
                      || Tag_val (f) == Double_tag){
                    /* Do not short-circuit the pointer. */
                  }else{
                    Field (cur, i) = curfield = f;
                    goto weak_again;
                  }
                }
              }
              if (Is_white_val (curfield)){
                Field (cur, i) = caml_weak_none;
              }
            }
          }
          weak_prev = &Field (cur, 0);
        }
        work -= Whsize_hd (hd);
      }else{
        /* Subphase_weak is done.  Handle finalised values. */
        gray_vals_cur = gray_vals_ptr;
        caml_final_update ();
        gray_vals_ptr = gray_vals_cur;
        gc_subphase = Subphase_final;
      }
    }else{
      Assert (gc_subphase == Subphase_final);
      /* Initialise the sweep phase. */
      gray_vals_cur = gray_vals_ptr;
      caml_gc_sweep_hp = caml_heap_start;
      caml_fl_init_merge ();
      caml_gc_phase = Phase_sweep;
      chunk = caml_heap_start;
      caml_gc_sweep_hp = chunk;
      limit = chunk + Chunk_size (chunk);
      work = 0;
      caml_fl_size_at_phase_change = caml_fl_cur_size;
    }
  }
  gray_vals_cur = gray_vals_ptr;
}

static void sweep_slice (long work)
{
  char *hp;
  header_t hd;

  caml_gc_message (0x40, "Sweeping %ld words\n", work);
  while (work > 0){
    if (caml_gc_sweep_hp < limit){
      hp = caml_gc_sweep_hp;
      hd = Hd_hp (hp);
      work -= Whsize_hd (hd);
      caml_gc_sweep_hp += Bhsize_hd (hd);
      switch (Color_hd (hd)){
      case Caml_white:
        if (Tag_hd (hd) == Custom_tag){
          void (*final_fun)(value) = Custom_ops_val(Val_hp(hp))->finalize;
          if (final_fun != NULL) final_fun(Val_hp(hp));
        }
        caml_gc_sweep_hp = caml_fl_merge_block (Bp_hp (hp));
        break;
      case Caml_blue:
        /* Only the blocks of the free-list are blue.  See [freelist.c]. */
        caml_fl_merge = Bp_hp (hp);
        break;
      default:          /* gray or black */
        Assert (Color_hd (hd) == Caml_black);
        Hd_hp (hp) = Whitehd_hd (hd);
        break;
      }
      Assert (caml_gc_sweep_hp <= limit);
    }else{
      chunk = Chunk_next (chunk);
      if (chunk == NULL){
        /* Sweeping is done. */
        ++ caml_stat_major_collections;
        work = 0;
        caml_gc_phase = Phase_idle;
      }else{
        caml_gc_sweep_hp = chunk;
        limit = chunk + Chunk_size (chunk);
      }
    }
  }
}

/* The main entry point for the GC.  Called after each minor GC.
   [howmuch] is the amount of work to do, 0 to let the GC compute it.
   Return the computed amount of work to do.
 */
long caml_major_collection_slice (long howmuch)
{
  double p, dp;
  long computed_work;
  /*
     Free memory at the start of the GC cycle (garbage + free list) (assumed):
                 FM = caml_stat_heap_size * caml_percent_free
                      / (100 + caml_percent_free)

     Assuming steady state and enforcing a constant allocation rate, then
     FM is divided in 2/3 for garbage and 1/3 for free list.
                 G = 2 * FM / 3
     G is also the amount of memory that will be used during this cycle
     (still assuming steady state).

     Proportion of G consumed since the previous slice:
                 PH = caml_allocated_words / G
                    = caml_allocated_words * 3 * (100 + caml_percent_free)
                      / (2 * caml_stat_heap_size * caml_percent_free)
     Proportion of extra-heap resources consumed since the previous slice:
                 PE = caml_extra_heap_resources
     Proportion of total work to do in this slice:
                 P  = max (PH, PE)
     Amount of marking work for the GC cycle:
                 MW = caml_stat_heap_size * 100 / (100 + caml_percent_free)
     Amount of sweeping work for the GC cycle:
                 SW = caml_stat_heap_size
     Amount of marking work for this slice:
                 MS = P * MW
                 MS = P * caml_stat_heap_size * 100 / (100 + caml_percent_free)
     Amount of sweeping work for this slice:
                 SS = P * SW
                 SS = P * caml_stat_heap_size
     This slice will either mark 2*MS words or sweep 2*SS words.
  */

  if (caml_gc_phase == Phase_idle) start_cycle ();

  p = (double) caml_allocated_words * 3.0 * (100 + caml_percent_free)
      / Wsize_bsize (caml_stat_heap_size) / caml_percent_free / 2.0;
  if (caml_dependent_size > 0){
    dp = (double) caml_dependent_allocated * (100 + caml_percent_free)
         / caml_dependent_size / caml_percent_free;
  }else{
    dp = 0.0;
  }
  if (p < dp) p = dp;
  if (p < caml_extra_heap_resources) p = caml_extra_heap_resources;

  caml_gc_message (0x40, "allocated_words = %lu\n", caml_allocated_words);
  caml_gc_message (0x40, "extra_heap_resources = %luu\n",
                   (unsigned long) (caml_extra_heap_resources * 1000000));
  caml_gc_message (0x40, "amount of work to do = %luu\n",
                   (unsigned long) (p * 1000000));

  if (caml_gc_phase == Phase_mark){
    computed_work = 2 * (long) (p * Wsize_bsize (caml_stat_heap_size) * 100
                                / (100 + caml_percent_free));
  }else{
    computed_work = 2 * (long) (p * Wsize_bsize (caml_stat_heap_size));
  }
  caml_gc_message (0x40, "ordered work = %ld words\n", howmuch);
  caml_gc_message (0x40, "computed work = %ld words\n", computed_work);
  if (howmuch == 0) howmuch = computed_work;
  if (caml_gc_phase == Phase_mark){
    mark_slice (howmuch);
    caml_gc_message (0x02, "!", 0);
  }else{
    Assert (caml_gc_phase == Phase_sweep);
    sweep_slice (howmuch);
    caml_gc_message (0x02, "$", 0);
  }

  if (caml_gc_phase == Phase_idle) caml_compact_heap_maybe ();

  caml_stat_major_words += caml_allocated_words;
  caml_allocated_words = 0;
  caml_dependent_allocated = 0;
  caml_extra_heap_resources = 0.0;
  return computed_work;
}

/* The minor heap must be empty when this function is called;
   the minor heap is empty when this function returns.
*/
/* This does not call caml_compact_heap_maybe because the estimations of
   free and live memory are only valid for a cycle done incrementally.
   Besides, this function is called by caml_compact_heap_maybe.
*/
void caml_finish_major_cycle (void)
{
  if (caml_gc_phase == Phase_idle) start_cycle ();
  while (caml_gc_phase == Phase_mark) mark_slice (LONG_MAX);
  Assert (caml_gc_phase == Phase_sweep);
  while (caml_gc_phase == Phase_sweep) sweep_slice (LONG_MAX);
  Assert (caml_gc_phase == Phase_idle);
  caml_stat_major_words += caml_allocated_words;
  caml_allocated_words = 0;
}

/* Make sure the request is at least Heap_chunk_min and round it up
   to a multiple of the page size.
*/
static asize_t clip_heap_chunk_size (asize_t request)
{
  if (request < Bsize_wsize (Heap_chunk_min)){
    request = Bsize_wsize (Heap_chunk_min);
  }
  return ((request + Page_size - 1) >> Page_log) << Page_log;
}

/* Make sure the request is >= caml_major_heap_increment, then call
   clip_heap_chunk_size, then make sure the result is >= request.
*/
asize_t caml_round_heap_chunk_size (asize_t request)
{
  asize_t result = request;

  if (result < caml_major_heap_increment){
    result = caml_major_heap_increment;
  }
  result = clip_heap_chunk_size (result);

  if (result < request){
    caml_raise_out_of_memory ();
    return 0; /* not reached */
  }
  return result;
}

void caml_init_major_heap (asize_t heap_size)
{
  asize_t i;
  asize_t page_table_size;
  page_table_entry *page_table_block;

  caml_stat_heap_size = clip_heap_chunk_size (heap_size);
  caml_stat_top_heap_size = caml_stat_heap_size;
  Assert (caml_stat_heap_size % Page_size == 0);
  caml_heap_start = (char *) caml_alloc_for_heap (caml_stat_heap_size);
  if (caml_heap_start == NULL)
    caml_fatal_error ("Fatal error: not enough memory for the initial heap.\n");
  Chunk_next (caml_heap_start) = NULL;
  caml_heap_end = caml_heap_start + caml_stat_heap_size;
  Assert ((unsigned long) caml_heap_end % Page_size == 0);

  caml_stat_heap_chunks = 1;

  caml_page_low = Page (caml_heap_start);
  caml_page_high = Page (caml_heap_end);

  page_table_size = caml_page_high - caml_page_low;
  page_table_block =
    (page_table_entry *) malloc (page_table_size * sizeof (page_table_entry));
  if (page_table_block == NULL){
    caml_fatal_error ("Fatal error: not enough memory for the initial heap.\n");
  }
  caml_page_table = page_table_block - caml_page_low;
  for (i = Page (caml_heap_start); i < Page (caml_heap_end); i++){
    caml_page_table [i] = In_heap;
  }

  caml_fl_init_merge ();
  caml_make_free_blocks ((value *) caml_heap_start,
                         Wsize_bsize (caml_stat_heap_size), 1);
  caml_gc_phase = Phase_idle;
  gray_vals_size = 2048;
  gray_vals = (value *) malloc (gray_vals_size * sizeof (value));
  if (gray_vals == NULL)
    caml_fatal_error ("Fatal error: not enough memory for the initial heap.\n");
  gray_vals_cur = gray_vals;
  gray_vals_end = gray_vals + gray_vals_size;
  heap_is_pure = 1;
  caml_allocated_words = 0;
  caml_extra_heap_resources = 0.0;
}
