/*
**   $Id$
**
**   acsmx2.c
**
**   Multi-Pattern Search Engine
**
**   Aho-Corasick State Machine - version 2.0
**
**   Supports both Non-Deterministic and Deterministic Finite Automata
**
**
**   Reference - Efficient String matching: An Aid to Bibliographic Search
**               Alfred V Aho and Margaret J Corasick
**               Bell Labratories
**               Copyright(C) 1975 Association for Computing Machinery,Inc
**
**   +++
**   +++ Version 1.0 notes - Marc Norton:
**   +++
**
**   Original implementation based on the 4 algorithms in the paper by Aho & Corasick,
**   some implementation ideas from 'Practical Algorithms in C', and some
**   of my own.
**
**   1) Finds all occurrences of all patterns within a text.
**
**   +++
**   +++ Version 2.0 Notes - Marc Norton/Dan Roelker:
**   +++
**
**   New implementation modifies the state table storage and access model to use
**   compacted sparse vector storage. Dan Roelker and I hammered this strategy out
**   amongst many others in order to reduce memory usage and improve caching performance.
**   The memory usage is greatly reduced, we only use 1/4 of what we use to. The caching
**   performance is better in pure benchmarking tests, but does not show overall improvement
**   in Snort.  Unfortunately, once a pattern match test has been performed Snort moves on to doing
**   many other things before we get back to a patteren match test, so the cache is voided.
**
**   This versions has better caching performance characteristics, reduced memory,
**   more state table storage options, and requires no a priori case conversions.
**   It does maintain the same public interface. (Snort only used banded storage).
**
**     1) Supports NFA and DFA state machines, and basic keyword state machines
**     2) Initial transition table uses Linked Lists
**     3) Improved state table memory options. NFA and DFA state
**        transition tables are converted to one of 4 formats during compilation.
**        a) Full matrix
**        b) Sparse matrix
**        c) Banded matrix (Default-this is the only one used in snort)
**        d) Sparse-Banded matrix
**     4) Added support for acstate_t in .h file so we can compile states as
**        16, or 32 bit state values for another reduction in memory consumption,
**        smaller states allows more of the state table to be cached, and improves
**        performance on x86-P4.  Your mileage may vary, especially on risc systems.
**     5) Added a bool to each state transition list to indicate if there is a matching
**        pattern in the state. This prevents us from accessing another data array
**        and can improve caching/performance.
**     6) The search functions are very sensitive, don't change them without extensive testing,
**        or you'll just spoil the caching and prefetching opportunities.
**
**   Extras for fellow pattern matchers:
**    The table below explains the storage format used at each step.
**    You can use an NFA or DFA to match with, the NFA is slower but tiny - set the structure directly.
**    You can use any of the 4 storage modes above -full,sparse,banded,sparse-bands, set the structure directly.
**    For applications where you have lots of data and a pattern set to search, this version was up to 3x faster
**    than the previous verion, due to caching performance. This cannot be fully realized in Snort yet,
**    but other applications may have better caching opportunities.
**    Snort only needs to use the banded or full storage.
**
**  Transition table format at each processing stage.
**  -------------------------------------------------
**  Patterns -> Keyword State Table (List)
**  Keyword State Table -> NFA (List)
**  NFA -> DFA (List)
**  DFA (List)-> Sparse Rows  O(m-avg # transitions per state)
**        -> Banded Rows  O(1)
**            -> Sparse-Banded Rows O(nb-# bands)
**        -> Full Matrix  O(1)
**
** Copyright(C) 2002,2003,2004 Marc Norton
** Copyright(C) 2003,2004 Daniel Roelker
** Copyright(C) 2002,2003,2004 Sourcefire,Inc.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*
* Notes:
*
* 8/28/06
* man - Sparse and SparseBands - fixed off by one in calculating matching index
*       SparseBands changed ps increment to 2+n to increment between bands.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "acsmx2.h"

/*
*
*/
#define MEMASSERT(p,s) if(!p){printf("ACSM-No Memory: %s!\n",s);exit(0);}

/*
*
*/
static int max_memory = 0;

/*
*
*/
static int s_verbose=0;

/*
*
*/
typedef struct acsm_summary_s
{
      unsigned    num_states;
      unsigned    num_transitions;
      ACSM_STRUCT2 acsm;

}acsm_summary_t;

/*
*
*/
static acsm_summary_t summary={0,0};

/*
** Case Translation Table
*/
static unsigned char xlatcase[256];
/*
*
*/
static
void
init_xlatcase()
{
  int i;
  for (i = 0; i < 256; i++)
    {
      xlatcase[i] = toupper(i);
    }
}
/*
*    Case Conversion
*/
static
inline
void
ConvertCaseEx (unsigned char *d, unsigned char *s, int m)
{
  int i;
#ifdef XXXX
  int n;
  n   = m & 3;
  m >>= 2;

  for (i = 0; i < m; i++ )
    {
      d[0] = xlatcase[ s[0] ];
      d[2] = xlatcase[ s[2] ];
      d[1] = xlatcase[ s[1] ];
      d[3] = xlatcase[ s[3] ];
      d+=4;
      s+=4;
    }

  for (i=0; i < n; i++)
    {
      d[i] = xlatcase[ s[i] ];
    }
#else
  for (i=0; i < m; i++)
    {
      d[i] = xlatcase[ s[i] ];
    }

#endif
}


/*
*
*/
void acsmSetVerbose2(int n)
{
     s_verbose = 1;
}

/*
*
*/
static void *
AC_MALLOC (int n)
{
  void *p;
  p = malloc (n);
  if (p)
    max_memory += n;
  return p;
}


/*
*
*/
static void
AC_FREE (void *p)
{
  if (p)
    free (p);
}


/*
*    Simple QUEUE NODE
*/
typedef struct _qnode
{
  int state;
   struct _qnode *next;
}
QNODE;

/*
*    Simple QUEUE Structure
*/
typedef struct _queue
{
  QNODE * head, *tail;
  int count;
}
QUEUE;

/*
*   Initialize the queue
*/
static void
queue_init (QUEUE * s)
{
  s->head = s->tail = 0;
  s->count= 0;
}

/*
*  Find a State in the queue
*/
static int
queue_find (QUEUE * s, int state)
{
  QNODE * q;
  q = s->head;
  while( q )
  {
      if( q->state == state ) return 1;
      q = q->next;
  }
  return 0;
}

/*
*  Add Tail Item to queue (FiFo/LiLo)
*/
static void
queue_add (QUEUE * s, int state)
{
  QNODE * q;

  if( queue_find( s, state ) ) return;

  if (!s->head)
  {
      q = s->tail = s->head = (QNODE *) AC_MALLOC (sizeof (QNODE));
      MEMASSERT (q, "queue_add");
      q->state = state;
      q->next = 0;
  }
  else
  {
      q = (QNODE *) AC_MALLOC (sizeof (QNODE));
      q->state = state;
      q->next = 0;
      s->tail->next = q;
      s->tail = q;
  }
  s->count++;
}


/*
*  Remove Head Item from queue
*/
static int
queue_remove (QUEUE * s)
{
  int state = 0;
  QNODE * q;
  if (s->head)
  {
      q       = s->head;
      state   = q->state;
      s->head = s->head->next;
      s->count--;

      if( !s->head )
      {
          s->tail = 0;
          s->count = 0;
      }
      AC_FREE (q);
  }
  return state;
}


/*
*   Return items in the queue
*/
static int
queue_count (QUEUE * s)
{
  return s->count;
}


/*
*  Free the queue
*/
static void
queue_free (QUEUE * s)
{
  while (queue_count (s))
    {
      queue_remove (s);
    }
}

/*
*  Get Next State-NFA
*/
static
int List_GetNextState( ACSM_STRUCT2 * acsm, int state, int input )
{
  trans_node_t * t = acsm->acsmTransTable[state];

  while( t )
  {
    if( t->key == input )
    {
        return t->next_state;
    }
    t=t->next;
  }

  if( state == 0 ) return 0;

  return ACSM_FAIL_STATE2; /* Fail state ??? */
}

