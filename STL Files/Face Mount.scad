$fn = 36;

module OriginalPost()
{
    translate([-79, -67, -4])
    intersection()
    {
        import("Face Black.stl", convexity = 10);
        translate([79, 66, 4])
            cylinder(r = 8, h = 100);
    }
}


module FixedPost()
{
    OriginalPost();
   // #translate([79, 66.9, 3.95])
        cylinder(r = 5, h = 30);
    
}

module OriginalFace()
{
    translate([-75, 0, 0])
    import("Face White.stl", convexity = 20);
}

module FixedFace()
{
    OriginalFace();
    translate([4, -68, 4]) FixedPost();
    translate([4, 67, 4]) FixedPost();
}

module FullPost()
{
    OriginalPost();
    cylinder(r = 5, h = 30);    
    translate([0, 0, -4])
        cylinder(d = 4, h = 4);
}

!FixedFace();
FullPost();
