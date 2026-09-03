/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#if ! defined( RAIMSG_DLL_EXP ) && defined( RAI_DLL )
#define RAIMSG_DLL_EXP __declspec(dllexport)
#endif

#include "msg/wildcard.h"

using namespace rai;

bool
WildStack::next( WildEndState *&end )
{
  WildTransition ** trans,
                  * t;
  const char     ** segs,
                  * s = NULL;

  while ( this->tos > 0 ) {
    /* start where we left off */
    trans = this->stack[ --this->tos ].trans;
    segs  = this->stack[ this->tos ].segs;

    if ( 0 ) {
  segment_matched:;
      /* save segment state of this wildcard level */
      if ( *trans != NULL ) {
        if ( trans[ 0 ]->match[ 0 ] <= s[ 0 ] ) {
          this->stack[ this->tos ].trans  = trans;
          this->stack[ this->tos++ ].segs = segs;
        }
      }
      /* goto next segment and match that */
      trans = t->u.next->first( *++segs );
    }

    if ( trans != NULL ) {
      t = *trans++;
      /* if end of subject string */
      if ( (s = *segs) == NULL ) {
        /* find an end of a pattern */
        if ( t != NULL && t->match[ 0 ] == 0 ) {
          end = t->u.end;
          return true;
        }
      }
      else {
        for ( ; t != NULL && t->match[ 0 ] <= '>'; t = *trans++ ) {
          if ( t->match[ 0 ] == '>' ) {
            if ( t->match[ 1 ] == '\0' ) {
              end = t->u.end;
              if ( *trans != NULL ) {
                if ( trans[ 0 ]->match[ 0 ] <= s[ 0 ] ) {
                  this->stack[ this->tos ].trans  = trans;
                  this->stack[ this->tos++ ].segs = segs;
                }
              }
              return true;
            }
          }
          else if ( t->match[ 0 ] == '*' ) {
            if ( t->match[ 1 ] == '\0' )
              goto segment_matched;
          }
          if ( t->match[ 0 ] == s[ 0 ] ) {
            if ( ::strcmp( &t->match[ 1 ], &s[ 1 ] ) == 0 )
              goto segment_matched;
          }
        }

        for ( ; t != NULL && t->match[ 0 ] < s[ 0 ]; t = *trans++ )
          ;

        for ( ; t != NULL && t->match[ 0 ] == s[ 0 ]; t = *trans++ ) {
          if ( ::strcmp( &t->match[ 1 ], &s[ 1 ] ) == 0 )
            goto segment_matched;
        }
      }
    }
  }

  return false;
}


bool
RaiWildStack::next( WildEndState *&end )
{
  WildTransition ** trans,
                  * t;
  const byte      * rvseg;
  byte              nsegs;

  while ( this->tos > 0 ) {
    /* start where we left off */
    trans = this->stack[ --this->tos ].trans;
    rvseg = this->stack[ this->tos ].rvseg;
    nsegs = this->stack[ this->tos ].nsegs;

    if ( 0 ) {
  segment_matched:;
      /* save segment state of this wildcard level */
      if ( *trans != NULL ) {
        if ( trans[ 0 ]->match[ 0 ] <= rvseg[ 1 ] ) {
          this->stack[ this->tos ].trans   = trans;
          this->stack[ this->tos ].rvseg   = rvseg;
          this->stack[ this->tos++ ].nsegs = nsegs;
        }
      }
      /* goto next segment and match that */
      if ( --nsegs == 0 )
        trans = t->u.next->first( NULL );
      else {
        rvseg = &rvseg[ rvseg[ 0 ] ];
        trans = t->u.next->first( (const char *) &rvseg[ 1 ] );
      }
    }

    if ( trans != NULL ) {
      t = *trans++;
      /* if end of subject string */
      if ( nsegs == 0 ) {
        /* find an end of a pattern */
        if ( t != NULL && t->match[ 0 ] == 0 ) {
          end = t->u.end;
          return true;
        }
      }
      else {
        for ( ; t != NULL && t->match[ 0 ] <= '>'; t = *trans++ ) {
          if ( t->match[ 0 ] == '>' ) {
            if ( t->match[ 1 ] == '\0' ) {
              end = t->u.end;
              if ( *trans != NULL ) {
                if ( trans[ 0 ]->match[ 0 ] <= rvseg[ 1 ] ) {
                  this->stack[ this->tos ].trans   = trans;
                  this->stack[ this->tos ].rvseg   = rvseg;
                  this->stack[ this->tos++ ].nsegs = nsegs;
                }
              }
              return true;
            }
          }
          else if ( t->match[ 0 ] == '*' ) {
            if ( t->match[ 1 ] == '\0' )
              goto segment_matched;
          }
          if ( t->match[ 0 ] == rvseg[ 1 ] ) {
            if ( ::memcmp( &t->match[ 1 ], &rvseg[ 2 ], rvseg[ 0 ] - 2 ) == 0 )
              goto segment_matched;
          }
        }

        for ( ; t != NULL && t->match[ 0 ] < rvseg[ 1 ]; t = *trans++ )
          ;

        for ( ; t != NULL && t->match[ 0 ] == rvseg[ 1 ]; t = *trans++ ) {
          if ( ::memcmp( &t->match[ 1 ], &rvseg[ 2 ], rvseg[ 0 ] - 2 ) == 0 )
            goto segment_matched;
        }
      }
    }
  }

  return false;
}


