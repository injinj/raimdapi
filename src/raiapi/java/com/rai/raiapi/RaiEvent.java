package com.rai.raiapi;
import com.rai.raimsg.*;

public class RaiEvent {
  public RaiSession  session = null;
  public String      receivedSubject = null;
  public String      subscribedSubject = null;

  public
  RaiEvent(RaiSession session) {
    this.session = session;
  }

  protected void AddSession (RaiSession session) {
    this.session = session;
  }

  protected void AddReceivedSubject (String subject) {
    this.receivedSubject = subject;
  }

  protected void AddSubscriptionSubject (String subject) {
    this.subscribedSubject = subject;
  }
}
