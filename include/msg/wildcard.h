/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#ifndef __rai_msg__wildcard_h__
#define __rai_msg__wildcard_h__

#if ! defined( RAIMSG_DLL_EXP )
#if defined( RAI_DLL )
#define RAIMSG_DLL_EXP __declspec(dllimport)
#else
#define RAIMSG_DLL_EXP
#endif
#endif

#include <string.h>

#ifndef __rai_base__mem_h__
#include "base/mem.h"
#endif

#ifndef __rai_msg__sass_const_h__
#include "msg/sass_const.h" /* for MAX_RV_SEGMENTS */
#endif

namespace rai {
struct WildMatchState;
struct WildEndState;


struct RAIMSG_DLL_EXP WildEndState {
  virtual ~WildEndState() {};

  /*! Test wether this end state is no longer valid (i.e. has no subscribers) */
  virtual bool prunable( void *closure ) { return false; };

  /*! If start->copy() is used, this should be defined */
  virtual WildEndState *copy( void ) { return NULL; };
};


struct WildTransition;

struct RAIMSG_DLL_EXP WildMatchState {
  WildTransition  ** trans; /*! Array of segment matches, terminated by NULL */
  unsigned short   * idx;

  void init( void ) {
    this->trans = NULL;
    this->idx   = NULL;
  };
  void release( void );

  void releaseAll( void );

  WildTransition **first( const char *s ) {
    return ( this->idx == NULL ) ? this->trans :
           ( ( s == NULL ) ? &this->trans[ this->idx[ 0 ] ] :
                             &this->trans[ this->idx[ (byte) *s ] ] );
  }
  void reindex( void );

  void copy( WildMatchState &state );
};


struct RAIMSG_DLL_EXP WildTransition {
  union {
    WildMatchState next[ 1 ]; /*! More segments to match */
    WildEndState * end;  /*! If input matched all of subject, the end state */
  } u;
  char match[ 1 ]; /*! The rv segment match, could be wildcard '*' or '>' */

  /*! If match is NULL, the wildcard is terminated by '*' or a literal segment,
   *  if match is '>', the wildcard is terminated by '>' */
  bool isEndTransition( void ) {
    return ( this->match[ 0 ] == 0 ||
             ( this->match[ 0 ] == '>' && this->match[ 1 ] == '\0' ) );
  }
  static WildTransition *create( const char *s );

  static void release( WildTransition *t );

  static void releaseAll( WildTransition *t );

  static WildTransition *copy( WildTransition *t );
};


struct RAIMSG_DLL_EXP WildStartState {
  WildMatchState state; /*! First segment match */

  WildStartState() {
    this->state.init();
  };
  ~WildStartState() {
    this->state.releaseAll();
  };
  /*! If has no transitions, no wildcards to match */
  bool isEmpty( void ) {
    return this->state.trans == NULL;
  };
  /*! Add a wildcard end state, segs terminated by NULL */
  void addWildcard( const char **segs,  WildEndState *end );
  /*! Add a wildcard end state, rai subject format */
  void addWildcard( const byte *subjbuf,  WildEndState *end );
  /*! Get a wildcard end state if it exists, segs terminated by NULL */
  bool getWildcard( const char **segs,  WildEndState *&end );
  /*! Get a wildcard end state, rai subject format */
  bool getWildcard( const byte *subjbuf,  WildEndState *&end );
  /*! Remove a wildcard end state, segs terminated by NULL, return end state */
  bool removeWildcard( const char **segs,  WildEndState *&end );
  /*! Remove a wildcard end state, rai subject format */
  bool removeWildcard( const byte *subjbuf,  WildEndState *&end );
  /*! Test validity of each of the end states prunable( closure ), rm if true */
  bool pruneWildcards( void *closure = NULL );
  /*! Copy & allocate everything */
  void copy( WildStartState &start ) {
    this->state.copy( start.state );
  };
};


struct RAIMSG_DLL_EXP WildStack {
  struct {
    WildTransition ** trans;  /*! The transition branches yet to be matched */
    const char     ** segs;   /*! The segments yet to be matched */
  } stack[ SassConst::MAX_RV_SEGMENTS + 1 ];
  unsigned int tos; /*! Current stack top */

  /*! Add start state to the stack */
  bool init( WildStartState &start,  const char **segs ) {
    WildTransition **trans;

    trans = start.state.first( segs[ 0 ] );
    if ( trans == NULL || trans[ 0 ] == NULL ) {
      this->tos = 0;
      return false;
    }
    this->stack[ 0 ].trans = trans;
    this->stack[ 0 ].segs  = segs;
    this->tos = 1;
    return true;
  };
  /*! Pop stack and match segment transitions to the end state */
  bool next( WildEndState *&end );
};


struct RAIMSG_DLL_EXP RaiWildStack {
  struct {
    WildTransition ** trans;  /*! The transition branches yet to be matched */
    const byte      * rvseg;  /*! The segments yet to be matched */
    byte              nsegs;
  } stack[ SassConst::MAX_RV_SEGMENTS + 1 ];
  unsigned int tos; /*! Current stack top */

  /*! Add start state to the stack */
  bool init( WildStartState &start,  const byte *subjbuf ) {
    WildTransition **trans;

    trans = start.state.first( (const char *) &subjbuf[ 2 ] );
    if ( trans == NULL || trans[ 0 ] == NULL ) {
      this->tos = 0;
      return false;
    }
    this->stack[ 0 ].trans = trans;
    this->stack[ 0 ].rvseg = &subjbuf[ 1 ];
    this->stack[ 0 ].nsegs = subjbuf[ 0 ];
    this->tos = 1;
    return true;
  };
  /*! Pop stack and match segment transitions to the end state */
  bool next( WildEndState *&end );
};


struct RAIMSG_DLL_EXP AllWildStack {
  struct {
    WildTransition ** trans;  /*! The transition branches yet to be matched */
  } stack[ SassConst::MAX_RV_SEGMENTS + 1 ];
  unsigned int tos; /*! Current stack top */

  /*! Add start state to the stack */
  bool init( WildStartState &start ) {
    WildTransition **trans;

    trans = start.state.trans;
    if ( trans == NULL || trans[ 0 ] == NULL ) {
      this->tos = 0;
      return false;
    }
    this->stack[ 0 ].trans = trans;
    this->tos = 1;
    return true;
  };
  /*! Pop stack and match segment transitions to the end state */
  bool next( WildEndState *&end );
};

} // namespace rai
#endif