/*
*  Get Next State-DFA
*/
static
int List_GetNextState2( ACSM_STRUCT2 * acsm, int state, int input )
{
  trans_node_t * t = acsm->acsmTransTable[state];

  while( t )
  {
    if( t->key == input )
    {
      return t->next_state;
    }
    t = t->next;
  }

  return 0; /* default state */
}
/*
*  Put Next State - Head insertion, and transition updates
*/
static
int List_PutNextState( ACSM_STRUCT2 * acsm, int state, int input, int next_state )
{
  trans_node_t * p;
  trans_node_t * tnew;

 // printf("   List_PutNextState: state=%d, input='%c', next_state=%d\n",state,input,next_state);


  /* Check if the transition already exists, if so just update the next_state */
  p = acsm->acsmTransTable[state];
  while( p )
  {
    if( p->key == input )  /* transition already exists- reset the next state */
    {
        p->next_state = next_state;
        return 0;
    }
    p=p->next;
  }

  /* Definitely not an existing transition - add it */
  tnew = (trans_node_t*)AC_MALLOC(sizeof(trans_node_t));
  if( !tnew ) return -1;

  tnew->key        = input;
  tnew->next_state = next_state;
  tnew->next       = 0;

  tnew->next = acsm->acsmTransTable[state];
  acsm->acsmTransTable[state] = tnew;

  acsm->acsmNumTrans++;

  return 0;
}
/*
*   Free the entire transition table
*/
static
int List_FreeTransTable( ACSM_STRUCT2 * acsm )
{
  int i;
  trans_node_t * t, *p;

  if( !acsm->acsmTransTable ) return 0;

  for(i=0;i< acsm->acsmMaxStates;i++)
  {
     t = acsm->acsmTransTable[i];

     while( t )
     {
       p = t->next;
       free(t);
       t = p;
       max_memory -= sizeof(trans_node_t);
     }
   }

   free(acsm->acsmTransTable);

   max_memory -= sizeof(void*) * acsm->acsmMaxStates;

   acsm->acsmTransTable = 0;

   return 0;
}

/*
*
*/
/*
static
int List_FreeList( trans_node_t * t )
{
  int tcnt=0;

  trans_node_t *p;

  while( t )
  {
       p = t->next;
       free(t);
       t = p;
       max_memory -= sizeof(trans_node_t);
       tcnt++;
   }

   return tcnt;
}
*/

/*
*    Print the trans table to stdout
*/
static
int List_PrintTransTable( ACSM_STRUCT2 * acsm )
{
  int i;
  trans_node_t * t;
  ACSM_PATTERN2 * patrn;

  if( !acsm->acsmTransTable ) return 0;

  printf("Print Transition Table- %d active states\n",acsm->acsmNumStates);

  for(i=0;i< acsm->acsmNumStates;i++)
  {
     t = acsm->acsmTransTable[i];

     printf("state %3d: ",i);

     while( t )
     {
       if( isprint(t->key) )
         printf("%3c->%-5d\t",t->key,t->next_state);
       else
         printf("%3d->%-5d\t",t->key,t->next_state);

       t = t->next;
     }

     patrn =acsm->acsmMatchList[i];

     while( patrn )
     {
         printf("%.*s ",patrn->n,patrn->patrn);

         patrn = patrn->next;
     }

     printf("\n");
   }
   return 0;
}


/*
*   Converts row of states from list to a full vector format
*/
static
int List_ConvToFull(ACSM_STRUCT2 * acsm, acstate_t state, acstate_t * full )
{
    int tcnt = 0;
    trans_node_t * t = acsm->acsmTransTable[ state ];

    memset(full,0,sizeof(acstate_t)*acsm->acsmAlphabetSize);

    if( !t ) return 0;

    while(t)
    {
      full[ t->key ] = t->next_state;
      tcnt++;
      t = t->next;
    }
    return tcnt;
}

/*
*   Copy a Match List Entry - don't dup the pattern data
*/
static ACSM_PATTERN2*
CopyMatchListEntry (ACSM_PATTERN2 * px)
{
  ACSM_PATTERN2 * p;

  p = (ACSM_PATTERN2 *) AC_MALLOC (sizeof (ACSM_PATTERN2));
  MEMASSERT (p, "CopyMatchListEntry");

  memcpy (p, px, sizeof (ACSM_PATTERN2));

  p->next = 0;

  return p;
}

/*
*  Check if a pattern is in the list already,
*  validate it using the 'id' field. This must be unique
*  for every pattern.
*/
/*
static
int FindMatchListEntry (ACSM_STRUCT2 * acsm, int state, ACSM_PATTERN2 * px)
{
  ACSM_PATTERN2 * p;

  p = acsm->acsmMatchList[state];
  while( p )
  {
    if( p->id == px->id ) return 1;
    p = p->next;
  }

  return 0;
}
*/


/*
*  Add a pattern to the list of patterns terminated at this state.
*  Insert at front of list.
*/
static void
AddMatchListEntry (ACSM_STRUCT2 * acsm, int state, ACSM_PATTERN2 * px)
{
  ACSM_PATTERN2 * p;

  p = (ACSM_PATTERN2 *) AC_MALLOC (sizeof (ACSM_PATTERN2));

  MEMASSERT (p, "AddMatchListEntry");

  memcpy (p, px, sizeof (ACSM_PATTERN2));

  p->next = acsm->acsmMatchList[state];

  acsm->acsmMatchList[state] = p;
}


static void
AddPatternStates (ACSM_STRUCT2 * acsm, ACSM_PATTERN2 * p)
{
  int            state, next, n;
  unsigned char *pattern;

  n       = p->n;
  pattern = p->patrn;
  state   = 0;

  if(s_verbose)printf(" Begin AddPatternStates: acsmNumStates=%d\n",acsm->acsmNumStates);
  if(s_verbose)printf("    adding '%.*s', nocase=%d\n", n,p->patrn, p->nocase );

  /*
  *  Match up pattern with existing states
  */
  for (; n > 0; pattern++, n--)
  {
      if(s_verbose)printf(" find char='%c'\n", *pattern );

      next = List_GetNextState(acsm,state,*pattern);
      if (next == ACSM_FAIL_STATE2 || next == 0)
      {
             break;
      }
      state = next;
  }

  /*
  *   Add new states for the rest of the pattern bytes, 1 state per byte
  */
  for (; n > 0; pattern++, n--)
  {
      if(s_verbose)printf(" add char='%c' state=%d NumStates=%d\n", *pattern, state, acsm->acsmNumStates );

      acsm->acsmNumStates++;
      List_PutNextState(acsm,state,*pattern,acsm->acsmNumStates);
      state = acsm->acsmNumStates;
  }

  AddMatchListEntry (acsm, state, p );

  if(s_verbose)printf(" End AddPatternStates: acsmNumStates=%d\n",acsm->acsmNumStates);
}

/*
*   Build A Non-Deterministic Finite Automata
*   The keyword state table must already be built, via AddPatternStates().
*/
static void
Build_NFA (ACSM_STRUCT2 * acsm)
{
    int r, s, i;
    QUEUE q, *queue = &q;
    acstate_t     * FailState = acsm->acsmFailState;
    ACSM_PATTERN2 ** MatchList = acsm->acsmMatchList;
    ACSM_PATTERN2  * mlist,* px;

    /* Init a Queue */
    queue_init (queue);


    /* Add the state 0 transitions 1st, the states at depth 1, fail to state 0 */
    for (i = 0; i < acsm->acsmAlphabetSize; i++)
    {
      s = List_GetNextState2(acsm,0,i);
      if( s )
      {
          queue_add (queue, s);
          FailState[s] = 0;
      }
    }

    /* Build the fail state successive layer of transitions */
    while (queue_count (queue) > 0)
    {
        r = queue_remove (queue);

        /* Find Final States for any Failure */
        for (i = 0; i < acsm->acsmAlphabetSize; i++)
        {
           int fs, next;

           s = List_GetNextState(acsm,r,i);

           if( s != ACSM_FAIL_STATE2 )
           {
                queue_add (queue, s);

                fs = FailState[r];

                /*
                 *  Locate the next valid state for 'i' starting at fs
                 */
                while( (next=List_GetNextState(acsm,fs,i)) == ACSM_FAIL_STATE2 )
                {
                  fs = FailState[fs];
                }

                /*
                 *  Update 's' state failure state to point to the next valid state
                 */
                FailState[s] = next;

                /*
                 *  Copy 'next'states MatchList to 's' states MatchList,
                 *  we copy them so each list can be AC_FREE'd later,
                 *  else we could just manipulate pointers to fake the copy.
                 */
                for( mlist = MatchList[next];
                     mlist;
                     mlist = mlist->next)
                {
                    px = CopyMatchListEntry (mlist);

                    /* Insert at front of MatchList */
                    px->next = MatchList[s];
                    MatchList[s] = px;
                }
           }
        }
    }

    /* Clean up the queue */
    queue_free (queue);

    if( s_verbose)printf("End Build_NFA: NumStates=%d\n",acsm->acsmNumStates);
}

