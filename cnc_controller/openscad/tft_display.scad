// TFT Display assets

// All dimensions in mm
// Orientation is display facing you, pins on left. Offsets are measured from lower-left
// corner. X is positive from left to right, Y is positive from bottom to top.
DisplayDefinitions =
   [
      [
         "3.2_320x240",
         [
            89.3, // Overall width (PCB X dimension)
            55.1, // Overall height (PCB Y dimension)
            1.6,  // PCB Thickness (PCB Z dimension)
            15.0, // Overall depth (Includes Pins)
            83.3, // Mounting hole center-to-center distance in X
            49.05, // Mounting hole center-to-center distance in Y
            3.25, // Mounting hole diameter
            3.20, // Mounting hole offset from vertical edge
            3.20, // Mounting hole offset from horizontal edge
            66.25, // Display window width (X axis)
            50.0, // Display window height (Y axis)
            4.0,  // Display window thick (Z axis) (top of glass from top of PCB)
            14.2, // Display window X offset
            2.8,  // Display window Y offset
            26.4, // SD Card holder X dimension
            16.8, // SD Card holder Y dimension
            3.2,  // SD Card holder Z dimension (off back of PCB)
            33.4, // SD Card holder X offset
            37.1, // SD Card holder Y offset
            20.0, // SD Card protrusion beyond top (Y direction)
            2.5, // pin row base width (X dimension)
            35.7, // pin row base length (Y dimension)
            2.5, // pin row base detph (Z dimension)
            5.9, // pin length
            0.7, // pin diameter
            1.0, // pin row X offset (to edge of base)
            9.8, // pin row Y offset (to edge of base)
            5.0  // Maximum standoff diameter (too large and it will interfere with installation.)
            ]
         ],
      [
         "2.8_320x240",
         [ // These values haven't been correctly filled in yet.
            86.0, // +Overall width
            50.0, // +Overall height
            1.5,  // +PCB Thickness
            15.0, // Overall depth
            76.0, // +Mounting hole center-to-center distance in X
            44.0, // +Mounting hole center-to-center distance in Y
            3.25, // Mounting hole diameter
            3.20, // Mounting hole offset from vertical edge
            3.20, // Mounting hole offset from horizontal edge
            60.0, // +Display window width (X axis)
            45.0, // +Display window height (Y axis)
            4.3,  // Display window thick (Z axis) (top of glass from top of PCB)
            17.0, // Display window X offset
            2.8,  // Display window Y offset
            26.44, // SD Card holder X dimension
            17.0,  // SD Card holder Y dimension
            3.2,  // SD Card holder Z dimension (off back of PCB)
            33.4, // SD Card holder X offset
            37.1, // SD Card holder Y offset
            20.0, // SD Card protrusion beyond top (Y direction)
            2.5, // pin row base width (X dimension)
            35.7, // pin row base length (Y dimension)
            2.5, // pin row base detph (Z dimension)
            5.9, // pin length
            0.7, // pin diameter
            1.0, // pin row X offset (to edge of base)
            9.8, // pin row Y offset (to edge of base)
            5.0  // Maximum standoff diameter (too large and it will interfere with installation.)
            ]
         ]
      ];

// Data indexes
D_PCB_X = 0; // Overall width (PCB X dimension)
D_PCB_Y = 1; // Overall height (PCB Y dimension)
D_PCB_Z = 2; // PCB Thickness (PCB Z dimension)
D_DEPTH = 3; // Overall depth (Includes Pins)
D_PIN_X = 4; // Mounting hole center-to-center distance in X
D_PIN_Y = 5; // Mounting hole center-to-center distance in Y
D_PIN_D = 6; // Mounting hole diameter
D_PIN_XO = 7; // Mounting hole offset from vertical edge
D_PIN_YO = 8; // Mounting hole offset from horizontal edge
D_WIN_X = 9;  // Display window width (X axis)
D_WIN_Y = 10; // Display window height (Y axis)
D_WIN_Z = 11; // Display window thick (Z axis) (top of glass from top of PCB)
D_WIN_XO = 12; // Display window X offset
D_WIN_YO = 13; // Display window Y offset
D_SD_X = 14; // SD Card holder X dimension
D_SD_Y = 15;  // SD Card holder Y dimension
D_SD_Z = 16; // SD Card holder Z dimension (off back of PCB)
D_SD_XO = 17; // SD Card holder X offset
D_SD_YO = 18; // SD Card holder Y offset
D_SD_YP = 19; // SD Card protrusion beyond top (Y direction)
D_PR_XB = 20; // pin row base width (X dimension)
D_PR_YB = 21; // pin row base length (Y dimension)
D_PR_ZB = 22; // pin row base length (Y dimension)
D_PR_PL = 23; // pin length
D_PR_PD = 24; // pin diameter
D_PR_XO = 25; // pin row X offset (to edge of base)
D_PR_YO = 26;  // pin row Y offset (to edge of base)
D_SO_MAX = 27; // Maximum standoff diameter

