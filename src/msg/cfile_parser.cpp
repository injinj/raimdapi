/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#if ! defined( RAIMSG_DLL_EXP ) && defined( RAI_DLL )
#define RAIMSG_DLL_EXP __declspec(dllexport)
#endif

#include <string.h>
#include <ctype.h>

#include "msg/cfile_parser.h"
#include "msg/cfile_strings.h"
#include "stream/file_stream.h"
#include "base/file.h"
#include "base/dir.h"
#include "base/log.h"

using namespace rai;

InputStream *
CFileLoc::openFile( const char *fileName,  const char *searchPath,
                    char pathSep,  char *path,  unsigned int maxPathLen )

{
  const char * sptr,
             * fptr;
  unsigned int i;

  if ( fileName == NULL )
    throw CFileErr::getErr( CFileErr::FILE_NAME_NULL );
  if ( searchPath == NULL )
    return this->openFile( fileName );

  if ( pathSep == 0 ) {
    sptr = searchPath;
    for ( i = 0; i < maxPathLen && *sptr != '\0'; ) {
#if defined( _WIN32 ) || defined( _WIN64 )
      if ( *sptr == ' ' || *sptr == ';' )
#else
      if ( *sptr == ' ' || *sptr == ':' || *sptr == ';' )
#endif
      {
        if ( i < maxPathLen ) {
          path[ i ] = '\0';
          if ( Dir::dirExists( path ) ) {
            pathSep = *sptr;
            break;
          }
        }
      }
      path[ i++ ] = *sptr++;
    }
  }
  /* walk throught search path, testing for file exists */
  sptr = searchPath;
  do {
    for ( i = 0; i < maxPathLen && *sptr != '\0'; ) {
      if ( *sptr == pathSep )
        break;
      path[ i++ ] = *sptr++;
    }
    if ( i > 0 ) {
      if ( i < maxPathLen && path[ i - 1 ] != '/' && path[ i - 1 ] != '\\' )
        path[ i++ ] = '/';
      for ( fptr = fileName; i < maxPathLen && *fptr != '\0'; )
        path[ i++ ] = *fptr++;
      if ( i == maxPathLen )
        throw CFileErr::getErr( CFileErr::PATH_TOO_BIG );

      path[ i++ ] = '\0';
      if ( File::fileExists( path ) )
        return this->openFile( path );
    }
    if ( pathSep != '\0' )
      while ( *sptr == pathSep )
        sptr++;
  } while ( *sptr != '\0' );
  throw CFileErr::getErr( CFileErr::FILE_NOT_FOUND );
}


InputStream *
CFileLoc::openFile( const char *fileName )
{
  return FileInputStream::open( fileName );
}


CFileParser::CFileParser( CFileStrings *strings )
{
  this->strings  = strings;
  this->fileName = NULL;
  this->in       = NULL;
  this->reset();
}


CFileParser::~CFileParser()
{
}


CFileStrings *
CFileParser::createStrings( void )
{
  return NEW CFileStrings();
}


void
CFileParser::releaseStrings( CFileStrings *strings )
{
  delete strings;
}