/*
*   Build Deterministic Finite Automata from the NFA
*/
static void
Convert_NFA_To_DFA (ACSM_STRUCT2 * acsm)
{
    int i, r, s, cFailState;
    QUEUE  q, *queue = &q;
    acstate_t * FailState = acsm->acsmFailState;

    /* Init a Queue */
    queue_init (queue);

    /* Add the state 0 transitions 1st */
    for(i=0; i<acsm->acsmAlphabetSize; i++)
    {
      s = List_GetNextState(acsm,0,i);
      if ( s != 0 )
      {
          queue_add (queue, s);
      }
    }

    /* Start building the next layer of transitions */
    while( queue_count(queue) > 0 )
    {
        r = queue_remove(queue);

        /* Process this states layer */
        for (i = 0; i < acsm->acsmAlphabetSize; i++)
        {
          s = List_GetNextState(acsm,r,i);

          if( s != ACSM_FAIL_STATE2 && s!= 0)
          {
              queue_add (queue, s);
          }
          else
          {
              cFailState = List_GetNextState(acsm,FailState[r],i);

              if( cFailState != 0 && cFailState != ACSM_FAIL_STATE2 )
              {
                  List_PutNextState(acsm,r,i,cFailState);
              }
          }
        }
    }

    /* Clean up the queue */
    queue_free (queue);

    if(s_verbose)printf("End Convert_NFA_To_DFA: NumStates=%d\n",acsm->acsmNumStates);

}

/*
*
*  Convert a row lists for the state table to a full vector format
*
*/
static int
Conv_List_To_Full(ACSM_STRUCT2 * acsm)
{
  int         k;
  acstate_t * p;
  acstate_t ** NextState = acsm->acsmNextState;

  for(k=0;k<acsm->acsmMaxStates;k++)
  {
    p = AC_MALLOC( sizeof(acstate_t) * (acsm->acsmAlphabetSize+2) );
    if(!p) return -1;

    List_ConvToFull( acsm, (acstate_t)k, p+2 );

    p[0] = ACF_FULL;
    p[1] = 0; /* no matches yet */

    NextState[k] = p; /* now we have a full format row vector  */
  }

  return 0;
}

/*
*   Convert DFA memory usage from list based storage to a sparse-row storage.
*
*   The Sparse format allows each row to be either full or sparse formatted.  If the sparse row has
*   too many transitions, performance or space may dictate that we use the standard full formatting
*   for the row.  More than 5 or 10 transitions per state ought to really whack performance. So the
*   user can specify the max state transitions per state allowed in the sparse format.
*
*   Standard Full Matrix Format
*   ---------------------------
*   acstate_t ** NextState ( 1st index is row/state, 2nd index is column=event/input)
*
*   example:
*
*        events -> a b c d e f g h i j k l m n o p
*   states
*     N            1 7 0 0 0 3 0 0 0 0 0 0 0 0 0 0
*
*   Sparse Format, each row : Words     Value
*                            1-1       fmt(0-full,1-sparse,2-banded,3-sparsebands)
*                            2-2       bool match flag (indicates this state has pattern matches)
*                            3-3       sparse state count ( # of input/next-state pairs )
*                            4-3+2*cnt 'input,next-state' pairs... each sizof(acstate_t)
*
*   above example case yields:
*     Full Format:    0, 1 7 0 0 0 3 0 0 0 0 0 0 0 0 0 0 ...
*     Sparse format:  1, 3, 'a',1,'b',7,'f',3  - uses 2+2*ntransitions (non-default transitions)
*/
static int
Conv_Full_DFA_To_Sparse(ACSM_STRUCT2 * acsm)
{
  int          cnt, m, k, i;
  acstate_t  * p, state, maxstates=0;
  acstate_t ** NextState = acsm->acsmNextState;
  acstate_t    full[MAX_ALPHABET_SIZE];

  for(k=0;k<acsm->acsmMaxStates;k++)
  {
    cnt=0;

    List_ConvToFull(acsm, (acstate_t)k, full );

    for (i = 0; i < acsm->acsmAlphabetSize; i++)
    {
       state = full[i];
       if( state != 0 && state != ACSM_FAIL_STATE2 ) cnt++;
    }

    if( cnt > 0 ) maxstates++;

    if( k== 0 || cnt > acsm->acsmSparseMaxRowNodes )
    {
       p = AC_MALLOC(sizeof(acstate_t)*(acsm->acsmAlphabetSize+2) );
       if(!p) return -1;

       p[0] = ACF_FULL;
       p[1] = 0;
       memcpy(&p[2],full,acsm->acsmAlphabetSize*sizeof(acstate_t));
    }
    else
    {
       p = AC_MALLOC(sizeof(acstate_t)*(3+2*cnt));
       if(!p) return -1;

       m      = 0;
       p[m++] = ACF_SPARSE;
       p[m++] = 0;   /* no matches */
       p[m++] = cnt;

       for(i = 0; i < acsm->acsmAlphabetSize ; i++)
       {
         state = full[i];
         if( state != 0 && state != ACSM_FAIL_STATE2 )
         {
           p[m++] = i;
           p[m++] = state;
         }
      }
    }

    NextState[k] = p; /* now we are a sparse formatted state transition array  */
  }

  return 0;
}
/*
    Convert Full matrix to Banded row format.

    Word     values
    1        2  -> banded
    2        n  number of values
    3        i  index of 1st value (0-256)
    4 - 3+n  next-state values at each index

*/
static int
Conv_Full_DFA_To_Banded(ACSM_STRUCT2 * acsm)
{
  int first = -1, last;
  acstate_t * p, state, full[MAX_ALPHABET_SIZE];
  acstate_t ** NextState = acsm->acsmNextState;
  int       cnt,m,k,i;

  for(k=0;k<acsm->acsmMaxStates;k++)
  {
    cnt=0;

    List_ConvToFull(acsm, (acstate_t)k, full );

    first=-1;
    last =-2;

    for (i = 0; i < acsm->acsmAlphabetSize; i++)
    {
       state = full[i];

       if( state !=0 && state != ACSM_FAIL_STATE2 )
       {
           if( first < 0 ) first = i;
           last = i;
       }
    }

    /* calc band width */
    cnt= last - first + 1;

    p = AC_MALLOC(sizeof(acstate_t)*(4+cnt));

    if(!p) return -1;

    m      = 0;
    p[m++] = ACF_BANDED;
    p[m++] = 0;   /* no matches */
    p[m++] = cnt;
    p[m++] = first;

    for(i = first; i <= last; i++)
    {
       p[m++] = full[i];
    }

    NextState[k] = p; /* now we are a banded formatted state transition array  */
  }

  return 0;
}

/*
*   Convert full matrix to Sparse Band row format.
*
*   next  - Full formatted row of next states
*   asize - size of alphabet
*   zcnt - max number of zeros in a run of zeros in any given band.
*
*  Word Values
*  1    ACF_SPARSEBANDS
*  2    number of bands
*  repeat 3 - 5+ ....once for each band in this row.
*  3    number of items in this band*  4    start index of this band
*  5-   next-state values in this band...
*/
static
int calcSparseBands( acstate_t * next, int * begin, int * end, int asize, int zmax )
{
   int i, nbands,zcnt,last=0;
   acstate_t state;

   nbands=0;
   for( i=0; i<asize; i++ )
   {
       state = next[i];
       if( state !=0 && state != ACSM_FAIL_STATE2 )
       {
           begin[nbands] = i;
           zcnt=0;
           for( ; i< asize; i++ )
           {
              state = next[i];
              if( state ==0 || state == ACSM_FAIL_STATE2 )
              {
                  zcnt++;
                  if( zcnt > zmax ) break;
              }
              else
              {
                  zcnt=0;
                  last = i;
              }
           }
           end[nbands++] = last;
       }
   }
   return nbands;
}


