/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#ifndef __rai_util__args_h__
#define __rai_util__args_h__

#if ! defined( RAIBASE_DLL_EXP )
#if defined( RAI_DLL )
#define RAIBASE_DLL_EXP __declspec(dllimport)
#else
#define RAIBASE_DLL_EXP
#endif
#endif

#ifndef __rai_base__types_h__
#include "base/types.h"
#endif

#ifndef __rai_base__mem_h__
#include "base/mem.h"
#endif

namespace rai {

class OutputStream;
class InputStream;

struct RAIBASE_DLL_EXP StringArg {
  const char * name,
             * defVal,
             * example,
             * description;

  StringArg( const char *name,  const char *defVal,  const char *example,
             const char *description ) {
    this->name        = name;
    this->defVal      = defVal;
    this->example     = example;
    this->description = description;
  };
};


struct RAIBASE_DLL_EXP BoolArg {
  const char * name;
  bool         defVal;
  const char * example,
             * description;

  BoolArg( const char *name,  bool defVal,  const char *example,
           const char *description ) {
    this->name        = name;
    this->defVal      = defVal;
    this->example     = example;
    this->description = description;
  };
};


struct RAIBASE_DLL_EXP UIntArg {
  const char * name;
  unsigned int defVal;
  const char * example,
             * description;

  UIntArg( const char *name,  unsigned int defVal,  const char *example,
           const char *description ) {
    this->name        = name;
    this->defVal      = defVal;
    this->example     = example;
    this->description = description;
  };
};


struct RAIBASE_DLL_EXP ULLongArg {
  const char * name;
  ullong       defVal;
  const char * example,
             * description;

  ULLongArg( const char *name,  ullong defVal,  const char *example,
             const char *description ) {
    this->name        = name;
    this->defVal      = defVal;
    this->example     = example;
    this->description = description;
  };
};


struct RAIBASE_DLL_EXP DoubleArg {
  const char * name;
  double       defVal;
  const char * example,
             * description;

  DoubleArg( const char *name,  double defVal,  const char *example,
             const char *description ) {
    this->name        = name;
    this->defVal      = defVal;
    this->example     = example;
    this->description = description;
  };
};


enum ArgFlags {
  IGNORE_ARG     = 0,
  RESOURCE_ARG   = 1, /* whether or not arg can be in a rc file */
  COMMAND_ARG    = 2, /* whether or not arg can be on command line */
  TIME_SEC_ARG   = 4, /* when parsing string, convert units to seconds */
  TIME_MS_ARG    = 8, /* when parsing string, convert units to milliseconds */
  MEM_ARG        = 16, /* when parsing string, convert units to bytes */
  HELP_ARG       = 32, /* is the -help argument */
  VERSION_ARG    = 64, /* is the -version argument */
  PRINTRC_ARG    = 128, /* is the -printRC argument */
  RCFILE_ARG     = 256, /* is the -rcFile argument */
  LIST_ARG       = 512, /* arg can have a list of values */
  NO_DEFAULT_VAL = 1024, /* don't process default values for arg */
  BITS_ARG       = 2048  /* when parsing string, convert units to bits */
};

class RAIBASE_DLL_EXP Args {
  public:
    static const unsigned int MAX_ARG_COUNT = 256U;

    enum ArgType {
      STRING_ARG  = 0,
      UINT_ARG    = 1,
      ULLONG_ARG  = 2,
      BOOL_ARG    = 3,
      DOUBLE_ARG  = 4
    };

    struct RAIBASE_DLL_EXP Arg {
      const char * name,
                 * example,
                 * description;
      unsigned int flags,
                   numValues;
      ArgType type;

      static const char * typeString( ArgType type ) {
        switch ( type ) {
          case STRING_ARG: return "string";
          case UINT_ARG:   return "uint";
          case ULLONG_ARG: return "ulong";
          case BOOL_ARG:   return "bool";
          case DOUBLE_ARG: return "double";
          default:         return "garbage";
        }
      };

      const char * typeString( void ) {
        return typeString( this->type );
      };

      union {
        const char * s;
        bool         b;
        unsigned int i;
        ullong       l;
        double       d;
      } defVal;    /* default value if arg not set */

      union {
        char       * s;
        bool         b;
        unsigned int i;
        ullong       l;
        double       d;
      } val, *vals; /* if list, vals[0] contains item 1, vals[1] item 2... */

      bool defaultToString( char *buf,  unsigned int bufLen ) const
;
      unsigned int parseUInt( const char *arg );

      ullong parseULLong( const char *arg );

      double parseDouble( const char *arg );
    };
    friend struct Arg;

