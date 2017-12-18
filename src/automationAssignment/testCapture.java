package automationAssignment;

import java.io.File;
import java.nio.file.Files;
import static java.nio.file.StandardCopyOption.*;
import java.io.IOException;
import java.nio.file.Path;

public class testCapture extends BaseTest {
	String deviceQuery = "@os='android'";
	private String zone = "web";
	private String agreement = "xpath=//*[@text='I agree' and @nodeName='DIV']";
	private int timeout = 100000;

	public String testAux() {
		String str0 = client.waitForDevice("@os='android'", 300000);
		reportsPath = System.getProperty("user.dir") + "\\testReports\\RUN_" + System.currentTimeMillis() + "\\"
				+ str0.split(":")[1];
		client.startLoggingDevice(reportsPath);
		System.out.println(reportsPath);
		client.openDevice();

		try {
			new File(reportsPath).mkdir();
		} catch (Exception e) {
			e.printStackTrace();
		}

		client.setReporter("xml", reportsPath, getTestName());

		client.setDevice("adb:samsung SM-N7505");
		String str5 = client.capture("Capture");
		client.sleep(5000);
		Path psource = (Path) new File(str0).toPath();
		Path pdest = (Path) new File(reportsPath + "Device reflection at the beginning of test.png").toPath();
		try {
			Files.copy(psource, pdest, REPLACE_EXISTING);
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}

		client.stopLoggingDevice();

		return this.status;
	}
}
