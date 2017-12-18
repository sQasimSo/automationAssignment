package resources;

import java.awt.image.BufferedImage;
import java.io.File;
import java.io.IOException;
import java.lang.System;
import java.lang.Thread;

import javax.imageio.ImageIO;

import com.experitest.client.Client;
import com.rational.test.ft.script.RationalTestScript;

public class SeeTestScript extends RationalTestScript {
	protected Client client;
	
	/**
	 * The logSeeTestCommand takes the client last command and send it result to the RFT log
	 */
	public void logSeeTestCommand(){
		if(client == null){
			client = new Client("localhost", 8889);
		}
		
		/*
		 * Read the last command status
		 */
		boolean stepPass = (Boolean)client.getLastCommandResultMap().get("status");
		String logLine = (String)(client.getLastCommandResultMap().get("logLine"));
		String outFile = (String)(client.getLastCommandResultMap().get("outFile"));
		
		/*
		 * Read the last command image file (if exists)
		 */
		BufferedImage image = null;
		if(outFile != null && !outFile.isEmpty()){
			File imageFile = new File(outFile);
            waitForImage(imageFile);
			if(imageFile.exists()){
				try {
					image = convertImage(ImageIO.read(imageFile));
				} catch (IOException e) {
					e.printStackTrace();
				}
			}
		}
		
		if(stepPass){
			logInfo(logLine, image);
		} else {
			logError(logLine, image);
		}
	}

    private void waitForImage(File imageFile) {
        long start = System.currentTimeMillis();
        while(!imageFile.exists() &&
                System.currentTimeMillis() - start < 5000){
            try {
                Thread.sleep(100);
            }catch(Exception ignored){ }
        }
    }

    private static BufferedImage convertImage(BufferedImage img){
		BufferedImage newImage = new BufferedImage(img.getWidth(), img.getHeight(), BufferedImage.TYPE_INT_RGB);
		for(int y = 0; y < img.getHeight(); y++){
			for(int x = 0; x < img.getWidth(); x++){
				newImage.setRGB(x, y, img.getRGB(x, y));
			}
		}
		return newImage;
	}
}