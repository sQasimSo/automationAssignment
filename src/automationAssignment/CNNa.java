package automationAssignment;

public class CNNa extends BaseTest
{
	String deviceQuery = "@os='android'";
	
	public void testAux()
	{
		String str0 = client.waitForDevice(deviceQuery, 300000);
		client.launch("http://www.facebook.com", true, false);
		client.deviceAction("Back");
	}
}