CFileExpr *
CFileParser::parsePath( CFileLocator *locator,  const char *fileName,
                        const char *searchPath,  char pathSep,
                        bool expandIncludes )
{
  CFileLoc      loc;
  CFileExpr   * node,
              * head,
              * tail,
              * parent,
              * grand,
              * next,
              * includeNode,
              * cfileIncludes;
  InputStream * in;
  CFileExprIter iter;
  char          path[ 1024 ];

  if ( locator == NULL )
    locator = &loc;

  try {
    in = locator->openFile( fileName, searchPath, pathSep, path,
                            sizeof( path ) );
  } catch ( Error e ) {
    logError( LERROR, e, "File \"%s\" Search path \"%s\" Path Separator \"%c\"", 
              fileName, searchPath, pathSep?pathSep:' ' );
    throw e;
  }

  logDebug( LDEBUG, "CFile parsing \"%s\"", path );

  try {
    node = this->parseStream( in, path );
    in->close();
    delete in;
    in = NULL;

    if ( node != NULL && expandIncludes ) {
      iter.push( node );
      while ( (cfileIncludes = iter.find( "CFILE_INCLUDES" )) != NULL ) {
        head = NULL;
        tail = NULL;

        if ( (includeNode = iter.child()) != NULL ) {
          do {
            try {
              in = locator->openFile( includeNode->ident, searchPath, pathSep,
                                      path, sizeof( path ) );
            } catch ( Error e ) {
              logError( LERROR, e, "File \"%s\"", includeNode->ident );
              includeNode = NULL;
            }

            if ( in != NULL ) {
              logDebug( LDEBUG, "CFile cfile_include \"%s\"", path );
              includeNode = this->parseStream( in, path );
              in->close();
              delete in;
              in = NULL;
            }

            if ( includeNode != NULL ) {
              if ( head == NULL )
                head = includeNode;
              else
                tail->setNext( includeNode );
              tail = includeNode;;
              while ( (next = tail->getNext()) != NULL )
                tail = next;
            }
          } while ( (includeNode = iter.next()) != NULL );
        }

        /* if expanded to empty list, remove include node */
        if ( head == NULL ) {
          if ( node == cfileIncludes ) {
            cfileIncludes->unlink();
            node = cfileIncludes->getNext();
          }
          else {
            parent = iter.pop();
            for (;;) {
              parent->unlink();
              next = parent->getNext();
              if ( parent == node ) {
                node = next;
                break;
              }
              grand = iter.pop();
              if ( grand == NULL || ! grand->valueEquals( parent ) )
                break;
              grand->setValue( next );
              if ( next != NULL )
                break;
              parent = grand;
            }
          }
        }
        /* expanded to something, replace include node with expanded list */
        else {
          if ( node == cfileIncludes )
            node = head;
          else if ( (parent = iter.parent()) != NULL ) {
            if ( parent->valueEquals( cfileIncludes ) )
              parent->setValue( head );
          }
          cfileIncludes->replace( head, tail );
        }

        iter.clear();
        iter.push( node );
      }
    }
  } catch ( ... ) {
    if ( in != NULL )
      delete in;
    throw;
  }

  return node;
}


CFileExpr *
CFileParser::parseStream( InputStream *in,  const char *path )
{
  CFileExpr * node;
  Error       e2;

  this->in       = in;
  this->fileName = this->strings->getFileName( path );

  e2   = NULL;
  node = NULL;

  try {
    node = this->parseNode();
    if ( this->getLookahead() != EOF_TOK )
      throw CFileErr::getErr( CFileErr::BAD_TOKEN );
  } catch ( Error e ) {
    e2 = e;
  }

  if ( e2 != NULL ) {
    logError( LERROR, e2, "In file \"%s\", on line %u", path,
              this->lookahead.lineNo );
  }

  this->reset();
  if ( e2 != NULL )
    throw e2;

  return node;
}


void
CFileParser::reset( void )
{
  this->fileName = NULL;
  this->in       = NULL;
  this->off      = 0;
  this->nBytes   = 0;
  this->lineNo   = 1;

  this->lookahead.type   = NULL_TOK;
  this->lookahead.off    = 0;
  this->lookahead.len    = 0;
  this->lookahead.lineNo = 0;
}


CFileExpr *
CFileParser::parseNode( void )
{
  CFileExpr * node,
            * head,
            * tail;

  head = NULL;
  tail = NULL;
  for (;;) {
    switch ( this->getLookahead() ) {
      case IDENT_TOK:
      case STRING_TOK:
        if ( this->lookahead.type == IDENT_TOK )
          node = this->createNode( &this->buf[ this->lookahead.off ],
                                   this->lookahead.len,
                                   this->lookahead.off + this->fileOff );
        else
          node = this->createNode( &this->buf[ this->lookahead.off + 1 ],
                                   this->lookahead.len - 2,
                                   this->lookahead.off + this->fileOff );
        this->nextToken();
        this->parseValue( node );
        break;

      case OPEN_CURLY_TOK:
        node = this->createNode( NULL, 0, this->lookahead.off + this->fileOff );
        this->parseValue( node );
        break;

      default:
        throw CFileErr::getErr( CFileErr::EXPECTING_IDENT );

      case CLOSE_CURLY_TOK:
      case EOF_TOK:
        return head;
    }

    if ( head == NULL )
      head = node;
    else
      tail->setNext( node );
    tail = node;
  }
}


