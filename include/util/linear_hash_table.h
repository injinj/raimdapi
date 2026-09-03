/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#ifndef __rai_util__linear_hash_table_h__
#define __rai_util__linear_hash_table_h__

#ifndef __rai_base__mem_h__ 
#include "base/mem.h"
#endif

namespace rai {

/*
if an ElemType is not a pointer, define ElemType(int) and
operator==( const ElemType &v ).  Ex:

struct ThreeInts {
  unsigned int one, two, three;
  ThreeInts( int zero ) { 
    this->one = 0; this->two = 0; this->three = 0;
  }
  ThreeInts() {}
  bool operator==( const ThreeInts &x ) const {
    return this->one == x.one && this->two == x.two && this->three == x.three;
  }
};

struct TestHT : public LinearHashTable<ThreeInts, ThreeInts> {}

The ThreeInts( int zero ) is used to create the null item ThreeInts( 0 ), it
must be a unique value.


When ElemType is a pointer, ElemTypePtr( 0 ) is equivalent to (ElemType *) 0
*/

template <class ElemType, class ValType>
class LinearHashTable {
  protected:
    struct Table {
      ElemType     elem;
      unsigned int hashVal;
    };
    Table      * tab;        /* test tab[ n ].hashVal & SLOT_USED if has data */
    unsigned int tabSize,    /* must be ^2, uses & (tabSize-1), not % tabSize */
                 loadThresh, /* does not shrink!, grows exponentially */
                 elemCount,
                 tabMask;       /* tabSize - 1 */
  public:
    static const unsigned int SLOT_USED = 1U << 31;
  protected:
    virtual ValType value( ElemType ptr ) = 0;

    virtual unsigned int hash( ValType key ) = 0;

    virtual bool equals( ValType key,  ElemType ptr ) = 0;

    void resize( void ) throw( Error ) {
      unsigned int oldSize,
                   newSize,
                   hashVal,
                   i,
                   j;

      if ( this->tab == NULL ) {
        oldSize = 0;
        /* make sure we're pow2 */
        for ( newSize = 2; newSize < this->tabSize; newSize <<= 1 )
          ;
        this->tabSize = newSize;
      }
      else {
        oldSize = this->tabSize;
        newSize = oldSize << 1;
        if ( newSize == SLOT_USED ) {
          static const ErrorRec e = { 0, "Too big", "LHT" };
          throw &e;
        }
      }

      REALLOC( newSize * sizeof( this->tab[ 0 ] ), &this->tab );
      for ( i = oldSize; i < newSize; i++ )
        this->tab[ i ].hashVal = 0;

      this->tabMask = newSize - 1;

      /* reposition elements in first half */
      for ( i = 0; i < oldSize ||
                   ( this->tab[ i ].hashVal & SLOT_USED ) != 0; i++ ) {
        if ( ( this->tab[ i ].hashVal & SLOT_USED ) != 0 ) {
          hashVal = this->tab[ i ].hashVal;
          j       = hashVal & tabMask;
          if ( i != j ) {
            this->tab[ i ].hashVal = 0;
            while ( ( this->tab[ j ].hashVal & SLOT_USED ) != 0 )
              j = ( j + 1 ) & tabMask;
            this->tab[ j ].elem    = this->tab[ i ].elem;
            this->tab[ j ].hashVal = hashVal;
          }
        }
      }

      this->tabSize    = newSize;
      this->loadThresh = newSize * 3 / 4;
    };

  public:
    LinearHashTable( unsigned int initialSize = 8 ) {
      this->tab        = NULL;
      this->tabSize    = initialSize;
      this->loadThresh = 0;
      this->elemCount  = 0;
      this->tabMask    = initialSize - 1;
    };

    virtual ~LinearHashTable() {
      if ( this->tab != NULL ) {
        FREE( this->tab );
        this->tab = NULL;
      }
    };

    bool isEmpty( void ) const {
      return this->elemCount == 0;
    };

    unsigned int length( void ) const {
      return this->elemCount;
    };

    unsigned int mask( void ) const {
      return this->tabMask;
    };

    void reset( void ) {
      unsigned int i;

      if ( this->elemCount > 0 ) {
        for ( i = 0; i < this->tabSize; i++ )
          this->tab[ i ].hashVal = 0;
        this->elemCount = 0;
      }
    };

