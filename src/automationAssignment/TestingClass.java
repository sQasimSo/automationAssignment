package automationAssignment;

import java.io.File;

public class TestingClass extends BaseTest
{
	String deviceQuery = "@os='android'";
	private String zone = "web";
	private String agreement = "xpath=//*[@text='I agree' and @nodeName='DIV']";
	private int timeout = 100000;
	
	public String testAux()
	{
		String str0 = client.waitForDevice("@os='android'", 300000);
		reportsPath = System.getProperty("user.dir") + "\\testReports\\RUN_" +System.currentTimeMillis() + "\\" + str0.split(":")[1];
		client.startLoggingDevice(reportsPath);
		System.out.println(reportsPath);
		client.openDevice();
		
		try
		{
			new File(reportsPath).mkdir();
		}
		catch (Exception e)
		{
			e.printStackTrace();
		}
		
		client.setReporter("xml", reportsPath, getTestName());
		
		client.launch("http://www.cnn.com", true, true);

		String element = "xpath=//*[@id='menu']";
		client.waitForElement(this.zone, element, 0, 12 * this.timeout);
		client.click(this.zone, element, 0, 1);
		
		if (client.isElementFound(this.zone, this.agreement))
		{
			client.click(zone, agreement, 0, 1);
		}
		
		String str2 = client.hybridRunJavascript("", 0, "history.go(-1);");
		client.hybridClearCache(true, true);
		
		System.out.println(client.getCounter("memory"));
		System.out.println(client.getCounter("cpu"));
		System.out.println(client.getCounter("battery"));

		client.report("This is a custom message inserted into the test report. It will be marked there as \"succeeded\"", true);
		
		this.status = "succeeded";
		
		
		client.stopLoggingDevice();
		
		return this.status;
	}
}
