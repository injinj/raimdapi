package com.rai.raiapi2;

/** RaiDataLossEvent is the object for notifying dataloss events.
 * <p>Example of capr/TCP connection loss:
 * <p><pre>
 * description: Connection to address tcp/192.168.4.45:8866, transport tcp/192.168.4.254:40817 &lt;-&gt; tcp/192.168.4.45:8866, reconnecting in 3.0 seconds
 * transportName: tcp/192.168.4.254:40817 &lt;-&gt; tcp/192.168.4.45:8866
 * inboundPacketLoss: 0
 * outboundPacketLoss: 0
 * connectionCount: 0
 * connetionLoss: true
 * isMulticast: false
 * </pre>
 * <p>Example of capr/PGMOUDP packet loss:
 * <p><pre>
 * description: Connection to multicast address pgmoudp/192.168.4.254;225.5.5.5:41888;225.5.5.5:7555, transport 192.168.4.254:63757 &lt;-&gt; 225.5.5.5:7555, host at 192.168.4.45, inbound 74846 packets lost
 * transportName: 192.168.4.254:63757 &lt;-&gt; 225.5.5.5:7555
 * inboundPacketLoss: 74846
 * outboundPacketLoss: 0
 * connectionCount: 1
 * connetionLoss: false
 * isMulticast: true
 * </pre>
 * <p>Example of TibRV packet loss:
 * <p><pre>
 * description: _RV.ERROR.SYSTEM.DATALOSS.INBOUND.BCAST = {ADV_CLASS="ERROR" ADV_SOURCE="SYSTEM" ADV_NAME="DATALOSS.INBOUND.BCAST" ADV_DESC="dataloss: remote daemon already timed out the data" host="192.168.4.45" lost=473 scid=7445 } (net=192.168.4.0;224.4.4.5, svc=7445)
 * transportName: net=192.168.4.0;224.4.4.5, svc=7445
 * inboundPacketLoss: 473
 * outboundPacketLoss: 0
 * connectionCount: 1
 * connetionLoss: false
 * isMulticast: true
 * </pre>
 * @see RaiSession#SetDataLossCB
 */
public class RaiDataLossEvent {
  /** The session in which the data loss event was generated. */
  public RaiSession session;
  /** The transport name that loss occured.  This will be a string that
   * describes the transport address.  It can be empty if there isn't a
   * transport. */
  public String     transportName;
  /** A description of the type of data loss.  This will be a string that
   * describes what kind off loss occured. */
  public String     description;
  /** A count of recv pkts lost, if available.  If the number of inbound packets
   * that were lost is available, this will have the count. */
  public long       inboundPacketLoss;
  /** A count of sent pkts lost, if available.  If the number of outbound
   * packets that were lost is available, this will have the count. */
  public long       outboundPacketLoss;
  /** A count of active connections.  The number of connections will normally be
   * one, except in the case of the "capr", which has the ability to manage
   * multiple fault-tolerant connections. */
  public long       connectionCount;
  /** If connection oriented, this will be true.  If the underlying transport
   * needs to be reconnected, as is the case with TCP, then this is true */
  public boolean    connectionLoss;
  /** If true then multicast connection, if false point to point connection. */
  public boolean    isMulticast;
  /** Internally used to construct an event for the callback
   * @see RaiSession#SetDataLossCB */
  protected RaiDataLossEvent( RaiSession s,  String tran,  String descr,
                              long ibl, long obl, long cnt,
                              boolean conn, boolean ismc ) {
    this.session            = s;
    this.transportName      = tran;
    this.description        = descr;
    this.inboundPacketLoss  = ibl;
    this.outboundPacketLoss = obl;
    this.connectionCount    = cnt;
    this.connectionLoss     = conn;
    this.isMulticast        = ismc;
  }
}
