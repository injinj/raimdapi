/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#ifndef __rai_msg__cfile_strings_h__
#define __rai_msg__cfile_strings_h__

#ifndef __rai_util__array_h__
#include "util/array.h"
#endif

namespace rai {
class CFileStrings : private SortedArray<const char *> {
  private:
    static const unsigned int BLOCK_SIZE = 8 * 1024;

    struct Block {
      byte       * mem;
      const char * fileName;
    };

    Array<Block> blocks;
    unsigned int memLeft,
                 len;

    virtual int compare( const char *s1,  const char *s2 );

  public:
    SYS_OPS( CFileStrings );
    CFileStrings();
    ~CFileStrings();

    const char *getFileName( const char *fileName );

    const char *getString( const char *ptr,  unsigned int len );

    void *allocMem( unsigned int len );
};
} // namespace rai

#endif
