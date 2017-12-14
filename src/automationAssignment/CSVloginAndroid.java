package automationAssignment;


import java.io.*;
import java.util.ArrayList;

public class CSVloginAndroid extends BaseTest
{
	public void readCSV(ArrayList<String> usernames, ArrayList<String> passwords) throws IOException
	{
		super.readCSV(usernames, passwords);
	}
	
	public void testAux()
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
	}

	
	private void loginTestAndroid(ArrayList<String> usernames, ArrayList<String> passwords)
	{
		String str0 = client.waitForDevice("@os='android'", 300000);
		if (client.install("cloud:com.experitest.ExperiBank/.LoginActivity", true, true))
		{
			// If statement
		}
		client.launch("com.experitest.ExperiBank/.LoginActivity", true, false);

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
					// If statement
				}
			}
		}
	}
}
