package com.rai.raiapi;

import java.util.*;
import com.tibco.tibrv.*;
import com.rai.raimsg.*;
import java.lang.*;
import java.io.*;
import java.net.*;
import java.util.*;

public class TcpDispatcher extends Thread
{
 byte [] buf = new byte[1048576]; //how big can a message be? 65536 isn't enough
 Socket skt;
 DataInputStream is;
 char newLine = '\n';
 char space = ' ';
 RaiSession session;
 //public ArrayList TCPPendingSubjects = Collections.synchronizedArrayList(new ArrayList());

 public TcpDispatcher(RaiSession theSession,Socket tskt,DataInputStream tis)
 {
	 session = theSession;
	 skt = tskt;
	 is = tis;
 }

protected synchronized void
doNotify()
{
	notifyAll(); // let 'em know we've started up
}
public void run()
{
	try
	{	if (RaiApi.logLevel >= RaiApi.Trace )
			System.out.println("TcpDispatcher:run started");
		boolean done = false;
		int c=0;
		while(!done)
		{
			try
			{   int counter = 0;
				byte b= 0;
				//while (true)
				//{
					if (session.TCPPendingSubjects.size() > 0)
					{
						String theString = (String) session.TCPPendingSubjects.get(0);
						session.TcpSend(theString);
						session.TCPPendingSubjects.remove(0);
					}


				//	if (is.available() >0)
				//	{
						while (true)
						{
							b= (byte) is.read(buf,counter,1);
							if (buf[counter++]==space)
								break;

						}
				//		break;
				//	} else
				//	{
				//  		Thread.sleep(50);
				//  		System.out.println(c++);
				//	}
				//}
				String PUB_KIND = new String(buf,0,counter-1);
				counter = 0;
				b= 0;
				while (true)
				{
					b= (byte) is.read(buf,counter,1);
					if (buf[counter++]==space)
						break;
				}
				String SUBJECT = new String(buf,0,counter-1);
				if (RaiApi.logLevel >= RaiApi.Debug )
					System.out.println("TCP Receive " + SUBJECT);

				counter = 0;
				b= 0;
				while (true)
				{
					b= (byte) is.read(buf,counter,1);
					if (buf[counter++]==newLine)
						break;
				}
				String NBYTES_st = new String(buf,0,counter-1);
				if (RaiApi.logLevel >= RaiApi.Trace )
					System.out.println("Received tcp callback " + PUB_KIND + " " + SUBJECT + " " + NBYTES_st );

				int NBYTES = Integer.parseInt(NBYTES_st);

				int cumulativeCount = 0;
				while (cumulativeCount != NBYTES)
				{
					int theCount = is.read(buf,cumulativeCount,NBYTES-cumulativeCount);
					//if (RaiApi.logLevel >= RaiApi.Full )
					//	System.out.println("Message retrieved buffer size" + theCount + " requested is " + NBYTES);
					cumulativeCount = cumulativeCount + theCount;
				}
				RaiMsg raiMsg = new RaiMsg();
				raiMsg.UnPack(buf,0,NBYTES);
				session.ReturnMessage(raiMsg,SUBJECT,PUB_KIND);

			}
			// Not sure what to do here since I can't throw an exception because the thread class doesn't
			catch ( RaiMsgException e ) {
			    System.out.println("caught a RaiMsg exception");
			}
			catch ( RaiException e ) {
			    System.out.println("caught a RaiAPI exception");
  			}
		//	catch(Exception e)
		//	{
				//System.out.println("An Exception Occurred: " +e.ToString());
			//	if (RaiApi.logLevel >= RaiApi.Major )
		//			System.out.println("Lost connection to remote session " + session.theHost+":"+ session.thePort);
						//lostConnection();
		//		return;
		//	}
		}
	}
	catch(Exception e)
	{
		RaiException re = new RaiException(RaiException.UNKNOWN,e);
		System.out.println("Lost connection to remote session " + session.theHost+":"+ session.thePort);
		//throw (re);
	}
}


}
