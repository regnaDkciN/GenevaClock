//
// LEDHolder.scad
//
// Generates a holder for an RGB LED for the Geneva Clock project.
//


LedHeight = 2;
LedDiameter = 5.2;
LedLength = 6;
Perimiter = 1;
$fn = 30;


difference()
{
    cube([LedDiameter + 2 * Perimiter, 
          LedDiameter + Perimiter + LedHeight, 
          LedLength], center = true);
    translate([0, LedHeight / 2, - LedLength / 2 -.1])
        cylinder(d = LedDiameter, h = LedLength + .2);
}
