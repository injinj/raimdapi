/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#ifndef __rai_msg__subject_h__
#define __rai_msg__subject_h__

#if ! defined( RAIMSG_DLL_EXP )
#if defined( RAI_DLL )
#define RAIMSG_DLL_EXP __declspec(dllimport)
#else
#define RAIMSG_DLL_EXP
#endif
#endif

#ifndef __rai_base__types_h__
#include "base/types.h"
#endif

#ifndef __rai_msg__sass_const_h__
#include "msg/sass_const.h"
#endif

#ifndef __rai_base__mem_h__
#include "base/mem.h"
#endif

class RaiSubject;

namespace rai {
class RAIMSG_DLL_EXP Subject {
  protected:
    byte         * buf;    /* if ! NULL && bufLen == 0, is allocated */
    unsigned int   hashVal;
    unsigned short bufLen;
    bool           allocFlag;

  public:
    /* if on heap */
    bool isAlloced() const { return this->allocFlag; }
    /* if on stack */
    bool isStatic() const { return ! this->allocFlag; }

    void move( Subject &subj ) {
      if ( this->isAlloced() )
        this->clear();
      this->buf       = subj.buf;
      this->hashVal   = subj.hashVal;
      this->bufLen    = subj.bufLen;
      this->allocFlag = subj.allocFlag;
      subj.buf        = NULL;
      subj.hashVal    = 0;
      subj.bufLen     = 0;
      subj.allocFlag  = false;
    }
    void copy( const Subject &subj )                            throw( Error );

    static void nullReference( void )                           throw( Error );

    static unsigned int computeHash( const byte *buf,  unsigned int len );

  public:
    void init( void ) {
      this->buf       = NULL;
      this->hashVal   = 0;
      this->bufLen    = 0;
      this->allocFlag = false;
    }
    void init( byte *buf,  unsigned int bufLen,  bool isAlloced ) {
      this->buf       = buf;
      this->hashVal   = 0;
      this->bufLen    = bufLen;
      this->allocFlag = isAlloced;
    }
    void init( byte *buf,  unsigned int bufLen,  unsigned int hashVal,
               bool isAlloced ) {
      this->buf       = buf;
      this->hashVal   = hashVal;
      this->bufLen    = bufLen;
      this->allocFlag = isAlloced;
    }
    void init( Subject &subj ) {
      this->buf       = subj.buf;
      this->hashVal   = subj.hashVal;
      this->bufLen    = subj.bufLen;
      this->allocFlag = false;
    }
    void clear( void );

    bool isEmpty( void ) const {
      return this->buf == NULL;
    }
    const byte *getBuf( void ) const {
      return this->buf;
    }
    bool getHashVal( unsigned int &h ) const {
      if ( (h = this->hashVal) == 0 )
        return false;
      return true;
    }
    Subject() {
      this->init();
    }
    ~Subject() {
      if ( this->allocFlag )
        this->clear();
    }

