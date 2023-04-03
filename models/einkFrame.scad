///////// START OF PARAMETERS /////////////////

// The height of the picture
height = 110;//front opening is this minus 2 times overlap

// The width of the picture
width = 138; //front opening is this minus 2 times overlap

// The width of the frame border
border = 16;

// How much the border overlaps the edge of the picture
overlap = 10;

// How thick the frame is in total
thickness = 4;

// How thick the backing is
back_thickness = 3;

// The diameter of the magnet holes, 0 if you don't want holes. Even if you're not adding magnets, adding holes uses less plastic.
magnet_diameter = 20;

// whether or not to include a stand
stand = "Stand"; //[ Stand, NoStand ]

// calibration, how many 10ths of a millimeter to adjust the backing size so it is a snug fit in the frame. Increase if it's too loose, decrease if it's too tight. Once you've found the right value for your printer you should be able to print frames of any size. Experiment with small frames first.
calibration = 0; //[-10:10]

// What to include on the plate
plate = "Back"; //[ Back, Front, Both ]

standOffHeight = 2;

/////////// END OF PARAMETERS /////////////////
rounded = 0.7*1;
overhang=45*1;

adjust = calibration/10;

$fs=0.3*1;
//$fa=5*1; //smooth
$fa=8*1; //nearly smooth
//$fa=20*1; //rough

module placeStandoffBack()
{

    standoffDisplay(
     height = back_thickness+15

    ,diameter = 8
    ,holediameter = 3.5
    ,firstTX = -45
    ,firstTY = -67
    ,firstTZ = back_thickness-2.1
    ,pcbWidth = 96
    ,pcbLength = 134
    ,fn = 25
    );
}
module placeStandoffFront()
{

    standoffDisplay(
     height = back_thickness+.1
    ,diameter = 6.5
    ,holediameter = 2.9
    ,firstTX = -45
    ,firstTY = -67
    ,firstTZ = back_thickness-2.1
    ,pcbWidth = 96
    ,pcbLength = 134
    ,fn = 25
    );
}
module placeStandoffInsides()
{
    thicknessBeforeScrew = 0.4;

    standoffInsides(
     height = back_thickness+5
    ,diameter = 4.5
    ,holediameter = 2.9
    ,firstTX = -45
    ,firstTY = -67
    ,firstTZ = back_thickness-2+thicknessBeforeScrew
    ,pcbWidth = 96
    ,pcbLength = 134
    ,fn = 25
    );
}
module placeStandoffInsidesBack()
{
    thicknessBeforeScrew = 0.4;

    standoffInsides(
     height = back_thickness+5
    ,diameter = 4.5
    ,holediameter = 3.5
    ,firstTX = -45
    ,firstTY = -67
    ,firstTZ = back_thickness-2+thicknessBeforeScrew
    ,pcbWidth = 96
    ,pcbLength = 134
    ,fn = 25
    );
}

module standoffHelperBoard()
{

    standoffOthers(
     height = back_thickness

    ,diameter = 5
    ,holediameter = 2.8
    ,firstTX = 37
    ,firstTY = 142
    ,firstTZ = back_thickness
    ,pcbWidth = 10.5
    ,pcbLength = 24
    ,fn = 25
    );
}

module standoffESP32()
{

    standoffOthers(
     height = back_thickness

    ,diameter = 4
    ,holediameter = 2.1
    ,firstTX = -30
    ,firstTY = 170
    ,firstTZ = back_thickness
    ,pcbWidth = 35
    ,pcbLength = 54
    ,fn = 25
    );
}



if( ( plate == "Front" ) || ( plate == "Both" ) )
{

placeStandoffFront();

difference()
{
	front( height, width, border, overlap, thickness, back_thickness, stand );
    
    placeStandoffInsides();
    
}
}

if( ( plate == "Back" ) || ( plate == "Both" ) )
{

    difference()
    {
        translate([0, width+border ,back_thickness*0.5])
        
        back( height+border-overlap, width+border-overlap, back_thickness, magnet_diameter);
        
        translate([0, width+border ,-2])
        placeStandoffInsidesBack();
        
    }
    
    translate([0, width+border ,0])
    placeStandoffBack();
    color("red")
    standoffHelperBoard();
    color("blue")
    standoffESP32();
}

module front( height, width, border, overlap, thickness, back_thickness, stand )
{
	difference()
	{
    
		translate([0,0,thickness*0.5])
		frame( height, width, border, overlap, thickness );
        
		translate([0,0,thickness-(0.5*back_thickness)])
		back( height+1, width+1, back_thickness*1.05 );
        
	}
    
    widthIncrease = 100;
    heightIncrease = 20;
    
    
	y = (height*0.7)*cos(75)+heightIncrease;
	x = y*cos(75);
	s = border-overlap-0.5;
   
	if( stand=="Stand" )
	{
		translate([height*0.5+s+0.5,0,thickness])
		hull()
		{
			translate([-s*0.5,0,0])
			cube([s,s+widthIncrease,0.01], true);
			translate([-1-x,0,y])
			rotate([90,0,0])
			cylinder(s+widthIncrease,1,1,true);
		}
	}
}


module back( height, width, back, diam  )
{	gap = 1;
	hremaining = (((height-diam)/2)-gap); 
	hnum=floor( hremaining/(diam+gap) );
	hspacing=((hremaining-((diam+gap)*hnum))/(hnum+1))+diam+gap;
	wremaining = (((width-diam)/2)-gap); 
	wnum=floor( wremaining/(diam+gap) );
	wspacing=((wremaining-((diam+gap)*wnum))/(wnum+1))+diam+gap;
	
