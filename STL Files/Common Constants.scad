/* [Outside Dimensions] */

// thickness of box walls
wallThick = 2.4; 

// used to give the pcb some room
slop = 1;

insideBoxX = 73.5 + slop;
insideBoxY = 52 + slop;
insideBoxZ = 33 + slop;

// width
boxX = insideBoxX + 2 * wallThick; 
// depth
boxY = insideBoxY + 2 * wallThick; 
// height
boxZ = insideBoxZ + 2 * wallThick;
// radius of curved corners
cornerRad = 3; 

/* [Lid] */
// amount of play in the lid in mm - the lid can't be the exact same size as the opening
// was .4, changed to .3 based on test print
lidSnugness = 0.3;


MountingHoleXDelta = 66.5;
MountingHoleYDelta = 41.25;

MountingHoleD = 2.5; 

MountingHoleXOffset = -MountingHoleXDelta / 2;
MountingHoleYOffset = -MountingHoleYDelta / 2; 

MountHeight = MountingHoleD + 1.5;

SupportXOffset = -10.5;
SupportYOffset = 3;


UsbXOffset = -17.35;
UsbYOffset = 16.4;
UsbXSize = 11.5;
UsbYSize = 7;


SwitchXOffset = 25.5;
SwitchYOffset = 9.;
SwitchD = 4.5;

LedXOffset = 16.8;
LedYOffset = 9.3;
LedD = 5.5;

CableEntryD = 13;   // Diameter of cable entry hole.
CableEntryOffset = 41;

BaseMountHoleD = 6;     // Diameter of mounting holes.


