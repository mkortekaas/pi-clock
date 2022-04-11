// -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
//
// Highly modified but based on port to C++ for Unreal Engine by Jorge Valle Hurtado - byValle
// Which is based on: http://www.tannerhelland.com/4435/convert-temperature-rgb-algorithm-code/
// See end of file
//

#include <math.h>
#include <bits/stdc++.h>

// You can go down a rabbit hole with color temps. I would recommend reading:
//   https://www.macworld.com/article/227655/ios-93-the-new-night-shift-feature-probably-wont-help-you-sleep-better.html
//   https://www.phonearena.com/news/iOS-9.3s-Night-Shift-explored-what-is-it-how-to-enable-and-manage-it_id77419
// The latter has the magic table (repeated here) that could be used to control a display
//          Setting                              Color temperature
//          Night Shift off                      7448K
//          Night Shift on, coolest setting      6395K
//          Night Shift on, average setting      5415K
//          Night Shift on, warmest setting      3026K
//
// There's also this; should you really want to go learn more:
//   https://www.waveformlighting.com/color-matching/what-is-d65-and-what-is-it-used-for

uint32_t k_tempratures[] = {3026, 5415, 6395, 7448}; // warm to cool - see above

void k_to_rgb(uint32_t temperature, uint8_t *r, uint8_t *g, uint8_t *b)
{
	float new_r, new_g, new_b;

	if (temperature <= 6600) {
		new_r = 255.0;
		new_g = 99.4708025861 * log(float(temperature)/100.0) - 161.1195681661;
		new_b = 255.0;
	} else {
		new_r = 329.698727446 * pow(float(temperature)/100.0 - 60.0, -0.1332047592);
		new_g = 288.1221695283 * pow(float(temperature)/100.0 - 60.0, -0.0755148492);
		if (temperature <= 1900) {
			new_b = 0.0;
		} else {
			new_b = 138.5177312231 * log(float(temperature)/100.0 - 10.0) - 305.0447927307;
		}
	}

	*r = (new_r < 0.0) ? 0 : ((new_r > 255.0) ? 255 : int(new_r));
	*g = (new_g < 0.0) ? 0 : ((new_g > 255.0) ? 255 : int(new_g));
	*b = (new_b < 0.0) ? 0 : ((new_b > 255.0) ? 255 : int(new_b));
}

//
//    Original pseudo code:
//    
//    Set Temperature = Temperature \ 100
//    
//    Calculate Red:
//
//    If Temperature <= 66 Then
//        Red = 255
//    Else
//        Red = Temperature - 60
//        Red = 329.698727446 * (Red ^ -0.1332047592)
//        If Red < 0 Then Red = 0
//        If Red > 255 Then Red = 255
//    End If
//    
//    Calculate Green:
//
//    If Temperature <= 66 Then
//        Green = Temperature
//        Green = 99.4708025861 * Ln(Green) - 161.1195681661
//        If Green < 0 Then Green = 0
//        If Green > 255 Then Green = 255
//    Else
//        Green = Temperature - 60
//        Green = 288.1221695283 * (Green ^ -0.0755148492)
//        If Green < 0 Then Green = 0
//        If Green > 255 Then Green = 255
//    End If
//    
//    Calculate Blue:
//
//    If Temperature >= 66 Then
//        Blue = 255
//    Else
//
//        If Temperature <= 19 Then
//            Blue = 0
//        Else
//            Blue = Temperature - 10
//            Blue = 138.5177312231 * Ln(Blue) - 305.0447927307
//            If Blue < 0 Then Blue = 0
//            If Blue > 255 Then Blue = 255
//        End If
//
//    End If
//
