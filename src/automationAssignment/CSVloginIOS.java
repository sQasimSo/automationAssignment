package automationAssignment;

import java.io.IOException;
import java.util.ArrayList;

public class CSVloginIOS extends BaseTest
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

		loginTestIOS(usernames, passwords);
	}

	private void loginTestIOS(ArrayList<String> usernames, ArrayList<String> passwords)
	{
		String str0 = client.waitForDevice("@os='ios'", 300000);
		if (client.install("cloud:com.experitest.ExperiBank", true, true))
		{
			// If statement
		}

		client.launch("com.experitest.ExperiBank", true, false);

		for (int i = 0; i < usernames.size(); i++)
		{
			client.elementSendText("CSVloginIOS", "Username", 0, usernames.get(i));
			client.elementSendText("CSVloginIOS", "Password", 0, passwords.get(i));
			client.click("CSVloginIOS", "Login", 0, 1);

			if (client.isElementFound("CSVloginIOS", "Dismiss", 0))
			{
				client.click("CSVloginIOS", "Dismiss", 0, 1);
			} else
			{
				if (client.isElementFound("CSVloginIOS", "Logout", 0))
				{
					// If statement
				}
			}
		}
	}
}
