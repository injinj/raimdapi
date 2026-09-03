/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#if ! defined( RAIBASE_DLL_EXP ) && defined( RAI_DLL )
#define RAIBASE_DLL_EXP __declspec(dllexport)
#endif

#include <string.h>

#include "base/sys.h"
#include "base/dir.h"
#include "util/uri.h"
#include "util/array.h"

using namespace rai;

/*
 * Based on rfc2396
 *
 * <absolute_uri>    -> <scheme> ":" ( <heir_part> | <opaque_part> )
 * <scheme>          -> alpha ( alnum | [\+\-\.] )*
 * <heir_part>       -> ( <net_path> | <abs_path> ) ( "?" <query> )?
 * <opaque_part>     -> <uri_char_no_slash> <uri_char>*
 * <net_path>        -> "//" <authority> ( <abs_path> )?
 * <abs_path>        -> "/" <path_segments>
 * <authority>       -> <server> | <reg_name>
 * <server>          -> [ <userinfo> "@" ] <hostport>
 * <reg_name>        -> ( <unreserved_char> | <escaped> | [\$\,\;\:\@\&\=\+] )+
 * <userinfo>        -> ( <unreserved_char> | <escaped> | [\;\:\&\=\+\$\,] )*
 * <hostport>        -> <host> ( ":" <port> )?
 * <host>            -> <hostname> | <IPv4address>
 * <hostname>        -> ( <domainlabel> "." )* <toplabel> "."?
 * <domainlabel>     -> alnum | ( alnum ( alnum | "-" )* alnum )
 * <toplabel>        -> alpha | ( alpha ( alnum | "-" )* alnum )
 * <IPv4address>     -> digit+ "." digit+ "." digit+ "." digit+
 * <port>            -> digit+
 * <path_segments>   -> <segment> ( "/" <segment> )*
 * <segment>         -> <pchar>* ( ";" <pchar>* )*
 * <pchar>           -> <unreserved_char> | <escaped_char> | [\:\@\&\=\+\$\,]
 * <query>           -> <uri_char>*
 * <uri_reference>   -> ( <absolute_uri> | <relative_uri> )? ( "#" <fragment> )?
 * <fragment>        -> <uri_char>*
 * <relative_uri>    -> ( <net_path> | <abs_path> | <rel_path> ) ( "?" <query> )?
 * <rel_path>        -> <rel_segment> ( <abs_path> )?
 * <rel_segment>     -> ( <unreserved_char> | <escaped> | [\;\@\&\=\+\$\,] )+
 * <uri_char>        -> <reserved_char> | <unreserved_char> | <escaped>
 * <reserved_char>   -> [\;\/\?\:\@\&\=\+\$\,]
 * <unreserved_char> -> alnum | <mark>
 * <mark>            -> [\-\_\.\!\~\*\'\(\)]
 * <escaped>         -> "%" hex hex
 * <ascii_entity>    -> "&amp;" | "&lt;" | "&gt;" | "&apos;" | "&quot;"
 */

namespace rai {
enum UriPartName {
  URI_ALL            = 0,
  URI_ALL_ENCODED    = 1,
  SCHEME             = 2,
  OPAQUE             = 3,
  OPAQUE_ENCODED     = 4,
  ABS_PATH           = 5,
  ABS_PATH_ENCODED   = 6,
  REL_PATH           = 7,
  REL_PATH_ENCODED   = 8,
  QUERY              = 9,
  QUERY_ENCODED      = 10,
  REG_NAME           = 11,
  USERINFO           = 12,
  HOST               = 13,
  PORT               = 14,
  REFERENCE          = 15,
  REFERENCE_ENCODED  = 16,
  HOST_PORT          = 17,
  PATH_QUERY         = 18,
  PATH_QUERY_ENCODED = 19,
  URI_BASE           = 20
#define MAX_PART_NAME 21
};

struct UriPart {
  UriPartName  name;
  char       * part;
  unsigned int len;
};


#if 0
class UriPartArray : public SortedArray<UriPart *> {
  protected:
    virtual int compare( UriPart *p1,  UriPart *p2 ) {
      return (int) p1->name - (int) p2->name;
    }

  public:
    UriPartArray( unsigned int growBy ) : SortedArray<UriPart *>( growBy ) {}
};
#endif
class UriPartArray {
  protected:
    UriPart    * ar[ MAX_PART_NAME ];
    unsigned int maxItem;

  public:
    UriPartArray() {
      ::memset( this->ar, 0, sizeof( this->ar ) );
      this->maxItem = 0;
      this->bufOff  = 0;
    }

    UriPart *find( UriPart *ptr,  unsigned int *pos = NULL ) const {
      if ( this->ar[ ptr->name ] != NULL ) {
        if ( pos != NULL )
          *pos = ptr->name;
        return this->ar[ ptr->name ];
      }
      return NULL;
    }

    void insert( UriPart *ptr ) {
      this->ar[ ptr->name ] = ptr;
      if ( (unsigned int) ptr->name >= this->maxItem )
        this->maxItem = (unsigned int) ptr->name + 1;
    }

    UriPart *get( unsigned int pos ) const {
      return this->ar[ pos ];
    }

    bool isEmpty( void ) const {
      return ( this->maxItem == 0 );
    }

    UriPart *popTail( void ) {
      if ( this->maxItem == 0 )
        return NULL;
      UriPart *p = this->ar[ --this->maxItem ];
      this->ar[ this->maxItem ] = NULL;
      for ( ; this->maxItem > 0; this->maxItem-- )
        if ( this->ar[ this->maxItem - 1 ] != NULL )
          break;
      return p;
    }

    void pushTail( UriPart *p ) {
      this->insert( p );
    }

    UriPart *put( unsigned int pos,  UriPart *p ) {
      UriPart *q = this->ar[ pos ];
      this->ar[ pos ] = p;
      return q;
    }

    UriPart *remove( unsigned int pos ) {
      UriPart *q = this->ar[ pos ];
      this->ar[ pos ] = NULL;
      if ( q != NULL ) {
        if ( pos == this->maxItem - 1 )
          for ( ; this->maxItem > 0; this->maxItem-- )
            if ( this->ar[ this->maxItem - 1 ] != NULL )
              break;
      }
      return q;
    }

    UriPart *first( unsigned int &i ) {
      i = 0;
      if ( this->maxItem == 0 )
        return NULL;
      for (;;) {
        if ( this->ar[ i ] != NULL )
          return this->ar[ i ];
        if ( ++i == this->maxItem )
          return NULL;
      }
    }