bool
AllWildStack::next( WildEndState *&end )
{
  WildTransition ** trans,
                  * t;

  if ( this->tos == 0 )
    return false;
  trans = this->stack[ --this->tos ].trans;
  for (;;) {
    /* start where we left off */
    t = *trans++;
    if ( *trans != NULL )
      this->stack[ this->tos++ ].trans = trans;
    if ( t->isEndTransition() ) {
      end = t->u.end;
      return true;
    }
    trans = t->u.next->trans;
  }
}


void
WildStartState::addWildcard( const char **segs,  WildEndState *end )
                throw( Error )
{
  WildMatchState  * state;
  WildTransition ** trans;
  const char      * s;
  unsigned int      i;

  state = &this->state;

  for (;;) {
  segment_matched:;
    s = *segs++;
    i = 0;

    if ( (trans = state->trans) != NULL ) {
      if ( s == NULL || ( s[ 0 ] == '>' && s[ 1 ] == '\0' ) ) {
        /* check if end of pattern subject already exists */
        if ( s == NULL ) {
          for ( ; trans[ i ] != NULL; i++ ) {
            if ( trans[ i ]->match[ 0 ] == 0 )
              break;
          }
        }
        else {
          for ( ; trans[ i ] != NULL; i++ ) {
            if ( trans[ i ]->match[ 0 ] == '>' &&
                 trans[ i ]->match[ 1 ] == '\0' )
              break;
          }
        }
        if ( trans[ i ] != NULL ) {
          if ( trans[ i ]->u.end != NULL && trans[ i ]->u.end != end )
            delete trans[ i ]->u.end;
          trans[ i ]->u.end = end;
          return;
        }
      }
      else {
        /* check if pattern segment exists */
        for ( ; trans[ i ] != NULL; i++ ) {
          if ( ::strcmp( trans[ i ]->match, s ) == 0 ) {
            state = trans[ i ]->u.next;
            goto segment_matched; /* exists, goto next segment */
          }
        }
      }
    }

    /* add pattern segment to array of transitions */
    REALLOC( sizeof( trans[ 0 ] ) * ( i + 2 ), &trans );
    state->trans   = trans;
    trans[ i ]     = NULL;
    trans[ i + 1 ] = NULL;
    trans[ i ]     = WildTransition::create( s );

    /* order the patterns by match[ 0 ] */
    while ( i > 0 && trans[ i ]->match[ 0 ] < trans[ i - 1 ]->match[ 0 ] ) {
      WildTransition * t = trans[ i ];
      trans[ i ]   = trans[ i - 1 ];
      trans[ --i ] = t;
    }
    state->reindex();

    /* if at end of pattern add an end state */
    if ( s == NULL || ( s[ 0 ] == '>' && s[ 1 ] == '\0' ) ) {
      trans[ i ]->u.end = end;
      return;
    }
    /* add another match state */
    else {
      state = trans[ i ]->u.next;
    }
  }
}

