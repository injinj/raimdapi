/* Copyright (c) 2003 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#ifndef __rai_util__array_h__
#define __rai_util__array_h__

#ifndef __rai_base__mem_h__
#include "base/mem.h"
#endif

namespace rai {
namespace ArrayErr {
  enum {
    BAD_INDEX = 0
  };
  static inline Error getErr( unsigned int ) {
    static const ErrorRec err = {
      BAD_INDEX, "Array index position out of bounds", "Array" };
    return &err;
  }
}


template <class ElemType>
class Array {
  protected:
    ElemType   * ar;      /* the list of elements */
    unsigned int arSize,  /* the current size of ar[] list */
                 count,   /* the number of elems in ar[] */
                 head,    /* the position of the first element in the list */
                 tail,    /* the position + 1 of the last element in the list */
                 growBy;  /* reallocation increment */

    void grow( void )
    {
      unsigned int i,
                   j,
                   newSize;

      /* allocate growBy space */
      newSize = this->arSize + this->growBy;
      REALLOC( newSize * sizeof( this->ar[ 0 ] ), &this->ar );

      /* copy the elems that wrapped around ar[] to new ar[] spc */
      for ( i = 0, j = this->arSize; i != this->tail; ) {
        this->ar[ j++ ] = this->ar[ i++ ];
        if ( i == this->arSize )
          i = 0;
        if ( j == newSize )
          j = 0;
      }

      this->tail   = j;
      this->arSize = newSize;
    };

  public:
    SYS_OPS( Array );
    Array( unsigned int growBy = 5 )
    {
      this->growBy = growBy;
      this->ar     = NULL;
      this->arSize = 0;
      this->head   = 0;
      this->tail   = 0;
      this->count  = 0;
    };

    virtual ~Array()
    {
      if ( this->arSize > 0 )
        this->clear();
    };

    /* Test if array is empty */
    bool isEmpty()
    {
      return this->count == 0 ? true : false;
    };

    /* Add element to the tail of the array */
    void pushTail( ElemType ptr )
    {
      if ( this->count == this->arSize )
        this->grow();
      this->ar[ this->tail++ ] = ptr;
      if ( this->tail == this->arSize )
        this->tail = 0;
      this->count++;
    };

    /* Add element to the head of the array */
    void pushHead( ElemType ptr )
    {
      if ( this->count == this->arSize )
        this->grow();
      if ( this->head == 0 )
        this->head = this->arSize;
      this->ar[ --this->head ] = ptr;
      this->count++;
    };

    /* Remove head element */
    ElemType popHead( void )
    {
      if ( this->count > 0 ) {
        ElemType p = this->ar[ this->head++ ];
        if ( this->head == this->arSize )
          this->head = 0;
        --this->count;
        return p;
      }

      throw ArrayErr::getErr( ArrayErr::BAD_INDEX );
    };

    /* Remove tail element */
    ElemType popTail( void )
    {
      if ( this->count > 0 ) {
        ElemType p;
        if ( this->tail == 0 )
          this->tail = this->arSize;
        p = this->ar[ --this->tail ];
        --this->count;
        return p;
      }

      throw ArrayErr::getErr( ArrayErr::BAD_INDEX );
    };

    /* Number of elems in array */
    unsigned int length()
    {
      return this->count;
    };

  protected:
    /* Peek at elem at position num */
    ElemType getElem( unsigned int num )
    {
      unsigned int n;

      n = this->head + num;
      if ( n >= this->arSize )
        return this->ar[ n - this->arSize ];
      return this->ar[ n ];
    };

    /* Replace elem at num with ptr, returning old elem */
    ElemType putElem( unsigned int num,  ElemType ptr )
    {
      ElemType   * pos,
                   old;
      unsigned int n;

      n = this->head + num;
      if ( n >= this->arSize )
        pos = &this->ar[ n - this->arSize ];
      else
        pos = &this->ar[ n ];
      old  = *pos;
      *pos = ptr;

      return old;
    };

    /* Insert elem at array[ num ], 0 <= num <= length() */
    void insertElem( ElemType ptr,  unsigned int num )
    {
      unsigned int i,
                   j,
                   pos;

      if ( this->count == this->arSize )
        this->grow();

      pos = this->head + num;
      if ( pos >= this->arSize )
        pos -= this->arSize;
      for ( i = this->tail; i != pos; i = j ) {
        if ( i == 0 )
          j = this->arSize;
        else
          j = i;
        this->ar[ i ] = this->ar[ --j ];
      }

      this->ar[ i ] = ptr;
      if ( ++this->tail == this->arSize )
        this->tail = 0;
      this->count++;
    };

    /* remove array[ num ] returning it, 0 <= num <= length() */
    ElemType removeElem( unsigned int num )
    {
      unsigned int i,
                   j,
                   pos;
      ElemType     old;

      /* shrink array by moving elements after ptr's position to position - 1 */
      pos = this->head + num;
      if ( pos >= this->arSize )
        pos -= this->arSize;
      old = this->ar[ pos ];
      j   = pos;
      i   = pos + 1;
      if ( i == this->arSize )
        i = 0;
      while ( i != this->tail ) {
        this->ar[ j ] = this->ar[ i ];
        j = i;
        if ( ++i == this->arSize )
          i = 0;
      }
      this->tail = j;
      --this->count;

      return old;
    };

  public:
    /* getElem with range checking */
    ElemType get( unsigned int num )
    {
      if ( num >= this->count )
        throw ArrayErr::getErr( ArrayErr::BAD_INDEX );
      return this->getElem( num );
    };

    /* putElem with range checking */
    ElemType put( unsigned int num,  ElemType ptr )
    {
      if ( num >= this->count )
        throw ArrayErr::getErr( ArrayErr::BAD_INDEX );
      return this->putElem( num, ptr );
    };

    /* insertElem with range checking */
    void insert( ElemType ptr,  unsigned int num )
    {
      if ( num > this->count )
        throw ArrayErr::getErr( ArrayErr::BAD_INDEX );
      this->insertElem( ptr, num );
    };

    /* removeElem with range checking ) */
    ElemType remove( unsigned int num )
    {
      if ( num >= this->count )
        throw ArrayErr::getErr( ArrayErr::BAD_INDEX );
      return this->removeElem( num );
    };

    bool getFirst( unsigned int *num,  ElemType *ptr ) 
    {
      if ( this->count == 0 )
        return false;
      *ptr = this->getElem( *num = 0 );
      return true;
    };

    bool getNext( unsigned int *num,  ElemType *ptr ) 
    {
      if ( ++(*num) >= this->count )
        return false;
      *ptr = this->getElem( *num );
      return true;
    };

    ElemType first( unsigned int &num )
    {
      if ( this->count == 0 )
        return (ElemType) 0;
      return this->getElem( num = 0 );
    };

    ElemType next( unsigned int &num )
    {
      if ( ++num >= this->count )
        return (ElemType) 0;
      return this->getElem( num );
    };

    /* Delete element at positions, 0 <= num[ ... ] <= length() */
    void remove( unsigned int *num,  unsigned int numCount )
    {
      unsigned int i,
                   j,
                   k,
                   n,
                   pos;

      if ( numCount == 0 )
        return;

      if ( num[ 0 ] >= this->count )
        throw ArrayErr::getErr( ArrayErr::BAD_INDEX );

      /* shrink array by moving elements after ptr's position to position - 1 */
      pos = this->head + num[ 0 ];
      if ( pos >= this->arSize )
        pos -= this->arSize;
      k   = 1;
      j   = pos;
      i   = pos + 1;
      if ( i == this->arSize )
        i = 0;
      while ( i != this->tail && k < numCount ) {
        n = this->head + num[ k ];
        if ( n >= this->arSize )
          n -= this->arSize;
        if ( i == n )
          k++;
        else
          this->ar[ j++ ] = this->ar[ i ];
        if ( ++i == this->arSize )
          i = 0;
      }
      while ( i != this->tail ) {
        this->ar[ j++ ] = this->ar[ i ];
        if ( ++i == this->arSize )
          i = 0;
      }
      this->tail   = j;
      this->count -= k;

      if ( k != numCount )
        throw ArrayErr::getErr( ArrayErr::BAD_INDEX );
    };

    /* Remove all elements */
    void clear( void )
    {
      if ( this->ar != NULL )
        FREE( this->ar );
      this->ar     = NULL;
      this->arSize = 0;
      this->head   = 0;
      this->tail   = 0;
      this->count  = 0;
    };
};


