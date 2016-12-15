#include "mbed.h"
#include "DRV2605.h"

DRV2605 rightLRA(p9, p10);
DRV2605 leftLRA(p28, p27);
DigitalOut myled(LED1);
Serial bt(p13, p14);
Serial pc(USBTX, USBRX);

//
// Plays a haptic waveform with specified delay and order
//
void simpleWaveform(int leftEffect, int rightEffect, int delay, bool rightToLeft = false)
{
  leftLRA.load_waveform_sequence(leftEffect);
  rightLRA.load_waveform_sequence(rightEffect);

  if(rightToLeft)
  {
    rightLRA.play();
    wait_ms(delay);
    leftLRA.play();
  }
  else
  {
    leftLRA.play();
    wait_ms(delay);
    rightLRA.play();
  }
}

//
// Invokes both LRA effects at once
//
void invokeBoth(int leftEffect, int rightEffect)
{
  rightLRA.load_waveform_sequence(rightEffect);
  leftLRA.load_waveform_sequence(leftEffect);

  rightLRA.play();
  leftLRA.play();
}

int main()
{
    int mediumIntensity = 0;
    bool charReceived = false;
    int timeSpacingOffset = 0;
    char readChar;
    bool continuousMode = false;
    bool continuousMode2 = false;
    long lastInvocation = 0;

    // Daignostics Routine
    printf("Diagnostics Result: %X\n", leftLRA.diagnostics());
    printf("Diagnostics Result: %X\n", rightLRA.diagnostics());

    // Initialization Procedure as outlined in Section 9.3 of Device Datasheet
    printf("Calibration Result: %X\n",leftLRA.init(3));
    printf("Calibration Result: %X\n",rightLRA.init(3));

    leftLRA.useLRA();
    rightLRA.useLRA();

    // Daignostics Routine
    printf("Diagnostics Result: %X\n", leftLRA.diagnostics());
    printf("Diagnostics Result: %X\n", rightLRA.diagnostics());

    // Play some waveforms on startup
    leftLRA.load_waveform_sequence(72);
    rightLRA.load_waveform_sequence(84);
    leftLRA.play();
    wait_ms(400);
    rightLRA.play();

    // Say hello to ensure our method of communication is working
    pc.printf("Hello World!\n");
    bt.printf("Hello World!\n");

    // Verify that initial offset between page-flip effects is 0
    pc.printf("Time spacing offset: %d ms\n", timeSpacingOffset);
    bt.printf("Time spacing offset: %d ms\n", timeSpacingOffset);

    while (true) {
        // Get a character from either PC or BT. If both are sending data,
        // the PC input will be handled first
        charReceived = false;
        if(pc.readable())
        {
          readChar = (char)pc.getc();
          charReceived = true;
        }
        else if(bt.readable())
        {
          readChar = (char)bt.getc();
          charReceived = true;
        }

        // Handle the simple command received
        if(charReceived)
        {
          // Helper commands
          if(readChar == 'l')
          {
            // Play left
            leftLRA.play();
          }
          else if(readChar == 'r')
          {
            // Play right
            rightLRA.play();
          }
          else if(readChar == '+')
          {
            // Increase spacing
            timeSpacingOffset += 5;
            pc.printf("Time spacing offset: %d ms\n", timeSpacingOffset);
          }
          else if(readChar == '-')
          {
            // Decrease spacing
            timeSpacingOffset -= 5;
            pc.printf("Time spacing offset: %d ms\n", timeSpacingOffset);
          }
          else if(readChar == 'm')
          {
            // In the effects table, each "medium" strength waveform
            // is the "high" strength + 24
            mediumIntensity = (mediumIntensity == 0) ? 24 : 0;
          }

          // Commands for page flipping
          else if(readChar == 'c')
          {
            // Click both LRAs
            leftLRA.load_waveform_sequence(1);
            rightLRA.load_waveform_sequence(1);
            leftLRA.play();
            rightLRA.play();
          }
          else if(readChar == 't')
          {
            // Tick both LRAs
            leftLRA.load_waveform_sequence(17);
            rightLRA.load_waveform_sequence(17);
            leftLRA.play();
            rightLRA.play();
          }
          else if(readChar == 'x')
          {
            // Tick both LRAs
            leftLRA.load_waveform_sequence(19);
            rightLRA.load_waveform_sequence(19);
            leftLRA.play();
            rightLRA.play();
          }
          else if(readChar == 'z')
          {
            // Tick both LRAs
            leftLRA.load_waveform_sequence(23);
            rightLRA.load_waveform_sequence(23);
            leftLRA.play();
            rightLRA.play();
          }
          else if(readChar == 'x')
          {
            // Tick both LRAs
            leftLRA.load_waveform_sequence(25);
            rightLRA.load_waveform_sequence(25);
            leftLRA.play();
            rightLRA.play();
          }
          else if(readChar == 'z')
          {
            // Tick both LRAs
            leftLRA.load_waveform_sequence(26);
            rightLRA.load_waveform_sequence(26);
            leftLRA.play();
            rightLRA.play();
          }

          else if(readChar == '1')
          {
            // Simple back swipe
            int timeSpace = 250;
            pc.printf("Time spacing: %d ms\n", timeSpace + timeSpacingOffset);

            simpleWaveform(84 + mediumIntensity, 72 + mediumIntensity, timeSpace + timeSpacingOffset);
          }
          else if(readChar == '2')
          {
            // Simple forward swipe
            int timeSpace = 250;
            pc.printf("Time spacing: %d ms\n", timeSpace + timeSpacingOffset);

            simpleWaveform(72 + mediumIntensity, 84 + mediumIntensity, timeSpace + timeSpacingOffset, true);
          }

          else if(readChar == '3')
          {
            // Simple back swipe short
            int timeSpace = 230;
            pc.printf("Time spacing: %d ms\n", timeSpace + timeSpacingOffset);

            simpleWaveform(86 + mediumIntensity, 74 + mediumIntensity, timeSpace + timeSpacingOffset);
          }
          else if(readChar == '4')
          {
            // Simple forward swipe short
            int timeSpace = 230;
            pc.printf("Time spacing: %d ms\n", timeSpace + timeSpacingOffset);

            simpleWaveform(74 + mediumIntensity, 86 + mediumIntensity, timeSpace + timeSpacingOffset, true);
          }

          else if(readChar == '5')
          {
            // Complex back swipe short sharp
            int timeSpace = 400;
            pc.printf("Time spacing: %d ms\n", timeSpace + timeSpacingOffset);
            pc.printf("Complex waveform: separate ramp-up bubbles. Left to right\n");

            leftLRA.load_waveform_sequence(92 + mediumIntensity, 80 + mediumIntensity);
            rightLRA.load_waveform_sequence(92 + mediumIntensity, 80 + mediumIntensity);

            leftLRA.play();
            wait_ms(timeSpace + timeSpacingOffset);
            rightLRA.play();
          }
          else if(readChar == '6')
          {
            // Complex forward swipe short sharp
            int timeSpace = 400;
            pc.printf("Time spacing: %d ms\n", timeSpace + timeSpacingOffset);
            pc.printf("Complex waveform: separate ramp-up bubbles. Right to left\n");

            leftLRA.load_waveform_sequence(92 + mediumIntensity, 80 + mediumIntensity);
            rightLRA.load_waveform_sequence(92 + mediumIntensity, 80 + mediumIntensity);

            rightLRA.play();
            wait_ms(timeSpace + timeSpacingOffset);
            leftLRA.play();
          }

          else if(readChar == '7')
          {
            // Simple back swipe short sharp
            int timeSpace = 500;
            pc.printf("Time spacing: %d ms\n", timeSpace + timeSpacingOffset);

            simpleWaveform(92 + mediumIntensity, 80 + mediumIntensity, timeSpace + timeSpacingOffset);
          }
          else if(readChar == '8')
          {
            // Simple forward swipe short sharp
            int timeSpace = 500;
            pc.printf("Time spacing: %d ms\n", timeSpace + timeSpacingOffset);

            simpleWaveform(80 + mediumIntensity, 92 + mediumIntensity, timeSpace + timeSpacingOffset, true);
          }

          else if(readChar == '9')
          {
            // Fast swipe
            int timeSpace = 100;
            pc.printf("Time spacing: %d ms\n", timeSpace + timeSpacingOffset);

            simpleWaveform(25, 25, timeSpacingOffset + timeSpace);
          }
          else if(readChar == '0')
          {
            // Fast swipe
            int timeSpace = 100;
            pc.printf("Time spacing: %d ms\n", timeSpace + timeSpacingOffset);

            simpleWaveform(25, 25, timeSpace + timeSpacingOffset, true);
          }

          // Haptic mouse commands
          else if(readChar == 'a')
          {
        	// Medium click, strong click
            int timeSpace = 120;
            pc.printf("Time spacing: %d ms\n", timeSpace + timeSpacingOffset);

            simpleWaveform(22, 1, timeSpace + timeSpacingOffset);
          }
          else if(readChar == 'w')
          {
        	// Medium click, strong click
            int timeSpace = 120;
            pc.printf("Time spacing: %d ms\n", timeSpace + timeSpacingOffset);

            simpleWaveform(1, 22, timeSpace + timeSpacingOffset, true);
          }

          else if(readChar == 's')
          {
        	// Soft bumps
            int timeSpace = 115;
            pc.printf("Time spacing: %d ms\n", timeSpace + timeSpacingOffset);

            simpleWaveform(7, 7, timeSpacingOffset + timeSpace);
          }
          else if(readChar == 'd')
          {
        	// Soft bumps
            int timeSpace = 115;
            pc.printf("Time spacing: %d ms\n", timeSpace + timeSpacingOffset);

            simpleWaveform(7, 7, timeSpace + timeSpacingOffset, true);
          }
          else if(readChar == 'Z')
          {
        	// Soft bumps
            pc.printf("Left bump, 100%\n");

            leftLRA.play_waveform(7);
          }
          else if(readChar == 'X')
          {
        	// Soft bumps
            pc.printf("Right bump, 100%\n");

            rightLRA.play_waveform(7);
          }
          else if(readChar == 'A')
          {
        	// Soft bumps
            pc.printf("Left bump, 30%\n");

            leftLRA.play_waveform(9);
          }
          else if(readChar == 'W')
          {
        	// Soft bumps
            pc.printf("Right bump, 30%\n");

            rightLRA.play_waveform(9);
          }

          // Commands for drag-and-drop demo
          else if(readChar == 'B')
          {
            // Buzz
            continuousMode = true;
          }
          else if(readChar == 'V')
          {
            // Buzz
            continuousMode2 = true;
          }
          else if(readChar == 'C')
          {
            // Sharp Click
            invokeBoth(4, 4);
          }
          else if(readChar == 'D')
          {
            // Double click
            invokeBoth(10, 10);
          }
          else if(readChar == 'T')
          {
            // Triple click
            invokeBoth(12, 12);
          }
          else if(readChar == 'S')
          {
            // Stop the buzz
            continuousMode = false;
            continuousMode2 = false;
          }
        }

        // Handle the continuous buzz mode in the while loop scope
        if(continuousMode || continuousMode2)
        {
          long currentTime = us_ticker_read();
          if(currentTime - lastInvocation > 200000) // 200ms
          {
        	  if(continuousMode)
        	  {
        		  // Invoke the buzz
				  invokeBoth(48, 48); // Buzz intensity 80%
        	  }
        	  if(continuousMode2)
			  {
				  // Invoke the buzz
				  invokeBoth(51, 51); // Buzz intensity 20%
			  }
        	  lastInvocation = currentTime;
          }
        }
    }
}