    UriPart *next( unsigned int &i ) {
      for (;;) {
        if ( ++i == this->maxItem )
          return NULL;
        if ( this->ar[ i ] != NULL )
          return this->ar[ i ];
      }
    }

    UriPart      buf[ 64 ];
    unsigned int bufOff;

    UriPart *newPart( unsigned int size ) {
      unsigned int n = ( size + sizeof( UriPart ) - 1 ) / sizeof( UriPart );
      UriPart    * p;
      if ( this->bufOff + n <=
           sizeof( this->buf ) / sizeof( this->buf[ 0 ] ) ) {
        p = &this->buf[ this->bufOff ];
        this->bufOff += n;
      }
      else {
        MALLOC( size, &p );
      }
      return p;
    }

    void freePart( UriPart *p ) {
      if ( p < &this->buf[ 0 ] ||
           p >= &this->buf[ sizeof( this->buf ) / sizeof( this->buf[ 0 ] ) ] )
        FREE( p );
    }
};


class URIParser : public URI {
  private:
    UriPartArray parts;
    Error        err;
    bool         isWild;

  public:
    SYS_OPS( URIParser );

    URIParser( bool isWild );
    virtual ~URIParser();

    static bool isMember( const char *s,  unsigned int len,  char c ) {
      return ::memchr( s, c, len ) != NULL;
    }
    static bool isReservedChar( char c ) {
      static char reservedChar[] = ";/?:@&=+$,";
      return isMember( reservedChar, sizeof( reservedChar ) - 1, c );
    }
    static bool isMark( char c ) {
      static char markChar[] = "-_.!~*'()";
      return isMember( markChar, sizeof( markChar ) - 1, c );
    }
    static bool isHex( char c ) {
      static char hexChar[] = "abcdefABCDEF0123456789";
      return isMember( hexChar, sizeof( hexChar ) - 1, c );
    }
    static bool isEscaped( const char *s ) {
      return s[ 0 ] == '%' && isHex( s[ 1 ] ) && isHex( s[ 2 ] );
    }
    static bool isAlpha( char c ) {
      return ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' );
    }
    static bool isDigit( char c ) {
      return c >= '0' && c <= '9';
    }
    static bool isAlnum( char c ) {
      return isAlpha( c ) || isDigit( c );
    }
    static bool isUnreservedChar( char c ) {
      return isAlnum( c ) || isMark( c );
    }
    static bool isLowerChar( char c,  char lc ) {
      if ( c == lc )
        return true;
      if ( c >= 'A' && c <= 'Z' && c + ( 'a' - 'A' ) == lc )
        return true;
      return false;
    }

    void decodeAll( void );
    void clear( void );
    char * makeUri( bool encoded,  bool baseonly );

    bool scanAbsoluteUri( const char **ptr );
    bool scanScheme( const char **ptr );
    bool scanHierPart( const char **ptr );
    bool scanOpaquePart( const char **ptr );
    bool scanNetPath( const char **ptr );
    bool scanAbsPath( const char **ptr );
    bool scanAuthority( const char **ptr );
    bool scanServer( const char **ptr );
    bool scanRegName( const char **ptr );
    bool scanUserinfo( const char **ptr );
    bool scanHostport( const char **ptr );
    bool scanHost( const char **ptr );
    bool scanHostname( const char **ptr );
    bool scanDomainlabel( const char **ptr );
    bool scanToplabel( const char **ptr );
    bool scanIPv4address( const char **ptr );
    bool scanPort( const char **ptr );
    bool scanPathSegments( const char **ptr );
    bool scanSegment( const char **ptr );
    bool scanPchar( const char **ptr );
    bool scanQuery( const char **ptr );
    bool scanUriReference( const char **ptr );
    bool scanFragment( const char **ptr );
    bool scanRelativeUri( const char **ptr );
    bool scanRelPath( const char **ptr );
    bool scanRelSegment( const char **ptr );
    bool scanUriChar( const char **ptr );
    bool scan( UriPartName name,
               bool ( URIParser::*scanFunc )( const char ** ),
               const char **ptr );
    char * getPart( UriPartName name );
    char * getEncodedPart( UriPartName name,  UriPartName encodedName )
;
    char * addRelated( UriPartName name,  char *part );
    void constructPath( const char *abs,  const char *rel );

    void parse( const char *uri,  URI *related );
    virtual char * getUri( bool encoded );
    virtual char * getOpaque( bool encoded );
    virtual char * getScheme( void );
    virtual char * getUserinfo( void );
    virtual char * getHost( void );
    virtual char * getPort( void );
    virtual char * getHostPort( unsigned int defaultPort );
    virtual char * getPath( bool encoded );
    virtual char * getQuery( bool encoded );
    virtual char * getPathQuery( bool encoded );
    virtual char * getReference( bool encoded );
    virtual void   stripReference( void );
    virtual URI  * copy( void );
    virtual char * getBase( void );
};
} // namespace rai

Error
URIErr::getErr( unsigned int status )
{
  static const char     mod[] = "URI";
  static const ErrorRec err[] = {
  /*  0 */ { PARSE_FAILED, "Unable to parse uri", mod },
  /*  1 */ { HAS_NO_PATH,  "Missing path", mod },
  /*  2 */ { BAD_URI,      "Part not found", mod },
  /*  3 */ { 3,            "Unknown uri error", mod }
  };
  static const unsigned int numErrs = sizeof( err ) / sizeof( err[ 0 ] ) - 1;

  return &err[ status < numErrs ? status : numErrs ];
}


URI *
URI::create( const char *uri,  URI *related )
{
  URIParser * u;

  if ( uri == NULL )
    return NULL;

  u = NULL;
  try {
    u = NEW URIParser( false );
    u->parse( uri, related );
  } catch ( ... ) {
    if ( u != NULL )
      delete u;
    throw;
  }

  return u;
}


URI *
URI::createWorkingDir( const char *uri )
{
  char currentDir[ 1024 ];
  URI * tmp,
      * u;

  ::strcpy( currentDir, "file:" );
  Dir::workingDirectory( &currentDir[ 5 ], sizeof( currentDir ) - 5 );
#if defined( _WIN32 ) || defined( _WIN64 )
  if ( currentDir[ 5 ] != '/' ) {
    /* if has a dir letter c:/path */
    ::memmove( &currentDir[ 6 ], &currentDir[ 5 ], sizeof( currentDir ) - 6 );
    currentDir[ 5 ] = '/';
  }
#endif
  tmp = URI::create( currentDir );

  try {
    u = URI::create( uri, tmp );
    delete tmp;
  } catch ( ... ) {
    delete tmp;
    throw;
  }
  return u;
}


