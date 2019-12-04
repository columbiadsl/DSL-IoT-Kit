/* LEDPin.h

 	Copyright (c) 2019 Jeff Gregorio. All rights reserved.

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef LEDPIN_H
#define LEDPIN_H

#include "Arduino.h"

#define LP_HIGH (LOW)
#define LP_LOW (HIGH)

class LEDPin {

public:

	LEDPin(int digitalpin, int blink_duration) 
	: pin(digitalpin), t0(0), dt(blink_duration) {}

	void blink() {
		digitalWrite(pin, LP_LOW);
		t0 = millis();
	}

	void loop() {
		if (t0 && ((millis() - t0) > dt)) {
			digitalWrite(pin, LP_HIGH);
			t0 = 0;
		}
	}

protected:

	int pin;
	int t0;
	int dt;
};

#endif
