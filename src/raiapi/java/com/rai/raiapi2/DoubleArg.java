package com.rai.raiapi2;

/**
 * Constructs a floating point double argument for the Args class.
 *
 * <p>Example:
 *
<pre>
DoubleArg time = new DoubleArg( "time", 0, "&lt;secs&gt;", "Set the time" );
Args a = new Args();
a.add( time, Args.TIME_SEC_ARG | Args.COMMAND_ARG | Args.RESOURCE_ARG );

$ java program -time 1 ms
$ java program -time 1 minute
$ java program -time 1.5 hours
</pre>
 * 
 * The first would set the time = 0.0010 seconds.
 * The next would set the time = 60 seconds.
 * The last would set the time = 5400 seconds.
 *
 * @see Args
 */
public class DoubleArg {
  public String name;
  public double defVal;
  public String example,
                description;
  public DoubleArg( String n,  double v,  String e,  String d ) {
    this.name        = n;
    this.defVal      = v;
    this.example     = e;
    this.description = d;
  }
};

