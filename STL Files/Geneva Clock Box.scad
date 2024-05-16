include <Common Constants.scad>


/* 
  This section is "hidden" from the customizer and is used for calculating
  variables based on those that the user inputs above
*/

/* [Hidden] */
// define the amount that needs to be subtracted to account for wall thicknesses
wallSub = 2 * wallThick;

// to avoid "z fighting" where surfaces line up exactly, add a bit of fudge
fudge = .001; 

$fn = 72;




/* 
  Add a ready made library for making rounded boxes
  Learn more about the MCAD libraries here: http://reprap.org/wiki/MCAD
*/
include <MCAD/boxes.scad>


/*
  Create the lid trapezoidal lid
  This sub creates both a lid and is used for cutting out the lid shape in the basic box
*/

module lid(cutter = false) 
{
    /*
    Lid profile

       boxX - 2* wall thickness + 2*(wall thickness/tan(trap angle))
    p3 ---------------- p2
       \) trap angle  /
        \            /
    P0   ------------   p1
         boxX - 2* wall thickness
         
    */

    // define how snug the fit should be on the lid
    // when this sub is used to "cut" enlarge the dimensions to make the opening larger
    snugness = (cutter == false) ? 0 : lidSnugness;

    // define the lid width (X)
    lidX = boxX - wallSub;

    // define the length of the lid (Y) 
    lidY = boxY - wallThick;

    // define the thickness of the lid
    lidZ = wallThick;

    // define slot dimensions
    slotX = lidX * .9;
    slotY = lidY * .05;

    // define the angle in the trapezoid; 
    // this needs to be more than 45
    trapAngle = 65;
  
    lidAdd = wallThick / tan(trapAngle);
  
    // define points for the trapezoidal lid
    p0 = [-snugness / 2, 0];
    p1 = [lidX + snugness / 2, 0];
    p2 = [lidX + lidAdd + snugness / 2, lidZ];
    p3 = [-lidAdd - snugness / 2, lidZ];
    p4 = [-snugness / 2, lidZ];
  
    //center the lid
    difference()
    {
        translate([-lidX / 2, -lidZ / 2, 0])
            rotate([-90])
        {    
            difference()
            {
                // draw a polygon using the points above and then extrude
                // linear extrude only works in the Z axis so everything has to be rotated after
                linear_extrude(height = lidY, center = true) 
                    polygon([p0, p1, p2, p3], paths = [[0, 1, 2, 3]]); 
        
                // add a fingernail slot for making opening the lid easier
                // the variable names are a bit wonky because the lid is drawn rotated -90 degrees
                if ( cutter == false)
                {
                    translate([lidX / 2, 0, lidY / 2 - slotY * 2])
                        roundedBox([slotX, lidZ, slotY], radius = slotY / 2);
                }
            }
            // Add the back bevel.
            translate([lidX / 2, 0, -(lidY - snugness) / 2])
                rotate([0, -90, 0])
                linear_extrude(height = lidX + snugness, center = true) 
                polygon([p0, p4, p3], paths = [[0, 1, 2]]); 
         
            // Fill in the back corners.
            // !! Don't know why the .07 in Z offset is needed.
            translate([-snugness / 2,
                        lidZ, 
                        -lidY / 2]) //-(lidY - snugness) / 2])//-boxY / 2 + lidAdd])// - snugness / 2 + .07])
                rotate([90, 0, 0])
                cylinder(r1 = lidAdd, r2 = 0, h = lidZ);

            translate([lidX + snugness / 2, 
                       lidZ,
                       -lidY / 2]) //-boxY / 2 + lidAdd +  .08])
                rotate([90, 0, 0])
                cylinder(r1 = lidAdd, r2 = 0, h = lidZ);
        }
        
        // Add notches to keep lid tight.
        translate([-lidX / 2 - lidAdd - snugness / 2,
                    -lidY * .4,
                    -2 * wallThick])
            cylinder(r = lidSnugness / 2, h = 4 * wallThick);
            
        translate([lidX / 2 + lidAdd + snugness / 2,
                    -lidY * .4,
                    -2 * wallThick])
            cylinder(r = lidSnugness / 2, h = 4 * wallThick);
    }
    
