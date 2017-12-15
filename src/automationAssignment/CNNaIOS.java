package automationAssignment;

import java.io.File;

import com.experitest.client.Client;

public class CNNaIOS extends BaseTest
{
	String deviceQuery = "@os='ios'";
	private String zone = "web";
	private String agreement = "xpath=//*[@text='I agree']";
	private int timeout = 100000;

	public String testAux()
	{
		String str0 = client.waitForDevice(deviceQuery, 300000);
		reportsPath = System.getProperty("user.dir") + "\\testReports\\RUN_" + System.currentTimeMillis() + "\\"
				+ str0.split(":")[1];
		System.out.println(reportsPath);

		try
		{
			new File(reportsPath).mkdir();
		} catch (Exception e)
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

		TestMenu(client, "xpath=//*[@text='U.S.']"); // U.S.
		TestMenu(client, "xpath=//*[@text='World']"); // World
		TestMenu(client, "xpath=//*[@href='/politics']"); // politics
		TestMenu(client, "xpath=//*[@href='//money.cnn.com']"); // Money
		TestMenu(client, "xpath=//*[@text='Opinion']"); // Opinion
		TestMenu(client, "xpath=//*[@text='Health']"); // Health
		TestMenu(client, "xpath=//*[@href='/entertainment']"); // entertainment
		TestMenu(client, "xpath=//*[@text='Tech']"); // tech
		TestMenu(client, "xpath=//*[@href='/style']"); // STYLE
		TestMenu(client, "xpath=//*[@text='Travel']"); // Travel
		TestMenu(client, "xpath=//*[@href='//bleacherreport.com']"); // bleacher
		TestMenu(client, "xpath=//*[@text='Living']"); // Living
		TestMenu(client, "xpath=//*[@text='Video']"); // Video
		TestMenu(client, "xpath=//*[@href='/vr']"); // VR
		TestMenu(client, "xpath=//*[@text='More…']"); // More…
		client.hybridClearCache(true, true);
		
		this.status = "succeeded";
		
		return this.status;
	}

	protected void TestMenu(Client client, String menuElement)
	{
		if (!client.isElementFound(this.zone, menuElement))
		{
			client.swipeWhileNotFound("Down", 1000, 2000, this.zone, menuElement, 0, 1000, 1, false);
		}
		// TODO:: verify if element exists. ask Navot if not existing means
		// failing the test
	}
}