void
CFileParser::parseValue( CFileExpr *node )
{
  CFileExpr  * expr;
  const char * val;
  unsigned int uintVal;
  int          intVal;
  bool         boolVal;

  switch ( this->getLookahead() ) {
    case IDENT_TOK:
      if ( this->isUInt( uintVal ) )
        node->setValue( uintVal );
      else if ( this->isInt( intVal ) )
        node->setValue( intVal );
      else if ( this->isBool( boolVal ) )
        node->setValue( boolVal );
      else {
        val = this->strings->getString( &this->buf[ this->lookahead.off ],
                                        this->lookahead.len );
        node->setValue( val );
      }
      break;
    case STRING_TOK:
      val = this->strings->getString( &this->buf[ this->lookahead.off + 1 ],
                                      this->lookahead.len - 2 );
      node->setValue( val );
      this->nextToken();
      if ( this->getLookahead() != SEMI_TOK )
        throw CFileErr::getErr( CFileErr::EXPECTING_SEMI );
      break;

    case OPEN_CURLY_TOK:
      this->nextToken();
      expr = this->parseNode();
      node->setValue( expr );
      if ( this->getLookahead() != CLOSE_CURLY_TOK )
        throw CFileErr::getErr( CFileErr::EXPECTING_CLOSE_CURLY );
      break;

    case SEMI_TOK:
      break;

    case EOF_TOK:
      throw CFileErr::getErr( CFileErr::EOF_PREMATURE );

    default:
      throw CFileErr::getErr( CFileErr::EXPECTING_VALUE );
  }
  do {
    this->nextToken();
  } while ( this->getLookahead() == SEMI_TOK );
}


CFileExpr *
CFileParser::createNode( const char *ident,  unsigned int len,
                         unsigned int off )
{
  CFileExpr * node;

  node       = (CFileExpr *) this->strings->allocMem( sizeof( CFileExpr ) );
  node->type = CFileExpr::NULL_TYPE;
  node->off  = off;
  node->next = NULL;
  node->back = NULL;

  if ( ident == NULL )
    node->ident = NULL;
  else
    node->ident = this->strings->getString( ident, len );

  return node;
}


CFileParser::TokenType
CFileParser::getToken( unsigned int &off,  unsigned int &len,
                       unsigned int &lineNo )
{
  unsigned int i;
  TokenType    type;
  char       * p;

  i = this->off;
  if ( i == this->nBytes && ! this->fillBuf( i ) )
    type = EOF_TOK;
  else {
    switch ( this->buf[ i++ ] ) {
      case ';':
        type = SEMI_TOK;
        break;
      case '#':
        type = COMMENT_TOK;
        for ( ;; i++ ) {
          if ( i == this->nBytes && ! this->fillBuf( i ) )
            break;
          if ( this->buf[ i ] == '\n' ) {
            i++;
            break;
          }
        }
        break;
      case '"':
        type = STRING_TOK;
        for ( ;; i++ ) {
          if ( i == this->nBytes && ! this->fillBuf( i ) )
            throw CFileErr::getErr( CFileErr::MISSING_QUOTE );
          if ( this->buf[ i ] == '"' ) {
            i++;
            break;
          }
          if ( this->buf[ i ] == '\\' ) {
            if ( i + 1 == this->nBytes && ! this->fillBuf( i ) )
              throw CFileErr::getErr( CFileErr::MISSING_QUOTE );
            if ( this->buf[ i + 1 ] == '\"' )
              i++;
          }
        }
        break;
      case '{':
        type = OPEN_CURLY_TOK;
        break;
      case '}':
        type = CLOSE_CURLY_TOK;
        break;
      default:
        if ( isspace( this->buf[ i-1 ] ) ) {
          type = WHITE_SPC_TOK;
          while ( i < this->nBytes && isspace( this->buf[ i ] ) )
            i++;
        }
        else {
          type = IDENT_TOK;
          for ( ;; i++ ) {
            if ( i == this->nBytes && ! this->fillBuf( i ) )
              break;
            if ( isspace( this->buf[ i ] ) || this->buf[ i ] == ';' ||
                 this->buf[ i ] == '#' || this->buf[ i ] == '"' ||
                 this->buf[ i ] == '{' || this->buf[ i ] == '}' )
              break;
          }
        }
        break;
    }
  }
  len    = i - this->off;
  off    = this->off;
  lineNo = this->lineNo;

  this->off = i;
  for ( i = off; i < this->off; ) {
    p = (char *) ::memchr( &this->buf[ i ], '\n', this->off - i );
    if ( p == NULL )
      break;
    i += &p[ 1 ] - &this->buf[ i ];
    this->lineNo++;
  }

  return type;
}


