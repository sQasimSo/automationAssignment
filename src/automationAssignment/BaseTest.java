package automationAssignment;

//package <set your test package>;
import com.experitest.client.*;
import org.junit.*;

public class BaseTest
{
	private String host = "localhost";
	private int port = 8889;
	private String projectBaseDirectory = "\\automationAssignment";
	protected Client client = null;

	@Before
	public void setUp()
	{
		/*client = new Client(host, port, true);
		client.setProjectBaseDirectory(projectBaseDirectory);
		client.setReporter("xml", "reports", "Untitled");*/
		System.out.println("test starting");
	}

	
	public void testAux()
	{
		String str0 = client.waitForDevice("@os='android'", 300000);
		client.launch("http://www.cnn.com", true, false);
		client.deviceAction("Back");	
	}
	
	@Test
	public void test()
	{
		testAux();
	}

	@After
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
