package automationAssignment;


import java.io.*;
import java.util.ArrayList;

public class CSVloginAndroid extends BaseTest
{	
	public void readCSV(ArrayList<String> usernames, ArrayList<String> passwords) throws IOException
	{
		super.readCSV(usernames, passwords);
	}
	
	public String testAux()
	{
		ArrayList<String> usernames = new ArrayList<String>();
		ArrayList<String> passwords = new ArrayList<String>();
		try
		{
			readCSV(usernames, passwords);
		} catch (IOException e)
		{
			e.printStackTrace();
		}
		
		loginTestAndroid(usernames, passwords);
		
		return status;
	}

	
	private void loginTestAndroid(ArrayList<String> usernames, ArrayList<String> passwords)
	{
		String str0 = client.waitForDevice("@os='android'", 300000);
		reportsPath = System.getProperty("user.dir") + "\\testReports\\RUN_" +System.currentTimeMillis() + "\\" + str0.split(":")[1];
		client.startLoggingDevice(reportsPath);
		System.out.println(reportsPath);
		client.openDevice();
		client.startMonitor("com.experitest.ExperiBank");

		try
		{
			new File(reportsPath).mkdir();
		}
		catch (Exception e)
		{
			e.printStackTrace();
		}
		
		client.setReporter("xml", reportsPath, getTestName());
		
		if (client.install("cloud:com.experitest.ExperiBank/.LoginActivity", true, true))
		{
			// If statement
		}
		
		System.out.println(this.status);
		
		client.launch("com.experitest.ExperiBank/.LoginActivity", true, true);


		System.out.println(this.status);
		
		for (int i = 0; i < usernames.size(); i++)
		{
			client.elementSendText("CSVlogin", "Username", 0, usernames.get(i));
			client.elementSendText("CSVlogin", "Password", 0, passwords.get(i));
			client.click("NATIVE", "xpath=//*[@text='Login']", 0, 1);

			if (client.isElementFound("CSVlogin", "Close", 0))
			{
				client.click("CSVlogin", "Close", 0, 1);
			} else
			{
				if (client.isElementFound("CSVlogin", "Logout", 0))
				{
					this.status = "succeeded";
					String str1 = client.hybridGetHtml("id=balanceWebView", 0);
					
				}
			}
		}
		
		client.applicationClearData("com.experitest.ExperiBank/.LoginActivity");
		
		if (client.uninstall("cloud:com.experitest.ExperiBank/.LoginActivity"))
		{
			// If statement
		}
		
		client.sleep(10000);
		String str4 = client.getMonitorsData("");
		client.stopLoggingDevice();
	}
}