static const char **
getsegs( const byte *subjbuf,  const char **segs )
{
  unsigned int i, j, n;
  n = (unsigned int) ( subjbuf == NULL ? 0 : subjbuf[ 0 ] );
  if ( n == 0 )
    segs[ 0 ] = NULL;
  else {
    j = 1;
    for ( i = 0; ; ) {
      segs[ i ] = (const char *) &subjbuf[ j + 1 ];
      if ( ++i == n ) {
        segs[ i ] = NULL;
        break;
      }
      j += (unsigned int) subjbuf[ j ];
    }
  }
  return segs;
}

void
WildStartState::addWildcard( const byte *subjbuf,  WildEndState *end )
                throw( Error )
{
  const char * segs[ 256 ];
  this->addWildcard( getsegs( subjbuf, segs ), end );
}


bool
WildStartState::removeWildcard( const byte *subjbuf,  WildEndState *&end )
{
  const char * segs[ 256 ];
  return this->removeWildcard( getsegs( subjbuf, segs ), end );
}


bool
WildStartState::getWildcard( const byte *subjbuf,  WildEndState *&end )
{
  const char * segs[ 256 ];
  return this->getWildcard( getsegs( subjbuf, segs ), end );
}


bool
WildStartState::getWildcard( const char **segs,  WildEndState *&end )
{
  WildMatchState  * state;
  WildTransition ** trans;
  const char      * s;
  unsigned int      i;

  state = &this->state;

segment_matched:;
  s = *segs++;
  i = 0;

  if ( (trans = state->trans) != NULL ) {
    if ( s == NULL || ( s[ 0 ] == '>' && s[ 1 ] == '\0' ) ) {
      /* check if end of pattern subject already exists */
      if ( s == NULL ) {
        for ( ; trans[ i ] != NULL; i++ ) {
          if ( trans[ i ]->match[ 0 ] == 0 )
            break;
        }
      }
      else {
        for ( ; trans[ i ] != NULL; i++ ) {
          if ( trans[ i ]->match[ 0 ] == '>' && trans[ i ]->match[ 1 ] == '\0' )
            break;
        }
      }
      if ( trans[ i ] != NULL ) {
        end = trans[ i ]->u.end;
        return true;
      }
    }
    else {
      /* check if pattern segment exists */
      for ( ; trans[ i ] != NULL; i++ ) {
        if ( ::strcmp( trans[ i ]->match, s ) == 0 ) {
          state = trans[ i ]->u.next;
          goto segment_matched; /* exists, goto next segment */
        }
      }
    }
  }

  return false;
}


