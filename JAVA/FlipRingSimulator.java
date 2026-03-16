package various_tests;

import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.Font;
import java.awt.FontFormatException;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.GraphicsEnvironment;
import java.awt.RenderingHints;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.geom.AffineTransform;
import java.awt.geom.Ellipse2D;
import java.awt.image.BufferedImage;
import java.awt.image.DataBufferByte;
import java.io.IOException;
import java.io.InputStream;
import java.text.DecimalFormat;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Date;
import javax.swing.JFrame;
import javax.swing.JPanel;
import javax.swing.Timer;

public class FlipRingSimulator extends JPanel implements ActionListener {
	private static final double FPS = 15.0;
	private CircularDisplay display;
	private double dotSize = 2.2;
	private static int xDotSize = 288;
	private static int yDotSize = 7;
	private static int xBufferSize = xDotSize*3;
	private static int yBufferSize = yDotSize;
	private BufferedImage sourceImage = new BufferedImage(xBufferSize, yBufferSize, BufferedImage.TYPE_BYTE_GRAY);
	private static Font font_8 = new Font(Font.MONOSPACED, Font.PLAIN, 5);

	private enum display_type {DIGITALHOURHAND, DIGITALMINUTEHAND, ANALOGHANDS, SCROLLINGDATE, ANIMATEDSINE, SPINNER};
	private display_type displayType = display_type.SPINNER;
	private DecimalFormat formater = new DecimalFormat("00");
	private int animationStep = 0;
	long animationStartMs = System.currentTimeMillis();

	public FlipRingSimulator() {
		setPreferredSize(new Dimension(400, 400));
		setBackground(Color.darkGray);
		setLayout(new BorderLayout());

		display = new CircularDisplay(dotSize); 
		add(display, BorderLayout.CENTER);

		try {
			GraphicsEnvironment ge = GraphicsEnvironment.getLocalGraphicsEnvironment();
			InputStream fontStream = this.getClass().getResourceAsStream("/various_tests/font8");
			font_8 = Font.createFont(Font.TRUETYPE_FONT, fontStream);
			ge.registerFont(font_8);
			font_8 = new Font(font_8.getName(), Font.PLAIN, 8);
		} catch (FontFormatException | IOException e) {
			e.printStackTrace();
		}

		Timer t = new Timer((int) (1000.0/FPS), this);
		t.start();
	}

	private void drawSpinner(Graphics2D gsource) {
		int minBarY = 0;
		int maxBarY = 6; 
		int minBarHeight = 3;
		int maxBarHeight = 8;
		int minBarWidth = 15;
		int maxExtraBarWidth = 60;
		float rotationPeriodMs = 3500.0f; // faster rotation
		float pulsePeriodMs = 2900.0f;    // sharper pulse

		int elapsedMs = (int) (System.currentTimeMillis() - animationStartMs);
		float rotationPhase = (float) (((elapsedMs % rotationPeriodMs) / rotationPeriodMs) * 2.0f * Math.PI);
		float pulsePhase = (float) (((elapsedMs % pulsePeriodMs) / pulsePeriodMs) * 2.0f * Math.PI);

		// Bar width: sharp pulse, more like a "tail"
		float widthPulse = (float) Math.pow((Math.sin(pulsePhase) + 1.0f) * 0.5f, 1.7f); // sharper peak
		int barWidth = (int) (minBarWidth + (widthPulse * maxExtraBarWidth));

		// Bar thickness: also varies, but less pronounced
		float thicknessPulse = (float) Math.pow((Math.sin(pulsePhase + 1.2f) + 1.0f) * 0.5f, 1.3f); // phase offset for variety
		int barHeight = (int) (minBarHeight + (thicknessPulse * (maxBarHeight - minBarHeight)));

		// Center the bar vertically
		int barY = minBarY + ((maxBarY - minBarY) / 2) - (barHeight / 2);

		int barX = (int) (((rotationPhase / (2.0f * Math.PI)) * xDotSize));

		gsource.setColor(Color.white);

		gsource.fillRect(barX, barY, barWidth, barHeight);

	}

	private void drawAnimatedWave(Graphics2D gsource, int animationStep) {

		gsource.setColor(Color.white);

		double yOffset = (System.currentTimeMillis()/4000.0)%1.0;
		yOffset *= 2*Math.PI;
		yOffset = (1.0 + Math.sin(yOffset))/2.0;


		for (int x = 0; x < xDotSize; x++) {
			double phi = ((double)(x+animationStep)/xDotSize)*24.0*Math.PI;
			int y = (int)( 4.0*yOffset+((3.0*yOffset)*Math.sin(phi)) );
			y += 7*yOffset;
			gsource.drawLine(x, 7, x, y);
		}

	}