URI *
URI::createWild( const char *uri )
{
  URIParser * u;

  if ( uri == NULL )
    return NULL;

  u = NULL;
  try {
    u = NEW URIParser( true );
    u->parse( uri, NULL );
  } catch ( ... ) {
    if ( u != NULL )
      delete u;
    throw;
  }

  return u;
}


URI *
URI::fixupHttp( const char *host,  const char *uri )

{
  URIParser * u,
            * hs;
  char      * hostScheme;

  if ( uri == NULL )
    return NULL;

  if ( host == NULL )
    host = "localhost";

  hs         = NULL;
  u          = NULL;
  char tmpBuf[ 80 ];
  hostScheme = tmpBuf;

  try {
    unsigned int len = ::strlen( host ) + 8;
    URIParser hs( false );

    if ( len > sizeof( tmpBuf ) )
      MALLOC( len, &hostScheme );
    ::strcpy( hostScheme, "http://" );
    ::strcpy( &hostScheme[ 7 ], host );

    hs.parse( hostScheme, NULL );
    u = NEW URIParser( false );
    u->parse( uri, &hs );

    if ( hostScheme != tmpBuf )
      FREE( hostScheme );
  } catch ( ... ) {
    if ( hs != NULL )
      delete hs;
    if ( u != NULL )
      delete u;
    if ( hostScheme != tmpBuf )
      FREE( hostScheme );
    throw;
  }

  return u;
}


URIParser::URIParser( bool isWild )
{
  this->isWild = isWild;
  this->err    = NULL;
}


URIParser::~URIParser()
{
  this->clear();
}


void
URIParser::clear( void )
{
  while ( ! this->parts.isEmpty() )
    this->parts.freePart( this->parts.popTail() );
}


URI *
URIParser::copy( void )
{
  URIParser  * uri;
  UriPart    * p,
             * p2;
  unsigned int i;

  uri = NULL;
  p2  = NULL;
  try {
    uri = NEW URIParser( this->isWild );

    if ( ( p = this->parts.first( i ) ) != NULL ) {
      do {
        p2 = uri->parts.newPart( sizeof( UriPart ) + p->len + 1 );

        p2->part = (char *) &p2[ 1 ];
        p2->name = p->name;
        p2->len  = p->len;
        ::memcpy( p2->part, p->part, p->len + 1 );

        uri->parts.pushTail( p2 );
        p2 = NULL;
      } while ( ( p = this->parts.next( i ) ) != NULL );
    }
  } catch ( ... ) {
    if ( uri != NULL )
      delete uri;
    if ( p2 != NULL )
      uri->parts.freePart( p2 );
    throw;
  }

  return uri;
}


void
URIParser::parse( const char *uri,  URI *related )
{
  unsigned int pos;
  char       * s,
             * relPath;
  UriPart      p,
             * q;

  /* clear existing uri */
  this->clear();

  /* no related, so uri must be absolute */
  if ( related == NULL ) {
    if ( ! this->scanAbsoluteUri( &uri ) ) {
      if ( this->err != NULL )
        throw this->err;
      throw URIErr::getErr( URIErr::PARSE_FAILED );
    }
    this->decodeAll();

    /* fix up path if it's empty */
    p.name = HOST;
    if ( this->parts.find( &p ) ) {
      p.name = ABS_PATH;
      if ( ! this->parts.find( &p, &pos ) ||
             ( (q = this->parts.get( pos )) != NULL && q->len == 0 ) )
        this->constructPath( ( this->isWild ? "*" : "/" ), "" );
    }
  }
  /* has related, so uri may use relative path, */
  else {
    if ( ! this->scanUriReference( &uri ) ) {
      if ( this->err != NULL )
        throw this->err;
      throw URIErr::getErr( URIErr::PARSE_FAILED );
    }
    this->decodeAll();

    /* if has no scheme, must be relative uri */
    if ( this->getScheme() == NULL ) {
      /* copy parent parts of related */
      if ( (s = related->getScheme()) != NULL ) {
        this->addRelated( SCHEME, s );
        /* user@ */
        if ( this->getUserinfo() == NULL )
          if ( (s = related->getUserinfo()) != NULL )
            this->addRelated( USERINFO, s );
        /* hostname */
        if ( this->getHost() == NULL )
          if ( (s = related->getHost()) != NULL )
            this->addRelated( HOST, s );
        /* port */
        if ( this->getPort() == NULL )
          if ( (s = related->getPort()) != NULL )
            this->addRelated( PORT, s );
        /* construct absolute path */
        if ( (relPath = this->getPart( REL_PATH )) != NULL ) {
          if ( (s = related->getPath( false )) != NULL )
            this->constructPath( s, relPath );
        }
        /* use related's path if don't have one */
        else if ( this->getPath( false ) == NULL ) {
          if ( (s = related->getPath( false )) != NULL )
            this->addRelated( ABS_PATH, s );
          /* use related's query */
          if ( this->getQuery( false ) == NULL )
            if ( (s = related->getQuery( false )) != NULL )
              this->addRelated( QUERY, s );
          /* use related's reference */
          if ( this->getReference( false ) == NULL )
            if ( (s = related->getReference( false )) != NULL )
              this->addRelated( REFERENCE, s );
        }
      }
    }
    /* otherwise was absolute uri, fix up path if empty */
    else {
      p.name = HOST;
      if ( this->parts.find( &p ) ) {
        p.name = ABS_PATH;
        if ( ! this->parts.find( &p, &pos ) ||
               ( (q = this->parts.get( pos )) != NULL && q->len == 0 ) )
          this->constructPath( ( this->isWild ? "*" : "/" ), "" );
      }
    }
  }
}