bool
CFileParser::fillBuf( unsigned int &off )
{
  unsigned int n;

  off -= this->off;
  if ( off == sizeof( this->buf ) )
    throw CFileErr::getErr( CFileErr::HUGE_TOKEN );

  if ( off > 0 ) {
    ::memmove( this->buf, &this->buf[ this->off ], off );
    this->nBytes -= this->off;
  }
  else {
    this->nBytes = 0;
  }
  this->fileOff += this->off;
  this->off = 0;

  n = this->in->readBytes( (byte *) &this->buf[ off ],
                           sizeof( this->buf ) - off );
  if ( n == 0 )
    return false;

  this->nBytes += n;
  return true;
}


CFileParser::TokenType
CFileParser::getLookahead( void )
{
  TokenType    type;
  unsigned int off,
               len;

  if ( this->lookahead.type != NULL_TOK )
    return this->lookahead.type;

  for (;;) {
    type = this->getToken( off, len, lineNo );
    if ( type != COMMENT_TOK && type != WHITE_SPC_TOK ) {
      this->lookahead.type   = type;
      this->lookahead.off    = off;
      this->lookahead.len    = len;
      this->lookahead.lineNo = lineNo;
      return type;
    }
  }
}


void
CFileParser::nextToken( void )
{
  if ( this->lookahead.type != NULL_TOK )
    this->lookahead.type = NULL_TOK;
}


bool
CFileParser::parseUInt( const char *ptr,  const char *endPtr,
                        unsigned int &uintVal )
{
  if ( ptr == endPtr )
    return false;

  if ( isdigit( ptr[ 0 ] ) ) {
    uintVal = 0;
    if ( ptr[ 0 ] == '0' && &ptr[ 2 ] < endPtr &&
         ( ptr[ 1 ] == 'x' || ptr[ 1 ] == 'X' ) && isxdigit( ptr[ 2 ] ) ) {
      ptr = &ptr[ 2 ];
      do {
        uintVal = ( uintVal << 4 ) | (unsigned int)
                  ( ( ptr[ 0 ] >= '0' && ptr[ 0 ] <= '9' ) ? ptr[ 0 ] - '0' :
                    ( ( ( ptr[ 0 ] >= 'a' && ptr[ 0 ] <= 'f' ) ?
                          ptr[ 0 ] - 'a' : ptr[ 0 ] - 'A' ) + 10 ) );
      } while ( ++ptr < endPtr && isxdigit( ptr[ 0 ] ) );
    }
    else {
      do {
        uintVal = ( uintVal * 10 ) + (unsigned int) ( ptr[ 0 ] - '0' );
      } while ( ++ptr < endPtr && isdigit( ptr[ 0 ] ) );
    }
  }
  if ( ptr == endPtr )
    return true;
  return false;
}


bool
CFileParser::isUInt( unsigned int &uintVal )
{
  const char * ptr,
             * endPtr;
 
  ptr    = &this->buf[ this->lookahead.off ];
  endPtr = &ptr[ this->lookahead.len ];

  if ( ptr[ 0 ] == '+' )
    ptr++;
  return this->parseUInt( ptr, endPtr, uintVal );
}


bool
CFileParser::isInt( int &intVal )
{
  const char * ptr,
             * endPtr;
  unsigned int uintVal;
 
  ptr    = &this->buf[ this->lookahead.off ];
  endPtr = &ptr[ this->lookahead.len ];

  if ( ptr[ 0 ] == '-' ) {
    ptr++;
    if ( this->parseUInt( ptr, endPtr, uintVal ) ) {
      intVal = -(int) uintVal;
      return true;
    }
  }
  return false;
}


bool
CFileParser::isBool( bool &boolVal )
{
  const char * ptr;
 
  ptr = &this->buf[ this->lookahead.off ];
  if ( this->lookahead.len == 4 &&
       toupper( ptr[ 0 ] ) == 'T' && toupper( ptr[ 1 ] ) == 'R' &&
       toupper( ptr[ 2 ] ) == 'U' && toupper( ptr[ 3 ] ) == 'E') {
    boolVal = true;
    return true;
  }
  if ( this->lookahead.len == 5 &&
       toupper( ptr[ 0 ] ) == 'F' && toupper( ptr[ 1 ] ) == 'A' &&
       toupper( ptr[ 2 ] ) == 'L' && toupper( ptr[ 3 ] ) == 'S' &&
       toupper( ptr[ 4 ] ) == 'E' ) {
    boolVal = false;
    return true;
  }
  return false;
}


void
CFileExpr::print( OutputStream *out,  bool siblings,  bool children,
                  unsigned int indent )

