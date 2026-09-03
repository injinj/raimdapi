/* Copyright (c) 2013 Rai Technology.  All rights reserved.
 *  *  *  http://www.raitechnology.com */
#ifndef __rai_msg__string_to_msg_h__
#define __rai_msg__string_to_msg_h__

#ifndef __rai_msg__msg_h__
#include "msg/msg.h"
#endif

#ifndef __rai_msg__field_h__
#include "msg/field.h"
#endif

#ifndef __rai_base__log_h__
#include "base/log.h"
#endif

namespace rai {

class StringToMsg {
  char       * bufPtr,
               tmpBuf[ 1024 ], /* temporary space */
             * fvalPtr,
               fvalBuf[ 1024 ],
               countBuf[ 16 ];
  unsigned int bufSize,
               fvalSize,
               countVal;

public:
  StringToMsg() {
    this->bufPtr   = this->tmpBuf;
    this->bufSize  = sizeof( this->tmpBuf );
    this->fvalPtr  = this->fvalBuf;
    this->fvalSize = sizeof( this->fvalBuf );
    this->countVal = 0;
  }

  ~StringToMsg() {
    if ( this->bufPtr != this->tmpBuf )
      FREE( this->bufPtr );
    if ( this->fvalPtr != this->fvalBuf )
      FREE( this->fvalPtr );
  }
  /* get a temporary buffer */
  char *getBuf( unsigned int size ) {
    if ( size > this->bufSize ) {
      if ( this->bufPtr == this->tmpBuf )
        MALLOC( size, &this->bufPtr );
      else
        REALLOC( size, &this->bufPtr );
      this->bufSize = size;
    }
    return this->bufPtr;
  }
  /* next count */
  char *getCounter( void ) {
    char tmp[ 16 ];
    unsigned int i = this->countVal++, off = 16;
    do {
      tmp[ --off ] = (char) ( i % 10 ) + '0';
      i /= 10;
    } while ( i != 0 );
    ::memcpy( this->countBuf, &tmp[ off ], 16 - off );
    this->countBuf[ 16 - off ] = '\0';
    return this->countBuf;
  }

  char *getFval( unsigned int size ) {
    if ( size > this->fvalSize ) {
      if ( this->fvalPtr == this->fvalBuf )
        MALLOC( size, &this->fvalPtr );
      else
        REALLOC( size, &this->fvalPtr );
      this->fvalSize = size;
    }
    return this->fvalPtr;
  }