    void insert( ElemType ptr ) throw( Error ) {
      unsigned int i,
                   hashVal;

      if ( this->tab == NULL || this->elemCount >= this->loadThresh )
        this->resize();

      hashVal = this->hash( this->value( ptr ) ) | SLOT_USED;
      i       = hashVal & tabMask;
      while ( ( this->tab[ i ].hashVal & SLOT_USED ) != 0 )
        i = ( i + 1 ) & tabMask;
      this->tab[ i ].elem    = ptr;
      this->tab[ i ].hashVal = hashVal;
      this->elemCount++;
    };

    // Insert using supplied hash
    void insertWithHash( ElemType ptr, unsigned int hashVal ) throw( Error ) {
      unsigned int i;
      
      if ( this->tab == NULL || this->elemCount >= this->loadThresh )
        this->resize();

      hashVal = hashVal | SLOT_USED;
      i       = hashVal & tabMask;
      while ( ( this->tab[ i ].hashVal & SLOT_USED ) != 0 )
        i = ( i + 1 ) & tabMask;
      this->tab[ i ].elem    = ptr;
      this->tab[ i ].hashVal = hashVal;
      this->elemCount++;
    };

 protected:
    ElemType scanKey( ValType key,  unsigned int &hashVal,
                      unsigned int &pos ) {
      hashVal = this->hash( key ) | SLOT_USED;
      pos     = hashVal & tabMask;

      while ( ( this->tab[ pos ].hashVal & SLOT_USED ) != 0 ) {
        if ( this->tab[ pos ].hashVal == hashVal &&
             this->equals( key, this->tab[ pos ].elem ) )
          return this->tab[ pos ].elem;
        pos = ( pos + 1 ) & tabMask;
      }
      return ElemType( 0 );
    };

    ElemType scanNextKey( ValType key,  unsigned int hashVal,
                          unsigned int &pos ) {
      do {
        pos = ( pos + 1 ) & tabMask;
        if ( this->tab[ pos ].hashVal == hashVal &&
             this->equals( key, this->tab[ pos ].elem ) )
          return this->tab[ pos ].elem;
      } while ( ( this->tab[ pos ].hashVal & SLOT_USED ) != 0 );
      return ElemType( 0 );
    };

    // Scan fo key using supplied hash
    ElemType scanKeyWithHash( ValType key, unsigned int hashVal, unsigned int &pos ) {
      hashVal = hashVal | SLOT_USED;
      pos     = hashVal & tabMask;
      
      while ( ( this->tab[ pos ].hashVal & SLOT_USED ) != 0 ) {
        if ( this->tab[ pos ].hashVal == hashVal &&
             this->equals( key, this->tab[ pos ].elem ) )
          return this->tab[ pos ].elem;
        pos = ( pos + 1 ) & tabMask;
      }
      return ElemType( 0 );
    };

  public:
    ElemType findInsert( ValType key,  unsigned int &i,
                         unsigned int &hashVal ) {
      if ( this->tab == NULL || this->elemCount >= this->loadThresh )
        this->resize();
      return this->scanKey( key, hashVal, i );
    };

    void addInsert( ElemType ptr,  unsigned int i,  unsigned int hashVal ) {
      if ( ( this->tab[ i ].hashVal & SLOT_USED ) == 0 )
        this->elemCount++;
      this->tab[ i ].elem    = ptr;
      this->tab[ i ].hashVal = hashVal;
    };

    ElemType insertUnique( ElemType ptr ) throw( Error ) {
      unsigned int i, hashVal;
      ElemType     ptr2;

      ptr2 = this->findInsert( this->value( ptr ), i, hashVal );
      if ( ptr2 == ElemType( 0 ) ) {
        this->tab[ i ].elem    = ptr;
        this->tab[ i ].hashVal = hashVal;
        this->elemCount++;
      }
      return ptr2;
    };

    bool keyExists( ValType key ) {
      unsigned int i, hashVal;
      return ! this->isEmpty() &&
               this->scanKey( key, hashVal, i ) != ElemType( 0 );
    };

    bool hashExists( unsigned int hashVal ) {
      if ( ! this->isEmpty() ) {
        unsigned int pos = hashVal & tabMask;
        while ( ( this->tab[ pos ].hashVal & SLOT_USED ) != 0 ) {
          if ( this->tab[ pos ].hashVal == ( hashVal | SLOT_USED ) )
            return true;
          pos = ( pos + 1 ) & tabMask;
        }
      }
      return false;
    };

