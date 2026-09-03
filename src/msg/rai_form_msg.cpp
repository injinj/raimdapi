/* Copyright (c) 2011 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */
#if ! defined( RAIMSG_DLL_EXP ) && defined( RAI_DLL )
#define RAIMSG_DLL_EXP __declspec(dllexport)
#endif

#include "msg/rai_form_msg.h"
#include "stream/io_stream.h"

using namespace rai;

void
RaiFormMsg::ConvertGet( RaiFormPtr &fld,  RaiMsg_data fdata,  RaiMsg_type ftype,
                        RaiMsg_size fsize )
{
  RaiField field;
  fld.entry->unpack( field, &this->buf[ fld.foff ], fld.fsize );
  field.Get( ftype, fsize, fdata );
}


void
RaiFormMsg::ConvertAppend( const RaiMsg_dict *f,  RaiMsg_data fdata,
                           RaiMsg_type ftype,
                           RaiMsg_size fsize )
{
  RaiField field;
  field.Update( NULL, ftype, fsize, fdata );
  f->pack( field, &this->buf[ this->bufOff ] );
}


void
RaiFormMsg::ConvertAppend( const RaiMsg_dict *f,  RaiFormPtr &fld )

{
  RaiField field;
  fld.entry->unpack( field, &fld.msg->buf[ fld.foff ], fld.fsize );
  f->pack( field, &this->buf[ this->bufOff ] );
}


void
RaiFormMsg::Print( rai::OutputStream *out )
{
  RaiMsg msg;
  msg.UnPack( TIB_SASS_PROTO, this->Packed(), this->PackSize(),
              RAIMSG_MEMORY_STATIC );
  msg.Print( out );
}