  private:
    Arg            args[ MAX_ARG_COUNT ];
    unsigned int   argCount;
    const char   * versionInfo,
                 * envPrefix;
    OutputStream * out;

    /* temporary memory storage for args.copy(), clear() frees this */
    struct RAIBASE_DLL_EXP TmpArgList {
      TmpArgList * next;
      union {
        StringArg * sa;
        BoolArg   * ba;
        UIntArg   * ia;
        ULLongArg * la;
        DoubleArg * da;
      } u;
    } * copyArgs;

    TmpArgList *allocArg( ArgType t,  const char *name,  const char *example,
                          const char *description,  const char *defVal )
;
    bool getExpansion( const char *name,  char *buf,  unsigned int bufLen )
;
    void matchArgs( const char *name,  unsigned int argc,  char **argv,
                    unsigned int *numMatched,  const char *source )
;
    Arg * getArg( const char *name );

    const char * getName( ArgFlags flags );

    Arg * addType( ArgType type,  const char *name,  const char *example,
                   const char *description,  unsigned int flags )
;
  public:
    SYS_OPS( Args );
    Args();
    /* number of args */
    unsigned int count( void ) const {
      return this->argCount;
    };
    /* iterate to the first argument */
    const Arg *getFirst( unsigned int &i ) const {
      i = 0;
      return this->getNext( i );
    };
    /* iterate to the next argument */
    const Arg *getNext( unsigned int &i ) const {
      if ( i < this->argCount )
        return &this->args[ i++ ];
      return NULL;
    };
    /* return Arg * or null if not found */
    const Arg *getArgByName( const char *name ) const;

