/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#ifndef __rai_util__uri_h__
#define __rai_util__uri_h__

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


/** 
 * URI: Represents a URI. Provides methods for accessing particular 
 * parts of the uri.If a methos is called with "encoded" true, then 
 * the part is returned in encoded form. For example, '&' is returned as
 * '%26'
 * 
 * 
 */
namespace rai {

class RAIBASE_DLL_EXP URI {
  public:
    virtual ~URI() {};
    /** Return complete URI */
    virtual char * getUri( bool encoded = false )          throw( Error ) = 0;
    /** Return opaque, as in mailto:<opaque> */
    virtual char * getOpaque( bool encoded = false )       throw( Error ) = 0;
    /** Return scheme, "http", "ftp", "telnet", "mailto", etc */
    virtual char * getScheme( void )                                      = 0;
    /** Return user:passwd as in ftp://<user:passwd>@host/ */
    virtual char * getUserinfo( void )                                    = 0;
    /** Return hostname part as in http://<host>/ */
    virtual char * getHost( void )                                        = 0;
    /** Return hostname part as in http://host:<port>/ */
    virtual char * getPort( void )                                        = 0;
    /** Return both host:port part as in http://<host:port>/ */
    virtual char * getHostPort( unsigned int defaultPort = 0 ) throw( Error )=0;
    /** Return path part as in http://host:port/<path> */
    virtual char * getPath( bool encoded = false )         throw( Error ) = 0;
    /** Return query part as in http://host:port/path?<query> */
    virtual char * getQuery( bool encoded = false )        throw( Error ) = 0;
    /** Return both path and query part as in http://host:port/<path?query> */
    virtual char * getPathQuery( bool encoded = false )    throw( Error ) = 0;
    /** Return reference part as in http://host:port/path?query#<reference> */
    virtual char * getReference( bool encoded = false )    throw( Error ) = 0;
    /** Remove reference part from URI */
    virtual void   stripReference( void )                  throw( Error ) = 0;
    /** Make a complete copy */
    virtual URI  * copy( void )                            throw( Error ) = 0;
    /** Return base: scheme + host + port (http://host:port) */
    virtual char * getBase( void )                                        = 0;

    /** Parse a URI from uri.  If related is NULL, then uri is absolute.  If
     *  related is not null, then uri can be relative.  Ex: if uri="/index.html"
     *  and related="http://www.x.net/path?query", then resulting uri will be
     *  "http://www.x.net/index.html" */
    static URI * create( const char *uri,  URI *related = NULL ) throw( Error );
    /** Use working directory as the anchor for uri */
    static URI * createWorkingDir( const char *uri )             throw( Error );
    /** Allow wildcardsfor parts of URI */
    static URI * createWild( const char *uri )                   throw( Error );
    /** Make a valid http URI from host and uri parts */
    static URI * fixupHttp( const char *host,  const char *uri ) throw( Error );
    /** Determine the escaped length of a URI after making %HH substitutions */
    static unsigned int escapeStringLen( const char *s,  bool isQuery );
    /** Copy the escaped version to buf by making %HH substitutions, if ampOk
     *  is true, then '&' can appear in escaped string, otherwise it is
     *  encoded as %26 */
    static char * escapeStringCopy( char *buf,  const char *s,  bool ampOk );
};


namespace URIErr {
  enum {
    PARSE_FAILED = 0,
    HAS_NO_PATH  = 1,
    BAD_URI      = 2
  };
  RAIBASE_DLL_EXP
  Error getErr( unsigned int status );
}
} // namespace uri

#endif