/*
*   Sparse Bands
*
*   Row Format:
*   Word
*   1    SPARSEBANDS format indicator
*   2    bool indicates a pattern match in this state
*   3    number of sparse bands
*   4    number of elements in this band
*   5    start index of this band
*   6-   list of next states
*
*   m    number of elements in this band
*   m+1  start index of this band
*   m+2- list of next states
*/
static int
Conv_Full_DFA_To_SparseBands(ACSM_STRUCT2 * acsm)
{
  acstate_t  * p;
  acstate_t ** NextState = acsm->acsmNextState;
  int          cnt,m,k,i,zcnt=acsm->acsmSparseMaxZcnt;

  int       band_begin[MAX_ALPHABET_SIZE];
  int       band_end[MAX_ALPHABET_SIZE];
  int       nbands,j;
  acstate_t full[MAX_ALPHABET_SIZE];

  for(k=0;k<acsm->acsmMaxStates;k++)
  {
    cnt=0;

    List_ConvToFull(acsm, (acstate_t)k, full );

    nbands = calcSparseBands( full, band_begin, band_end, acsm->acsmAlphabetSize, zcnt );

    /* calc band width space*/
    cnt = 3;
    for(i=0;i<nbands;i++)
    {
       cnt += 2;
       cnt += band_end[i] - band_begin[i] + 1;

       /*printf("state %d: sparseband %d,  first=%d, last=%d, cnt=%d\n",k,i,band_begin[i],band_end[i],band_end[i]-band_begin[i]+1); */
    }

    p = AC_MALLOC(sizeof(acstate_t)*(cnt));

    if(!p) return -1;

    m      = 0;
    p[m++] = ACF_SPARSEBANDS;
    p[m++] = 0; /* no matches */
    p[m++] = nbands;

    for( i=0;i<nbands;i++ )
    {
      p[m++] = band_end[i] - band_begin[i] + 1;  /* # states in this band */
      p[m++] = band_begin[i];   /* start index */

      for( j=band_begin[i]; j<=band_end[i]; j++ )
      {
         p[m++] = full[j];  /* some states may be state zero */
      }
    }

    NextState[k] = p; /* now we are a sparse-banded formatted state transition array  */
  }

  return 0;
}

void
Print_DFA_MatchList( ACSM_STRUCT2 * acsm, int state )
{
     ACSM_PATTERN2 * mlist;

     for (mlist = acsm->acsmMatchList[state];
          mlist;
          mlist = mlist->next)
     {
        printf("%.*s ", mlist->n, mlist->patrn);
     }
}
/*
*
*/
static void
Print_DFA(ACSM_STRUCT2 * acsm)
{
  int  k,i;
  acstate_t * p, state, n, fmt, index, nb;
  acstate_t ** NextState = acsm->acsmNextState;

  printf("Print DFA - %d active states\n",acsm->acsmNumStates);

  for(k=0;k<acsm->acsmNumStates;k++)
  {
    p   = NextState[k];

    if( !p ) continue;

    fmt = *p++;

    /* bmatch =  */ p++;

    printf("state %3d, fmt=%d: ",k,fmt);

    if( fmt ==ACF_SPARSE )
    {
       n = *p++;
       for( ; n>0; n--, p+=2 )
       {
         if( isprint(p[0]) )
         printf("%3c->%-5d\t",p[0],p[1]);
         else
         printf("%3d->%-5d\t",p[0],p[1]);
      }
    }
    else if( fmt ==ACF_BANDED )
    {

       n = *p++;
       index = *p++;

       for( ; n>0; n--, p++ )
       {
         if( isprint(p[0]) )
         printf("%3c->%-5d\t",index++,p[0]);
         else
         printf("%3d->%-5d\t",index++,p[0]);
      }
    }
    else if( fmt ==ACF_SPARSEBANDS )
    {
       nb    = *p++;
       for(i=0;i<nb;i++)
       {
         n     = *p++;
         index = *p++;
         for( ; n>0; n--, p++ )
         {
           if( isprint(index) )
           printf("%3c->%-5d\t",index++,p[0]);
           else
           printf("%3d->%-5d\t",index++,p[0]);
         }
       }
    }
    else if( fmt == ACF_FULL )
    {

      for( i=0; i<acsm->acsmAlphabetSize; i++ )
      {
         state = p[i];

         if( state != 0 && state != ACSM_FAIL_STATE2 )
         {
           if( isprint(i) )
             printf("%3c->%-5d\t",i,state);
           else
             printf("%3d->%-5d\t",i,state);
         }
      }
    }

    Print_DFA_MatchList( acsm, k);

    printf("\n");
  }
}
/*
*  Write a state table to disk
*/
/*
static void
Write_DFA(ACSM_STRUCT2 * acsm, char * f)
{
  int  k,i;
  acstate_t * p, n, fmt, index, nb, bmatch;
  acstate_t ** NextState = acsm->acsmNextState;
  FILE * fp;

  printf("Dump DFA - %d active states\n",acsm->acsmNumStates);

  fp = fopen(f,"wb");
  if(!fp)
   {
     printf("*** WARNING: could not write dfa to file - %s\n",f);
     return;
   }

  fwrite( &acsm->acsmNumStates, 4, 1, fp);

  for(k=0;k<acsm->acsmNumStates;k++)
  {
    p   = NextState[k];

    if( !p ) continue;

    fmt = *p++;

    bmatch = *p++;

    fwrite( &fmt,    sizeof(acstate_t), 1, fp);
    fwrite( &bmatch, sizeof(acstate_t), 1, fp);

    if( fmt ==ACF_SPARSE )
    {
       n = *p++;
       fwrite( &n,     sizeof(acstate_t), 1, fp);
       fwrite(  p, n*2*sizeof(acstate_t), 1, fp);
    }
    else if( fmt ==ACF_BANDED )
    {
       n = *p++;
       fwrite( &n,     sizeof(acstate_t), 1, fp);

       index = *p++;
       fwrite( &index, sizeof(acstate_t), 1, fp);

       fwrite(  p, sizeof(acstate_t), n, fp);
    }
    else if( fmt ==ACF_SPARSEBANDS )
    {
       nb    = *p++;
       fwrite( &nb,    sizeof(acstate_t), 1, fp);
       for(i=0;i<nb;i++)
       {
         n     = *p++;
         fwrite( &n,    sizeof(acstate_t), 1, fp);

         index = *p++;
         fwrite( &index,sizeof(acstate_t), 1, fp);

         fwrite( p,     sizeof(acstate_t), 1, fp);
       }
    }
    else if( fmt == ACF_FULL )
    {
      fwrite( p,  sizeof(acstate_t), acsm->acsmAlphabetSize,  fp);
    }

    //Print_DFA_MatchList( acsm, k);

  }

  fclose(fp);
}
*/

/*
*
*   Convert an NFA or DFA row from sparse to full format
*   and store into the 'full'  buffer.
*
*   returns:
*     0 - failed, no state transitions
*    *p - pointer to 'full' buffer
*
*/
/*
static
acstate_t * acsmConvToFull(ACSM_STRUCT2 * acsm, acstate_t k, acstate_t * full )
{
    int i;
    acstate_t * p, n, fmt, index, nb, bmatch;
    acstate_t ** NextState = acsm->acsmNextState;

    p   = NextState[k];

    if( !p ) return 0;

    fmt = *p++;

    bmatch = *p++;

    if( fmt ==ACF_SPARSE )
    {
       n = *p++;
       for( ; n>0; n--, p+=2 )
       {
         full[ p[0] ] = p[1];
      }
    }
    else if( fmt ==ACF_BANDED )
    {

       n = *p++;
       index = *p++;

       for( ; n>0; n--, p++ )
       {
         full[ index++ ] = p[0];
      }
    }
    else if( fmt ==ACF_SPARSEBANDS )
    {
       nb    = *p++;
       for(i=0;i<nb;i++)
       {
         n     = *p++;
         index = *p++;
         for( ; n>0; n--, p++ )
         {
           full[ index++ ] = p[0];
         }
       }
    }
    else if( fmt == ACF_FULL )
    {
      memcpy(full,p,acsm->acsmAlphabetSize*sizeof(acstate_t));
    }

    return full;
}
*/

