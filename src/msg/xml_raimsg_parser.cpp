#include <string.h>
#include <ctype.h>
#if defined( USE_ICONV_APPEND )
#include <iconv.h>
#endif
#include "base/log.h"
#include "msg/rai_msg.h"
#include "msg/subject.h"
#include "doc/sax_parser.h"
#include "util/str_util.h"
#include "msg/xml_raimsg_parser.h"

using namespace rai;

struct XmlRaiMsgParser : public SaxParser {
  RaiMsg     & msg;
  RaiSubject & subj;
  unsigned int el,
               maxEl,
               off[ 32 ];
  byte       * fdata[ 32 ];
  unsigned int fdataSize[ 32 ],
               ftype[ 32 ],
               fsize[ 32 ],
               mangle[ 32 ],
               base64[ 32 ];
#if defined( USE_ICONV_APPEND )
  iconv_t      iconv;
  byte       * fdata2;
  unsigned int fdata2Size;
#endif

  XmlRaiMsgParser( RaiMsg &m,  RaiSubject &s ) : msg( m ), subj( s ) {
    this->el         = 0;
    this->maxEl      = 0;
#if defined( USE_ICONV_APPEND )
    this->iconv      = (iconv_t) -1;
    this->fdata2     = NULL;
    this->fdata2Size = 0;
#endif
  };
  virtual ~XmlRaiMsgParser() {
    for ( unsigned int i = 0; i < this->maxEl; i++ )
      if ( this->fdata[ i ] != NULL )
        FREE( this->fdata[ i ] );
#if defined( USE_ICONV_APPEND )
    if ( this->fdata2 != NULL )
      FREE( this->fdata2 );
#endif
  };
  virtual void startDocument( const char *encoding ) throw( Error );

  virtual void endDocument( void ) throw( Error );

  virtual void startElement( const byte *name,
                             const byte **atts ) throw( Error );
#if defined( USE_ICONV_APPEND )
  bool iconvAppend( const byte *name );
#endif
  virtual void endElement( const byte *name ) throw( Error );

  void appendFdata( const char *fname,  char *str,  unsigned int ftp,
                    unsigned int fsz,  unsigned int b64 ) throw( Error );
  void appendField( const byte *ch,  int len ) throw( Error );

  virtual void characters( const byte *ch,  int len ) throw( Error );

  virtual void cdataBlock( const byte *value,  int len ) throw( Error );

  virtual void ignorableWhitespace( const byte *ch,  int len ) throw( Error );

  virtual void warning( const char *msg,  va_list ap ) throw( Error );

  virtual void error( const char *msg,  va_list ap ) throw( Error );

  virtual void fatalError( const char *msg,  va_list ap ) throw( Error );

  bool parse( byte *data,  unsigned int dataLen ) throw( Error );
};


bool
XMLRaiMsgParser::parse( RaiMsg &msg,  RaiSubject &subj,
                        byte *data,  unsigned int dataLen ) throw( Error )
{
  XmlRaiMsgParser p( msg, subj );

  return p.parse( data, dataLen );
}


void
XmlRaiMsgParser::startDocument( const char *encoding ) throw( Error )
{
#if defined( USE_ICONV_APPEND )
  if ( encoding != NULL &&
       StrUtil::strcasecmp( encoding, "UTF8" ) != 0 &&
       StrUtil::strcasecmp( encoding, "UTF-8" ) != 0 ) {
    this->iconv = ::iconv_open( encoding, "UTF-8" );
  }
#endif
}


void
XmlRaiMsgParser::endDocument( void ) throw( Error )
{
#if defined( USE_ICONV_APPEND )
  if ( this->iconv != (iconv_t) -1 ) {
    ::iconv_close( this->iconv );
    this->iconv = (iconv_t) -1;
  }
#endif
}


