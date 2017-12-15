package automationAssignment;

import com.experitest.client.Client;

public class customClient extends Client
{
	public customClient(String host, int port, boolean useSessionID)
	{
		super(host, port, true);
	}

	@Override
	public String generateReport(boolean releaseClient)
	{
		String text = super.generateReport(releaseClient);
		System.out.println(String.format("generateReport(%s) > %s", releaseClient, text));
		return text;
	}
}