  /* Take the data values, break them into fields and and add to the msg */
  void addFields( RaiMsg &raiMsg,  const char *datavals ) {
    const char * ptr = datavals,
               * tmp,
               * nextPtr;
    char         fname[ 256 ], /* name of the field */
               * fval,         /* value of the field */
               * buf;
    unsigned int fnameLen,
                 fvalLen;
    for (;;) {
      tmp = ptr;
      if ( ptr == NULL || (ptr = ::strchr( ptr, '=' )) == NULL )
        return;
      fnameLen = (unsigned int) ( ptr - tmp );
      if ( fnameLen > sizeof( fname ) - 1 )
        fnameLen = sizeof( fname ) - 1;
      ::strncpy( fname, tmp, fnameLen );
      fname[ fnameLen ] = '\0';

      tmp = ++ptr;
      nextPtr = NULL;
      if ( ptr[ 0 ] == '{' || ptr[ 0 ] == '[' ) {
        unsigned int depth = 0;
        char stk[ 16 ];
        stk[ depth++ ] = ( ptr[ 0 ] == '{' ? '}' : ']' );
        for ( const char *x = &ptr[ 1 ]; *x != '\0'; x++ ) {
          if ( *x == stk[ depth - 1 ] ) {
            if ( --depth == 0 ) {
              tmp++;
              ptr = x;
              nextPtr = ptr + 1;
              if ( *nextPtr == ',' )
                nextPtr++;
              break;
            }
          }
          if ( *x == '{' || *x == '[' )
            stk[ depth++ ] = ( *x == '{' ? '}' : ']' );
        }
      }
      if ( nextPtr == NULL ) {
        if ( (ptr = ::strchr( ptr, ',' )) == NULL ) {
          ptr = &tmp[ ::strlen( tmp ) ];
          nextPtr = NULL;
        }
        else {
          nextPtr = ptr + 1;
        }
      }
      fvalLen = ptr - tmp;
      fval = this->getFval( fvalLen + 1 );
      if ( fvalLen == sizeof( "__count__" ) - 1 &&
           ::strncmp( tmp, "__count__", fvalLen ) == 0 ) {
        ::strcpy( fval, this->getCounter() );
        fvalLen = ::strlen( fval );
      }
      else {
        ::strncpy( fval, tmp, fvalLen );
        fval[ fvalLen ] = '\0';
      }

      /* Add the field and data to the message. */
      logDebug( LDEBUG, "Setting field %s=%s", fname, fval );
      try {
        RaiField_data data;

        /* if message is SASS QForm, then the type is determined by the dict */
        if ( raiMsg.isSass() )
          raiMsg.Append( fname, (const char *) fval );
        else {
          unsigned int wbytes, width = 0, p = 1, i;
          /* parse the width of the field value (s1024 = string 1024 bytes) */
          for ( i = fnameLen; i > 2 && fname[ i - 1 ] >= '0' &&
                                       fname[ i - 1 ] <= '9'; ) {
            width += ( fname[ --i ] - '0' ) * p;
            p *= 10;
          }
          switch ( width ) {
            default: wbytes = width; break;
            case 8:  wbytes = 1; break;
            case 16: wbytes = 2; break;
            case 32: wbytes = 4; break;
            case 64: wbytes = 8; break;
          }
          /* check if the fname has a type specifier field_name:s1024 */
          if ( i > 2 && fname[ i - 2 ] == ':' ) {
            fname[ i - 2 ] = '\0';
            /* :m, :s, :o, :f, :i, :u, :b */
            switch ( fname[ i - 1 ] ) {
              case 'm': {
                RaiMsg      msg2( raiMsg.GetProtocol() );
                StringToMsg str2;
                str2.addFields( msg2, fval );
                raiMsg.Append( fname, &msg2 );
                break;
              }
              case 'I': /* int array */
              case 'U': /* uint array */
              case 'F': { /* float array */
                char       * arp,
                           * ars = fval;
                unsigned int sz = 0;
                ullong       alignedarray[ 1024 / sizeof( ullong ) ];
                byte       * ar = (byte *) (void *) alignedarray;
                RaiMsg_type  ftype;
                if ( fname[ i - 1 ] == 'I' )
                  ftype = RAIMSG_INT;
                else if ( fname[ i - 1 ] == 'U' )
                  ftype = RAIMSG_UINT;
                else
                  ftype = RAIMSG_REAL;
                for ( arp = fval; ; arp++ ) {
                  if ( *arp == ',' || *arp == '\0' ) {
                    if ( arp == ars )
                      break;
                    RaiField::Convert( ftype, wbytes, &ar[ wbytes * sz ],
                                       RAIMSG_STRING, arp - ars, ars );
                    sz++;
                    if ( sz * wbytes >= 1024 )
                      break;
                    if ( *arp == '\0' )
                      break;
                    ars = &arp[ 1 ];
                  }
                }
                raiMsg.Append( fname, ar, sz, ftype, wbytes );
                break;
              }
              case 's': /* string, may be padded with spaces */
                if ( width != 0 ) {
                  buf = this->getBuf( width + 1 );
                  ::memset( buf, ' ', width );
                  buf[ width ] = '\0';
                  ::memcpy( buf, fval, fvalLen );
                  raiMsg.Append( fname, RAIMSG_STRING, width,
                                 (RaiMsg_data) buf );
                }
                else {
                  raiMsg.Append( fname, (const char *) fval );
                }
                break;
              case 'o': /* opaque, may be paddded with nuls */
                if ( width != 0 ) {
                  buf = this->getBuf( width + 1 );
                  ::memset( buf, '\0', width );
                  buf[ width ] = '\0';
                  ::memcpy( buf, fval, fvalLen );
                  raiMsg.Append( fname, RAIMSG_OPAQUE, width,
                                 (RaiMsg_data) buf );
                }
                else {
                  raiMsg.Append( fname, RAIMSG_OPAQUE, fvalLen + 1,
                                  (RaiMsg_data) fval );
                }
                break;
              case 'f': /* real  :f32, :f64 */
                if ( wbytes == 0 ) wbytes = 8;
                RaiField::Convert( RAIMSG_REAL, wbytes, &data,
                                   RAIMSG_STRING, fvalLen, fval );
                raiMsg.Append( fname, RAIMSG_REAL, wbytes, &data );
                break;
              case 'u': /* uint  :u8, :u16, :u32, :u64 */
                if ( wbytes == 0 ) wbytes = 4;
                RaiField::Convert( RAIMSG_UINT, wbytes, &data,
                                   RAIMSG_STRING, fvalLen, fval );
                raiMsg.Append( fname, RAIMSG_UINT, wbytes, &data );
                break;
              case 'i': /* int  :i8, :i16, :i32, :i64 */
                if ( wbytes == 0 ) wbytes = 4;
                RaiField::Convert( RAIMSG_INT, wbytes, &data,
                                   RAIMSG_STRING, fvalLen, fval );
                raiMsg.Append( fname, RAIMSG_INT, wbytes, &data );
                break;
              case 'b': /* boolean */
                RaiField::Convert( RAIMSG_BOOLEAN, 1, &data.boolean,
                                   RAIMSG_STRING, fvalLen, fval );
                raiMsg.Append( fname, data.boolean );
                break;
              case 'p': { /* price */
                RaiField::ConvertCtx ctx;
                if ( wbytes == 0 ) wbytes = 8;
                ctx.init( RAIMSG_REAL, wbytes, &data,
                          RAIMSG_STRING, fvalLen, fval );
                RaiField::Convert( ctx );
                raiMsg.Append( fname, RAIMSG_REAL, wbytes, &data,
                                      RAIMSG_UINT, 1, &ctx.destHint );
                break;
              }
              default:
                static const ErrorRec e = { 0, "Bad type", "Convert field" };
                logError( LERROR, &e, "Field: %s", fname );
                throw &e;
            }
          }
          else {
            /* no type specified, append string value */
            raiMsg.Append( fname, (const char *) fval );
          }
        }
      } catch ( RaiException e ) {
        /* if there is a conversion error from string to type */
        logError( LERROR, e, "Setting field %s=%s", fname, fval );
        throw e;
      }
      if ( nextPtr == NULL || *nextPtr == '\0' )
        return;
      ptr = nextPtr;
    }
  }
};

} // namespace rai

#endif