void
XmlRaiMsgParser::startElement( const byte *name,  const byte **atts )
                 throw( Error )
{
  if ( ::strcmp( (char *) name, "MSG" ) == 0 ) {
    if ( atts != NULL ) {
      unsigned int i;
      for ( i = 0; atts[ i ] != NULL; i += 2 ) {
        if ( StrUtil::strncasecmp( (char *) atts[ i ], "subj", 4 ) == 0 )
          this->subj.encode( (char *) atts[ i + 1 ] );
        else if ( StrUtil::strcasecmp( (char *) atts[ i ], "type" ) == 0 )
          this->msg.SetProtocol(
            RaiMsg::StringToProto( (char *) atts[ i + 1 ] ) );
      }
    }
    this->el = 0;
  }
  else if ( StrUtil::strcasecmp( (char *) name, "base64" ) == 0 ) {
    if ( atts != NULL && this->el > 0 ) {
      for ( unsigned int j = 0; atts[ j ] != NULL; j += 2 ) {
        if ( atts[ j + 1 ] == NULL )
          break;
        if ( StrUtil::strncasecmp( (char *) atts[ j ], "len", 3 ) == 0 )
          this->base64[ this->el - 1 ] = atoi( (char *) atts[ j + 1 ] );
      }
    }
  }
  else {
    const unsigned int i = this->el++;
    if ( this->el < sizeof( this->off ) / sizeof( this->off[ 0 ] ) ) {
      this->off[ i ]       = 0;
      this->fdataSize[ i ] = 0;
      this->fsize[ i ]     = 0;
      this->ftype[ i ]     = 0;
      this->mangle[ i ]    = 0;
      this->base64[ i ]    = 0;
      if ( i == this->maxEl ) {
        this->fdata[ i ] = NULL;
        this->maxEl++;
      }
      if ( atts != NULL ) {
        for ( unsigned int j = 0; atts[ j ] != NULL; j += 2 ) {
          if ( atts[ j + 1 ] == NULL )
            break;
          if ( StrUtil::strncasecmp( (char *) atts[ j ], "typ", 3 ) == 0 ) {
            this->ftype[ i ] = RaiMsg::StrType( (char *) atts[ j+1 ] );
            if ( this->ftype[ i ] == (unsigned int) RAIMSG_MESSAGE ) {
              try {
                this->msg.Append( (char *) name, (RaiMsg *) NULL );
                this->msg.Activate( "." );
              } catch ( Error e ) {
                logError( LERROR, e, "Field name %s", (char *) name );
                throw e;
              }
            }
          }
          else if ( StrUtil::strncasecmp( (char *) atts[ j ], "siz", 3 ) == 0 )
            this->fsize[ i ] = atoi( (char *) atts[ j + 1 ] );
          else if ( StrUtil::strncasecmp( (char *) atts[ j ], "nam", 3 ) == 0 )
            this->mangle[ i ] = atoi( (char *) atts[ j + 1 ] );
        }
      }
    }
  }
}


#if defined( USE_ICONV_APPEND )
bool
XmlRaiMsgParser::iconvAppend( const byte *name )
{
  if ( this->iconv == (iconv_t) -1 )
    return false;
  if ( this->fdata2Size < this->off * 5 ) {
    REALLOC( this->off * 5 + 1, &this->fdata2 );
    this->fdata2Size = this->off * 5 + 1;
  }
  size_t inLen  = (size_t) this->off + 1,
         outLen = (size_t) this->fdata2Size,
         ret;
  char * in     = (char *) this->fdata,
       * out    = (char *) this->fdata2;
#ifdef __sun__
  ret = ::iconv( this->iconv, (const char **) &in, &inLen, &out, &outLen);
#else
  ret = ::iconv( this->iconv, &in, &inLen, &out, &outLen );
#endif
  if ( ret != (size_t) -1 ) {
    if ( inLen == 0 ) {
      this->appendFdata( (const char *) name, (char *) this->fdata2 );
      return true;
    }
  }
  return false;
}
#endif


void
XmlRaiMsgParser::endElement( const byte *name ) throw( Error )
{
  if ( StrUtil::strcasecmp( (char *) name, "base64" ) == 0 )
    return;
  if ( this->el > 0 ) {
    char * fname = NULL;
    try {
      const unsigned int i = --this->el;
      if ( i < sizeof( this->off ) / sizeof( this->off[ 0 ] ) ) {
        if ( this->ftype[ i ] == (unsigned int) RAIMSG_MESSAGE ) {
          this->msg.Activate( NULL );
        }
        else {
          if ( this->mangle[ i ] == 0 ) {
            fname = (char *) name;
          }
          else {
            STRDUP( fname, (char *) name );
            if ( ( this->mangle[ i ] & 16 ) != 0 ) /* mt */
              fname[ 0 ] = '\0';
            else if ( ( this->mangle[ i ] & 1 ) != 0 ) { /* _[0-9] */
              ::memmove( fname, &fname[ 1 ], ::strlen( fname ) );
            }
            if ( ( this->mangle[ i ] & ( 2 | 4 | 8 ) ) != 0 ) {
              for ( unsigned int j = 0; fname[ j ] != '\0'; j++ ) {
                if ( ( this->mangle[ i ] & 2 ) != 0 && fname[ j ] == '-' )
                  fname[ j ] = ' ';
                else if ( ( this->mangle[ i ] & 4 ) != 0 && fname[ j ] == '_' )
                  fname[ j ] = ',';
                else if ( ( this->mangle[ i ] & 8 ) != 0 && fname[ j ] == '_' )
                  fname[ j ] = ';';
              }
            }
          }
          if ( this->off[ i ] > 0 ) {
            this->fdata[ i ][ this->off[ i ] ] = '\0';
#if defined( USE_ICONV_APPEND )
            if ( this->base64[ i ] != 0 || ! this->iconvAppend( fname ) )
#endif
            this->appendFdata( fname, (char *) this->fdata[ i ],
                               this->ftype[ i ], this->fsize[ i ],
                               this->base64[ i ] );
          }
          else {
            this->msg.Append( fname, (char *) "" );
          }
        }
      }
      if ( fname != NULL && fname != (char *) name )
        FREE( fname );
    } catch ( Error e ) {
      logError( LERROR, e, "Field name %s", (char *) name );
      if ( fname != NULL && fname != (char *) name )
        FREE( fname );
      throw e;
    }
  }
}