// This function returns the data associated with the given display type (from the vectors above).
function display_data(display_type) = DisplayDefinitions[search([display_type], DisplayDefinitions)[0]][1];

// This function returns a translation vector that centers the given display data at the X Y locations
// in the input vector. The Z location is at the surface of the display.
function center_display(disp_data, location_vec) =
   [ (location_vec[0] - (disp_data[D_WIN_X]/2.0+disp_data[D_WIN_XO])),
     (location_vec[1] - (disp_data[D_WIN_Y]/2.0+disp_data[D_WIN_YO])),
     (location_vec[2] - (disp_data[D_WIN_Z]+disp_data[D_PCB_Z])) ];

// This module creates the display model. In preview mode, it creates an entire
// model for visualization. In render mode, it creates only the positive model
// components that would be included in the enclosure (such as the TFT supports/standoffs.)
// Positioned with lower-left corner at 0 and underside of PCB on XY plane (Z=0).
// To get data argument, data = display_data(display_type); where display_type is the
// name of the display (a string).
module display(data)
{

   if ( $preview )
   {
      // PCB
      difference()
      {
         color("Red")
         {
            cube([data[D_PCB_X], data[D_PCB_Y], data[D_PCB_Z]]);
         }
         translate([0, 0, -data[D_PCB_Z]*2.0])
            display_supports(data, data[D_PIN_D], data[D_PCB_Z] * 4.0, 0.0);
      }

      // Display Window
      translate([data[D_WIN_XO], data[D_WIN_YO], data[D_PCB_Z]])
      {
         color ("Gray")
            cube([data[D_WIN_X], data[D_WIN_Y], data[D_WIN_Z]]); // Thick enough to cut through something above.
      }

      // SD Card Access (adding 1 mm top and bottom. Width should be fine.)
      translate([data[D_SD_XO], data[D_SD_YO], -data[D_SD_Z]])
      {
         color("Gainsboro")
            cube([data[D_SD_X], data[D_SD_Y], data[D_SD_Z]]);
      }

      // Pins
      translate([data[D_PR_XO], data[D_PR_YO], -1.0*(data[D_PR_ZB]+data[D_PR_PL])])
      {
         color("Yellow")
            cube([data[D_PR_XB], data[D_PR_YB], data[D_PR_ZB]+data[D_PR_PL]]);
      }
   }

   // Standoffs
   display_supports(data, data[D_SO_MAX], data[D_WIN_Z]);
   
} // display()

 // This module creates a shape that can be used to subtract from a surrounding
 // enclosure to make space for the display. Positioned with lower-left corner at 0
 // and underside of PCB on Z=0 XY plane
// To get data argument, data = display_data(display_type); where display_type is the
// name of the display (a string).
module display_subtraction(data)
{
   // PCB
   cube([data[D_PCB_X], data[D_PCB_Y], data[D_PCB_Z]]);

   // Display Window
   translate([data[D_WIN_XO], data[D_WIN_YO], data[D_PCB_Z]])
   {
      cube([data[D_WIN_X], data[D_WIN_Y], data[D_WIN_Z]*3]); // Thick enough to cut through something above.
   }

   // SD Card Access (adding 1 mm top and bottom. Width should be fine.)
   translate([data[D_SD_XO], data[D_SD_YO], -data[D_SD_Z]+1.0])
   {
      cube([data[D_SD_X], data[D_SD_Y]+data[D_SD_YP], data[D_SD_Z]+1.0]);
   }

   // Pins
   translate([data[D_PR_XO], data[D_PR_YO], -1.0*(data[D_PR_ZB]+data[D_PR_PL])])
   {
      cube([data[D_PR_XB], data[D_PR_YB], data[D_PR_ZB]+data[D_PR_PL]]);
   }
   
} // display_subtraction()
   
module display_supports(data, standoff_dia, standoff_len, hole_dia=1.6)
{
   translate([data[D_PIN_XO], data[D_PIN_YO], data[D_PCB_Z]])
   {
      rect_standoff(standoff_dia, hole_dia, standoff_len, data[D_PIN_X], data[D_PIN_Y]);
   }

} // display_supports()