template <class ElemType>
class QueueArray : public Array<ElemType>
{
  public:
    QueueArray( unsigned int growBy = 5 ) : Array<ElemType>( growBy ) {};

    /* push, queuelike */
    void push( ElemType ptr ) { this->pushTail( ptr ); };

    /* pop, queuelike */
    ElemType pop( void ) { return this->popHead(); };
};


template <class ElemType>
class StackArray : public Array<ElemType>
{
  public:
    StackArray( unsigned int growBy = 5 ) : Array<ElemType>( growBy ) {};

    /* push, stacklike */
    void push( ElemType ptr ) { this->pushTail( ptr ); };

    /* pop, stacklike */
    ElemType pop( void ) { return this->popTail(); };
};


template <class ElemType>
class CompareArray : public Array<ElemType>
{
  protected:
    /* should be overridden by subclass */
    virtual int compare( ElemType el1,  ElemType el2 ) = 0;

    void swap( unsigned int x,  unsigned int y )
    {
      ElemType v;

      v = this->getElem( x );
      this->putElem( x, this->getElem( y ) );
      this->putElem( y, v );
    };

  public:
    CompareArray( unsigned int growBy = 5 ) : Array<ElemType>( growBy ) {};

    void shellSort( void )
    {
      static unsigned int /* from sedgewick */
        incs[] = { 13776, 4592, 1968, 861, 336, 112, 48, 21, 7, 3, 1 };
      unsigned int i,
                   j,
                   k,
                   ink;
      ElemType     v;

      if ( this->count <= 4 ) {
        switch ( this->count ) {
          case 4:
            if ( this->compare( this->getElem( 0 ), this->getElem( 3 ) ) > 0 )
              this->swap( 0, 3 );
            if ( this->compare( this->getElem( 1 ), this->getElem( 3 ) ) > 0 )
              this->swap( 1, 3 );
            if ( this->compare( this->getElem( 2 ), this->getElem( 3 ) ) > 0 )
              this->swap( 2, 3 );
            if ( this->compare( this->getElem( 1 ), this->getElem( 2 ) ) > 0 )
              this->swap( 1, 2 );
          case 3:
            if ( this->compare( this->getElem( 0 ), this->getElem( 2 ) ) > 0 )
              this->swap( 0, 2 );
            if ( this->compare( this->getElem( 1 ), this->getElem( 2 ) ) > 0 )
              this->swap( 1, 2 );
          case 2:
            if ( this->compare( this->getElem( 0 ), this->getElem( 1 ) ) > 0 )
              this->swap( 0, 1 );
            break;
        }
      }
      else {
        for ( k = 0; k < sizeof( incs ) / sizeof( incs[ 0 ] ); k++ ) {
          ink = incs[ k ];
          for ( i = ink; i < this->count; i++ ) {
            if ( this->compare( this->getElem( i ),
                                this->getElem( i - ink ) ) < 0 ) {
              v = this->getElem( i ); 
              j = i;
              do { 
                this->putElem( j, this->getElem( j - ink ) );
                j -= ink; 
              } while ( j >= ink &&
                        this->compare( v, this->getElem( j - ink ) ) < 0 );
              this->putElem( j, v );
            }
          }
        }
      }
    };
};


