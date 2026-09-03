/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#ifndef __rai_msg__cfile_parser_h__
#define __rai_msg__cfile_parser_h__

#if ! defined( RAIMSG_DLL_EXP )
#if defined( RAI_DLL )
#define RAIMSG_DLL_EXP __declspec(dllimport)
#else
#define RAIMSG_DLL_EXP
#endif
#endif

#ifndef __rai_base__mem_h__
#include "base/mem.h"
#endif

namespace rai {
class CFileStrings;
class CFileExpr;
class CFileParser;
class CFileExprIter;
class InputStream;
class OutputStream;


union CFileVal {
  const char * stringVal;
  unsigned int uintVal;
  int          intVal;
  bool         boolVal;
  char         punctVal;
  CFileExpr  * exprVal;
};


class RAIMSG_DLL_EXP CFileExpr {
  public:
    enum ExprType {
      NULL_TYPE     = 0,
      UINT_TYPE     = 1,
      INT_TYPE      = 2,
      STRING_TYPE   = 3,
      BOOL_TYPE     = 4,
      SUBEXPR_TYPE  = 5
    };

  private:
    ExprType     type;
    const char * ident;
    unsigned int off;
    CFileVal     val;
    CFileExpr  * next,
               * back;

    friend class CFileParser;
    friend class CFileExprIter;
    CFileExpr() {};
    ~CFileExpr() {};

  public:
    void setValue( unsigned int val ) {
      this->type        = UINT_TYPE;
      this->val.uintVal = val;
    };

    void setValue( int val ) {
      this->type       = INT_TYPE;
      this->val.intVal = val;
    };

    void setValue( const char *val ) {
      this->type          = STRING_TYPE;
      this->val.stringVal = val;
    };

    void setValue( bool val ) {
      this->type        = BOOL_TYPE;
      this->val.boolVal = val;
    };

    void setValue( CFileExpr *val ) {
      this->type        = SUBEXPR_TYPE;
      this->val.exprVal = val;
    };

    void setNext( CFileExpr *nextExpr ) {
      this->next = nextExpr;
      nextExpr->back = this;
    };

    void unlink( void ) {
      if ( this->next != NULL )
        this->next->back = this->back;
      if ( this->back != NULL )
        this->back->next = this->next;
    };

    void replace( CFileExpr *first,  CFileExpr *last ) {
      if ( this->next != NULL )
        this->next->back = last;
      if ( this->back != NULL )
        this->back->next = first;
      first->back = this->back;
      last->next  = this->next;
    };

    static CFileExpr *append( CFileExpr *expr,  CFileExpr *expr2 ) {
      CFileExpr * expr3;
      if ( expr == NULL )
        return expr2;
      if ( expr2 == NULL )
        return expr;
      for ( expr3 = expr; expr3->next != NULL; expr3 = expr3->next )
        ;
      expr3->setNext( expr2 );
      return expr;
    };

    void print( OutputStream *out,  bool siblings = true,  bool children = true,
                unsigned int indent = 0 )                       throw( Error );

    ExprType getType( void ) const {
      return this->type;
    };

    const char *getIdent( void ) const {
      return this->ident;
    };

    bool valueEquals( CFileExpr *val ) const {
      return this->type == SUBEXPR_TYPE && val == this->val.exprVal;
    };

    CFileExpr *getNext( void ) {
      return this->next;
    };

    CFileExpr *getChild( void ) {
      if ( this->type == SUBEXPR_TYPE )
        return this->val.exprVal;
      return NULL;
    };

    bool getValue( unsigned int &val );

    bool getValue( bool &val );

    bool getValue( const char *&val ) {
      if ( this->type == STRING_TYPE ) {
        val = this->val.stringVal;
        return true;
      }
      return false;
    };
};


class RAIMSG_DLL_EXP CFileExprIter {
  public:
    static const unsigned int MAX_EXPR_DEPTH = 16;
  private:
    CFileExpr  * stack[ MAX_EXPR_DEPTH ];
    unsigned int tos;
  public:
    CFileExprIter() {
      this->tos = 0;
    };

    CFileExprIter( CFileExpr *node ) {
      this->stack[ 0 ] = node;
      this->tos = 1;
    };

    void clear( void ) {
      this->tos = 0;
    };

    CFileExpr *push( CFileExpr *node )                          throw( Error );

    CFileExpr *find( const char *identName )                    throw( Error );

    CFileExpr *findNext( const char *identName )                throw( Error );

    CFileExpr *parent( unsigned int grand = 0 ) {
      if ( this->tos >= 2 + grand )
        return this->stack[ this->tos - ( 2 + grand ) ];
      return NULL;
    };