/*
*   Select the desired storage mode
*/
int acsmSelectFormat2( ACSM_STRUCT2 * acsm, int m )
{
 switch( m )
 {
    case ACF_FULL:
    case ACF_SPARSE:
    case ACF_BANDED:
    case ACF_SPARSEBANDS:
      acsm->acsmFormat = m;
      break;
    default:
      return -1;
 }

 return 0;
}
/*
*
*/
void acsmSetMaxSparseBandZeros2( ACSM_STRUCT2 * acsm, int n )
{
    acsm->acsmSparseMaxZcnt = n;
}
/*
*
*/
void acsmSetMaxSparseElements2( ACSM_STRUCT2 * acsm, int n )
{
    acsm->acsmSparseMaxRowNodes = n;
}
/*
*
*/
int acsmSelectFSA2( ACSM_STRUCT2 * acsm, int m )
{
 switch( m )
 {
    case FSA_TRIE:
    case FSA_NFA:
    case FSA_DFA:
      acsm->acsmFSA = m;
    default:
      return -1;
 }
}
/*
*
*/
int acsmSetAlphabetSize2( ACSM_STRUCT2 * acsm, int n )
{
   if( n <= MAX_ALPHABET_SIZE )
   {
     acsm->acsmAlphabetSize = n;
   }
   else
   {
      return -1;
   }
   return 0;
}
/*
*  Create a new AC state machine
*/
ACSM_STRUCT2 * acsmNew2 ()
{
  ACSM_STRUCT2 * p;

  init_xlatcase ();

  p = (ACSM_STRUCT2 *) AC_MALLOC(sizeof (ACSM_STRUCT2));
  MEMASSERT (p, "acsmNew");

  if (p)
  {
    memset (p, 0, sizeof (ACSM_STRUCT2));

    /* Some defaults */
    p->acsmFSA               = FSA_DFA;
    p->acsmFormat            = ACF_FULL;//ACF_BANDED;
    p->acsmAlphabetSize      = 256;
    p->acsmSparseMaxRowNodes = 256;
    p->acsmSparseMaxZcnt     = 10;
  }

  return p;
}
/*
*   Add a pattern to the list of patterns for this state machine
*
*/
int
acsmAddPattern2 (ACSM_STRUCT2 * p, unsigned char *pat, int n, int nocase,
        int offset, int depth, void * id, int iid)
{
  ACSM_PATTERN2 * plist;

  plist = (ACSM_PATTERN2 *) AC_MALLOC (sizeof (ACSM_PATTERN2));
  MEMASSERT (plist, "acsmAddPattern");

  plist->patrn = (unsigned char *) AC_MALLOC ( n );
  MEMASSERT (plist->patrn, "acsmAddPattern");

  ConvertCaseEx(plist->patrn, pat, n);

  plist->casepatrn = (unsigned char *) AC_MALLOC ( n );
  MEMASSERT (plist->casepatrn, "acsmAddPattern");

  memcpy (plist->casepatrn, pat, n);

  plist->n      = n;
  plist->nocase = nocase;
  plist->offset = offset;
  plist->depth  = depth;
  plist->id     = id;
  plist->iid    = iid;

  plist->next     = p->acsmPatterns;
  p->acsmPatterns = plist;

  return 0;
}
/*
*   Add a Key to the list of key+data pairs
*/
int acsmAddKey2(ACSM_STRUCT2 * p, unsigned char *key, int klen, int nocase, void * data)
{
  ACSM_PATTERN2 * plist;

  plist = (ACSM_PATTERN2 *) AC_MALLOC (sizeof (ACSM_PATTERN2));
  MEMASSERT (plist, "acsmAddPattern");

  plist->patrn = (unsigned char *) AC_MALLOC (klen);
  memcpy (plist->patrn, key, klen);

  plist->casepatrn = (unsigned char *) AC_MALLOC (klen);
  memcpy (plist->casepatrn, key, klen);

  plist->n      = klen;
  plist->nocase = nocase;
  plist->offset = 0;
  plist->depth  = 0;
  plist->id     = 0;
  plist->iid = 0;

  plist->next = p->acsmPatterns;
  p->acsmPatterns = plist;

  return 0;
}

/*
*  Copy a boolean match flag int NextState table, for caching purposes.
*/
static
void acsmUpdateMatchStates( ACSM_STRUCT2 * acsm )
{
  acstate_t        state;
  acstate_t     ** NextState = acsm->acsmNextState;
  ACSM_PATTERN2 ** MatchList = acsm->acsmMatchList;

  for( state=0; state<acsm->acsmNumStates; state++ )
  {
     if( MatchList[state] )
     {
         NextState[state][1] = 1;
     }
     else
     {
         NextState[state][1] = 0;
     }
  }
}

