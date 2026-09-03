package com.rai.raiapi2;

/** RaiConnectionEvent is the object for notifying connection events.
 * <p>Example of capr/TCP connection event:
 * <p><pre>
 * description: Connection to address tcp/192.168.4.45:8866, transport tcp/192.168.4.254:40817 &lt;-&gt; tcp/192.168.4.45:8866, reconnecting in 3.0 seconds
 * transportName: tcp/192.168.4.254:40817 &lt;-&gt; tcp/192.168.4.45:8866
 * connectionCount: 1
 * connetionOriented: true
 * isMulticast: false
 * </pre>
 * <p>Example of capr/PGMOUDP packet event:
 * <p><pre>
 * description: Connection to multicast address pgmoudp/192.168.4.254;225.5.5.5:41888;225.5.5.5:7555, transport 192.168.4.254:63757 &lt;-&gt; 225.5.5.5:7555, host at 192.168.4.45, inbound 74846 packets lost
 * transportName: 192.168.4.254:63757 &lt;-&gt; 225.5.5.5:7555
 * connectionCount: 1
 * connetionOriented: false
 * isMulticast: true
 * </pre>
 * @see RaiSession#SetDataLossCB
 */
public class RaiConnectionEvent {
  /** The session in which the data loss event was generated. */
  public RaiSession session;
  /** The transport name that loss occured.  This will be a string that
   * describes the transport address.  It can be empty if there isn't a
   * transport. */
  public String     transportName;
  /** A description of the type of data loss.  This will be a string that
   * describes what kind off loss occured. */
  public String     description;
  /** A count of active connections.  The number of connections will normally be
   * one, except in the case of the "capr", which has the ability to manage
   * multiple fault-tolerant connections. */
  public long       connectionCount;
  /** If connection oriented, this will be true.  If the underlying transport
   * needs to be reconnected, as is the case with TCP, then this is true */
  public boolean    connectionOriented;
  /** If true then multicast connection, if false point to point connection. */
  public boolean    isMulticast;
  /** Internally used to construct an event for the callback
   * @see RaiSession#SetDataLossCB */
  protected RaiConnectionEvent( RaiSession s,  String tran,  String descr,
                                long cnt, boolean conn, boolean ismc ) {
    this.session            = s;
    this.transportName      = tran;
    this.description        = descr;
    this.connectionCount    = cnt;
    this.connectionOriented = conn;
    this.isMulticast        = ismc;
  }
}