    ElemType findKey( ValType key,  unsigned int &i,  unsigned int &hashVal ) {
      return this->isEmpty() ? ElemType( 0 ) : this->scanKey( key, hashVal, i );
    };

    ElemType findSlot( ValType key,  unsigned int &i ) {
      unsigned int hashVal;
      return this->isEmpty() ? ElemType( 0 ) : this->scanKey( key, hashVal, i );
    };

    ElemType findElem( ValType key ) {
      unsigned int i, hashVal;
      return this->isEmpty() ? ElemType( 0 ) : this->scanKey( key, hashVal, i );
    };

    // Search for 'key' and return entry. hashVal is set to hash of key 
    ElemType findElemHash( ValType key, unsigned int &hashVal ) {
      unsigned int i;
      if( this->isEmpty() ) {
        hashVal = SLOT_USED | this->hash( key );
        return ElemType( 0 );
      } 
      return this->scanKey( key, hashVal, i );
    };

    // Search for 'key' using supplied hash 
    ElemType findElemWithHash( ValType key, unsigned int hashVal ) {
      unsigned int i;
      return this->isEmpty() ? ElemType( 0 ) :
                               this->scanKeyWithHash( key, hashVal, i );
    }; 

    ElemType getUsedSlot( unsigned int i ) const {
      if ( ( this->tab[ i ].hashVal & SLOT_USED ) != 0 )
        return this->tab[ i ].elem;
      return ElemType( 0 );
    };

    ElemType getSlot( unsigned int i ) const {
      return this->tab[ i ].elem;
    };

    unsigned int getSlotHash( unsigned int i ) const {
      return this->tab[ i ].hashVal;
    };

    ElemType removeSlot( unsigned int i ) {
      unsigned int j,
                   hashVal;
      ElemType     ptr;

      ptr = this->tab[ i ].elem;
      this->tab[ i ].hashVal = 0;
      for (;;) {
        i = ( i + 1 ) & tabMask;
        if ( ( this->tab[ i ].hashVal & SLOT_USED ) == 0 )
          break;
        hashVal = this->tab[ i ].hashVal;
        j       = hashVal & tabMask;
        if ( i != j ) {
          this->tab[ i ].hashVal = 0;
          while ( ( this->tab[ j ].hashVal & SLOT_USED ) != 0 )
            j = ( j + 1 ) & tabMask;
          this->tab[ j ].elem    = this->tab[ i ].elem;
          this->tab[ j ].hashVal = hashVal;
        }
      }
      this->elemCount--;

      return ptr;
    };

    ElemType removeElem( ValType key ) {
      unsigned int i,
                   hashVal;

      if ( ! this->isEmpty() ) {
        hashVal = this->hash( key ) | SLOT_USED;
        i       = hashVal & tabMask;
        while ( ( this->tab[ i ].hashVal & SLOT_USED ) != 0 ) {
          if ( this->tab[ i ].hashVal == hashVal &&
               this->equals( key, this->tab[ i ].elem ) ) {
            return this->removeSlot( i );
          }
          i = ( i + 1 ) & tabMask;
        }
      }
      return ElemType( 0 );
    };

    ElemType first( unsigned int &i ) {
      i = 0;
      if ( this->isEmpty() )
        return ElemType( 0 );
      return this->nextSlot( i );
    };

    ElemType next( unsigned int &i ) {
      if ( ++i >= this->tabSize )
        return ElemType( 0 );
      return this->nextSlot( i );
    };

    ElemType nextSlot( unsigned int &i ) {
      while ( ( this->tab[ i ].hashVal & SLOT_USED ) == 0 ) {
        if ( ++i == this->tabSize )
          return ElemType( 0 );
      }
      return this->tab[ i ].elem;
    };

    ElemType removeFirst( unsigned int &i ) {
      i = 0;
      if ( this->isEmpty() )
        return ElemType( 0 );
      return removeNext( i );
    };

    ElemType removeNext( unsigned int &i ) {
      if ( ! nextSlot( i ) )
        return ElemType( 0 );
      return this->removeSlot( i );
    };

};
} // namespace rai

#endif