bool
WildStartState::removeWildcard( const char **segs,  WildEndState *&end )
{
  WildMatchState  * stack[ SassConst::MAX_RV_SEGMENTS + 1 ],
                  * state;
  WildTransition ** trans;
  const char      * s;
  unsigned int      i,
                    offset[ SassConst::MAX_RV_SEGMENTS + 1 ],
                    tos;

  state = &this->state;
  tos   = 0;

  for (;;) {
    s = *segs++;
    i = 0;

    if ( (trans = state->trans) == NULL )
      return false;

    if ( s == NULL || ( s[ 0 ] == '>' && s[ 1 ] == '\0' ) ) {
      /* check if end of pattern subject exists */
      if ( s == NULL ) {
        for ( ; ; i++ ) {
          if ( trans[ i ] == NULL )
            return false;
          if ( trans[ i ]->match[ 0 ] == 0 )
            break;
        }
      }
      else {
        for ( ; ; i++ ) {
          if ( trans[ i ] == NULL )
            return false;
          if ( trans[ i ]->match[ 0 ] == '>' && trans[ i ]->match[ 1 ] == '\0' )
            break;
        }
      }

      /* return ptr to end */
      end = trans[ i ]->u.end;
      trans[ i ]->u.end = NULL;
      WildTransition::release( trans[ i ] );

      /* if trans[ i ] is the only transition from this state, remove it */
      if ( i == 0 && trans[ 1 ] == NULL ) {
        state->release();
        /* pop up the stack, removing states if necessary */
        while ( tos > 0 ) {
          state = stack[ --tos ];
          trans = state->trans;
          i     = offset[ tos ];
          WildTransition::release( trans[ i ] );
          if ( i != 0 || trans[ 1 ] != NULL )
            goto move_transitions;
          state->release();
        }
      }
      else {
      move_transitions:;
        /* transition is gone, move the valid ones over 1 spot */
        while ( (trans[ i ] = trans[ i + 1 ]) != NULL )
          i++;
        state->reindex();
      }

      return true;
    }
    else {
      /* check if pattern segment exists */
      for ( ; ; i++ ) {
        if ( trans[ i ] == NULL )
          return false;
        /* if matches goto next segment */
        if ( ::strcmp( trans[ i ]->match, s ) == 0 ) {
          stack[ tos ]    = state;
          offset[ tos++ ] = i;
          state           = trans[ i ]->u.next;
          break;
        }
      }
    }
  }
}


bool
WildStartState::pruneWildcards( void *closure )
{
  WildMatchState  * stack[ SassConst::MAX_RV_SEGMENTS + 1 ],
                  * state;
  unsigned int      offset[ SassConst::MAX_RV_SEGMENTS + 1 ],
                    count[ SassConst::MAX_RV_SEGMENTS + 1 ];
  WildTransition ** trans;
  unsigned int      off,
                    j,
                    k,
                    delcnt,
                    tos;
  bool              stateDeleted;

  /* nothing to prune */
  if ( this->state.trans == NULL )
    return true;

  tos = 0;
  offset[ tos ]   = 0;
  count[ tos ]    = 0;
  stack[ tos++ ]  = &this->state;
  stateDeleted    = false;

  while ( tos > 0 ) {
    state = stack[ --tos ];
    trans = state->trans;
    off   = offset[ tos ];

    if ( stateDeleted ) {
      delcnt           = count[ tos ] + 1;
      trans[ off - 1 ] = NULL;
      stateDeleted     = false;
    }
    else {
      delcnt = count[ tos ];
    }
    while ( trans[ off ] != NULL ) {
      if ( trans[ off ]->isEndTransition() ) {
        if ( trans[ off ]->u.end->prunable( closure ) ) {
          WildTransition::release( trans[ off ] );
          trans[ off ] = NULL;
          delcnt++;
        }
        off++;
      }
      else {
        offset[ tos ]  = off + 1;
        count[ tos ]   = delcnt;
        stack[ tos++ ] = state;

        state  = trans[ off ]->u.next;
        trans  = state->trans;
        off    = 0;
        delcnt = 0;
      }
    }

    /* if anything deleted */
    if ( delcnt > 0 ) {
      /* if some deleted */
      if ( delcnt < off ) {
        /* move undeleted items together */
        for ( j = 0; trans[ j ] != NULL; j++ )
          ;
        for ( k = j + 1; k < off; k++ ) {
          if ( (trans[ j ] = trans[ k ]) != NULL )
            j++;
        }
        trans[ j ] = NULL;
        state->reindex();
      }
      /* all were deleted */
      else {
        state->release();
        if ( tos > 0 )
          stateDeleted = true;
      }
    }
  }

  if ( this->state.trans == NULL )
    return true;
  return false;
}