/*
*   Compile State Machine - NFA or DFA and Full or Banded or Sparse or SparseBands
*/
int
acsmCompile2 (ACSM_STRUCT2 * acsm)
{
    int               k;
    ACSM_PATTERN2    * plist;

    /* Count number of states */
    for (plist = acsm->acsmPatterns; plist != NULL; plist = plist->next)
    {
     acsm->acsmMaxStates += plist->n;
     /* acsm->acsmMaxStates += plist->n*2; if we handle case in the table */
    }
    acsm->acsmMaxStates++; /* one extra */

    /* Alloc a List based State Transition table */
    acsm->acsmTransTable =(trans_node_t**) AC_MALLOC(sizeof(trans_node_t*) * acsm->acsmMaxStates );
    MEMASSERT (acsm->acsmTransTable, "acsmCompile");

    memset (acsm->acsmTransTable, 0, sizeof(trans_node_t*) * acsm->acsmMaxStates);

    if(s_verbose)printf ("ACSMX-Max Memory-TransTable Setup: %d bytes, %d states, %d active states\n", max_memory,acsm->acsmMaxStates,acsm->acsmNumStates);

    /* Alloc a failure table - this has a failure state, and a match list for each state */
    acsm->acsmFailState =(acstate_t*) AC_MALLOC(sizeof(acstate_t) * acsm->acsmMaxStates );
    MEMASSERT (acsm->acsmFailState, "acsmCompile");

    memset (acsm->acsmFailState, 0, sizeof(acstate_t) * acsm->acsmMaxStates );

    /* Alloc a MatchList table - this has a lis tof pattern matches for each state, if any */
    acsm->acsmMatchList=(ACSM_PATTERN2**) AC_MALLOC(sizeof(ACSM_PATTERN2*) * acsm->acsmMaxStates );
    MEMASSERT (acsm->acsmMatchList, "acsmCompile");

    memset (acsm->acsmMatchList, 0, sizeof(ACSM_PATTERN2*) * acsm->acsmMaxStates );

    if(s_verbose)printf ("ACSMX-Max Memory- MatchList Table Setup: %d bytes, %d states, %d active states\n", max_memory,acsm->acsmMaxStates,acsm->acsmNumStates);

    /* Alloc a separate state transition table == in state 's' due to event 'k', transition to 'next' state */
    acsm->acsmNextState=(acstate_t**)AC_MALLOC( acsm->acsmMaxStates * sizeof(acstate_t*) );
    MEMASSERT(acsm->acsmNextState, "acsmCompile-NextState");

    for (k = 0; k < acsm->acsmMaxStates; k++)
    {
      acsm->acsmNextState[k]=(acstate_t*)0;
    }

    if(s_verbose)printf ("ACSMX-Max Memory-Table Setup: %d bytes, %d states, %d active states\n", max_memory,acsm->acsmMaxStates,acsm->acsmNumStates);

    /* Initialize state zero as a branch */
    acsm->acsmNumStates = 0;

    /* Add the 0'th state,  */
    //acsm->acsmNumStates++;

    /* Add each Pattern to the State Table - This forms a keywords state table  */
    for (plist = acsm->acsmPatterns; plist != NULL; plist = plist->next)
    {
        AddPatternStates (acsm, plist);
    }

    acsm->acsmNumStates++;

    if(s_verbose)printf ("ACSMX-Max Trie List Memory : %d bytes, %d states, %d active states\n",
                 max_memory,acsm->acsmMaxStates,acsm->acsmNumStates);

    if(s_verbose)List_PrintTransTable( acsm );

    if( acsm->acsmFSA == FSA_DFA || acsm->acsmFSA == FSA_NFA )
    {
      /* Build the NFA */
      if(s_verbose)printf("Build_NFA\n");

      Build_NFA (acsm);

      if(s_verbose)printf("NFA-Trans-Nodes: %d\n",acsm->acsmNumTrans);
      if(s_verbose)printf("ACSMX-Max NFA List Memory  : %d bytes, %d states / %d active states\n", max_memory,acsm->acsmMaxStates,acsm->acsmNumStates);

      if(s_verbose)List_PrintTransTable( acsm );
    }

    if( acsm->acsmFSA == FSA_DFA )
    {
       /* Convert the NFA to a DFA */
       if(s_verbose)printf("Convert_NFA_To_DFA\n");

       Convert_NFA_To_DFA (acsm);

       if(s_verbose)printf("DFA-Trans-Nodes: %d\n",acsm->acsmNumTrans);
       if(s_verbose)printf("ACSMX-Max NFA-DFA List Memory  : %d bytes, %d states / %d active states\n", max_memory,acsm->acsmMaxStates,acsm->acsmNumStates);

       if(s_verbose)List_PrintTransTable( acsm );
    }

    /*
    *
    *  Select Final Transition Table Storage Mode
    *
    */

    if(s_verbose)printf("Converting Transition Lists -> Transition table, fmt=%d\n",acsm->acsmFormat);

    if( acsm->acsmFormat == ACF_SPARSE )
    {
      /* Convert DFA Full matrix to a Sparse matrix */
      if( Conv_Full_DFA_To_Sparse(acsm) )
          return -1;

      if(s_verbose)printf ("ACSMX-Max Memory-Sparse: %d bytes, %d states, %d active states\n", max_memory,acsm->acsmMaxStates,acsm->acsmNumStates);
      if(s_verbose)Print_DFA(acsm);
    }

    else if( acsm->acsmFormat == ACF_BANDED )
    {
      /* Convert DFA Full matrix to a Sparse matrix */
      if( Conv_Full_DFA_To_Banded(acsm) )
          return -1;

       if(s_verbose)printf ("ACSMX-Max Memory-banded: %d bytes, %d states, %d active states\n", max_memory,acsm->acsmMaxStates,acsm->acsmNumStates);
       if(s_verbose)Print_DFA(acsm);
    }

    else if( acsm->acsmFormat == ACF_SPARSEBANDS )
    {
      /* Convert DFA Full matrix to a Sparse matrix */
      if( Conv_Full_DFA_To_SparseBands(acsm) )
          return -1;

       if(s_verbose)printf ("ACSMX-Max Memory-sparse-bands: %d bytes, %d states, %d active states\n", max_memory,acsm->acsmMaxStates,acsm->acsmNumStates);
       if(s_verbose)Print_DFA(acsm);
    }
    else if( acsm->acsmFormat == ACF_FULL )
    {
      if( Conv_List_To_Full( acsm ) )
            return -1;

       if(s_verbose)printf ("ACSMX-Max Memory-Full: %d bytes, %d states, %d active states\n", max_memory,acsm->acsmMaxStates,acsm->acsmNumStates);
       if(s_verbose)Print_DFA(acsm);
    }

    acsmUpdateMatchStates( acsm ); /* load boolean match flags into state table */

    /* Free up the Table Of Transition Lists */
    List_FreeTransTable( acsm );

    if(s_verbose)printf ("ACSMX-Max Memory-Final: %d bytes, %d states, %d active states\n", max_memory,acsm->acsmMaxStates,acsm->acsmNumStates);

    /* For now -- show this info */
    /*
    *  acsmPrintInfo( acsm );
    */


    /* Accrue Summary State Stats */
    summary.num_states      += acsm->acsmNumStates;
    summary.num_transitions += acsm->acsmNumTrans;

    memcpy( &summary.acsm, acsm, sizeof(ACSM_STRUCT2));

    return 0;
}

/*
*   Get the NextState from the NFA, all NFA storage formats use this
*/
inline
acstate_t SparseGetNextStateNFA(acstate_t * ps, acstate_t state, unsigned  input)
{
   acstate_t fmt;
   acstate_t n;
   unsigned int       index;
   int       nb;

   fmt = *ps++;

   ps++;  /* skip bMatchState */

   switch( fmt )
   {
    case  ACF_BANDED:
    {
     n     = ps[0];
     index = ps[1];

     if( input <  index     )
     {
         if(state==0)
         {
           return 0;
         }
         else
         {
           return (acstate_t)ACSM_FAIL_STATE2;
         }
     }
     if( input >= index + n )
     {
          if(state==0)
          {
              return 0;
          }
          else
          {
              return (acstate_t)ACSM_FAIL_STATE2;
          }
     }
     if( ps[input-index] == 0  )
     {
         if( state != 0 )
         {
           return ACSM_FAIL_STATE2;
         }
     }

     return (acstate_t) ps[input-index];
    }

    case ACF_SPARSE:
    {
     n = *ps++; /* number of sparse index-value entries */

     for( ; n>0 ; n-- )
     {
       if( ps[0] > input ) /* cannot match the input, already a higher value than the input  */
       {
           return (acstate_t)ACSM_FAIL_STATE2; /* default state */
       }
       else if( ps[0] == input )
       {
           return ps[1]; /* next state */
       }
       ps+=2;
     }
     if( state == 0 )
     {
       return 0;
     }
     return ACSM_FAIL_STATE2;
    }

    case ACF_SPARSEBANDS:
    {
     nb  = *ps++;   /* number of bands */

     while( nb > 0 )  /* for each band */
     {
       n     = *ps++;  /* number of elements */
       index = *ps++;  /* 1st element value */

       if( input <  index )
       {
           if( state != 0 )
           {
               return (acstate_t)ACSM_FAIL_STATE2;
           }
           return (acstate_t)0;
       }
       if( (input >=  index) && (input < (index + n)) )
       {
           if( ps[input-index] == 0 )
           {
               if( state != 0 )
               {
                   return ACSM_FAIL_STATE2;
               }
           }
           return (acstate_t) ps[input-index];
       }
       nb--;
       ps += n;
     }
     if( state != 0 )
     {
       return (acstate_t)ACSM_FAIL_STATE2;
     }
     return (acstate_t)0;
    }

    case ACF_FULL:
    {
      if( ps[input] == 0 )
      {
        if( state != 0 )
        {
            return ACSM_FAIL_STATE2;
        }
      }
      return ps[input];
    }
  }

  return 0;
}



