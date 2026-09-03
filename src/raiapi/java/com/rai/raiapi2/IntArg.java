package com.rai.raiapi2;

/**
 * Constructs a integer argument for the Args class.
 *
 * <p>Example:
 *
<pre>
IntArg count = new IntArg( "count", 0, "&lt;num&gt;", "Set the count" );
Args a = new Args();
a.add( count, Args.MEM_ARG | Args.COMMAND_ARG | Args.RESOURCE_ARG );

$ java program -count 100
$ java program -count 100k
</pre>
 * 
 * <p>The last sets count = 102400.
 *
 * @see Args
 */
public class IntArg {
  public String name;
  public int    defVal;
  public String example,
                description;
  public IntArg( String n,  int v,  String e,  String d ) {
    this.name        = n;
    this.defVal      = v;
    this.example     = e;
    this.description = d;
  }
};