    static unsigned int hashSegment( unsigned int mask,
                                     unsigned int numHashBits,
                                     const byte *segment,
                                     unsigned int segmentLen );
};

class RAIMSG_DLL_EXP MsaSubject : public Subject {
 protected:
    /* copy segments to buffer, no toBuf bounds checking */
    static unsigned int layout( const char *segments[ 4 ],
                                unsigned int lens[ 4 ],  byte *toBuf );
  public:
    SYS_OPS( MsaSubject );
    MsaSubject() {}
    /* set from another subject, must use static buffers or both will free */
    void set( MsaSubject &subj ) throw( Error ) {
      this->set( subj.buf, subj.length() );
      this->hashVal = subj.hashVal;
    }
    /* set the buf and len ptrs from parameters, for comparing and decoding */
    void set( byte *buf,  unsigned int bufLen,  bool isAlloced = false )
                                                                throw( Error );
    /* allocate and copy from subj */
    void copy( const MsaSubject &subj ) throw( Error ) {
      this->Subject::copy( subj );
    }
    /* move buffer from subj to this, reset subj buffer so both don't free */
    void move( MsaSubject &subj );
    /* if has empty segments */
    bool isWildcard( void ) const;
    /* decode binary into c string buffer, returns length of subject used */
    unsigned int decode( char *subject,  unsigned int maxSubjLen ) const
    /* same as decode(), but return a string ptr */             throw( Error );
    char *toString( char *buf,  unsigned int bufLen ) const throw( Error ) {
      this->decode( buf, bufLen );
      return buf;
    }
    /* allocate space for subject and copy to it */
    unsigned int encode( const char *subject )                  throw( Error );
    /* make sure bufSize is big enough and copy it */
    unsigned int encode( const char *segments[ 4 ],  unsigned int lens[ 4 ],
                         byte *toBuf = NULL,  unsigned int bufSize = 0 )
    /* make sure subject is valid, return length of encoded */  throw( Error );
    static unsigned int validate( unsigned int lens[ 4 ] )      throw( Error );
    /* convert to sass subject, using toBuf */
    unsigned int convert( const RaiSubject &subj,  bool isWild,
                          byte *toBuf = NULL,  unsigned int bufLen = 0 )
    /* the binary length of subject */                          throw( Error );
    unsigned int length( void ) const throw( Error ) {
      if ( this->buf == NULL )
        this->Subject::nullReference();
      return this->bufLen;
    }
    unsigned int queryLength( void ) const                      throw( Error );

    unsigned int copyTo( byte *toBuf,  unsigned int toBufLen ) const
                                                                throw( Error );
    unsigned int copyQueryTo( byte *toBuf,  unsigned int toBufLen ) const
                                                                throw( Error );
    bool equals( const MsaSubject &subj ) const;

    /* return a ptr into the buffer, which is a c style string */
    const char *segment( unsigned int segNum ) const            throw( Error );

    bool matches( const MsaSubject &subj ) const;

    unsigned int hash( void ) const                             throw( Error );

    static unsigned int hashQuery( unsigned int hash )          throw( Error );

    /* return true if hash1 == hash2 or if hash2 is within wildard hash1 */
    static bool hashMatches( unsigned int hash1, unsigned int hash2 );

    /* return number of wildcard parts in hash */
    static int hashWildParts( unsigned int hash );

