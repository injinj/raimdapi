package com.rai.raimsg;

/** Partial is a field string or opaque type which partially updates a field,
 * it has an offset and a length. */
public class Partial {
  byte [] data;
  int     offset;

  public Partial( byte [] data,  int offset ) {
    this.data   = data;
    this.offset = offset;
  }

  public String toString() {
    return "partial off=" + offset + " len=" + data.length;
  }

  public byte [] getData() {
    return this.data;
  }

  public int getOffset() {
    return this.offset;
  }
};