void
XmlRaiMsgParser::appendFdata( const char *fname,  char *str,  unsigned int ftp,
                            unsigned int fsz,  unsigned int b64 ) throw( Error )
{
  char buf[ 1024 ], *tmp = buf;
  try {
    switch ( ftp ) {
      default:
        if ( b64 > 0 ) {
          unsigned int len = ::strlen( str );
          if ( len > sizeof( buf ) * 3 / 4 )
            MALLOC( len * 3 / 4 + 3, &tmp );
          len = StrUtil::base64decode( str, len, tmp );
          while ( len < b64 )
            tmp[ len++ ] = 0;
          if ( ftp == RAIMSG_NODATA )
            ftp = RAIMSG_STRING;
          this->msg.Append( fname, (RaiMsg_type) ftp, len, (RaiMsg_data) tmp );
          if ( tmp != buf )
            FREE( tmp );
        }
        else {
          this->msg.Append( fname, str );
        }
        break;
      case RAIMSG_IPDATA:
      case RAIMSG_INT:
      case RAIMSG_UINT:
      case RAIMSG_REAL:
      case RAIMSG_BOOLEAN: {
        RaiField_data val;
        if ( fsz == 0 )
          fsz = 4;
        RaiField::Convert( (RaiMsg_type) ftp, fsz, &val, RAIMSG_STRING,
                           ::strlen( str ) + 1, str );
        this->msg.Append( fname, (RaiMsg_type) ftp, fsz, (RaiMsg_data) &val );
        break;
      }
    }
  } catch ( Error e ) {
    logError( LERROR, e, "Field name %s", (char *) fname );
    if ( tmp != buf )
      FREE( tmp );
    throw e;
  }
}


void
XmlRaiMsgParser::appendField( const byte *ch,  int len ) throw( Error )
{
  const unsigned int i = this->el - 1;
  if ( this->off[ i ] + len + 1 > this->fdataSize[ i ] ) {
    REALLOC( this->off[ i ] + len + 1, &this->fdata[ i ] );
    this->fdataSize[ i ] = this->off[ i ] + len + 1;
  }
  ::memcpy( &this->fdata[ i ][ this->off[ i ] ], ch, len );
  this->off[ i ] += len;
}


void
XmlRaiMsgParser::characters( const byte *ch,  int len ) throw( Error )
{
  if ( this->el > 0 &&
       this->el <= sizeof( this->off ) / sizeof( this->off[ 0 ] ) )
    this->appendField( ch, len );
}


void
XmlRaiMsgParser::cdataBlock( const byte *value,  int len ) throw( Error )
{
  if ( this->el > 0 &&
       this->el <= sizeof( this->off ) / sizeof( this->off[ 0 ] ) )
    this->appendField( value, len );
}


void
XmlRaiMsgParser::ignorableWhitespace( const byte *ch,  int len ) throw( Error )
{
  if ( this->el > 0 &&
       this->el <= sizeof( this->off ) / sizeof( this->off[ 0 ] ) )
    this->appendField( ch, len );
}


namespace XmlRaiMsgErr {
  enum {
    XML_WARN  = 0,
    XML_ERR   = 1,
    XML_FATAL = 2
  };
  Error getErr( unsigned int status ) {
    static const char     mod[] = "XmlRaiMsgParser";
    static const ErrorRec err[] = {
      /*  0 */ { XML_WARN, "Libxml warning", mod },
      /*  1 */ { XML_ERR,  "Libxml error", mod },
      /*  2 */ { XML_FATAL,"Libxml fatal error", mod },
      /*  3 */ { 3,        "Unknown error", mod }
    };
    static const unsigned int numErrs = sizeof( err ) / sizeof( err[ 0 ] ) - 1;

    return &err[ status < numErrs ? status : numErrs ];
  }
};


void
XmlRaiMsgParser::warning( const char *msg,  va_list ap ) throw( Error )
{
  vlogNormal( LNORMAL, XmlRaiMsgErr::getErr( XmlRaiMsgErr::XML_WARN ), msg, ap);
}


void
XmlRaiMsgParser::error( const char *msg,  va_list ap ) throw( Error )
{
  vlogNormal( LNORMAL, XmlRaiMsgErr::getErr( XmlRaiMsgErr::XML_ERR ), msg, ap);
}


void
XmlRaiMsgParser::fatalError( const char *msg,  va_list ap ) throw( Error )
{
  vlogNormal( LNORMAL, XmlRaiMsgErr::getErr( XmlRaiMsgErr::XML_FATAL), msg, ap);
}


bool
XmlRaiMsgParser::parse( byte *data,  unsigned int dataLen ) throw( Error )
{
  Error e2 = NULL;
  this->el = 0;
  this->msg.Release();
  this->subj.clear();
  this->SaxParser::initializeSax( NULL, true );
  try {
    this->SaxParser::parse( (char *) data, dataLen, true );
  } catch ( Error e ) {
    e2 = e;
  }
  this->SaxParser::terminateSax();
  if ( e2 != NULL )
    throw e2;
  if ( this->msg.PackSize() > RaiMsg::HeaderSize( msg.GetProtocol() ) )
    return true;
  return false;
}

