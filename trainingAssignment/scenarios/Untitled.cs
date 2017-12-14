using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using experitestClient;

namespace Experitest
{
    [TestClass]
    public class Untitled
    {
        private string host = "localhost";
        private int port = 8889;
        private string projectBaseDirectory = "C:\\Users\\karou\\workspace\\project2";
        protected Client client = null;
        
        [TestInitialize()]
        public void SetupTest()
        {
            client = new Client(host, port, true);
            client.SetProjectBaseDirectory(projectBaseDirectory);
            client.SetReporter("xml", "reports", "Untitled");
        }

        [TestMethod]
        public void TestUntitled()
        {
            client.SetDevice("adb:LGE LG G5");
            client.Launch("cloud:com.experitest.eribank/com.experitest.ExperiBank.LoginActivity", true, true);
            client.ElementSendText("NATIVE", "UserName", 0, "");
            client.ElementSendText("NATIVE", "Password", 0, "");
            client.Click("NATIVE", "Login", 0, 1);
            client.VerifyElementFound("NATIVE", "Make Payment", 0);
        }

        [TestCleanup()]
        public void TearDown()
        {
        // Generates a report of the test case.
        // For more information - https://docs.experitest.com/display/public/SA/Report+Of+Executed+Test
        client.GenerateReport(false);
        // Releases the client so that other clients can approach the agent in the near future. 
        client.ReleaseClient();
        }
    }
}