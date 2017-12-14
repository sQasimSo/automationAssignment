package automationAssignment;

import java.io.*;
import java.util.ArrayList;
import java.util.Scanner;

public class CSVlogin extends BaseTest
{
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

		for (int i = 0; i < usernames.size(); i++)
		{
			System.out.println("Username: " + usernames.get(i) + " and Password: " + passwords.get(i));
		}
	}
}
