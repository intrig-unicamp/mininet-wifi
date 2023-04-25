/*
**   ACSMX2.H 
**
**   Version 2.0
**
**   Copyright (C) 2002,2003,2004 Marc Norton
**   Copyright (C) 2003,2004 Sourcefire,Inc.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef ACSMX2S_H
#define ACSMX2S_H

#ifdef WIN32

#ifdef inline
#undef inline
#endif

#define inline __inline

#else
#ifdef inline
#undef inline
#endif

#define inline 
#endif

/*
*   DEFINES and Typedef's
*/
#define MAX_ALPHABET_SIZE 256     

/*
   FAIL STATE for 1,2,or 4 bytes for state transitions

   Uncomment this define to use 32 bit state values
   #define AC32
*/

/* #define AC32 */

#ifdef AC32

typedef  unsigned int   acstate_t;
#define ACSM_FAIL_STATE2  0xffffffff

#else

typedef    unsigned short acstate_t;
#define ACSM_FAIL_STATE2 0xffff

#endif

/*
*
*/
typedef 
struct _acsm_pattern2
{      
    struct  _acsm_pattern2 *next;

    unsigned char         *patrn;
    unsigned char         *casepatrn;
    int      n;
    int      nocase;
    int      offset;
    int      depth;
    void *   id;
    int      iid;

} ACSM_PATTERN2;

/*
*    transition nodes  - either 8 or 12 bytes
*/
typedef 
struct trans_node_s {

  acstate_t    key;           /* The character that got us here - sized to keep structure aligned on 4 bytes */
                              /* to better the caching opportunities. A value that crosses the cache line */
                              /* forces an expensive reconstruction, typing this as acstate_t stops that. */
  acstate_t    next_state;    /*  */
  struct trans_node_s * next; /* next transition for this state */

} trans_node_t;


/*
*  User specified final storage type for the state transitions
*/
enum {
  ACF_FULL,
  ACF_SPARSE,
  ACF_BANDED,
  ACF_SPARSEBANDS,
};

/*
*   User specified machine types
*
*   TRIE : Keyword trie
*   NFA  : 
*   DFA  : 
*/
enum {
  FSA_TRIE,
  FSA_NFA,
  FSA_DFA,
};

/*
*   Aho-Corasick State Machine Struct - one per group of pattterns
*/
typedef struct {
  
    int acsmMaxStates;  
    int acsmNumStates;  

    ACSM_PATTERN2    * acsmPatterns;
        acstate_t        * acsmFailState;
        ACSM_PATTERN2   ** acsmMatchList;

        /* list of transitions in each state, this is used to build the nfa & dfa */
        /* after construction we convert to sparse or full format matrix and free */
        /* the transition lists */
        trans_node_t ** acsmTransTable;

        acstate_t ** acsmNextState;
        int          acsmFormat;
        int          acsmSparseMaxRowNodes;
        int          acsmSparseMaxZcnt;
        
        int          acsmNumTrans;
        int          acsmAlphabetSize;
        int          acsmFSA;

}ACSM_STRUCT2;

/*
*   Prototypes
*/
ACSM_STRUCT2 * acsmNew2 ();
int acsmAddPattern2( ACSM_STRUCT2 * p, unsigned char * pat, int n,
                    int nocase, int offset, int depth, void *  id, int iid );
int acsmCompile2 ( ACSM_STRUCT2 * acsm );
int acsmSearch2 ( ACSM_STRUCT2 * acsm,unsigned char * T, int n, 
          int (*Match)( void * id, int index, void * data ),
                  void * data );
void acsmFree2 ( ACSM_STRUCT2 * acsm );


int  acsmSelectFormat2( ACSM_STRUCT2 * acsm, int format );
int  acsmSelectFSA2( ACSM_STRUCT2 * acsm, int fsa );

void acsmSetMaxSparseBandZeros2( ACSM_STRUCT2 * acsm, int n );
void acsmSetMaxSparseElements2( ACSM_STRUCT2 * acsm, int n );
int  acsmSetAlphabetSize2( ACSM_STRUCT2 * acsm, int n );
void acsmSetVerbose2(int n);

void acsmPrintInfo2( ACSM_STRUCT2 * p);

int acsmPrintDetailInfo2(ACSM_STRUCT2*);
int acsmPrintSummaryInfo2();

#endif