void
URIParser::decodeAll( void )
{
  static UriPartName partNames[] = {
    ABS_PATH, REL_PATH, QUERY, USERINFO, REFERENCE
  };
  UriPart      p,
             * q;
  char       * s,
             * t,
               c;
  byte         hi,
               lo;
  unsigned int i,
               pos;

  for ( i = 0; i < sizeof( partNames ) / sizeof( partNames[ 0 ] ); i++ ) {
    p.name = partNames[ i ];

    if ( this->parts.find( &p, &pos ) ) {
      q = this->parts.get( pos );

      for ( s = q->part, t = q->part; s < &q->part[ q->len ]; *t++ = c ) {
        switch ( s[ 0 ] ) {
          case '%':
            /* check if the two chars following % are hex chars */
            if ( &s[ 3 ] <= &q->part[ q->len ] &&
                isHex( s[ 1 ] ) && isHex( s[ 2 ] ) ) {

              hi = (byte) s[ 1 ];
              lo = (byte) s[ 2 ];

              hi = (byte) ( this->isDigit( s[ 1 ] ) ? s[ 1 ] - '0' :
                     ( s[ 1 ] >= 'A' ? s[ 1 ] - 'A' : s[ 1 ] - 'a' ) + 10 );
              lo = (byte) ( this->isDigit( s[ 2 ] ) ? s[ 2 ] - '0' :
                     ( s[ 2 ] >= 'A' ? s[ 2 ] - 'A' : s[ 2 ] - 'a' ) + 10 );

              c = (char) (byte) ( ( hi << 4 ) | lo );
              s = &s[ 3 ];
            }
            else {
              c = '%'; s++;
            }
            break;

          case '+':
            if ( partNames[ i ] == QUERY )
              c = ' ';
            else
              c = '+';
            s++;
            break;

          case '&':
            switch ( &q->part[ q->len ] - s ) {
              default: /* &apos; &quot; */
                if ( s[ 5 ] == ';' ) {
                  if ( isLowerChar( s[ 1 ], 'a' ) &&
                       isLowerChar( s[ 2 ], 'p' ) &&
                       isLowerChar( s[ 3 ], 'o' ) &&
                       isLowerChar( s[ 4 ], 's' ) ) {
                    c = '\''; s = &s[ 6 ];
                    break;
                  }
                  if ( isLowerChar( s[ 1 ], 'q' ) &&
                       isLowerChar( s[ 2 ], 'u' ) &&
                       isLowerChar( s[ 3 ], 'o' ) &&
                       isLowerChar( s[ 4 ], 't' ) ) {
                    c = '"'; s = &s[ 6 ];
                    break;
                  }
                }
              case 5: /* &amp; */
                if ( s[ 4 ] == ';' &&
                     isLowerChar( s[ 1 ], 'a' ) &&
                     isLowerChar( s[ 2 ], 'm' ) &&
                     isLowerChar( s[ 3 ], 'p' ) ) {
                  c = '&'; s = &s[ 5 ];
                  break;
                }
              case 4: /* &lt; &gt; */
                if ( s[ 3 ] == ';' ) {
                  if ( isLowerChar( s[ 1 ], 'l' ) &&
                       isLowerChar( s[ 2 ], 't' ) ) {
                    c = '<'; s = &s[ 4 ];
                    break;
                  }
                  if ( isLowerChar( s[ 1 ], 'g' ) &&
                       isLowerChar( s[ 2 ], 't' ) ) {
                    c = '>'; s = &s[ 4 ];
                    break;
                  }
                }
              case 3: case 2: case 1:
                c = '&'; s++;
                break;
            }
            break;

          default:
            c = *s++;
            break;
        }
      }
      *t = '\0';
      q->len = t - q->part;
    }
  }
}


char *
URIParser::addRelated( UriPartName name,  char *part )
{
  UriPart    * p;
  unsigned int len;

  len = ::strlen( part );
  p = this->parts.newPart( sizeof( UriPart ) + len + 1 );

  p->part = (char *) &p[ 1 ];
  p->name = name;
  p->len  = len;
  ::memcpy( p->part, part, len );
  p->part[ len ] = '\0';
  this->parts.insert( p );

  return p->part;
}


void
URIParser::constructPath( const char *abs,  const char *rel )
{
  UriPart    * p,
               q,
             * old;
  unsigned int absLen,
               relOff,
               relLen,
               pos;

  absLen = ::strlen( abs );
  relLen = ::strlen( rel );

  p = this->parts.newPart( sizeof( UriPart ) + absLen + relLen + 2 );

  p->part = (char *) &p[ 1 ];
  p->name = ABS_PATH;

  ::strcpy( p->part, abs );

  if ( relLen > 0 ) {
    while ( absLen > 0 && abs[ absLen - 1 ] != '/' ) 
      absLen--;

    for ( relOff = 0; relOff < relLen; ) {
      if ( rel[ relOff ] == '.' && rel[ relOff + 1 ] == '/' ) {
        relOff += 2;
        while ( rel[ relOff ] == '/' )
          relOff++;
      }
      else if ( rel[ relOff ] == '.' && rel[ relOff + 1 ] == '.' &&
                rel[ relOff + 2 ] == '/' ) {
        if ( absLen <= 1 )
          break;
        do {
          absLen--;
        } while ( absLen > 0 && abs[ absLen - 1 ] != '/' );
        relOff += 3;
        while ( rel[ relOff ] == '/' )
          relOff++;
      }
      else {
        break;
      }
    }

    ::strcpy( &p->part[ absLen ], &rel[ relOff ] );

    p->part[ absLen + relLen ] = '\0';
  }
  p->len = absLen + relLen;

  if ( this->parts.find( p, &pos ) ) {
    old = this->parts.put( pos, p );
    if ( old != NULL )
      this->parts.freePart( old );
  }
  else {
    this->parts.insert( p );
  }

  q.name = REL_PATH;
  if ( this->parts.find( &q, &pos ) ) {
    old = this->parts.remove( pos );
    this->parts.freePart( old );
  }
}


char *
URIParser::getUri( bool encoded )
{
  char * uri;

  if ( encoded ) {
    if ( (uri = this->getPart( URI_ALL_ENCODED )) == NULL )
      uri = this->makeUri( true, false );
  }
  else {
    if ( (uri = this->getPart( URI_ALL )) == NULL )
      uri = this->makeUri( false, false );
  }

  return uri;
}


char *
URIParser::getBase( void )
{
  char * base;

  if ( (base = this->getPart( URI_BASE )) == NULL )
    base = this->makeUri( false, true );

  return base;
}


