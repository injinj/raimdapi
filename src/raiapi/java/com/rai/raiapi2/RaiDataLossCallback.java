package com.rai.raiapi2;

/** RaiDataLossCallback is the interface for notifying dataloss events.
 * @see RaiSession#SetDataLossCB
 */
public interface RaiDataLossCallback {
  /** The dataloss callback.
   * @see RaiSession#SetDataLossCB */
  public void onDataLoss( RaiDataLossEvent event,  Object closure );
  /** The connection callback.
   * @see RaiSession#SetDataLossCB */
  public void onConnection( RaiConnectionEvent event,  Object closure );
}
