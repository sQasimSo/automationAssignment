package automationAssignment;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Scanner;

import org.testng.ITestContext;
import org.testng.ITestListener;
import org.testng.ITestResult;
import org.testng.annotations.*;
import org.testng.annotations.BeforeMethod;
import com.experitest.client.*;

public class BaseTest
{
	protected String host = "localhost";
	protected int port = 8889;
	protected String projectBaseDirectory = System.getProperty("user.dir") + "\\trainingAssignment";
	protected customClient client = null;
	protected String reportsPath;
	protected String status = "failed";
	public long startTime;
	public long endTime;
	private int repetitionsCount = 1;
	String testingReportsDir = "C:\\Users\\karou\\Desktop\\testsReport";
	ITestListener testListener;

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

	public void setupTests()
	{
		client = new customClient(host, port, true);
		client.setProjectBaseDirectory(projectBaseDirectory);
		client.setReporter("xml", "reports", "unicode");
	}

	@BeforeMethod
	public void setUp()
	{
		setupTests();
	}

	public String getTestName()
	{
		Class<?> enclosingClass = getClass().getEnclosingClass();
		if (enclosingClass != null)
		{
			return enclosingClass.getName().split(".")[1];
		} else
		{
			return getClass().getName();
		}
	}

	public String testAux()
	{
		System.out.println("testAux was not overriden correctly!");
		return "failed";
	}

	@Test
	@Parameters(
	{ "timeInMillis", "timerON", "threadsCount" })
	public void test(long timeInMillis, String timerON, int threadsCount)
	{
		while (repetitionsCount < 4)
		{
			this.startTime = System.currentTimeMillis();
			this.endTime = startTime + timeInMillis;

			if (timerON.equals("true"))
			{
				while (System.currentTimeMillis() < endTime)
				{
					this.status = testAux();
				}
			} else
			{
				this.status = testAux();
			}

			if (this.status.equals("succeeded") || this.repetitionsCount == 3)
				this.repetitionsCount = 4;
			else if (this.status.equals("failed"))
			{
				if (this.repetitionsCount == 1)
					this.repetitionsCount++;
				else if (this.repetitionsCount == 2)
				{
					client.reboot(120000);
					this.repetitionsCount++;
				}
			}
		}
	}

	@AfterMethod
	public void tearDown()
	{
		// client.setReporter("xml",
		// "C:\\Users\\karou\\Desktop\\testsReport\\Run_" + this.startTime ,
		// "testingTheReporter");
		// Generates a report of the test case.
		// For more information -
		// https://docs.experitest.com/display/public/SA/Report+Of+Executed+Test
		String report = client.generateReport(false);
		System.out.println(report);
		// Releases the client so that other clients can approach the agent in
		// the near future.
		client.releaseClient();
	}
}