char *
URIParser::makeUri( bool encoded,  bool baseonly )
{
  const char * sch = NULL,
             * opa = NULL,
             * ui  = NULL,
             * ho  = NULL,
             * po  = NULL,
             * pa  = NULL,
             * qy  = NULL,
             * r   = NULL;
  UriPart    * q;
  unsigned int len,
               schl = 0,
               opal = 0,
               uil  = 0,
               hol  = 0,
               pol  = 0,
               pal  = 0,
               qyl  = 0,
               rl   = 0;

  /* calc length of uri string */
  len = ( (sch = this->getScheme()) == NULL ) ? 0 : (schl = ::strlen( sch ));

  if ( (opa = this->getOpaque( encoded )) != NULL )
    len += (opal = ::strlen( opa ));
  else {
    len += ( (ui = this->getUserinfo()) == NULL ) ? 0 : (uil = ::strlen( ui ));
    len += ( (ho = this->getHost()) == NULL ) ? 0 : (hol = ::strlen( ho ));
    len += ( (po = this->getPort()) == NULL ) ? 0 : (pol = ::strlen( po ));
    if ( ! baseonly ) {
      len += ( (pa = this->getPath( encoded )) == NULL ) ? 0 :
               (pal = ::strlen( pa ));
      len += ( (qy = this->getQuery( encoded )) == NULL ) ? 0 :
               (qyl = ::strlen( qy ));
      len += ( (r = this->getReference( encoded )) == NULL ) ? 0 :
               (rl = ::strlen( r ));
    }
  }

  q = this->parts.newPart( sizeof( UriPart ) + len + 12 );
  q->part = (char *) &q[ 1 ];
  if ( encoded )
    q->name = URI_ALL_ENCODED;
  else if ( ! baseonly )
    q->name = URI_ALL;
  else
    q->name = URI_BASE;
  q->len = 0;

  try {
    /* copy parsed parts into uri string */
    if ( sch != NULL ) {
      ::memcpy( q->part, sch, schl );
      len = schl;
      q->part[ len++ ] = ':';
    }
    else {
      len = 0;
    }

    if ( opa != NULL ) {
      ::memcpy( &q->part[ len ], opa, opal );
      len += opal;
    }
    else {
      /* if has host, must have net path part -- xxx://net */
      if ( ho != NULL ) {
        q->part[ len++ ] = '/';
        q->part[ len++ ] = '/';

        if ( ui != NULL ) {
          ::memcpy( &q->part[ len ], ui, uil );
          len += uil;
          q->part[ len++ ] = '@';
        }

        ::memcpy( &q->part[ len ], ho, hol );
        len += hol;

        if ( po != NULL ) {
          q->part[ len++ ] = ':';
          ::memcpy( &q->part[ len ], po, pol );
          len += pol;
        }
      }

      if ( ! baseonly ) {
        /* add path, query, refererence */
        if ( pa != NULL ) {
          ::memcpy( &q->part[ len ], pa, pal );
          len += pal;
        }
        if ( qy != NULL ) {
          q->part[ len++ ] = '?';
          ::memcpy( &q->part[ len ], qy, qyl );
          len += qyl;
        }
        if ( r != NULL ) {
          q->part[ len++ ] = '#';
          ::memcpy( &q->part[ len ], r, rl );
          len += rl;
        }
      }
    }

    q->part[ len ] = '\0';
    q->len = len;

    this->parts.insert( q );
    return q->part;

  } catch ( ... ) {
    this->parts.freePart( q );
    throw;
  }
}


char *
URIParser::getOpaque( bool encoded )
{
  if ( encoded )
    return this->getEncodedPart( OPAQUE, OPAQUE_ENCODED );
  return this->getPart( OPAQUE );
}


char *
URIParser::getScheme( void )
{
  return this->getPart( SCHEME );
}


char *
URIParser::getUserinfo( void )
{
  return this->getPart( USERINFO );
}


char *
URIParser::getHost( void )
{
  char * host,
       * opaque;

  host = this->getPart( HOST );
  if ( this->isWild && host == NULL ) {
    if ( (opaque = this->getPart( OPAQUE )) != NULL )
      if ( ::strcmp( opaque, "*" ) == 0 )
        return opaque;
  }
  return host;
}


char *
URIParser::getPort( void )
{
  return this->getPart( PORT );
}


char *
URIParser::getHostPort( unsigned int defaultPort )
{
  unsigned int i,
               len;
  char       * host,
             * port,
             * scheme,
             * hostPort,
               portBuf[ 8 ],
               buf[ 8 ],
             * ptr;

  if ( (host = this->getPart( HOST_PORT )) != NULL )
    return host;

  if ( (host = this->getHost()) == NULL )
    return NULL;
  port = this->getPort();

  if ( port == NULL ) {
    if ( defaultPort == 0 ) {
      if ( (scheme = this->getScheme()) != NULL ) {
        ::strncpy( buf, scheme, sizeof( buf ) );
        for ( len = 0; len < sizeof( buf ) && buf[ len ] != '\0'; len++ ) {
          if ( buf[ len ] >= 'A' && buf[ len ] <= 'Z' )
            buf[ len ] += 'a' - 'A';
        }
        if ( len == 4 && ::strncmp( buf, "http", 4 ) == 0 )
          defaultPort = 80;
        else if ( len == 5 && ::strncmp( buf, "https", 5 ) == 0 )
          defaultPort = 443;
        else if ( len == 3 && ::strncmp( buf, "ftp", 3 ) == 0 )
          defaultPort = 21;
        else if ( len == 4 && ( ::strncmp( buf, "nntp", 4 ) == 0 ||
                                ::strncmp( buf, "news", 4 ) == 0 ) )
          defaultPort = 119;
        else if ( len == 5 && ::strncmp( buf, "snews", 5 ) == 0 )
          defaultPort = 563;
      }
    }
    ptr = portBuf;
    for ( i = 10000; i > 0; i /= 10 ) {
      if ( defaultPort >= i )
        *ptr++ = (char) ( ( defaultPort % ( i * 10 ) ) / i ) + '0';
    }
    *ptr = '\0';
    port = portBuf;
  }

  len = ::strlen( host );
  char tmpBuf[ 80 ]; ptr = tmpBuf;
  unsigned int len2 = len + ::strlen( port ) + 2;

  if ( len2 > sizeof( tmpBuf ) )
    MALLOC( len2, &ptr );
  ::strcpy( ptr, host );
  ptr[ len ] = ':';
  ::strcpy( &ptr[ len + 1 ], port );

  try {
    hostPort = this->addRelated( HOST_PORT, ptr );
  } catch( Error ) {
    if ( ptr != tmpBuf )
      FREE( ptr );
    throw;
  }
  if ( ptr != tmpBuf )
    FREE( ptr );

  return hostPort;
}


char *
URIParser::getPath( bool encoded )
{
  char * path,
       * opaque;

  if ( encoded ) {
    if ( (path = this->getEncodedPart( REL_PATH, REL_PATH_ENCODED )) != NULL )
      return path;
  }
  else {
    if ( (path = this->getPart( REL_PATH )) != NULL )
      return path;
  }

  if ( encoded ) {
    path = this->getEncodedPart( ABS_PATH, ABS_PATH_ENCODED );
  }
  else {
    path = this->getPart( ABS_PATH );

    if ( this->isWild && path == NULL ) {
      if ( (opaque = this->getPart( OPAQUE )) != NULL )
        if ( opaque[ 0 ] == '*' )
          return opaque;
    }
  }

  return path;
}