/*
*   Get the NextState from the DFA Next State Transition table
*   Full and banded are supported separately, this is for
*   sparse and sparse-bands
*/
inline
acstate_t SparseGetNextStateDFA(acstate_t * ps, acstate_t state, unsigned  input)
{
   acstate_t  n, nb;
   unsigned int        index;

   switch( ps[0] )
   {
    /*   BANDED   */
    case  ACF_BANDED:
    {
       /* n=ps[2] : number of entries in the band */
       /* index=ps[3] : index of the 1st entry, sequential thereafter */

       if( input  <  ps[3]        )  return 0;
       if( input >= (unsigned)(ps[3]+ps[2]) )  return 0;

       return  ps[4+input-ps[3]];
    }

    /*   FULL   */
    case ACF_FULL:
    {
       return ps[2+input];
    }

    /*   SPARSE   */
    case ACF_SPARSE:
    {
       n = ps[2]; /* number of entries/ key+next pairs */

       ps += 3;

       for( ; n>0 ; n-- )
       {
          if( input < ps[0]  ) /* cannot match the input, already a higher value than the input  */
          {
            return (acstate_t)0; /* default state */
          }
          else if( ps[0] == input )
          {
            return ps[1]; /* next state */
          }
          ps += 2;
       }
       return (acstate_t)0;
    }


    /*   SPARSEBANDS   */
    case ACF_SPARSEBANDS:
    {
       nb  =  ps[2]; /* number of bands */

       ps += 3;

       while( nb > 0 )  /* for each band */
       {
          n     = ps[0];  /* number of elements in this band */
          index = ps[1];  /* start index/char of this band */
          if( input <  index )
          {
            return (acstate_t)0;
          }
          if( (input < (index + n)) )
          {
            return (acstate_t) ps[2+input-index];
          }
          nb--;
          ps += 2 + n;
       }
       return (acstate_t)0;
    }
  }

  return 0;
}
/*
*   Search Text or Binary Data for Pattern matches
*
*   Sparse & Sparse-Banded Matrix search
*/
static
inline
int
acsmSearchSparseDFA(ACSM_STRUCT2 * acsm, unsigned char *Tx, int n,
            int (*Match) (void * id, int index, void *data),
            void *data)
{
  acstate_t state;
  ACSM_PATTERN2   * mlist;
  unsigned char   * Tend;
  int               nfound = 0;
  unsigned char   * T, * Tc;
  int               index;
  acstate_t      ** NextState = acsm->acsmNextState;
  ACSM_PATTERN2  ** MatchList = acsm->acsmMatchList;

  Tc   = Tx;
  T    = Tx;
  Tend = T + n;

  for( state = 0; T < Tend; T++ )
  {
      state = SparseGetNextStateDFA ( NextState[state], state, xlatcase[*T] );

      /* test if this state has any matching patterns */
      if( NextState[state][1] )
      {
        for( mlist = MatchList[state];
             mlist!= NULL;
             mlist = mlist->next )
        {
             index = T - mlist->n - Tc + 1;
             if( mlist->nocase )
             {
                nfound++;
                if (Match (mlist->id, index, data))
                    return nfound;
             }
             else
             {
                if( memcmp (mlist->casepatrn, Tx + index, mlist->n) == 0 )
                {
                    nfound++;
                    if (Match (mlist->id, index, data))
                        return nfound;
                }
             }
        }
      }
  }
  return nfound;
}
/*
*   Full format DFA search
*   Do not change anything here without testing, caching and prefetching
*   performance is very sensitive to any changes.
*
*   Perf-Notes:
*    1) replaced ConvertCaseEx with inline xlatcase - this improves performance 5-10%
*    2) using 'nocase' improves performance again by 10-15%, since memcmp is not needed
*    3)
*/
static
inline
int
acsmSearchSparseDFA_Full(ACSM_STRUCT2 * acsm, unsigned char *Tx, int n,
            int (*Match) (void * id, int index, void *data),
            void *data)
{
  ACSM_PATTERN2   * mlist;
  unsigned char   * Tend;
  unsigned char   * T;
  int               index;
  acstate_t         state;
  acstate_t       * ps;
  acstate_t         sindex;
  acstate_t      ** NextState = acsm->acsmNextState;
  ACSM_PATTERN2  ** MatchList = acsm->acsmMatchList;
  int               nfound    = 0;

  T    = Tx;
  Tend = Tx + n;

  for( state = 0; T < Tend; T++ )
  {
      ps     = NextState[ state ];

      sindex = xlatcase[ T[0] ];

      /* check the current state for a pattern match */
      if( ps[1] )
      {
        for( mlist = MatchList[state];
             mlist!= NULL;
             mlist = mlist->next )
        {
             index = T - mlist->n - Tx;

             if( mlist->nocase )
             {
                nfound++;
                if (Match (mlist->id, index, data))
                    return nfound;
             }
             else
             {
                if( memcmp (mlist->casepatrn, Tx + index, mlist->n ) == 0 )
                {
                    nfound++;
                    if (Match (mlist->id, index, data))
                        return nfound;
                }
             }
        }
      }

      state = ps[ 2u + sindex ];
  }

  /* Check the last state for a pattern match */
  for( mlist = MatchList[state];
       mlist!= NULL;
       mlist = mlist->next )
  {
      index = T - mlist->n - Tx;

      if( mlist->nocase )
      {
        nfound++;
        if (Match (mlist->id, index, data))
            return nfound;
      }
      else
      {
        if( memcmp (mlist->casepatrn, Tx + index, mlist->n) == 0 )
        {
            nfound++;
            if (Match (mlist->id, index, data))
                return nfound;
        }
      }
  }

  return nfound;
}
/*
*   Banded-Row format DFA search
*   Do not change anything here, caching and prefetching
*   performance is very sensitive to any changes.
*
*   ps[0] = storage fmt
*   ps[1] = bool match flag
*   ps[2] = # elements in band
*   ps[3] = index of 1st element
*/
static
inline
int
acsmSearchSparseDFA_Banded(ACSM_STRUCT2 * acsm, unsigned char *Tx, int n,
            int (*Match) (void * id, int index, void *data),
            void *data)
{
  acstate_t         state;
  unsigned char   * Tend;
  unsigned char   * T;
  int               sindex;
  int               index;
  acstate_t      ** NextState = acsm->acsmNextState;
  ACSM_PATTERN2  ** MatchList = acsm->acsmMatchList;
  ACSM_PATTERN2   * mlist;
  acstate_t       * ps;
  int               nfound = 0;

  T    = Tx;
  Tend = T + n;

  for( state = 0; T < Tend; T++ )
  {
      ps     = NextState[state];

      sindex = xlatcase[ T[0] ];

      /* test if this state has any matching patterns */
      if( ps[1] )
      {
        for( mlist = MatchList[state];
             mlist!= NULL;
             mlist = mlist->next )
        {
             index = T - mlist->n - Tx;

             if( mlist->nocase )
             {
                nfound++;
                if (Match (mlist->id, index, data))
                    return nfound;
             }
             else
             {
                if( memcmp (mlist->casepatrn, Tx + index, mlist->n) == 0 )
                {
                    nfound++;
                    if (Match (mlist->id, index, data))
                        return nfound;
                }
             }
        }
      }

      if(      sindex <   ps[3]          )  state = 0;
      else if( sindex >= (ps[3] + ps[2]) )  state = 0;
      else                                  state = ps[ 4u + sindex - ps[3] ];
  }

  /* Check the last state for a pattern match */
  for( mlist = MatchList[state];
       mlist!= NULL;
       mlist = mlist->next )
  {
      index = T - mlist->n - Tx;

      if( mlist->nocase )
      {
        nfound++;
        if (Match (mlist->id, index, data))
            return nfound;
      }
      else
      {
        if( memcmp (mlist->casepatrn, Tx + index, mlist->n) == 0 )
        {
          nfound++;
          if (Match (mlist->id, index, data))
            return nfound;
        }
      }
  }

  return nfound;
}



/*
*   Search Text or Binary Data for Pattern matches
*
*   Sparse Storage Version
*/
static
inline
int
acsmSearchSparseNFA(ACSM_STRUCT2 * acsm, unsigned char *Tx, int n,
            int (*Match) (void * id, int index, void *data),
            void *data)
{
  acstate_t         state;
  ACSM_PATTERN2   * mlist;
  unsigned char   * Tend;
  int               nfound = 0;
  unsigned char   * T /* , *Tc */;
  int               index;
  acstate_t      ** NextState= acsm->acsmNextState;
  acstate_t       * FailState= acsm->acsmFailState;
  ACSM_PATTERN2  ** MatchList = acsm->acsmMatchList;
  unsigned char     Tchar;

  // Tc   = Tx;
  T    = Tx;
  Tend = T + n;

  for( state = 0; T < Tend; T++ )
  {
      acstate_t nstate;

      Tchar = xlatcase[ *T ];

      while( (nstate=SparseGetNextStateNFA(NextState[state],state,Tchar))==ACSM_FAIL_STATE2 )
              state = FailState[state];

      state = nstate;

      for( mlist = MatchList[state];
           mlist!= NULL;
           mlist = mlist->next )
      {
           index = T - mlist->n - Tx;
           if( mlist->nocase )
           {
              nfound++;
              if (Match (mlist->id, index, data))
                  return nfound;
           }
           else
           {
              if( memcmp (mlist->casepatrn, Tx + index, mlist->n) == 0 )
              {
                nfound++;
                if (Match (mlist->id, index, data))
                    return nfound;
              }
           }
      }
  }

  return nfound;
}

