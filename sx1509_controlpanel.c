
	/// THIS IS A CODE SNIPPET - NOT MEANT TO BE USED DIRECTLY

	#include <SparkFunSX1509.h> 	// SparkFun SX1509 library

	// Define SX1509 parameters
	const byte SX1509_ADDRESS = 0x3E;  // SX1509 I2C address
	SX1509 io; // Create an SX1509 object to be used throughout
	const int SX1509_INT_PIN = 1;
	
	const int numCols = 7;
	const int numRows = 8;

	
	/////////////////////////
	/// SETUP
	/////////////////////////

	// Initialize the SX1509 I/O expander
	if (!io.begin(SX1509_ADDRESS)) while (1) ;

	// SX1509 Keypad engine settings

	// After a set number of milliseconds, the keypad engine 
	// will go into a low-current sleep mode.
	// Sleep time range: 128 ms - 8192 ms (powers of 2) 0=OFF
	unsigned int sleepTime = 8192;

	// Scan time defines the number of milliseconds devoted to
	// each row in the matrix.
	// Scan time range: 1-128 ms, powers of 2
	byte scanTime = 8; // Scan time per row, in ms

	// Debounce sets the minimum amount of time that must pass
	// before a button can be pressed again.
	// Debounce time range: 0.5 - 64 ms (powers of 2)
	byte debTime = 8; // Debounce time
	
	// Initialize SX1509 Keypad engine
	io.keypad(numRows, numCols, sleepTime, scanTime, debTime);


	
	/////////////////////
	/// BUTTON MATRIX
	/////////////////////
	
	/*
	Functionality
		Physical:	SX1509 I/O expander board with a built-in button keypad engine
					Communicates to the MCU via I2C bus
					Connected to the matrix are three position toggles and momentary buttons
					Arranged in a 7-by-8 matrix of rows and columns - total of 56 inputs						
		Virtual:	Each frame the SX1509 is queried for the current keypad (matrix) state					
					Each unique matrix column-row index is associated with a controller button
	
	Variables
		unsigned int keyData		- intermittent storage for keypad state read from the SX1509 engine
									  contains both row and col state bits - 8 bits each, 16 bits total
		byte rowByte				- 8 bits of the row state extracted from keyData
		byte colByte				- 8 bits of the column state extracted from keyData
		byte row					- decimal row number extracted from rowByte
		byte col					- decimal column number extracted from colByte
		const int numCols						- constant, number of connected columns
		const int numRows						- constant, number of connected rows
		int timer_Matrix[numCols][numRows]		- two-dimensional array of run-down timers for each input
		const int matrixJoy[numCols][numRows]	- constant, two-dimensional array of virtual controller buttons
												  activated by the associated Col-Row index of an input
		
	*/
	
	// Read the interrupt pin on the SX1509 - goes LOW when there is new data availible
	if (digitalRead(SX1509_INT_PIN) == LOW) {
		
		// Read matrix state from the SX1509 "keypad engine"
		// Returns a two byte long binary sequence - one bit for each of the 16 pins on the SX1509
		keyData = io.readKeypad();
	  
		// Split the two byte return into a pair of 8-bit row and column states
		byte rowByte = keyData & 0x00FF;
		byte colByte = (keyData & 0xFF00) >> 8;
	  
		// If both the row and col has a bit set (i.e. value above zero), some key is pressed
		// If both are zeros, nothing is pressed
		// Also, a condition exists were io.readKeypad() only returns a single set bit - missing either col or row information
		// That is not a valid return and is to be ignored
		// If multiple buttons are pressed on the same row, colByte will contain multiple bits set
		// If multiple buttons are pressed on different row, that state will be sent on the next frame
		// In other words, at any one time keyData will only contain the state of a single ROW and all of it's COLS
		if (rowByte && colByte) 
		{    
 
			if (DEBUG_OUTPUT && DEBUG_OUTPUT_SX1509_PACKET_DETAILS) Serial.println(">>>> KEYDATA: [" + String(keyData, BIN) + "] ");
			
			// "log(BINARY_VAL)/log(2)" will return a DECIMAL position of most significant bit that is set HIGH
			// i.e. given "00010000" - it will return 4		
			byte row = log(rowByte)/log(2);

			// Loop while colByte is not zero (i.e. has a bit set)
			while (colByte) {
				
				// extract the position of the most significant set bit
				byte col = log(colByte)/log(2);
				
				// remove the detected bit from colByte
				// if there was only one set bit in colByte to start, it will now be equal zero
				// if there were multiple set bits, they will be processed (and removed) on the next loop
				colByte -= (1 << col);
				
				// Press the joystick button only if this is the first time it's been detected pressed recently
				if (timer_Matrix[col][row] == 0) {
					if (DEBUG_OUTPUT) Serial.println("BTN PRESSED: " + String(col) + " / " + String(row)+ " #" + String(matrixJoy[col][row]));
					Joystick.pressButton(matrixJoy[col][row]);    
				}
				
				// Reset the time to release timer
				timer_Matrix[col][row] = momentaryTime + 100;
			}
		}
	}

	// Cycle though the matrix and process the timers, if there are any running
	// A timer_Matrix above zero indicates that the joystick button is PRESSED
	// Once the timer runs out, RELEASE the joystick button
	for (int mtx_c = 0; mtx_c < numCols; mtx_c++) {
		for (int mtx_r = 0; mtx_r < numRows; mtx_r++) {
			
			if (timer_Matrix[mtx_c][mtx_r] > 0) {
				timer_Matrix[mtx_c][mtx_r] -= frameTime;
				
				if (timer_Matrix[mtx_c][mtx_r] <= 0) {
					timer_Matrix[mtx_c][mtx_r] = 0;  
					Joystick.releaseButton(matrixJoy[mtx_c][mtx_r]);
					
					if (DEBUG_OUTPUT) Serial.println("BTN RELEASED: " + String(mtx_c) + " / " + String(mtx_r)+ " #" + String(matrixJoy[mtx_c][mtx_r]));
				}      
			}
		}
	}