template <class ElemType>
class HeapSortArray : public CompareArray<ElemType>
{
  protected:
    /* Replace elem at num with ptr */
    void replaceElem( unsigned int num,  ElemType ptr )
    {
      unsigned int pos;

      pos = this->head + num;
      if ( pos >= this->arSize )
        pos -= this->arSize;
      this->ar[ pos ] = ptr;
    };

  public:
    HeapSortArray( unsigned int growBy = 5 )
      : CompareArray<ElemType>( growBy ) {};

    /* push, heapsort like */
    void push( ElemType ptr )
    {
      unsigned int insertPoint,
                   parent;
      ElemType     elem;

      if ( this->count == 0 )
        this->pushTail( ptr );
      else {
        parent = ( this->count + 1 ) / 2 - 1;
        elem   = this->getElem( parent );
        if ( this->compare( ptr, elem ) >= 0 ) {
          this->pushTail( ptr );
        }
        else {
          this->pushTail( elem );

          for ( insertPoint = parent; insertPoint > 0; insertPoint = parent ) {
            parent = ( insertPoint + 1 ) / 2 - 1;
            elem   = this->getElem( parent );
            if ( this->compare( ptr, elem ) >= 0 )
              break;
            this->replaceElem( insertPoint, elem );
          }
          this->replaceElem( insertPoint, ptr );
        }
      }
    };

