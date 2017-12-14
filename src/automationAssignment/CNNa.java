package automationAssignment;

import org.testng.annotations.Test;

public class CNNa extends BaseTest
{
	String deviceQuery = "@os='android'";
	
	@Test
	public void testAux()
	{
		String str0 = client.waitForDevice(deviceQuery, 300000);
		client.launch("http://www.facebook.com", true, false);
		client.deviceAction("Back");
	}
}