    // Add the front.
    difference()
    {
        adjustedLidX = lidX + 2 * lidAdd + snugness;
        adjustedLidY = boxZ - (3 * wallThick / 2) + snugness / 2;
        adjustedLidZ = wallThick + snugness / 2;
        translate([0, 
                   lidY / 2 -  wallThick - snugness / 4,
                   -adjustedLidY / 2 - wallThick + fudge])
            rotate([90, 0, 0])
            cube([adjustedLidX, adjustedLidY, adjustedLidZ], center = true);
          
        // Cut out front panel holes if needed.    
        if (!cutter)
        {
            // Cut out USB opening.
            translate([UsbXOffset, 
                       lidY / 2 - wallThick, //insideBoxZ + wallThick - UsbYOffset - UsbYSize / 2, 
                       -boxZ + wallThick + UsbYOffset])
                cube([UsbXSize, UsbYSize, 3 * wallThick], center = true);
                
            // Cut out the switch opening.
            translate([SwitchXOffset, 
                        lidY / 2 - 2 * wallThick, 
                        -boxZ + wallThick + SwitchYOffset])
                rotate([-90, 0, 0])
                cylinder(d = SwitchD, h = 3 * wallThick);

            // Cut out the LED opening.
            translate([LedXOffset, 
                        lidY / 2 - 2 * wallThick, 
                        -boxZ + wallThick + LedYOffset])
                rotate([-90, 0, 0])
                cylinder(d = LedD, h = 3 * wallThick);
        }
    }
}

//!lid(true);

module roundedLid(cutter = false)
{
    outerBox = [boxX, boxY, boxZ];
    intersection()
    {
        lid(cutter);
        translate([0, -wallThick, -boxZ / 2 + wallThick / 5])  
            roundedBox(size = outerBox + [0, 0, 5], radius = cornerRad, sidesonly = 1);
    }
}



module MountSpacer()
{
    difference()
    {
        cylinder(d1 = MountingHoleD + 5, d2 = MountingHoleD + 2, h = MountHeight);
        translate([0, 0, -.5]) cylinder(d = MountingHoleD, h = MountHeight + 2);
    }
}

module PcbMount()
{
    //rotate([0, 0, 180])
    {
    for (x = [MountingHoleXOffset, MountingHoleXOffset + MountingHoleXDelta],
         y = [MountingHoleYOffset, MountingHoleYOffset + MountingHoleYDelta])
    {
        translate([x, y, -(insideBoxZ + slop) / 2]) MountSpacer();
    }
    translate([SupportXOffset, SupportYOffset, -(insideBoxZ + slop) / 2])
        MountSpacer();
    }
}


/*
  create the basic box
*/
module basicBox(outerBox, innerBox)
{
    // Create the box then subtract the inner volume and make a cut for the sliding lid
    difference()
    {
        // call the MCAD rounded box command imported from MCAD/boxes.scad
        // size = dimensions; radius = corner curve radius; sidesonly = round only some sides

        // First draw the outer box
        roundedBox(size = outerBox, radius = cornerRad, sidesonly = 1);
        // Then cut out the inner box
        translate([0, 0, wallThick])
            roundedBox(size = innerBox, radius = cornerRad, sidesonly = 1);
        // Then cut out the lid
        translate([0, 
                   -wallThick , 
                   (boxZ / 2) + fudge])
            rotate([0, 0, 180])
            roundedLid(cutter = true); 
            
        // Cut out a cable entry hole in the back of the box.
        translate([boxX / 2 - CableEntryOffset, boxY / 2 + wallThick, 0])
            rotate([90,0, 0])
            cylinder(d = CableEntryD, h = 3 * wallThick);
            
        // Cut mounting holes.
        translate([(boxX - wallThick) / 2, boxY / 4, 0]) rotate([0, 90, 0])
            cylinder(d = BaseMountHoleD, wallThick * 2, center = true);
        translate([(boxX - wallThick) / 2, -boxY / 4, 0]) rotate([0, 90, 0])
            cylinder(d = BaseMountHoleD, wallThick * 2, center = true);
    }
    PcbMount();
}

/*
  This module will call all of the other modules and assemble the final box
*/
module finalBox()
{
    echo ("This is the finalBox module");
    // Define the volume that will be the outer box
    outerBox = [boxX, boxY, boxZ];

    // Define the volume that will be subtracted from the box
    // a "fudge factor" is added to avoid Z fighting when faces align perfectly
    innerBox = [boxX-wallSub, boxY-wallSub, boxZ]; //-wallSub+fudge*2];

    // call the basic box module to add the box
    basicBox(outerBox, innerBox);

    // explain what's going on
    echo ("innerBox=", innerBox);
/*
    // move the lid to the left of the box and also move it down  so it sits at the
    // same level as the bottom of the box
    //translate([-boxX, 0, -boxZ/2+wallThick/2])
    translate([-boxX, 0, -boxZ / 2])
    rotate([0, 180, 0])
    // translate([0, -wallThick, 10.9]) rotate([0, 0, 180])
    roundedLid();
*/    
}






// call the finalBox module
!finalBox();
roundedLid();

