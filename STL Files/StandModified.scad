include <Common Constants.scad>


OriginalBaseLength = 55;
DesiredBaseLength  = 60;
BaseExtensionLength = DesiredBaseLength > OriginalBaseLength ? 
                      OriginalBaseLength - DesiredBaseLength : 0;
                      
PostBlockSize = [12, 22, 1];

$fn = 30;



module OriginalStand()
{
    import("stand.stl", convexity = 10);
}


module StandWithPostBlocked()
{
    OriginalStand();
    translate([-9, -10, 0]) cube(PostBlockSize);
}


module PcbHolderRemoved()
{
    difference()
    {
        StandWithPostBlocked();
        translate([-68.7, 8.7, 6.]) 
            cube([60, 10, 70]);
        translate([-63, 0, 43]) 
            cube([20, 10, 20]);
        translate([0, 0, 36])
            cube([40, 40, 40], center = true);
    }
    translate([-7.8, 6.9, 0]) difference()
    {
        cylinder(r = 3, h = 16);
        translate([0, -5, 0]) cube([5, 5, 17]);
    }
}


module BaseExtension()
{
    intersection()
    {
        PcbHolderRemoved();
        translate([-66, -11, -1]) cube([20, 24, 10]);
    }
}


module BaseWithExtension()
{
    union()
    {
        PcbHolderRemoved();
        translate([BaseExtensionLength, 0, 0]) BaseExtension();
    }
}

module FullBaseWithMounts()
{
    BaseWithExtension();
    translate([-11 - 1 - boxY /4, 0, 0])
        cylinder(d = BaseMountHoleD - .2, h = wallThick + 6);
    translate([-11 - 1 - 3 * boxY /4, 0, 0])
        cylinder(d = BaseMountHoleD - .2, h = wallThick + 6);
}


!FullBaseWithMounts();

