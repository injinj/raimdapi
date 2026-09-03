package com.rai.raiapi2;

/**
 * Constructs a string argument for the Args class.
 *
 * <p>Example:
 *
<pre>
StringArg subject = new StringArg( "subject", null, "&lt;string&gt;", "Subject to subscribe" );
Args a = new Args();
a.add( subject );
</pre>
 * @see Args
 */
public class StringArg {
  public String name,
                defVal,
                example,
                description;
  public StringArg( String n,  String v,  String e,  String d ) {
    this.name        = n;
    this.defVal      = v;
    this.example     = e;
    this.description = d;
  }
};