void
WildMatchState::reindex( void ) throw( Error )
{
  WildTransition ** trans;
  unsigned int      i,
                    nil;

  trans = this->trans; 
  nil = 0;
  if ( trans != NULL )
    for ( nil = 0; trans[ nil ] != NULL; nil++ )
      ;
  if ( nil < 4 || nil >= ( 1 << ( sizeof( this->idx[ 0 ] ) * 8 ) ) ) {
    if ( this->idx != NULL ) {
      FREE( this->idx );
      this->idx = NULL;
    }
    return;
  }
  if ( this->idx == NULL )
    MALLOC( sizeof( this->idx[ 0 ] ) * 256, &this->idx );

  for ( i = 0; i < 256; i++ )
    this->idx[ i ] = nil;

  /* re-index transitions */
  for ( i = 0; trans[ i ] != NULL; i++ ) {
    if ( this->idx[ (byte) trans[ i ]->match[ 0 ] ] == nil )
      this->idx[ (byte) trans[ i ]->match[ 0 ] ] = i;
  }

  if ( this->idx[ (byte) '*' ] != nil ) {

    for ( i = 1; i < '*'; i++ )
      if ( this->idx[ i ] == nil )
        this->idx[ i ] = this->idx[ (byte) '*' ];

    for ( i = '*' + 1; i < 256; i++ )
      this->idx[ i ] = this->idx[ (byte) '*' ];
  }
  else if ( this->idx[ (byte) '>' ] != nil ) {

     for ( i = 1; i < '>'; i++ )
       if ( this->idx[ i ] == nil )
         this->idx[ i ] = this->idx[ (byte) '>' ];

    for ( i = '>' + 1; i < 256; i++ )
      this->idx[ i ] = this->idx[ (byte) '>' ];
  }
}


void
WildMatchState::copy( WildMatchState &state ) throw( Error )
{
  WildTransition ** trans;
  unsigned short  * idx;
  unsigned int      i;

  trans = NULL;
  idx   = NULL;

  if ( state.trans != NULL ) {
    for ( i = 0; state.trans[ i++ ] != NULL; )
      ;
    MALLOC( sizeof( state.trans[ 0 ] ) * i, &trans );
    if ( state.idx != NULL ) {
      MALLOC( sizeof( state.idx[ 0 ] ) * 256, &idx );
      ::memcpy( idx, state.idx, sizeof( state.idx[ 0 ] ) * 256 );
    }
    else {
      idx = NULL;
    }
    for ( i = 0; state.trans[ i ] != NULL; i++ )
      trans[ i ] = WildTransition::copy( state.trans[ i ] );
    trans[ i ] = NULL;
  }

  this->trans = trans;
  this->idx   = idx;
}


void
WildMatchState::release( void )
{
  if ( this->trans != NULL ) {
    FREE( this->trans );
    this->trans = NULL;
  }
  if ( this->idx != NULL ) {
    FREE( this->idx );
    this->idx = NULL;
  }
}


void
WildMatchState::releaseAll( void )
{
  if ( this->trans != NULL ) {
    for ( unsigned int i = 0; this->trans[ i ] != NULL; i++ )
      WildTransition::releaseAll( this->trans[ i ] );
  }
  this->release();
}


WildTransition *
WildTransition::create( const char *s ) throw( Error )
{
  WildTransition *t;
  MALLOC( sizeof( WildTransition ) + ( s == NULL ? 0 : ::strlen( s ) ), &t );
  if ( s != NULL )
    ::strcpy( t->match, s );
  else
    t->match[ 0 ] = 0;
  t->u.next[ 0 ].init();
  t->u.end = NULL;
  return t;
}


void
WildTransition::release( WildTransition *t )
{
  if ( t->isEndTransition() ) {
    if ( t->u.end != NULL )
      delete t->u.end;
  }
  else {
    t->u.next[ 0 ].release();
  }
  FREE( t );
}


WildTransition *
WildTransition::copy( WildTransition *t ) throw( Error )
{
  WildTransition *t2;

  t2 = WildTransition::create( t->match );
  if ( t->isEndTransition() )
    t2->u.end = t->u.end->copy();
  else
    t2->u.next[ 0 ].copy( t->u.next[ 0 ] );
  return t2;
}


void
WildTransition::releaseAll( WildTransition *t )
{
  if ( t->isEndTransition() ) {
    if ( t->u.end != NULL )
      delete t->u.end;
  }
  else {
    t->u.next[ 0 ].releaseAll();
  }
  FREE( t );
}