    difference()
	{
		cube( [ height, width, back ], true );
		if( diam > 0 )
		{
			translate([0,0,-back])
			for(i=[-hnum:1:hnum])
			{
				for(j=[-wnum:1:wnum])
				{
					translate([i*hspacing,j*wspacing,0])
					cylinder( back*2, diam*0.5, diam*0.5 );
				}
			}
		}
	}
}

module frame( height, width, border, overlap, back )
{
	difference()
	{
		cube( [ height+2*(border-overlap), width+2*(border-overlap), back ], true );
		translate([0,0,-1])
		cube( [ height+2*(-overlap), width+2*(-overlap), back+2 ], true );
	}
}

module cutter(dist, overhang)
{
	size = dist*2;

	translate([dist,-dist,-dist])
	cube([size,size,size]);

	translate([-dist-size,-dist,-dist])
	cube([size,size,size]);

	translate([dist,-dist,0])
	rotate([0,-overhang,0])
	cube([size,size,size]);

	translate([-dist,-dist,0])
	rotate([0,-90+overhang,0])
	cube([size,size,size]);

	translate([dist,-dist,0])
	rotate([0,90+overhang,0])
	cube([size,size,size]);

	translate([-dist,-dist,0])
	rotate([0,180-overhang,0])
	cube([size,size,size]);

}


//https://github.com/Wollivan/OpenSCAD-PCB-Standoff-Module
module standoffDisplay(height, diameter, holediameter, firstTX, firstTY, firstTZ, pcbWidth, pcbLength, fn=25){
    //Standoff 1 TOP LEFT
    
    difference(){
        translate([firstTX, firstTY, firstTZ])
            cylinder(h=height, d=diameter, $fn = fn);
        
        translate([firstTX, firstTY, firstTZ])
            cylinder(h=height, d=holediameter, $fn = fn);
    }
    //Standoff 2 bottom left
    difference(){
        translate([firstTX+pcbWidth, firstTY-1, firstTZ])
            cylinder(h=height, d=diameter, $fn = fn);
        
        translate([firstTX+pcbWidth, firstTY-1, firstTZ])
            cylinder(h=height, d=holediameter, $fn = fn);
    }
    //Standoff 3 TOP RIGHT
    difference(){
        translate([firstTX, firstTY+pcbLength, firstTZ])
            cylinder(h=height, d=diameter, $fn = fn);
        
        translate([firstTX, firstTY+pcbLength, firstTZ])
            cylinder(h=height, d=holediameter, $fn = fn);
    }
    //Standoff 4 BOTTOM RIGHT
    difference(){
        translate([firstTX+pcbWidth, firstTY+pcbLength-1, firstTZ])
            cylinder(h=height, d=diameter, $fn = fn);
        
        translate([firstTX+pcbWidth, firstTY+pcbLength-1, firstTZ])
            cylinder(h=height, d=holediameter, $fn = fn);
    }
}

module standoffInsides(height, diameter, holediameter, firstTX, firstTY, firstTZ, pcbWidth, pcbLength, fn=25){
    //Standoff 1
    
  //  difference(){
       /// translate([firstTX, firstTY, firstTZ])
          //  cylinder(h=height, d=diameter, $fn = fn);
        
        translate([firstTX, firstTY, firstTZ])
            cylinder(h=height, d=holediameter, $fn = fn);
    
    //Standoff 2
  //  difference(){
   //     translate([firstTX+pcbWidth, firstTY, firstTZ])
   //         cylinder(h=height, d=diameter, $fn = fn);
        
        translate([firstTX+pcbWidth, firstTY-1, firstTZ])
            cylinder(h=height, d=holediameter, $fn = fn);
 //   }
    //Standoff 3
  //  difference(){
   //     translate([firstTX, firstTY+pcbLength, firstTZ])
   //         cylinder(h=height, d=diameter, $fn = fn);
        
        translate([firstTX, firstTY+pcbLength, firstTZ])
            cylinder(h=height, d=holediameter, $fn = fn);
  //  }
    //Standoff 4
  //  difference(){
   //     translate([firstTX+pcbWidth, firstTY+pcbLength, firstTZ])
   //         cylinder(h=height, d=diameter, $fn = fn);
        
        translate([firstTX+pcbWidth, firstTY+pcbLength-1, firstTZ])
            cylinder(h=height, d=holediameter, $fn = fn);
   // }
}


module standoffOthers(height, diameter, holediameter, firstTX, firstTY, firstTZ, pcbWidth, pcbLength, fn=25){
    //Standoff 1 TOP LEFT
    
    difference(){
        translate([firstTX, firstTY, firstTZ])
            cylinder(h=height, d=diameter, $fn = fn);
        
        translate([firstTX, firstTY, firstTZ])
            cylinder(h=height, d=holediameter, $fn = fn);
    }
    //Standoff 2 bottom left
    difference(){
        translate([firstTX+pcbWidth, firstTY, firstTZ])
            cylinder(h=height, d=diameter, $fn = fn);
        
        translate([firstTX+pcbWidth, firstTY, firstTZ])
            cylinder(h=height, d=holediameter, $fn = fn);
    }
    //Standoff 3 TOP RIGHT
    difference(){
        translate([firstTX, firstTY+pcbLength, firstTZ])
            cylinder(h=height, d=diameter, $fn = fn);
        
        translate([firstTX, firstTY+pcbLength, firstTZ])
            cylinder(h=height, d=holediameter, $fn = fn);
    }
    //Standoff 4 BOTTOM RIGHT
    difference(){
        translate([firstTX+pcbWidth, firstTY+pcbLength, firstTZ])
            cylinder(h=height, d=diameter, $fn = fn);
        
        translate([firstTX+pcbWidth, firstTY+pcbLength, firstTZ])
            cylinder(h=height, d=holediameter, $fn = fn);
    }
}