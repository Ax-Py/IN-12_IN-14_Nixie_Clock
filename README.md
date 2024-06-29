# IN-12_IN-14_Nixie_Clock

Nixie tube clock that can accept any tube that has a maximum current draw of 2.5mA. All digits are driven using a 2mA constant current sink to force digit brightness to be consistent. The RTC can be either the original and more accurate DS3231 or the cheaper MEMS version DS3231M (dual footprint). 

PM and alarm indicators are separate drives to allow for different tube configurations. A small signal relay is used to simulate the ticking sound of a normal mechanical time piece.