char *
URIParser::getQuery( bool encoded )
{
  if ( encoded )
    return this->getEncodedPart( QUERY, QUERY_ENCODED );
  return this->getPart( QUERY );
}


char *
URIParser::getPathQuery( bool encoded )
{
  unsigned int len;
  char       * path,
             * query,
             * ptr;

  if ( (path = this->getPart( encoded ? PATH_QUERY_ENCODED :
                                        PATH_QUERY )) != NULL )
    return path;

  path  = this->getPath( encoded );
  query = this->getQuery( encoded );

  if ( path == NULL )
    throw URIErr::getErr( URIErr::HAS_NO_PATH );
  if ( query == NULL )
    return path;

  len = ::strlen( path );
  unsigned int len2 = len + ::strlen( query ) + 2;
  char tmpBuf[ 256 ]; ptr = tmpBuf;
  if ( len2 > sizeof( tmpBuf ) )
    MALLOC( len2, &ptr );
  ::strcpy( ptr, path );
  ptr[ len ] = '?';
  ::strcpy( &ptr[ len + 1 ], query );

  try {
    path = this->addRelated( encoded ? PATH_QUERY_ENCODED :
                                       PATH_QUERY, ptr );
  } catch( Error ) {
    if ( ptr != tmpBuf )
      FREE( ptr );
    throw;
  }
  if ( ptr != tmpBuf )
    FREE( ptr );

  return path;
}


void
URIParser::stripReference( void )
{
  UriPart      p,
             * old;
  unsigned int pos;

  p.name = REFERENCE;
  if ( ! this->parts.find( &p, &pos ) )
    return;
  old = this->parts.remove( pos );
  this->parts.freePart( old );

  /* recreate these upon request to getUri() */
  p.name = URI_ALL;
  if ( this->parts.find( &p, &pos ) ) {
    old = this->parts.remove( pos );
    this->parts.freePart( old );
  }

  p.name = URI_ALL_ENCODED;
  if ( this->parts.find( &p, &pos ) ) {
    old = this->parts.remove( pos );
    this->parts.freePart( old );
  }
}


char *
URIParser::getReference( bool encoded )
{
  if ( ! encoded )
    return this->getPart( REFERENCE );
  return this->getEncodedPart( REFERENCE, REFERENCE_ENCODED );
}


char *
URIParser::getPart( UriPartName name )
{
  UriPart      p;
  unsigned int pos;

  p.name = name;
  if ( this->parts.find( &p, &pos ) )
    return this->parts.get( pos )->part;

  return NULL;
}


char *
URIParser::getEncodedPart( UriPartName name,  UriPartName encodedName )

{
  UriPart      p,
             * q;
  const char * s;
  unsigned int pos,
               len;
  bool         isQuery;

  p.name = encodedName;
  if ( this->parts.find( &p, &pos ) )
    return this->parts.get( pos )->part;

  if ( encodedName == QUERY_ENCODED )
    isQuery = true;
  else
    isQuery = false;
  p.name = name;
  if ( this->parts.find( &p, &pos ) ) {

    q   = this->parts.get( pos );
    len = this->escapeStringLen( q->part, isQuery );
    s   = q->part;

    q = this->parts.newPart( sizeof( UriPart ) + len + 1 );
    q->name = encodedName;
    q->len  = len;
    q->part = (char *) &q[ 1 ];

    this->escapeStringCopy( q->part, s, isQuery );
    this->parts.insert( q );
    return q->part;
  }

  return NULL;
}


static char okCharSet[] = ";/:@=+$,-_.!~*()";

unsigned int
URI::escapeStringLen( const char *s,  bool ampOk )
{
  unsigned int i,
               j;

  for ( i = 0, j = 0; s[ i ] != '\0'; i++ ) {
    if ( URIParser::isAlpha( s[ i ] ) || URIParser::isDigit( s[ i ] ) ||
         URIParser::isMember( okCharSet, sizeof( okCharSet ) - 1, s[ i ] ) ||
         ( s[ i ] == '&' && ampOk ) ||
         ( s[ i ] == ' ' && ampOk ) )
      j++;
    else
      j += 3;
  }
  return j;
}


char *
URI::escapeStringCopy( char *buf,  const char *s,  bool ampOk )
{
  unsigned int i,
               j;

  for ( i = 0, j = 0; s[ i ] != '\0'; i++ ) {
    if ( URIParser::isAlpha( s[ i ] ) || URIParser::isDigit( s[ i ] ) ||
         URIParser::isMember( okCharSet, sizeof( okCharSet ) - 1, s[ i ] ) ||
         ( s[ i ] == '&' && ampOk ) )
      buf[ j++ ] = s[ i ];
    else if ( s[ i ] == ' ' && ampOk )
      buf[ j++ ] = '+';
    else {
      static char hexChar[] = "0123456789ABCDEF";
      buf[ j ]     = '%';
      buf[ j + 1 ] = hexChar[ (unsigned int) (byte) s[ i ] >> 4 ];
      buf[ j + 2 ] = hexChar[ (unsigned int) (byte) s[ i ] & 0xfU ];
      j += 3;
    }
  }
  buf[ j ] = '\0';

  return buf;
}


bool
URIParser::scan( UriPartName name,
                 bool ( URIParser::*scanFunc )( const char ** ),
                 const char **ptr )
{
  UriPart    * p;
  const char * start;
  unsigned int len;

  start = *ptr;
  if ( ( this->*scanFunc )( ptr ) ) {
    len  = *ptr - start;
    try {
      p = this->parts.newPart( sizeof( UriPart ) + len + 1 );
    } catch( Error e ) {
      this->err = e;
      return false;
    }
    p->part = (char *) &p[ 1 ];
    p->name = name;
    p->len  = len;
    ::memcpy( p->part, start, len );
    p->part[ len ] = '\0';
    this->parts.insert( p );
    return true;
  }
  return false;
}


/* <absolute_uri>    -> <scheme> ":" ( <heir_part> | <opaque_part> ) */
bool
URIParser::scanAbsoluteUri( const char **ptr )
{
  const char * s;

  s = *ptr;
  if ( this->scan( SCHEME, &URIParser::scanScheme, &s ) ) {
    s++;
    if ( this->scanHierPart( &s ) ||
         this->scan( OPAQUE, &URIParser::scanOpaquePart, &s ) ) {
      *ptr = s;
      return true;
    }
  }
  return false;
}