	private void drawDigitalMinuteHand(Graphics2D gsource, int animationStep) {
		Calendar rightNow = Calendar.getInstance();
		int minute = rightNow.get(Calendar.MINUTE);
		int hour = rightNow.get(Calendar.HOUR_OF_DAY);
		//		hour = 10;
		//		minute = 50;

		int xMinutePosition = (int) ((((hour%12)/12.0)*(xDotSize)) + ((minute/60.0)*(14.0)));
		String text = formater.format(minute);

		AffineTransform affineTransform = new AffineTransform();
		affineTransform.rotate(Math.PI, 0, 0);
		Font rotatedFont = font_8.deriveFont(affineTransform);

		gsource.setColor(Color.white);

		if (hour%12 >=3 && hour%12 < 9) {
			gsource.setFont(rotatedFont);
			gsource.drawString(text, xMinutePosition+10, -5);
		} else {
			gsource.setFont(font_8);
			gsource.drawString(text, xMinutePosition, 12);
		}

		if (hour >= 12 && hour < 24) {
			gsource.fillRect(0, 2, xMinutePosition-3, 3);
		} else {
			gsource.fillRect(xMinutePosition+14, 2, xDotSize - (xMinutePosition+14) , 3);
		}
	}

	private void drawDigitalHourHand(Graphics2D gsource, int animationStep) {
		Calendar rightNow = Calendar.getInstance();
		int minute = rightNow.get(Calendar.MINUTE);
		int hour = rightNow.get(Calendar.HOUR_OF_DAY);
		int secOfHour = rightNow.get(Calendar.SECOND) + minute*60;

		int xHourPosition = (int) ((secOfHour/3600.0)*(xDotSize));
		if (xHourPosition > xDotSize-10) xHourPosition = xDotSize-10;
		String text = hour+"";
		gsource.setColor(Color.white);
		gsource.setFont(font_8);
		gsource.drawString(text, xHourPosition, 12);
	}

	private void drawAnalogHands(Graphics2D gsource, int animationStep) {

		Calendar rightNow = Calendar.getInstance();
		int minute = rightNow.get(Calendar.MINUTE);
		int hour = rightNow.get(Calendar.HOUR);
		int seconds = rightNow.get(Calendar.SECOND);
		int milli = rightNow.get(Calendar.MILLISECOND);

		//		animationStep += 2*60*40;

		//		int hour = ((int) (animationStep/(60*60)))%12;
		//		int minute = ((int) (animationStep/(60)))%60;
		//		int seconds = (animationStep)%60;
		//		int milli = 0;

		gsource.setColor(Color.white);

		int xHourPosition = (int) ((((hour*60)+minute)/(12.0*60.0))*(xDotSize));
		int xMinutePosition = (int) (((((minute*60.0)+seconds)/(60.0*60.0)) *(xDotSize)) );
		int xSecondPosition = (int) ((((seconds*1000.0)+milli) / (60.0*1000.0) ) *xDotSize);

		xHourPosition+=xDotSize;
		xMinutePosition+=xDotSize;
		xSecondPosition+=xDotSize;

		gsource.fillRect(xSecondPosition, 0, 1, 1);
		gsource.fillRect(xMinutePosition-1, 1, 3, 6);
		gsource.fillRect(xHourPosition-3, 5, 7, 2);

	}

	private void drawScrollingDate(Graphics2D gsource, int animationStep) {

		Date d = new Date();
		SimpleDateFormat ft1 = new SimpleDateFormat ("EEEEE dd MMMMM YYYY");
		String textDate = ft1.format(d);
		SimpleDateFormat ft2 = new SimpleDateFormat ("HH:mm");
		String textTime = ft2.format(d);

		gsource.setColor(Color.white);
		gsource.setFont(font_8);
		gsource.drawString(textDate, (animationStep%(xBufferSize+xDotSize))-(xDotSize-10)+100, 12);
		gsource.drawString(textTime, (animationStep%(xBufferSize+xDotSize))-(xDotSize-10), 12);
	}


	private void drawStuff(Graphics2D gsource, int animationStep) {
		gsource.setColor(Color.black);
		gsource.fillRect(0, 0, xBufferSize, yBufferSize);

		switch (displayType) {
		case DIGITALHOURHAND:
			drawDigitalHourHand(gsource, animationStep);
			break;

		case DIGITALMINUTEHAND:
			drawDigitalMinuteHand(gsource, animationStep);
			break;

		case ANALOGHANDS:
			drawAnalogHands(gsource, animationStep);
			break;

		case SCROLLINGDATE:
			drawScrollingDate(gsource, animationStep);
			break;

		case ANIMATEDSINE:
			drawAnimatedWave(gsource, animationStep);
			break;

		case SPINNER:
			drawSpinner(gsource);
			break;

		default:
			break;
		}

	}

