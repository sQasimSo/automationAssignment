package automationAssignment;

public class timerTEst extends BaseTest
{

	public void testAux()
	{
		System.out.println(System.currentTimeMillis());
		try
		{
			Thread.sleep(2000);
		} catch (InterruptedException e)
		{
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
}
