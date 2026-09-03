package com.rai.raiapi2;

/**
 * Constructs a boolean argument for the Args class.
 *
 * <p>Example:
 *
<pre>
BoolArg flag = new BoolArg( "flag", false, null, "Sets a flag" );
Args a = new Args();
a.add( flag );

$ java program -flag false
$ java program -flag true
$ java program -flag
</pre>
 * <p>The last example would cause flag = true.
 * 
 * @see Args
 */
public class BoolArg {
  public String  name;
  public boolean defVal;
  public String  example,
                 description;
  public BoolArg( String n,  boolean v,  String e,  String d ) {
    this.name        = n;
    this.defVal      = v;
    this.example     = e;
    this.description = d;
  }
};

