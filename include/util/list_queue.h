/* Copyright (c) 2011 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#ifndef __rai_util__list_queue_h__
#define __rai_util__list_queue_h__

namespace rai {
template <class LIST>
struct SListQueue {
  LIST * hd, * tl;

  SListQueue() : hd( 0 ), tl( 0 ) {}
  void init( void ) {
    this->hd = this->tl = NULL;
  }
  bool isEmpty( void ) {
    return this->hd == NULL;
  }
  void moveTo( LIST *&list ) {
    list = this->hd;
    this->init();
  }
  void moveTo( SListQueue<LIST> &q ) {
    q.hd = this->hd; q.tl = this->tl;
    this->init();
  }
  void appendList( SListQueue<LIST> &q ) {
    if ( q.hd != NULL ) {
      if ( this->tl == NULL )
        this->hd = q.hd;
      else
        this->tl->next = q.hd;
      this->tl = q.tl;
      q.init();
    }
  }
  void pushHead( LIST *p ) {
    p->next = this->hd;
    if ( this->hd == NULL )
      this->tl = p;
    this->hd = p;
  }
  void pushTail( LIST *p ) {
    if ( this->tl == NULL )
      this->hd = p;
    else
      this->tl->next = p;
    this->tl = p;
    p->next = NULL;
  }
  LIST *pop( LIST *p ) {
    if ( this->hd == p )
      return this->pop();
    for ( LIST *x = this->hd; ; ) {
      if ( x->next == NULL )
        return NULL;
      if ( x->next == p ) {
        x->next = p->next;
        break;
      }
    }
    p->next = NULL;
    return p;
  }
  LIST *pop( void ) {
    if ( this->hd == NULL )
      return NULL;
    LIST *p = this->hd;
    if ( (this->hd = (LIST *) p->next) == NULL )
      this->tl = NULL;
    p->next = NULL;
    return p;
  }
  unsigned int count( void ) {
    unsigned int cnt = 0;
    LIST *p = this->hd;
    for ( ; p != NULL; p = p->next )
      cnt++;
    return cnt;
  }
};

template <class LIST>
struct DListQueue {
  LIST * hd, * tl;

  DListQueue() : hd( 0 ), tl( 0 ) {}
  void init( void ) {
    this->hd = this->tl = NULL;
  }
  bool isEmpty( void ) {
    return this->hd == NULL;
  }
  void moveTo( LIST *&list ) {
    list = this->hd;
    this->init();
  }
  void moveTo( DListQueue<LIST> &q ) {
    q.hd = this->hd; q.tl = this->tl;
    this->init();
  }
  void pushHead( LIST *p ) {
    p->next = this->hd;
    p->back = NULL;
    if ( this->hd == NULL )
      this->tl = p;
    else
      this->hd->back = p;
    this->hd = p;
  }
  void pushTail( LIST *p ) {
    if ( this->tl == NULL )
      this->hd = p;
    else
      this->tl->next = p;
    p->back = this->tl;
    this->tl = p;
    p->next = NULL;
  }
  LIST *pop( LIST *p ) {
    if ( p->back == NULL )
      this->hd = (LIST *) p->next;
    else
      p->back->next = p->next;
    if ( p->next == NULL )
      this->tl = (LIST *) p->back;
    else
      p->next->back = p->back;
    p->next = p->back = NULL;
    return p;
  }
  LIST *pop( void ) {
    if ( this->hd == NULL )
      return NULL;
    LIST *p = this->hd;
    if ( (this->hd = (LIST *) p->next) == NULL )
      this->tl = NULL;
    else
      p->next->back = NULL;
    p->next = p->back = NULL;
    return p;
  }
  void insertBefore( LIST *p,  LIST *el ) {
    if ( p == NULL || p == this->hd )
      this->pushHead( el );
    else {
      el->next = p;  el->back       = p->back;
      p->back  = el; el->back->next = el;
    }
  }
  void insertAfter( LIST *p,  LIST *el ) {
    if ( p == NULL || p == this->tl )
      this->pushTail( el );
    else {
      el->next = p->next; el->back       = p;
      p->next  = el;      el->next->back = el;
    }
  }
  unsigned int count( void ) {
    unsigned int cnt = 0;
    LIST *p = this->hd;
    for ( ; p != NULL; p = p->next )
      cnt++;
    return cnt;
  }
};
}
#endif
