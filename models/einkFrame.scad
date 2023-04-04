//PCB MOUNT PARAMETERS

waveshare_driver_pcb_width = 29.5;
waveshare_driver_pcb_length = 48.5;
waveshare_driver_pcb_thickness = 2; //1.75
waveshare_driver_thin_wall = 1.2;
waveshare_driver_clip = 1; // radius of the waveshare_driver_clip that sticks out at the top of a pillar.
waveshare_driver_tab_width = 4;
waveshare_driver_belowPCBSpacing = 10+1; //add at least 1mm to actual measurement


///////// START OF PARAMETERS /////////////////
// What to include on the plate
plate = "Back"; //[ Back, Front, Both, espMount ]

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
magnet_diameter = 10;
magnet_shape = "hex"; //round, hex

// whether or not to include a stand
stand = "Stand"; //[ Stand, NoStand ]

// calibration, how many 10ths of a millimeter to adjust the backing size so it is a snug fit in the frame. Increase if it's too loose, decrease if it's too tight. Once you've found the right value for your printer you should be able to print frames of any size. Experiment with small frames first.
calibration = 0; //[-10:10]

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
    ,firstTY = -67-2 //fixes jank
    ,firstTZ = back_thickness-2
    ,pcbWidth = 96
    ,pcbLength = 136
    ,fn = 25
    );
}

module standoffHelperBoard()
{

    standoffOthers(
     height = back_thickness

    ,diameter = 5
    ,holediameter = 2.8
    ,firstTX = 20+10.5
    ,firstTY = 146
    ,firstTZ = back_thickness
    ,pcbWidth = 10.5
    ,pcbLength = 26
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
        
        back( height+border-overlap, width+border-overlap, back_thickness, magnet_diameter,magnet_shape);
        
        translate([0, width+border ,-2])
        placeStandoffInsidesBack();
        
        
        standBlocker();
    }
    
    //standBlocker();
    color("blue")
    translate([0, width+border ,0])
    placeStandoffBack();
    
    color("red")
    standoffHelperBoard();
    
    //color("blue")
   // standoffESP32();
    
    color("green")
    renderPCBMount();
}


if( ( plate == "espMount" )  )
{

    color("green")
    renderPCBMount();
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


module back( height, width, back, diam,mag_shape  )
{
    VERTADJUST = -25;
    HORIZADJUST = 10;
    
    HORIZCOUNTADJUST = 2;
    VERTCOUNTADJUST = 0;

	gap = .75;
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
			for(i=[-hnum:1:hnum-VERTCOUNTADJUST])
			{
				for(j=[-wnum:1:wnum-HORIZCOUNTADJUST])
				{
					translate([i*hspacing+VERTADJUST,j*wspacing+HORIZADJUST,0])
                    
                    if(mag_shape=="round")
                    {
                    cylinder( back*2, diam*0.5, diam*0.5 );
					}
                    else if(mag_shape=="hex")
                        {
                    cylinder( back*2, diam*0.5, diam*0.5, $fn=6);
					}
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

module standBlocker()
{
    color("green")
    translate([35+9,97-5.5,0])
    cube([25,123,10]);
    
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

/**
* essentially a cube with one of the edges cut off.
*/
module edged_box()
{
    rib = 4;
    sag = .4;
    dims = [rib,rib,waveshare_driver_pcb_thickness+sag];
    edge_off = 2;
    d = .001;
    d3 = [d,d,d];
    // somewhat convoluted, but it works
    mirror([0,1,0]) translate( [0, -rib, 0])
    difference()
    {
        cube( dims);
        translate([0,rib-edge_off,-d]) 
            rotate([0,0,45]) cube(dims + 2*d3);
    }
}

/**
* Pillar that holds the NE,NW corners of the PCB (the dull-edged corners)
*/
module frame_corner( height)
{
    waveshare_driver_thin_wall = 1.2;
    waveshare_driver_clip = 1;
    d3 = [.001, .001, .001];
    edge = waveshare_driver_pcb_thickness + 2*waveshare_driver_clip;
    pillar_dims = [4,4,height + edge] + [waveshare_driver_thin_wall, waveshare_driver_thin_wall, 0] - d3;

    difference()
    {
        translate([-waveshare_driver_thin_wall,-waveshare_driver_thin_wall,-height]) cube (pillar_dims);
        edged_box();
    }
}
/**
* A pillar that keeps the edge of the PCB in place. If doClip is true, 
* there will be a little tab at the top of the pillar to push the PCB down.
*/
module pillar( height, doClip=true)
{
    d = .001;
    edge = waveshare_driver_pcb_thickness + 2*waveshare_driver_clip;
    tab_dims = [waveshare_driver_thin_wall, 4, height + edge];
    if (doClip)
    {
        translate([0,-waveshare_driver_tab_width/2,edge - waveshare_driver_clip]) rotate([90,0,0]) cylinder(h = waveshare_driver_tab_width, r = waveshare_driver_clip, center=true, $fn = 50);
    }
    translate([0,-waveshare_driver_tab_width,-height]) cube( tab_dims);
    translate([-waveshare_driver_thin_wall+d, -waveshare_driver_tab_width, -height]) cube([waveshare_driver_thin_wall, waveshare_driver_tab_width, height]);
}

/** 
* clip to hold a wemos d1 mini
* This module can be used in enclosures. It generates clips that can
* hold a Wemos D1 Mini in place
**/
module waveshare_driver_clip( offset = 5)
{

    // frame corners on the NW, NE side
    rotate([0,0,180]) pillar( offset);
    
    translate([0, waveshare_driver_pcb_width, 0]) rotate([0,0,180]) mirror([0,1,0]) pillar(offset);
    
    //two tabs supporting the SE side
    translate([waveshare_driver_pcb_length, waveshare_driver_pcb_width, 0]) pillar(offset);
    translate([waveshare_driver_pcb_length-waveshare_driver_tab_width - 2*waveshare_driver_clip, waveshare_driver_pcb_width, 0]) rotate([0,0,90]) pillar(offset);
    
    // two pillar supporting the SW side
    translate([waveshare_driver_pcb_length, waveshare_driver_tab_width, 0]) pillar(offset);
    
        translate([waveshare_driver_pcb_length-6, 0, 0])mirror([-1,-1,0])
        pillar(offset);
    
    // two extra tabs holding the sides in the E and W position
    translate([waveshare_driver_pcb_length + waveshare_driver_tab_width, 0, 0] * .5) rotate([0,0,-90]) pillar( offset, false);
    translate([(waveshare_driver_pcb_length - waveshare_driver_tab_width)/2, waveshare_driver_pcb_width, 0] )rotate([0,0,90]) pillar( offset, false);
}

module renderPCBMount()
{
    	rotate([0,0,90])
    {
        translate([176,0,8+(waveshare_driver_belowPCBSpacing-6)])
        {
           

            waveshare_driver_clip( waveshare_driver_belowPCBSpacing);
            
            // bottom to hold it all together
            translate([-waveshare_driver_thin_wall, -waveshare_driver_thin_wall,-waveshare_driver_belowPCBSpacing]) 
            cube([ waveshare_driver_pcb_length, waveshare_driver_pcb_width, 1] + 2*[waveshare_driver_thin_wall, waveshare_driver_thin_wall,0]);
        
            
        }
    }
}