    /* pop, heapsort like */
    ElemType pop( void )
    {
      ElemType     topElem,
                   bottomElem,
                   childElem1,
                   childElem2;
      unsigned int parent,
                   child;

      bottomElem = this->popTail();
      if ( this->isEmpty() )
        return bottomElem;

      topElem = this->getElem( 0 );
      parent  = 0;
      for ( child = 1; child < this->count; child = ( parent + 1 ) * 2 - 1 ) {
        childElem1 = this->getElem( child );
        if ( child < this->count - 1 ) {
          childElem2 = this->getElem( child + 1 );
          if ( this->compare( childElem1, childElem2 ) >= 0 ) {
            child++;
            childElem1 = childElem2;
          }
        }
        if ( this->compare( childElem1, bottomElem ) >= 0 )
          break;
        this->replaceElem( parent, childElem1 );
        parent = child;
      }
      this->replaceElem( parent, bottomElem );

      return topElem;
    };
};


template <class ElemType>
class SortedArray : public CompareArray<ElemType>
{
  protected:
    bool unique;

  public:
    SortedArray( unsigned int growBy = 5,  bool unique = false ) 
      : CompareArray<ElemType>( growBy ) { this->unique = unique; };

    /* binary sorted insert */
    bool insert( ElemType ptr )
    {
      unsigned int len,
                   size,
                   piv,
                   base,
                   pos;
      ElemType     elem;
      int          cmp;

      len  = this->count;
      size = len;
      piv  = size / 2;
      base = 0;
      cmp  = 0;

      while ( size > 0 ) {
        pos  = base + piv;
        elem = this->getElem( pos );
        cmp  = this->compare( ptr, elem );
        if ( cmp == 0 ) {
          if ( unique )
            return false;
          this->insertElem( ptr, pos );
          return true;
        }
        if ( cmp < 0 ) {
          size = piv;
        }
        else {
          size -= piv + 1;
          base += piv + 1;
        }

        piv = size / 2;
      }

      if ( base == len ) {
        this->pushTail( ptr );
        return true;
      }

      if ( cmp > 0 ) {
        elem = this->getElem( base );
        if ( this->compare( ptr, elem ) > 0 )
          base++;
      }
      this->insertElem( ptr, base );

      return true;
    };

    /* binary search find, returns true if found and index in position */
    bool find( ElemType ptr,  unsigned int *position = NULL )
    {
      unsigned int size,
                   piv,
                   base,
                   pos;
      ElemType     elem;
      int          cmp;

      size = this->count;
      piv  = size / 2;
      base = 0;
      cmp  = 0;

      while ( size > 0 ) {
        pos  = base + piv;
        elem = this->getElem( pos );
        cmp  = this->compare( ptr, elem );
        if ( cmp == 0 ) {
          if ( position != NULL )
            *position = pos;
          return true;
        }
        if ( cmp < 0 ) {
          size = piv;
        }
        else {
          size -= piv + 1;
          base += piv + 1;
        }

        piv = size / 2;
      }

      if ( position != NULL )
        *position = 0xffffffffU;
      return false;
    };
};
} // namespace rai

#endif