    /* frees the string arguments which allocated storage */
    void clear( void );
    /* the version displayed by -version */
    void setVersion( const char *versionInfo ) {
      this->versionInfo = versionInfo;
    }
    const char *getVersion( void ) const {
      return this->versionInfo;
    }
    /* addEnv() checks args in the environment with this prefix */
    void setEnvPrefix( const char *envPrefix ) {
      this->envPrefix = envPrefix;
    }
    const char *getEnvPrefix( void ) const {
      return this->envPrefix;
    }
    /* any output by this class goes to this stream, which defaults to stderr */
    void setOutputStream( OutputStream *out ) {
      this->out = out;
    }
    OutputStream *getOutputStream( void ) {
      return this->out;
    }
    /* sets the arguments in command line format: -subject TEST,
     * then tests whether -help or -rcFile, or -version is specified and
     * performs the action.  If program should continue, this returns true */
    bool processArgs( unsigned int argc,  char **argv );
    /* sets the arguments in parms[] array with the values in parmVals[],
     * example: parms[] = { "subject" }; parmVals[] = { "TEST" };
     * args.processParms( parms, parmVals, 1, "program" ) */
    void processParms( const char * const parms[],  char *parmVals[],
                       unsigned int numParms,  const char *source )
    /* examines the string values and expands environment */;
    /* values specified by ${ENV_VAR} */
    void expandArgs( void );
    /* parse command line style arguments, which have one or two '-' prefixes
     * indicating the start of an arg list, as in -subject TEST */
    void addArgs( unsigned int argc,  char **argv );
    /* parse an "rc" file, which is a file which specifies arg values
     * separated by '=', as in subject=TEST */
    void addRCFile( const char *path );
    /* parse an "rc" input stream, same as above */
    void addRCInput( InputStream *in,  const char *src );
    /* do getenv() on all the args with prefix specified in addDefaults(),
     * so if prefix == "rai", and arg subject exists, then
     * addEnv will getenv( "rai_subject" ) and parse the result*/
    void addEnv( void );
    /* add arg types */
    void add( const StringArg *arg,
              unsigned int flags = COMMAND_ARG | RESOURCE_ARG );
    void add( const UIntArg *arg,
              unsigned int flags = COMMAND_ARG | RESOURCE_ARG );
    void add( const ULLongArg *arg,
              unsigned int flags = COMMAND_ARG | RESOURCE_ARG );
    void add( const BoolArg *arg,
              unsigned int flags = COMMAND_ARG | RESOURCE_ARG );
    void add( const DoubleArg *arg,
              unsigned int flags = COMMAND_ARG | RESOURCE_ARG );
    /* copy arg types, above just uses the pointers, this copies the strings */
    void copy( const StringArg &arg,
              unsigned int flags = COMMAND_ARG | RESOURCE_ARG );
    void copy( const UIntArg &arg,
              unsigned int flags = COMMAND_ARG | RESOURCE_ARG );
    void copy( const ULLongArg &arg,
              unsigned int flags = COMMAND_ARG | RESOURCE_ARG );
    void copy( const BoolArg &arg,
              unsigned int flags = COMMAND_ARG | RESOURCE_ARG );
    void copy( const DoubleArg &arg,
              unsigned int flags = COMMAND_ARG | RESOURCE_ARG );
    /* if arg exists, remove it */
    bool removeArg( const char *name );
    /* the following arguments are added by addDefaults() */
    static const StringArg * log;
    static const StringArg * logLevel;
    static const UIntArg   * logVerb;
    static const UIntArg   * logRollCnt;
    static const StringArg * logRollDateFmt;
    static const StringArg * logRollType;
    static const DoubleArg * logSizeLimit;
    static const BoolArg   * logXml;
    static const BoolArg   * help;
    static const BoolArg   * version;
    static const BoolArg   * printRc;
    static const StringArg * rcFile;
    /* add version (set as vers arg), log, help, version, rc arguments */
    void addDefaults( const char *vers,  const char *pref,
                      OutputStream *out,  const char *argv0 );
    void addDefaults( const char *vers,  const char *pref,
                      OutputStream *out,  const char *argv0,
                      bool addLogRoll );
    /* pretty print args with '-' arg prefix, as on cmd line */
    void printHelp( OutputStream *o = NULL ) const;
    /* pretty print args with '=' separator */
    void printOptions( OutputStream *o = NULL ) const;
    /* used by printHelp and printOptions */
    void printHelp2( char optChar1,  char optChar2,
                     OutputStream *o = NULL ) const;
    /* prints the version, to OutputStream set in addDefaults() or stderr */
    void printVersion( OutputStream *o = NULL ) const;
    /* prints all of the args, help, default values to OutputStream in "RC",
     * format.  Sys::out is usually stdout, if null uses the addDefaults()
     * stream.  RC format can be loaded and parsed with addRCFile() */
    void printRC( OutputStream *rcOut = NULL ) const;
    /* return whether an arg is known */
    bool exists( const char *name ) const;
    /* return a bit mask of ArgFlags associated with arg */
    unsigned int getArgFlags( const char *name ) const;
    /* return the type of arg "name" */
    ArgType getArgType( const char *name ) const;
    /* return the number of items associated with arg "name", will be 0
     * if arg has no value, 1 if arg has a value, N if arg has flags
     * LIST_ARG set and more than 1 value associated with it */
    unsigned int getNumValues( const char *name ) const;
    /* get the string value, if n > 0, then get the nth item, use
     * getNumValues() to determine the number of items, the type of arg
     * must be a string, the getType() functions don't convert args to type */
    const char *getString( const char *name,  unsigned int n = 0 ) const
;
    /* set the value of arg "name", this allocates storage for string which
     * is freed by clear().  If arg is not a string type, then this function
     * attepts to parse the argument and convert it to uint, ullong, double */
    void setString( const char *name,  const char *val );
    /* get the uint value of arg "name", if n > 0, then get the nth item, use
     * getNumValues() to determine the number of items */
    unsigned int getUInt( const char *name,  unsigned int n = 0 ) const
    /* set the uint value of arg "name", type must be UINT_ARG */;
    void setUInt( const char *name,  unsigned int val );
    /* get the ullong value of arg "name", type must be ULLONG_ARG */
    ullong getULLong( const char *name,  unsigned int n = 0 ) const
;
    /* set the ullong value of arg "name", type must be ULLONG_ARG */
    void setULLong( const char *name,  ullong val );
    /* get the boolean value of arg "name", type must be BOOL_ARG */
    bool getBoolean( const char *name,  unsigned int n = 0 ) const
;
    /* set the boolean value of arg "name", type must be BOOL_ARG */
    void setBoolean( const char *name,  bool val );
    /* get the double value of arg "name", type must be DOUBLE_ARG */
    double getDouble( const char *name,  unsigned int n = 0 ) const
;
    /* set the double value of arg "name", type must be DOUBLE_ARG */
    void setDouble( const char *name,  double val );
    /* if arg has a value, or a default value and orHasDefVal == true */
    bool isSet( const char *name,  bool orHasDefVal = false ) const
;
};


namespace ArgsErr {
  enum {
    UNKNOWN_ARG      = 0,
    MISSING_ARG      = 1,
    ARG_UNMATCHED    = 2,
    BAD_RESOURCE     = 3,
    EXPECTING_STRING = 4,
    EXPECTING_UINT   = 5,
    EXPECTING_ULLONG = 6,
    EXPECTING_BOOL   = 7,
    EXPECTING_DOUBLE = 8,
    ARG_EXISTS       = 9,
    TOO_MANY_ARGS    = 10,
    NOT_A_LIST       = 11
  };
  RAIBASE_DLL_EXP
  Error getErr( unsigned int status );
}

} // namespace rai

#endif