    static unsigned int hashSegMask( int seg );
};


class RAIMSG_DLL_EXP SassSubject : public Subject {
  protected:
    /* copy to buffer, no toBuf bounds checking */
    static unsigned int layout( const char *subject,  byte *toBuf );
    /* copy segments to buffer, no toBuf bounds checking */
    static unsigned int layout( const char *segments[ 4 ],  byte *toBuf );
    /* copy segments to buffer, no toBuf bounds checking */
    static unsigned int layout( const char *segments[ 4 ],
                                unsigned int lens[ 4 ],  byte *toBuf );
  public:
    SYS_OPS( SassSubject );
    SassSubject() {}
    /* set from another subject, must use static buffers or both will free */
    void set( SassSubject &subj ) throw( Error ) {
      this->set( subj.buf, subj.length() );
      this->hashVal = subj.hashVal;
    }
    /* set the buf and len ptrs from parameters, for comparing and decoding */
    void set( byte *buf,  unsigned int bufLen,  bool isAlloced = false )
                                                                throw( Error );
    /* allocate and copy from subj */
    void copy( const SassSubject &subj ) throw( Error ) {
      this->Subject::copy( subj );
    }
    /* copy from subj to static buffer */
    void copy( const SassSubject &subj,  byte *toBuf,  unsigned int toBufLen )
                                                                throw( Error );
    /* move buffer from subj to this, reset subj buffer so both don't free */
    void move( SassSubject &subj );
    /* if has empty segments */
    bool isWildcard( void ) const;
    /* decode binary into c string buffer, returns length of subject used */
    unsigned int decode( char *subject,  unsigned int maxSubjLen ) const
    /* same as decode(), but return a string ptr */             throw( Error );
    char *toString( char *buf,  unsigned int bufLen ) const throw( Error ) {
      this->decode( buf, bufLen );
      return buf;
    }
    /* convert XYZ.N.M.O to XYZ...QUERY string */
    unsigned int decodeQuery( char *subject,  unsigned int maxSubjLen ) const
                                                                throw( Error );
    char *queryToString( char *buf,  unsigned int bufLen ) const throw( Error ) {
      this->decodeQuery( buf, bufLen );
      return buf;
    }
    static unsigned int validate( const char *subject )         throw( Error );
    /* make sure bufSize is big enough for subject and copy to it */
    unsigned int encode( const char *subject,  byte *toBuf = NULL,
                         unsigned int bufSize = 0 )             throw( Error );
    /* make sure subject is valid, return length of encoded */
    static unsigned int validate( const char *segments[ 4 ] )   throw( Error );
    /* make sure bufSize is big enough and copy it */
    unsigned int encode( const char *segments[ 4 ],  byte *toBuf = NULL,
                         unsigned int bufSize = 0 )             throw( Error );
    /* make sure subject is valid, return length of encoded */
    static unsigned int validate( unsigned int lens[ 4 ] )      throw( Error );
    /* make sure bufSize is big enough and copy it */
    unsigned int encode( const char *segments[ 4 ],  unsigned int lens[ 4 ],
                         byte *toBuf = NULL,  unsigned int bufSize = 0 )
    /* convert to sass subject, using toBuf */                  throw( Error );
    unsigned int convert( const RaiSubject &subj,  bool isWild,
                          byte *toBuf = NULL,  unsigned int bufLen = 0 )
    /* the binary length of subject */                          throw( Error );
    unsigned int length( void ) const throw( Error ) {
      if ( this->buf == NULL )
        this->Subject::nullReference();
      return this->bufLen;
    }
    /* the binary length of subject, in ci SUB..QUERY style */
    unsigned int queryLength( void ) const                      throw( Error );
    /* copy binary image to buf */
    unsigned int copyTo( byte *toBuf,  unsigned int toBufLen ) const
    /* copy binary image of ci SUB..QUERY */                    throw( Error );
    unsigned int copyQueryTo( byte *toBuf,  unsigned int toBufLen ) const
    /* compare in strcmp sense */                               throw( Error );
    bool equals( const SassSubject &subj ) const;
    /* compare, if segment is empty, is wildcard and matches anything */
    bool matches( const SassSubject &subj ) const;
    /* return a ptr into the buffer, which is a c style string */
    const char *segment( unsigned int segNum ) const            throw( Error );
    /* return count of segments in copied into segs[] array */
    unsigned int getSegments( const char **segs,  unsigned int maxSegs ) const
                                                                throw( Error );
    unsigned int getSegments( const char **segs,  unsigned int *lens,
                              unsigned int maxSegs ) const      throw( Error );
    /* return count of segments, 0 if is empty */
    unsigned int segmentCount( void ) const;
    /* the ci hash, which divides the lower 22 bits in to 4 pieces */
    unsigned int hash( void ) const                             throw( Error );
    /* a real hash */
    unsigned int hash2( void ) throw( RaiException ) {
      if ( this->hashVal == 0 )
        this->hashVal = rai::Subject::computeHash( this->buf, this->length() );
      return this->hashVal;
    }
    /* the ci SUB..QUERY hash */
    static unsigned int hashQuery( unsigned int hash )          throw( Error );
};
} // namespace rai

#define RvSubject RaiSubject
class RAIMSG_DLL_EXP RaiSubject : public rai::Subject {
  /*protected:*/
  public:
    /* copy to buffer, no toBuf bounds checking */
    static unsigned int layout( const char *subject,  unsigned int segCount,
                                byte *toBuf, const char sep='.' );
    static unsigned int layout2( const char *subject,  unsigned int maxLen,
                                 unsigned int segCount,  byte *toBuf,
                                 const char sep='.' );
    /* copy segments to buffer, no toBuf bounds checking */
    static unsigned int layout( const char **segments,  unsigned int segCount,
                                byte *toBuf );
    /* set subject buffer, return 0 if not enough space */
    unsigned int staticLayout( const char *subject,  byte *toBuf,
                               unsigned int toBufLen,
                               const char sep='.' )       throw( RaiException );
    unsigned int staticLayout2( const char *subject,  unsigned int maxLen,
                                byte *toBuf,  unsigned int toBufLen,
                                const char sep='.' )      throw( RaiException );
    static unsigned int layout( const char **segments,  unsigned int *lens,
                                unsigned int segCount,  byte *toBuf );
  public:
    void * operator new( size_t sz, void *ptr ) { return ptr; }