{
  const CFileExpr * expr;

  expr = this;
  do {
    switch ( expr->type ) {
      case NULL_TYPE:
        if ( expr->ident != NULL )
          out->printf( "%*s\"%s\";\n", indent, "", expr->ident );
        break;
      case UINT_TYPE:
        out->printf( "%*s%s %u;\n", indent, "", expr->ident, expr->val.uintVal );
        break;
      case INT_TYPE:
        out->printf( "%*s%s %d;\n", indent, "", expr->ident, expr->val.intVal );
        break;
      case STRING_TYPE:
        out->printf( "%*s%s \"%s\";\n", indent, "", expr->ident,
                     expr->val.stringVal );
        break;
      case BOOL_TYPE:
        out->printf( "%*s%s %s;\n", indent, "", expr->ident,
                     expr->val.boolVal ? "true" : "false" );
        break;
      case SUBEXPR_TYPE:
        if ( children && expr->val.exprVal != NULL ) {
          if ( expr->ident != NULL )
            out->printf( "%*s%s {\n", indent, "", expr->ident );
          else
            out->printf( "%*s{\n", indent, "" );
          expr->val.exprVal->print( out, siblings, children, indent + 4 );
          out->printf( "%*s}\n", indent, "" );
        }
        else {
          if ( expr->ident != NULL )
            out->printf( "%*s%s {}\n", indent, "", expr->ident );
          else
            out->printf( "%*s{}\n", indent, "" );
        }
        break;
    }
  } while ( siblings && (expr = expr->next) != NULL );
}


bool
CFileExpr::getValue( unsigned int &val )
{
  const char * ptr;

  switch ( this->type ) {
    case UINT_TYPE:
      val = this->val.uintVal;
      return true;
    case INT_TYPE:
      val = (unsigned int) this->val.intVal;
      return true;
    case STRING_TYPE:
      ptr = this->val.stringVal;
      if ( ptr == NULL )
        return false;
      for ( val = 0; isdigit( *ptr ); ptr++ )
        val = val * 10 + (unsigned int) ( *ptr - '0' );
      if ( *ptr == '\0' )
        return true;
      break;
    case BOOL_TYPE:
      val = (unsigned int) this->val.boolVal;
      return true;
    default:
      break;
  }
  return false;
}


bool
CFileExpr::getValue( bool &val )
{
  switch ( this->type ) {
    case UINT_TYPE:
      if ( this->val.uintVal == 0 ) {
        val = false;
        return true;
      }
      if ( this->val.uintVal == 1 ) {
        val = true;
        return true;
      }
      break;
    case INT_TYPE:
      if ( this->val.intVal == 0 ) {
        val = false;
        return true;
      }
      if ( this->val.intVal == 1 ) {
        val = true;
        return true;
      }
      break;
    case STRING_TYPE:
      if ( this->val.stringVal != NULL ) {
        if ( toupper( this->val.stringVal[ 0 ] ) == 'T' &&
             toupper( this->val.stringVal[ 1 ] ) == 'R' &&
             toupper( this->val.stringVal[ 2 ] ) == 'U' &&
             toupper( this->val.stringVal[ 3 ] ) == 'E' &&
             this->val.stringVal[ 4 ] == '\0' ) {
          val = true;
          return true;
        }
        if ( toupper( this->val.stringVal[ 0 ] ) == 'F' &&
             toupper( this->val.stringVal[ 1 ] ) == 'A' &&
             toupper( this->val.stringVal[ 2 ] ) == 'L' &&
             toupper( this->val.stringVal[ 3 ] ) == 'S' &&
             toupper( this->val.stringVal[ 4 ] ) == 'E' &&
             this->val.stringVal[ 5 ] == '\0' ) {
          val = true;
          return true;
        }
      }
      break;
    case BOOL_TYPE:
      val = this->val.boolVal;
      return true;
    default:
      break;
  }
  return false;
}


CFileExpr *
CFileExprIter::push( CFileExpr *node )
{
  if ( this->tos == MAX_EXPR_DEPTH )
    throw CFileErr::getErr( CFileErr::ITER_TOO_DEEP );
  return this->stack[ this->tos++ ] = node;
}


CFileExpr *
CFileExprIter::find( const char *identName )
{
  CFileExpr  * node;

  if ( (node = this->top()) != NULL ) {
    if ( identName == NULL ) {
      for (;;) {
        do {
          if ( node->ident == NULL )
            return node;
        } while ( (node = this->child()) != NULL );
        while ( (node = this->next()) == NULL )
          if ( this->tos == 0 )
            return NULL;
      }
    }
    else {
      for (;;) {
        do {
          if ( node->ident != NULL && ::strcmp( identName, node->ident ) == 0 )
            return node;
        } while ( (node = this->child()) != NULL );
        while ( (node = this->next()) == NULL )
          if ( this->tos == 0 )
            return NULL;
      }
    }
  }
  return NULL;
}


