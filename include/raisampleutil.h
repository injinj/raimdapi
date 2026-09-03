/* Copyright (c) 2004 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#ifndef __rai_raiapi__raisampleutil_h__
#define __rai_raiapi__raisampleutil_h__

#if ! defined(NO_WHATLIST) && defined(__GNUC__)
static char CVS_ID_raiutil__raisampleutil_h[] __attribute__ ((__unused__)) = "$Header$";
#endif /* ! defined(NO_WHATLIST) */

/*

 These are utility functions used by the raiapi sample programs.
 They are not part of the API itself.

*/

enum {
  ARG_NOT_FOUND	= 0,
  ARG_FOUND	= 1,
  UNKNOWN_ARG	= 2
};

class ArgList;

class Argument {
  friend class ArgList;
 protected:
  Argument	* next;
 public:
  const char	* name,
		* defVal,
		* example,
		* description,
		* value;
  Argument( const char * name, const char * defVal, const char * example, const char * description );
  ~Argument() {};
};

class ArgList {
  Argument	* first,
		* last;
  Argument *	find( const char *name );

  public:
  void add( Argument * arg );
  const char * getString( const char * name );
  unsigned int getUInt( const char * name );
  unsigned long getULLong( const char * name );
  int processArgs( int argc, char * argv[] );
  void help();
  void version();
  ArgList( ) { first = last = NULL; };
};

#endif /* __rai_raiapi__raisampleutil_h__ */