    SYS_OPS( RaiSubject );
    RaiSubject() {}
    /* set from another subject, must use static buffers or both will free */
    void set( RaiSubject &subj ) throw( RaiException ) {
      this->set( subj.buf, subj.length() );
      this->hashVal = subj.hashVal;
    }
    /* set the buf and len ptrs from parameters, for comparing and decoding */
    void set( byte *buf,  unsigned int bufLen,  unsigned int hashVal,
              bool isAlloced ) throw( RaiException ) {
      this->set( buf, bufLen, isAlloced );
      this->hashVal = hashVal;
    }
    /* set the buf and len ptrs from parameters, for comparing and decoding */
    void set( byte *buf,  unsigned int bufLen,  bool isAlloced = false )
                                                          throw( RaiException );
    /* allocate and copy from subj */
    void copy( const RaiSubject &subj ) throw( RaiException ) {
      this->rai::Subject::copy( subj );
    }
    /* copy from subj to static buffer */
    void copy( const RaiSubject &subj,  byte *toBuf,  unsigned int toBufLen )
                                                          throw( RaiException );
    /* move buffer from subj to this, reset subj buffer so both don't free */
    void move( RaiSubject &subj );
    /* the binary length of subject */
    unsigned int length( void ) const throw( RaiException ) {
      if ( this->buf == NULL )
        this->rai::Subject::nullReference();
      return this->bufLen;
    }
    /* if has '*' or trailing '>' */
    bool isWildcard( void ) const;
    /* decode binary buffer at ptr into string buffer */
    unsigned int decode( char *subject,  unsigned int maxSubjLen,
                         const char sep = '.' ) const     throw( RaiException );
    /* same as decode(), but return a string ptr */
    char *toString( char *buf,  unsigned int bufLen,  const char sep='.' ) const
                                                         throw( RaiException ) {
      this->decode( buf, bufLen, sep );
      return buf;
    }
    /* make sure subject is valid, return length of encoded and num segments */
    static unsigned int validate( const char *subject,  unsigned int &numSegs,
                                  const char sep='.')     throw( RaiException );
    static unsigned int validate2( const char *subject,  unsigned int maxLen,
                                   unsigned int &numSegs,  const char sep= '.' )
                                                          throw( RaiException );
    static unsigned int encodedSize( const char *subject,  const char sep= '.' )
                                                         throw( RaiException ) {
      unsigned int numSegs;
      return RaiSubject::validate( subject, numSegs, sep );
    }
    /* make sure bufSize is big enough for subject and copy to it */
    unsigned int encode( const char *subject,  byte *toBuf = NULL,
                         unsigned int bufSize = 0,  const char sep = '.' )
                                                          throw( RaiException );
    unsigned int encode2( const char *subject,  unsigned int maxLen,
                          byte *toBuf = NULL,  unsigned int bufSize = 0,
                          const char sep = '.' )          throw( RaiException );
    /* make sure subject is valid, return length of encoded */
    static unsigned int validate( const char **segments,  unsigned int segCount)
                                                          throw( RaiException );
    /* make sure bufSize is big enough and copy it */
    unsigned int encode( const char **segments,  unsigned int segCount,
                         byte *toBuf = NULL,  unsigned int bufSize = 0 )
                                                          throw( RaiException );
    static unsigned int validate( unsigned int *lens,
                                  unsigned int segCount ) throw( RaiException );
    unsigned int encode( const char **segments,  unsigned int *lens,
                         unsigned int segCount,  byte *toBuf = NULL,
                         unsigned int bufSize = 0 )       throw( RaiException );
    /* convert to rv subject, using toBuf */
    unsigned int convert( const rai::SassSubject &subj,  bool isWild,
                          byte *toBuf = NULL,  unsigned int bufLen = 0 )
                                                          throw( RaiException );
    /* convert to rv subject, using toBuf */
    unsigned int convert( const rai::MsaSubject &subj,  bool isWild,
                          byte *toBuf = NULL,  unsigned int bufLen = 0 )
                                                          throw( RaiException );
    /* convert to rv subject, adding prefix, using toBuf */
    unsigned int convert2( const rai::SassSubject &subj,  const char * prefix, 
                          bool isWild, byte *toBuf = NULL,
                          unsigned int bufLen = 0 )       throw( RaiException );
    /* copy binary image to buf */
    unsigned int copyTo( byte *toBuf,  unsigned int toBufLen ) const
                                                          throw( RaiException );
    /* compare in strcmp sense */
    bool equals( const RaiSubject &subj ) const;
    /* compare, if segment is empty, is wildcard and matches anything */
    bool matches( const RaiSubject &subj ) const;

