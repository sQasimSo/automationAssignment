//package <set your test package>;
import com.experitest.client.*;
import org.junit.*;
/**
 *
*/
public class CSVtest {
    private String host = "localhost";
    private int port = 8889;
    private String projectBaseDirectory = "C:\\Users\\karou\\workspace\\project2";
    protected Client client = null;

    @Before
    public void setUp(){
        client = new Client(host, port, true);
        client.setProjectBaseDirectory(projectBaseDirectory);
        client.setReporter("xml", "reports", "CSVtest");
    }

    @Test
    public void testCSVtest(){
        client.setDevice("adb:LGE LG G5");
        client.launch("cloud:com.experitest.eribank/com.experitest.ExperiBank.LoginActivity", true, true);
        client.elementSendText("NATIVE", "UserName", 0, "");
        client.elementSendText("NATIVE", "Password", 0, "");
        client.click("NATIVE", "Login", 0, 1);
        client.verifyElementFound("NATIVE", "Make Payment", 0);
    }

    @After
    public void tearDown(){
        // Generates a report of the test case.
        // For more information - https://docs.experitest.com/display/public/SA/Report+Of+Executed+Test
        client.generateReport(false);
        // Releases the client so that other clients can approach the agent in the near future. 
        client.releaseClient();
    }
}