CFileExpr *
CFileExprIter::findNext( const char *identName )
{
  if ( this->child() == NULL ) {
    while ( this->next() == NULL )
      if ( this->tos == 0 )
        return NULL;
  }
  return this->find( identName );
}


CFileStrings::CFileStrings()
            : SortedArray<const char *>( BLOCK_SIZE / 8, true ), blocks( 8 )
{
  this->memLeft = 0;
  this->len     = 0;
}


CFileStrings::~CFileStrings()
{
  while ( ! this->blocks.isEmpty() ) {
    Block b = this->blocks.popTail();
    FREE( b.mem );
  }
}


int
CFileStrings::compare( const char *s1,  const char *s2 )
{
  int cmp;
  
  cmp = ::strncmp( s1, s2, this->len );
  if ( cmp == 0 )
    cmp = 0 - s2[ this->len ];
  return cmp;
}


void *
CFileStrings::allocMem( unsigned int len )
{
  unsigned int pos;
  Block        b;

  len = ( len + sizeof( void * ) - 1 ) & ~( sizeof( void * ) - 1 );
  if ( len > BLOCK_SIZE )
    throw CFileErr::getErr( CFileErr::STRING_TOO_BIG );

  if ( (pos = this->blocks.length()) > 0 )
    b = this->blocks.get( pos - 1 );
  else
    b.fileName = NULL;

  if ( this->memLeft >= len ) {
    this->memLeft -= len;
  }
  else {
    b.mem = NULL;
    try {
      MALLOC( BLOCK_SIZE, &b.mem );
      this->blocks.pushTail( b );
      this->memLeft = BLOCK_SIZE - len;
    } catch ( ... ) {
      if ( b.mem != NULL )
        FREE( b.mem );
      throw;
    }
  }

  return &b.mem[ this->memLeft ];
}


const char *
CFileStrings::getFileName( const char *fileName )
{
  unsigned int pos;
  void       * mem;
  Block        b;

  this->memLeft = 0;
  mem = this->allocMem( ::strlen( fileName ) + 1 );
  ::strcpy( (char *) mem, fileName );

  fileName   = (char *) mem;
  pos        = this->blocks.length() - 1;
  b          = this->blocks.get( pos );
  b.fileName = fileName;
  this->blocks.put( pos, b );

  return fileName;
}


const char *
CFileStrings::getString( const char *ptr,  unsigned int len )
{
  unsigned int pos;
  void       * mem;

  this->len = len;
  if ( this->find( ptr, &pos ) )
    return this->get( pos );

  mem = this->allocMem( len + 1 );
  ::memcpy( mem, ptr, len );
  ((char *) mem)[ len ] = '\0';
  ptr = (const char *) mem;
  this->insert( ptr );

  return ptr;
}


Error
CFileErr::getErr( unsigned int status )
{
  static const char     mod[] = "CFile";
  static const ErrorRec err[] = {
  /*  0 */ { MISSING_QUOTE,         "Missing end quote", mod },
  /*  1 */ { HUGE_TOKEN,            "Token is too large to parse", mod },
  /*  2 */ { BAD_TOKEN,             "Token unexpected", mod },
  /*  3 */ { EXPECTING_IDENT,       "Expecting identifier", mod },
  /*  4 */ { EXPECTING_SEMI,        "Expecting semicolon", mod },
  /*  5 */ { EXPECTING_CLOSE_CURLY, "Expecting close curly bracket", mod },
  /*  6 */ { EXPECTING_VALUE,       "Expecting value", mod },
  /*  7 */ { EOF_PREMATURE,         "Found premature EOF while parsing", mod },
  /*  8 */ { STRING_TOO_BIG,        "Identifier or string too large to "
                                    "allocate", mod },
  /*  9 */ { ITER_TOO_DEEP,         "Iterator ran out of stack space", mod },
  /* 10 */ { FILE_NOT_FOUND,        "File not found in search path", mod },
  /* 11 */ { PATH_TOO_BIG,          "Path too large", mod },
  /* 12 */ { FILE_NAME_NULL,        "Null filename", mod },
  /* 13 */ { 13,                    "Unknown cfile error", mod }
  };
  static const unsigned int numErrs = sizeof( err ) / sizeof( err[ 0 ] ) - 1;

  return &err[ status < numErrs ? status : numErrs ];
}