    CFileExpr *child( void ) throw( Error ) {
      CFileExpr * node;
      if ( (node = this->top()) != NULL &&
           node->type == CFileExpr::SUBEXPR_TYPE &&
           node->val.exprVal != NULL )
        return this->push( node->val.exprVal );
      return NULL;
    };

    CFileExpr *top( void ) {
      if ( this->tos > 0 )
        return this->stack[ this->tos - 1 ];
      return NULL;
    };

    CFileExpr *pop( void ) {
      if ( this->tos > 0 )
        return this->stack[ --this->tos ];
      return NULL;
    };

    CFileExpr *next( void ) {
      CFileExpr * node;
      if ( (node = this->pop()) != NULL && node->next != NULL )
        return this->push( node->next );
      return NULL;
    };
};


class RAIMSG_DLL_EXP CFileLocator {
  public:
    virtual ~CFileLocator() {};

    virtual InputStream *openFile( const char *fileName )   throw( Error ) = 0;

    virtual InputStream *openFile( const char *fileName,
                                   const char *searchPath,  char pathSep,
                                   char *path,  unsigned int maxPathLen )
                                                            throw( Error ) = 0;
};


class RAIMSG_DLL_EXP CFileLoc : public CFileLocator {
  public:
    virtual ~CFileLoc() {};

    virtual InputStream *openFile( const char *fileName )   throw( Error );

    virtual InputStream *openFile( const char *fileName,
                                   const char *searchPath,  char pathSep,
                                   char *path,  unsigned int maxPathLen )
                                                            throw( Error );
};


class RAIMSG_DLL_EXP CFileParser {
  private:
    enum TokenType {
      NULL_TOK        = 0,
      IDENT_TOK       = 1,
      STRING_TOK      = 2,
      SEMI_TOK        = 3,
      OPEN_CURLY_TOK  = 4,
      CLOSE_CURLY_TOK = 5,
      COMMENT_TOK     = 6,
      WHITE_SPC_TOK   = 7,
      EOF_TOK         = 8
    };

    struct CFileToken {
      TokenType    type;
      unsigned int off,
                   len,
                   lineNo;
    };

    CFileStrings * strings;
    InputStream  * in;
    const char   * fileName;
    char           buf[ 2048 ];
    unsigned int   off,
                   nBytes,
                   fileOff,
                   lineNo;
    CFileToken     lookahead;

    void reset( void );

    static bool findFile( const char *fileName,  const char *searchPath,
                          char pathSep,  char fileSep,  char *path,
                          unsigned int maxPathLen )             throw( Error );
    CFileExpr *parseNode( void )                                throw( Error );

    void parseValue( CFileExpr *node )                          throw( Error );

    CFileExpr *createNode( const char *ident,  unsigned int len,
                           unsigned int off )                   throw( Error );
    TokenType getToken( unsigned int &off,  unsigned int &len,
                        unsigned int &lineNo )                  throw( Error );
    bool fillBuf( unsigned int &off )                           throw( Error );

    TokenType getLookahead( void )                              throw( Error );

    void nextToken( void );

    static bool parseUInt( const char *ptr,  const char *endPtr,
                           unsigned int &uintVal );
    bool isUInt( unsigned int &uintVal );

    bool isInt( int &intVal );

    bool isBool( bool &boolVal );

  public:
    SYS_OPS( CFileParser );
    CFileParser( CFileStrings *strings );
    ~CFileParser();

    static CFileStrings *createStrings( void )                  throw( Error );

    static void releaseStrings( CFileStrings *strings );

#if defined( _WIN32 ) || defined( _WIN64 )
    static const char PATH_SEP = ';';
    static const char FILE_SEP = '\\';
#else
    static const char PATH_SEP = ':';
    static const char FILE_SEP = '/';
#endif
    CFileExpr *parsePath( CFileLocator *locator,  const char *fileName,
                          const char *searchPath = NULL,
                          char pathSep = PATH_SEP,
                          bool expandIncludes = true )          throw( Error );
    CFileExpr *parseStream( InputStream *in,  const char *path = "<input>" )
                                                                throw( Error );
};


namespace CFileErr {
  enum {
    MISSING_QUOTE         = 0,
    HUGE_TOKEN            = 1,
    BAD_TOKEN             = 2,
    EXPECTING_IDENT       = 3,
    EXPECTING_SEMI        = 4,
    EXPECTING_CLOSE_CURLY = 5,
    EXPECTING_VALUE       = 6,
    EOF_PREMATURE         = 7,
    STRING_TOO_BIG        = 8,
    ITER_TOO_DEEP         = 9,
    FILE_NOT_FOUND        = 10,
    PATH_TOO_BIG          = 11,
    FILE_NAME_NULL        = 12
  };

  RAIMSG_DLL_EXP
  Error getErr( unsigned int status );
}
} // namespace rai

#endif