/*
*   Search Function
*/
int
acsmSearch2(ACSM_STRUCT2 * acsm, unsigned char *Tx, int n,
           int (*Match) (void * id, int index, void *data),
           void *data)
{

   switch( acsm->acsmFSA )
   {
       case FSA_DFA:

       if( acsm->acsmFormat == ACF_FULL )
       {
         return acsmSearchSparseDFA_Full( acsm, Tx, n, Match,data );
       }
       else if( acsm->acsmFormat == ACF_BANDED )
       {
         return acsmSearchSparseDFA_Banded( acsm, Tx, n, Match,data );
       }
       else
       {
         return acsmSearchSparseDFA( acsm, Tx, n, Match,data );
       }

       case FSA_NFA:

         return acsmSearchSparseNFA( acsm, Tx, n, Match,data );

       case FSA_TRIE:

         return 0;
   }
  return 0;
}


/*
*   Free all memory
*/
  void
acsmFree2 (ACSM_STRUCT2 * acsm)
{
  int i;
  ACSM_PATTERN2 * mlist, *ilist;
  for (i = 0; i < acsm->acsmMaxStates; i++)
  {
      mlist = acsm->acsmMatchList[i];

      while (mlist)
      {
          ilist = mlist;
          mlist = mlist->next;
          AC_FREE (ilist);
      }
          AC_FREE(acsm->acsmNextState[i]);
  }
  AC_FREE(acsm->acsmFailState);
  AC_FREE(acsm->acsmMatchList);
}

/*
*
*/
void acsmPrintInfo2( ACSM_STRUCT2 * p)
{
    char * sf[]={
      "Full Matrix",
      "Sparse Matrix",
      "Banded Matrix",
      "Sparse Banded Matrix",
    };
    char * fsa[]={
      "TRIE",
      "NFA",
      "DFA",
    };


    printf("+--[Pattern Matcher:Aho-Corasick]-----------------------------\n");
    printf("| Alphabet Size    : %d Chars\n",p->acsmAlphabetSize);
    printf("| Sizeof State     : %d bytes\n",(int)(sizeof(acstate_t)));
    printf("| Storage Format   : %s \n",sf[ p->acsmFormat ]);
    printf("| Sparse Row Nodes : %d Max\n",p->acsmSparseMaxRowNodes);
    printf("| Sparse Band Zeros: %d Max\n",p->acsmSparseMaxZcnt);
    printf("| Num States       : %d\n",p->acsmNumStates);
    printf("| Num Transitions  : %d\n",p->acsmNumTrans);
    printf("| State Density    : %.1f%%\n",100.0*(double)p->acsmNumTrans/(p->acsmNumStates*p->acsmAlphabetSize));
    printf("| Finite Automatum : %s\n", fsa[p->acsmFSA]);
    if( max_memory < 1024*1024 )
    printf("| Memory           : %.2fKbytes\n", (float)max_memory/1024 );
    else
    printf("| Memory           : %.2fMbytes\n", (float)max_memory/(1024*1024) );
    printf("+-------------------------------------------------------------\n");

    /* Print_DFA(acsm); */

}

/*
 *
 */
int acsmPrintDetailInfo2( ACSM_STRUCT2 * p )
{

    return 0;
}

/*
 *   Global sumary of all info and all state machines built during this run
 *   This feeds off of the last pattern groupd built within snort,
 *   all groups use the same format, state size, etc..
 *   Combined with accrued stats, we get an average picture of things.
 */
int acsmPrintSummaryInfo2()
{
    char * sf[]={
      "Full",
      "Sparse",
      "Banded",
      "Sparse-Bands",
    };

    char * fsa[]={
      "TRIE",
      "NFA",
      "DFA",
    };

    ACSM_STRUCT2 * p = &summary.acsm;

    if( !summary.num_states )
        return 0;

    printf("+--[Pattern Matcher:Aho-Corasick Summary]----------------------\n");
    printf("| Alphabet Size    : %d Chars\n",p->acsmAlphabetSize);
    printf("| Sizeof State     : %d bytes\n",(int)(sizeof(acstate_t)));
    printf("| Storage Format   : %s \n",sf[ p->acsmFormat ]);
    printf("| Num States       : %d\n",summary.num_states);
    printf("| Num Transitions  : %d\n",summary.num_transitions);
    printf("| State Density    : %.1f%%\n",100.0*(double)summary.num_transitions/(summary.num_states*p->acsmAlphabetSize));
    printf("| Finite Automatum : %s\n", fsa[p->acsmFSA]);
    if( max_memory < 1024*1024 )
    printf("| Memory           : %.2fKbytes\n", (float)max_memory/1024 );
    else
    printf("| Memory           : %.2fMbytes\n", (float)max_memory/(1024*1024) );
    printf("+-------------------------------------------------------------\n");


    return 0;
}




#ifdef ACSMX2S_MAIN

/*
*  Text Data Buffer
*/
unsigned char text[512];

/*
*    A Match is found
*/
 int
MatchFound (void* id, int index, void *data)
{
  fprintf (stdout, "%s\n", (char *) id);
  return 0;
}

/*
*
*/
int
main (int argc, char **argv)
{
  int i, nc, nocase = 0;
  ACSM_STRUCT2 * acsm;
  char * p;

  if (argc < 3)

    {
      fprintf (stderr,"Usage: %s search-text pattern +pattern... [flags]\n",argv[0]);
      fprintf (stderr,"  flags: -nfa -nocase -full -sparse -bands -sparsebands -z zcnt (sparsebands) -sparsetree -v\n");
      exit (0);
    }

  acsm = acsmNew2 ();
  if( !acsm )
  {
     printf("acsm-no memory\n");
     exit(0);
  }

  strcpy (text, argv[1]);

  acsm->acsmFormat = ACF_FULL;

  for (i = 1; i < argc; i++)
  {
    if (strcmp (argv[i], "-nocase") == 0){
      nocase = 1;
    }
    if (strcmp (argv[i], "-v") == 0){
      s_verbose=1;
    }

    if (strcmp (argv[i], "-full") == 0){
       acsm->acsmFormat            = ACF_FULL;
    }
    if (strcmp (argv[i], "-sparse") == 0){
       acsm->acsmFormat            = ACF_SPARSE;
       acsm->acsmSparseMaxRowNodes = 10;
    }
    if (strcmp (argv[i], "-bands") == 0){
       acsm->acsmFormat            = ACF_BANDED;
    }

    if (strcmp (argv[i], "-sparsebands") == 0){
       acsm->acsmFormat            = ACF_SPARSEBANDS;
       acsm->acsmSparseMaxZcnt     = 10;
    }
    if (strcmp (argv[i], "-z") == 0){
       acsm->acsmSparseMaxZcnt     = atoi(argv[++i]);
    }

    if (strcmp (argv[i], "-nfa") == 0){
       acsm->acsmFSA     = FSA_NFA;
    }
    if (strcmp (argv[i], "-dfa") == 0){
       acsm->acsmFSA     = FSA_DFA;
    }
    if (strcmp (argv[i], "-trie") == 0){
       acsm->acsmFSA     = FSA_TRIE;
    }
  }

  for (i = 2; i < argc; i++)
  {
      if (argv[i][0] == '-')
          continue;

      p = argv[i];

      if ( *p == '+')
      {
          nc=1;
          p++;
      }
      else
      {
          nc = nocase;
      }

      acsmAddPattern2 (acsm, p, strlen(p), nc, 0, 0,(void*)p, i - 2);
  }

  if(s_verbose)printf("Patterns added\n");

  Print_DFA (acsm);

  acsmCompile2 (acsm);

  //Write_DFA(acsm, "acsmx2-snort.dfa") ;

  if(s_verbose) printf("Patterns compiled--written to file.\n");

  acsmPrintInfo2 ( acsm );

  acsmSearch2 (acsm, text, strlen (text), MatchFound, (void *)0 );

  acsmFree2 (acsm);

  printf ("normal pgm end\n");

  return (0);
}
#endif /*  */