    bool matchesEverything( void ) const {
      if ( this->segmentCount() == 1 ) {
        const char *seg = this->segment( 0 );
        if ( seg[ 0 ] == '>' && seg[ 1 ] == '\0' )
          return true;
      }
      return false;
    }
    /* return a ptr into the buffer, which is a c style string */
    const char *segment( unsigned int segNum ) const      throw( RaiException );
    /* return count of segments in copied into segs[] array */
    unsigned int getSegments( const char **segs,  unsigned int maxSegs ) const
                                                          throw( RaiException );
    unsigned int getSegments( const char **segs,  unsigned int *lens,
                              unsigned int maxSegs ) const throw( RaiException );
    /* return count of segments, 0 if is empty */
    unsigned int segmentCount( void ) const;
    /* concatenate subjects, return length */
    unsigned int concat( RaiSubject &subj1,  RaiSubject &subj2,
                         byte *toBuf = NULL,  unsigned int bufSize = 0 )
                                                          throw( RaiException );
    unsigned int hash( void ) throw( RaiException ) {
      if ( this->hashVal == 0 )
        this->hashVal = rai::Subject::computeHash( this->buf, this->length() );
      return this->hashVal;
    }
    unsigned int toRIC( char *ricBuf,  unsigned int maxLen,
                        bool dontMapCaretToDot,  bool dontAppendNaE )
                                                          throw( RaiException );
    unsigned int toRIC( char *ricBuf,  unsigned int maxLen,
                        const char *sector,  bool dontMapCaretToDot,
                        bool dontAppendNaE )              throw( RaiException );
    unsigned int toRIC( char *ricBuf,  unsigned int maxLen,
                        unsigned int prefixSegs,  bool dontMapCaretToDot,
                        bool dontAppendNaE )              throw( RaiException );
    unsigned int toRIC2( const char **seg,  unsigned int *lens,
                         unsigned int nSegs,  char *ricBuf,
                         unsigned int off,  unsigned int maxLen,
                         bool dontMapCaretToDot,  bool dontAppendNaE )
                                                          throw( RaiException );
    unsigned int encodeRIC( const char *prefix,  const char *ric,
                            byte *toBuf,  unsigned int bufLen,
                            bool dontMapDotToCaret,
                            bool dontAppendNaE )          throw( RaiException );
    unsigned int encodeRIC( const char *prefix,  const char *sector,
                            const char *ric,  byte *toBuf,  unsigned int bufLen,
                            bool dontMapDotToCaret,
                            bool dontAppendNaE )          throw( RaiException );
    static RaiSubject *create( const char *s )            throw( RaiException );
};

namespace rai {
namespace SubjectErr {
  enum {
    SUBJECT_TOO_LONG   = 0,
    BUF_TOO_SMALL      = 1,
    SUBJECT_IS_NULL    = 2,
    BAD_SEGMENT_NUM    = 3,
    LABEL_TOO_BIG      = 4,
    RV_EMPTY_SEG       = 5,
    RV_TOO_MANY_SEGS   = 6,
    MAX_SEG_OVERFLOW   = 7,
    BAD_RIC            = 8,
    CARET_IN_RIC       = 9
  };
  RAIMSG_DLL_EXP
  Error getErr( unsigned int status );
}
} // namespace rai

#endif
