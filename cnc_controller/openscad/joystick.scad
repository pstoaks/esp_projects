// Model of analog XY joystick Arduino shield

// All dimensions in mm
JoystickData =
[
   34.0, // PCB X dimension
   26.6, // PCB Y dimension
   1.5,  // Thickness
   28.0, // Knob hole diameter
   11.0, // Height of bottom of knob from top of PCB
   3.0,  // Mounting hole diameter
   26.2, // Mounting pin pitch in X direction
   19.7, // Mounting pin pitch in Y direction
   5.06, // Mounting pin left offset (X)
   3.64, // Mounting pin bottom offset (Y)
   4.6,  // Mounting standoff max dia
   2.5,  // PB_X X dimension of pin base
   12.5, // PB_Y Y dimension of pin base
   2.5,  // PB_Z Z dimension of pin base
   1.0,  // PB_XO  X offset of pin base
   7.75, // PB_YO  Y offset of pin base
   7.5   // PN_LX  Pin length (X direction)
// Knob is centered between mounting holes
];

J_PCB_X = 0;  // PCB X dimension
J_PCB_Y = 1;  // PCB Y dimension
J_PCB_Z = 2;  // Thickness
J_KNOB_HOLE_DIA = 3;
J_PCB_ZO = 4; // Height of bottom of knob from top of PCB
J_MOUNT_HOLE_DIA = 5;
J_PIN_X = 6;  // Mounting pin pitch in X direction
J_PIN_Y = 7;  // Mounting pin pitch in Y direction
J_PIN_XO = 8; // Mounting pin left offset (X)
J_PIN_YO = 9; // Mounting pin bottom offset (Y)
J_SO_MAX = 10; // Mounting standoff max dia
J_PB_X = 11;  // X dimension of pin base
J_PB_Y = 12;  // Y dimension of pin base
J_PB_Z = 13;  // Z dimension of pin base
J_PB_XO = 14; // X offset of pin base
J_PB_YO = 15; // Y offset of pin base
J_PN_LX = 16; //  Pin length (X direction)


// This function returns a translation vector that centers the given joystick at the X Y locations
// in the input vector. The Z location is at the base of the knob (surface of the panel).
function center_joystick(data, location_vec) =
   [ (location_vec[0] - (data[J_PIN_X]/2.0 + data[J_PIN_XO])),
     (location_vec[1] - (data[J_PIN_Y]/2.0 + data[J_PIN_YO])),
     (location_vec[2] - (data[J_PCB_ZO] + data[J_PCB_Z])) ];


// Origin of part is lower left hand corner as knob is facing you and pins are
// on the left. PCB is resting on the XY plane (Z=0). Ignore the protrusion on the
// underside.
module joystick(data)
{
   if ( $preview )
   {
      // PCB
      difference()
      {
         color("Green")
         {
            cube([data[J_PCB_X], data[J_PCB_Y], data[J_PCB_Z]]);
         }
         translate([0, 0, -data[J_PCB_Z]*2.0])
            joystick_supports(data, data[J_MOUNT_HOLE_DIA], data[J_PCB_Z] * 4.0, 0.0);
      }

      // Knob
      center = [data[J_PIN_X]/2.0 + data[J_PIN_XO], data[J_PIN_Y]/2.0 + data[J_PIN_YO], data[J_PCB_ZO]];
      color("DarkSlateGray")
      {
         translate(center)
         {
            difference()
            {
               sphere(d=data[J_KNOB_HOLE_DIA]);
               translate([0, 0, -data[J_PCB_ZO]/2.0+data[J_PCB_Z]])
                  cube([data[J_KNOB_HOLE_DIA], data[J_KNOB_HOLE_DIA], data[J_PCB_ZO]], true);
            }
            translate([0, 0, data[J_KNOB_HOLE_DIA]*.4])
               cylinder(d = 10, h=10);
         }
      }

      // Pins
      color("Yellow")
      {
         translate([data[J_PB_XO], data[J_PB_YO], data[J_PCB_Z]])
         {
            cube([data[J_PB_X], data[J_PB_Y], data[J_PB_Z]]);
            translate([-data[J_PN_LX], 0, data[J_PB_Z]])
               cube([data[J_PN_LX]+data[J_PB_X], data[J_PB_Y], data[J_PB_Z]]);
         }
      }

   }

   // Standoffs
   joystick_supports(data, data[J_SO_MAX], data[J_PCB_ZO]);

} // joystick()

 // This module creates a shape that can be used to subtract from a surrounding
 // enclosure to make space for the joystick. Positioned with lower-left corner at 0
 // and underside of PCB on Z=0 XY plane
module joystick_subtraction(data)
{
   // PCB
   cube([data[J_PCB_X], data[J_PCB_Y], data[J_PCB_ZO]]);

   // Knob
   center = [data[J_PIN_X]/2.0 + data[J_PIN_XO], data[J_PIN_Y]/2.0 + data[J_PIN_YO], data[J_PCB_ZO]];
   translate(center)
   {
      cylinder(d = data[J_KNOB_HOLE_DIA], h=40);
   }

   translate([data[J_PB_XO], data[J_PB_YO], 0])
   {
      cube([data[J_PB_X], data[J_PB_Y], data[J_PB_Z]]);
      translate([-data[J_PN_LX], 0, 0])
         cube([data[J_PN_LX]+data[J_PB_X], data[J_PB_Y], data[J_PB_Z]*2.0+data[J_PCB_Z]]);
   }
   
} // joystick_subtraction()


module joystick_supports(data, standoff_dia, standoff_len, hole_dia=1.6)
{

   translate([data[J_PIN_XO], data[J_PIN_YO], data[J_PCB_Z]])
   {
      rect_standoff(standoff_dia, hole_dia, standoff_len, data[J_PIN_X], data[J_PIN_Y]);
   }

} // joystick_supports()