/* <scheme>          -> alpha ( alnum | [\+\-\.] )* */
bool
URIParser::scanScheme( const char **ptr )
{
  static char valid[] = "+-.";
  const char * s;

  s = *ptr;
  if ( this->isWild && ( s[ 0 ] == '*' && s[ 1 ] == ':' ) ) {
    s++;
    *ptr = s;
    return true;
  }
  else if ( this->isAlpha( *s ) ) {
    s++;
#if defined( _WIN32 ) || defined( _WIN64 )
    /* check for windows C:/path or C:\path */
    if ( *s == ':' ) {
      if ( ( s[ 1 ] == '/' && s[ 2 ] != '/' ) ||
           ( s[ 1 ] == '\\' ) )
        return false;
    }
#endif
    while ( this->isAlnum( *s ) ||
            this->isMember( valid, sizeof( valid ) - 1, *s ) )
      s++;
    if ( *s == ':' ) {
      *ptr = s;
      return true;
    }
  }
  return false;
}


/* <heir_part>       -> ( <net_path> | <abs_path> ) ( "?" query )? */
bool
URIParser::scanHierPart( const char **ptr )
{
  const char * s;

  s = *ptr;
  if ( this->scanNetPath( &s ) ||
       this->scan( ABS_PATH, &URIParser::scanAbsPath, &s ) ) {
    if ( *s == '?' ) {
      s++;
      this->scan( QUERY, &URIParser::scanQuery, &s );
    }
    if ( *s == '#' ) {
      s++;
      this->scan( REFERENCE, &URIParser::scanFragment, &s );
    }
    *ptr = s;
    return true;
  }
  return false;
}


/* <opaque_part>     -> <uri_char_no_slash> <uri_char>* */
bool
URIParser::scanOpaquePart( const char **ptr )
{
  const char * s;

  s = *ptr;
  if ( *s != '/' && this->scanUriChar( &s ) ) {
    while ( this->scanUriChar( &s ) )
      ;
    *ptr = s;
    return true;
  }
  return false;
}


/* <net_path>        -> "//" <authority> ( <abs_path> )? */
bool
URIParser::scanNetPath( const char **ptr )
{
  const char * s;

  s = *ptr;
  if ( s[ 0 ] == '/' && s[ 1 ] == '/' ) {
    s = &s[ 2 ];
    if ( this->scanAuthority( &s ) ) {
      this->scan( ABS_PATH, &URIParser::scanAbsPath, &s );
      *ptr = s;
      return true;
    }
  }
  return false;
}


/* <abs_path>        -> "/" <path_segments> */
bool
URIParser::scanAbsPath( const char **ptr )
{
  const char * s;

  s = *ptr;
  if ( *s == '/' ) {
    s++;
    this->scanPathSegments( &s );
    *ptr = s;
    return true;
  }
  /* windows c:/ absolute style */
  else if ( this->isAlpha( *s ) && s[ 1 ] == ':' && s[ 2 ] == '/' ) {
    s += 3;
    this->scanPathSegments( &s );
    *ptr = s;
    return true;
  }
  return false;
}


/* <authority>       -> <server> | <reg_name> */
bool
URIParser::scanAuthority( const char **ptr )
{
  const char * s;

  s = *ptr;
  if ( this->scanServer( &s ) ||
       this->scan( REG_NAME, &URIParser::scanRegName, &s ) ) {
    *ptr = s;
    return true;
  }
  return false;
}


/* <server>          -> [ <userinfo> "@" ] <hostport> */
bool
URIParser::scanServer( const char **ptr )
{
  const char * s;

  s = *ptr;
  if ( this->scan( USERINFO, &URIParser::scanUserinfo, &s ) ) {
    s++;
    if ( this->scanHostport( &s ) ) {
      *ptr = s;
      return true;
    }
  }
  s = *ptr;
  if ( this->scanHostport( &s ) ) {
    *ptr = s;
    return true;
  }
  return false;
}


/* <reg_name>      -> ( <unreserved_char> | <escaped> | [\$\,\;\:\@\&\=\+] )+ */
bool
URIParser::scanRegName( const char **ptr )
{
  static char valid[] = "$,;:@&=+";
  const char * s;

  s = *ptr;
  for (;;) {
    if ( this->isUnreservedChar( *s ) )
      s++;
    else if ( this->isEscaped( s ) )
      s = &s[ 3 ];
    else if ( this->isMember( valid, sizeof( valid ) - 1, *s ) )
      s++;
    else
      break;
  }
  if ( s > *ptr ) {
    *ptr = s;
    return true;
  }
  return false;
}


/* <userinfo>    -> ( <unreserved_char> | <escaped> | [\;\:\&\=\+\$\,] )* '@' */
bool
URIParser::scanUserinfo( const char **ptr )
{
  static char valid[] = ";:&=+$,";
  const char * s;

  s = *ptr;

  for (;;) {
    if ( this->isUnreservedChar( *s ) )
      s++;
    else if ( this->isEscaped( s ) )
      s = &s[ 3 ];
    else if ( this->isMember( valid, sizeof( valid ) - 1, *s ) )
      s++;
    else
      break;
  }
  if ( *s == '@' ) {
    *ptr = s;
    return true;
  }

  return false;
}


/* <hostport>        -> <host> ( ":" <port> )? */
bool
URIParser::scanHostport( const char **ptr )
{
  const char * s;

  s = *ptr;

  if ( this->scan( HOST, &URIParser::scanHost, &s ) ) {
    if ( *s == ':' ) {
      s++;
      if ( ! this->scan( PORT, &URIParser::scanPort, &s ) )
        return false;
    }
    *ptr = s;
    return true;
  }
  return false;
}


/* <host>            -> <hostname> | <IPv4address> */
bool
URIParser::scanHost( const char **ptr )
{
  const char * s;

  s = *ptr;
  if ( this->scanHostname( &s ) ||
       this->scanIPv4address( &s ) ) {
    *ptr = s;
    return true;
  }
  return false;
}


/* <hostname>        -> ( <domainlabel> "." )* <toplabel> "."? */
bool
URIParser::scanHostname( const char **ptr )
{
  const char * s,
             * t,
             * matched;

  matched = NULL;
  s = *ptr;
  if ( this->scanToplabel( &s ) ) {
    if ( *s == '.' )
      s++;
    matched = s;
  }

  s = *ptr;
  while ( this->scanDomainlabel( &s ) && *s == '.' ) {
    s++;
    t = s;
    if ( this->scanToplabel( &t ) ) {
      if ( *t == '.' )
        t++;
      matched = t;
    }
  }

  if ( matched != NULL ) {
    *ptr = matched;
    return true;
  }
  return false;
}


