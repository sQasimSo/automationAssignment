package automationAssignment;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Scanner;

import org.testng.annotations.*;
import org.testng.annotations.BeforeMethod;
import com.experitest.client.*;

public class BaseTest
{
	private String host = "localhost";
	private int port = 8889;
	private String projectBaseDirectory = System.getProperty("user.dir") + "\\trainingAssignment";
	protected Client client = null;

	public void readCSV(ArrayList<String> usernames, ArrayList<String> passwords) throws IOException
	{
		String csvFilePath = System.getProperty("user.dir") + "/src/sources/csvfile.csv";
		String line = "";
		Scanner inputStream;

		String csvSplitBy = ",";

		try
		{
			inputStream = new Scanner(new File(csvFilePath));
			inputStream.nextLine();

			while (inputStream.hasNextLine())
			{
				line = inputStream.nextLine();

				if (line.equals(""))
				{
					System.out.println("end of file");
				} else
				{
					String[] credentials = line.split(csvSplitBy);
					if (credentials[0].equals(""))
						usernames.add(" ");
					else
						usernames.add(credentials[0]);

					if (credentials[1].equals(""))
						passwords.add(" ");
					else
						passwords.add(credentials[1]);
				}
			}
		} catch (FileNotFoundException e)
		{
			e.printStackTrace();
		}
	}

	@BeforeMethod
	public void setUp()
	{
		System.out.println(projectBaseDirectory);
		client = new Client(host, port, true);
		client.setProjectBaseDirectory(projectBaseDirectory);
		client.setReporter("xml", "reports", "Untitled");
	}

	public void testAux()
	{
		System.out.println("testAux was not overriden correctly!");
	}

	@Test
	@Parameters(
	{ "timeInMillis", "timerON" })
	public void test(long timeInMillis, String timerON)
	{
		long startTime = System.currentTimeMillis();
		long endTime = startTime + timeInMillis;

		if (timerON.equals("true"))
		{
			while (System.currentTimeMillis() < endTime)
				testAux();
		}
		else
		{
			testAux();
		}
	}

	@AfterMethod
	public void tearDown()
	{
		// Generates a report of the test case.
		// For more information -
		// https://docs.experitest.com/display/public/SA/Report+Of+Executed+Test
		client.generateReport(false);
		// Releases the client so that other clients can approach the agent in
		// the near future.
		client.releaseClient();
	}
}