	@Override
	public void actionPerformed(ActionEvent e) {
		animationStep++;
		Graphics2D gsource = sourceImage.createGraphics();
		gsource.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_OFF);
		drawStuff(gsource, animationStep);
		gsource.dispose();
		sourceImageToDisplay();
	}

	private void sourceImageToDisplay() {
		int x, y;
		byte[] sourcePixels = ((DataBufferByte)sourceImage.getRaster().getDataBuffer()).getData();

		display.clearDotMatrixWith(false);
		for (int i = 0; i < sourcePixels.length; i++) {
			if (sourcePixels[i] != 0) {
				x = i%xBufferSize;
				y = i/xBufferSize;
				display.setDotState(x, y, true);
			}
		}
	}

	private class CircularDisplay extends JPanel {
		private static final long serialVersionUID = 1L;
		private double dotSize;
		private double innerRadius;
		double xOffset;
		double yOffset;
		private double[][] xDotCoordinates = new double[xDotSize][yDotSize];
		private double[][] yDotCoordinates = new double[xDotSize][yDotSize];
		private boolean[][] dotStates = new boolean[xDotSize][yDotSize];

		private double[][] xTickCoordinates = new double[12][2];
		private double[][] yTickCoordinates = new double[12][2];

		public CircularDisplay(double dotSize) {
			this.dotSize = dotSize;
			innerRadius = dotSize*((xDotSize*1.3)/(Math.PI*2));
			xOffset = ((dotSize*16)+(innerRadius));
			yOffset = ((dotSize*16)+(innerRadius));
			setOpaque(true);
			setPreferredSize(new Dimension(800, 800));
			computeDotsCoordinates();
		}

		public void clearDotMatrixWith(boolean state) {
			for (int i = 0; i < xDotSize; i++) {
				for (int j = 0; j < yDotSize; j++) {
					dotStates[i][j] = state;
				}
			}
		}

		public void setDotState(int x, int y, boolean state) {
			x = x%xDotSize;
			y = y%yDotSize;
			dotStates[x][y] = state;
			repaint();
		}

		private void computeDotsCoordinates() {
			double x, y;
			double phase, amplitude;

			// dots coordinate
			for (int i = 0; i < xDotSize; i++) {
				for (int j = 0; j < yDotSize; j++) {

					phase = 2.0*Math.PI*(i/(double)xDotSize);
					phase += Math.PI;
					amplitude = innerRadius+ ((dotSize+(dotSize*0.3))*(yDotSize-1-j));

					x =  (-amplitude * Math.sin(phase)) ;
					y =  (amplitude * Math.cos(phase)) ;
					x += xOffset;
					y += yOffset;

					xDotCoordinates[i][j] = x;
					yDotCoordinates[i][j] = y;
				}
			}

			// clock ticks coordinate
			for (int i = 0; i < 12; i++) {

				phase = 2.0*Math.PI*(i/(double)12);
				phase += Math.PI;
				amplitude = innerRadius + (dotSize*13);

				x = (-amplitude * Math.sin(phase)) ;
				y = (amplitude * Math.cos(phase)) ;
				x += xOffset;
				y += yOffset;

				xTickCoordinates[i][0] = x;
				yTickCoordinates[i][0] = y;

				amplitude = innerRadius - (dotSize*5.5);

				x = (-amplitude * Math.sin(phase)) ;
				y = (amplitude * Math.cos(phase)) ;
				x += xOffset;
				y += yOffset;

				xTickCoordinates[i][1] = x;
				yTickCoordinates[i][1] = y;

			}
		}

		@Override
		public void paintComponent(Graphics g) {
			Graphics2D g2 = (Graphics2D)(g);
			g2.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
			g2.setColor(Color.lightGray);
			g2.fillRect(0, 0, getWidth(), getHeight());

			g2.setColor(Color.darkGray);
			double radius = (innerRadius+(dotSize*14));
			Ellipse2D.Double clockFrame = new Ellipse2D.Double(xOffset-radius, yOffset-radius, radius*2, radius*2);
			g2.fill(clockFrame);

			g2.setColor(Color.lightGray);
			radius = (innerRadius-(dotSize*6.5));
			clockFrame = new Ellipse2D.Double(xOffset-radius, yOffset-radius, radius*2, radius*2);
			g2.fill(clockFrame);

			g2.setColor(Color.white);
			for (int i = 0; i < 12; i++) {
				Ellipse2D.Double tick = new Ellipse2D.Double(xTickCoordinates[i][0]-(dotSize), yTickCoordinates[i][0]-(dotSize), (dotSize*2), (dotSize*2));
				g2.fill(tick);
			}
			for (int i = 0; i < 12; i++) {
				Ellipse2D.Double tick = new Ellipse2D.Double(xTickCoordinates[i][1]-dotSize, yTickCoordinates[i][1]-dotSize, (dotSize*2), (dotSize*2));
				g2.fill(tick);
			}

			for (int i = 0; i < xDotSize; i++) {
				for (int j = 0; j < yDotSize; j++) {
					if(dotStates[i][j]) g2.setColor(Color.yellow);
					else g2.setColor(Color.black);

					Ellipse2D.Double dot = new Ellipse2D.Double(xDotCoordinates[i][j]-(dotSize/2), yDotCoordinates[i][j]-(dotSize/2), dotSize, dotSize);
					g2.fill(dot);
				}

			}


		}

	}

	public static void main(String[] args) {

		JFrame frame = new JFrame("Flip-Ring");
		frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		frame.setSize(350, 370);
		frame.setBackground(Color.black);
		frame.add(new FlipRingSimulator());
		frame.setVisible(true);

	}

}