/* <domainlabel>     -> alnum | ( alnum ( alnum | "-" )* alnum ) */
bool
URIParser::scanDomainlabel( const char **ptr )
{
  const char * s,
             * t;

  s = *ptr;
  for (;;) {
    if ( this->isAlnum( *s ) || ( this->isWild && *s == '*' ) )
      s++;
    else if ( s > *ptr && *s == '-' ) {
      t = s + 1;
      while ( *t == '-' )
        t++;
      if ( this->isAlnum( *t ) ) {
        s = t + 1;
      }
      else
        break;
    }
    else
      break;
  }
  if ( s > *ptr ) {
    *ptr = s;
    return true;
  }
  return false;
}


/* <toplabel>        -> alpha | ( alpha ( alnum | "-" )* alnum ) */
bool
URIParser::scanToplabel( const char **ptr )
{
  const char * s;

  s = *ptr;
  if ( this->isAlpha( *s ) || ( this->isWild && *s == '*' ) ) {
    s++;
    this->scanDomainlabel( &s );
    *ptr = s;
    return true;
  }
  return false;
}


/* <IPv4address>     -> digit+ "." digit+ "." digit+ "." digit+ */
bool
URIParser::scanIPv4address( const char **ptr )
{
  const char * s;
  unsigned int count;

  s = *ptr;
  /* match quad */
  for ( count = 0;; ) {
    if ( this->isWild && *s == '*' )
      s++;
    else {
      if ( ! this->isDigit( *s ) )
        return false;
      s++;
      if ( this->isDigit( *s ) ) {
        s++;
        if ( this->isDigit( *s ) )
          s++;
      }
    }
    if ( ++count == 4 ) {
      *ptr = s;
      return true;
    }
    if ( *s != '.' )
      return false;
    s++;
  }
}


/* <port>            -> digit+ */
bool
URIParser::scanPort( const char **ptr )
{
  const char * s;

  s = *ptr;
  if ( this->isDigit( *s ) ) {
    while ( this->isDigit( *s ) )
      s++;
    *ptr = s;
    return true;
  }
  return false;
}


/* <path_segments>   -> <segment> ( "/" <segment> )* */
bool
URIParser::scanPathSegments( const char **ptr )
{
  const char * s,
             * t;

  s = *ptr;
  if ( this->scanSegment( &s ) ) {
    while ( *s == '/' ) {
      t = s + 1;
      if ( this->scanSegment( &t ) )
        s = t;
      else
        break;
    }
    *ptr = s;
    return true;
  }
  return false;
}


/* <segment>         -> <pchar>* ( ";" <pchar>* )* */
bool
URIParser::scanSegment( const char **ptr )
{
  const char * s;

  s = *ptr;
  for (;;) {
    if ( ! this->scanPchar( &s ) ) {
      if ( *s == ';' )
        s++;
      else
        break;
    }
  }
  *ptr = s;
  return true;
}


/* <pchar>           -> <unreserved_char> | <escaped_char> | [\:\@\&\=\+\$\,] */
bool
URIParser::scanPchar( const char **ptr )
{
  static char valid[] = ":@&=+$,";
  const char * s;

  s = *ptr;
  if ( this->isUnreservedChar( *s ) )
    s++;
  else if ( this->isEscaped( s ) )
    s = &s[ 3 ];
  else if ( this->isMember( valid, sizeof( valid ) - 1, *s ) )
    s++;
  else
    return false;
  *ptr = s;
  return true;
}


/* <query>           -> <uri_char>* */
bool
URIParser::scanQuery( const char **ptr )
{
  const char * s;

  s = *ptr;
  while ( this->scanUriChar( &s ) )
    ;
  *ptr = s;
  return true;
}


/* <uri_reference>   -> ( <absolute_uri> | <relative_uri> )? ( "#" <fragment> )? */
bool
URIParser::scanUriReference( const char **ptr )
{
  const char * s;

  s = *ptr;
  if ( this->scanAbsoluteUri( &s ) ||
       this->scanRelativeUri( &s ) ) {
    *ptr = s;
    return true;
  }
  return false;
}


/* <fragment>        -> <uri_char>* */
bool
URIParser::scanFragment( const char **ptr )
{
  const char * s;

  s = *ptr;
  while ( this->scanUriChar( &s ) )
    ;
  *ptr = s;
  return true;
}


/* <relative_uri>    -> ( <net_path> | <abs_path> | <rel_path> ) ( "?" <query> ) */
bool
URIParser::scanRelativeUri( const char **ptr )
{
  const char * s,
             * t;

  s = *ptr;
  if ( ! this->scanNetPath( &s ) ) {
    if ( ! this->scan( ABS_PATH, &URIParser::scanAbsPath, &s ) )
      this->scan( REL_PATH, &URIParser::scanRelPath, &s );
  }
  if ( *s == '?' ) {
    t = s + 1;
    if ( this->scan( QUERY, &URIParser::scanQuery, &t ) )
      s = t;
  }
  if ( *s == '#' ) {
    t = s + 1;
    if ( this->scan( REFERENCE, &URIParser::scanFragment, &t ) )
      s = t;
  }
  *ptr = s;
  return true;
}


/* <rel_path>        -> <rel_segment> ( <abs_path> )? */
bool
URIParser::scanRelPath( const char **ptr )
{
  const char * s;

  s = *ptr;
  if ( this->scanRelSegment( &s ) ) {
    if ( *s == '/' ) {
      s++;
      this->scanPathSegments( &s );
    }
    *ptr = s;
    return true;
  }
  return false;
}


/* <rel_segment>     -> ( <unreserved_char> | <escaped> | [\;\@\&\=\+\$\,] )+ */
bool
URIParser::scanRelSegment( const char **ptr )
{
  static char valid[] = ";@&=+$,";
  const char * s;

  s = *ptr;
  for (;;) {
    if ( this->isUnreservedChar( *s ) )
      s++;
    else if ( this->isEscaped( s ) )
      s = &s[ 3 ];
    else if ( this->isMember( valid, sizeof( valid ) - 1, *s ) )
      s++;
    else
      break;
  }
  if ( s > *ptr ) {
    *ptr = s;
    return true;
  }
  return false;
}


/* <uri_char>        -> <reserved_char> | <unreserved_char> | <escaped> */
bool
URIParser::scanUriChar( const char **ptr )
{
  const char * s;

  s = *ptr;
  if ( this->isUnreservedChar( *s ) )
    s++;
  else if ( this->isEscaped( s ) )
    s = &s[ 3 ];
  else if ( this->isReservedChar( *s ) )
    s++;
  else
    return false;
  *ptr = s;
  return true;
}

