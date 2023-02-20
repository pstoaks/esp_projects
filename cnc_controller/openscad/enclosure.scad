// This file provides parameterized enclosures. Enclosures are made up of a lower_box()
// module that provides the deep part and a lid() that is designed to fit on top and
// inside. The lower_box() can optionally be ventilated with slots in all walls for
// free airflow or just to reduce material usage.
// Also has a simple PCB model with supports.

// Makes use of the rounded_cube.scad library by Daniel Upshaw
// https://danielupshaw.com/openscad-rounded-corners/
use <rounded_cube.scad>

module lower_box(width, length, depth, wall_thick = 1.2, with_ventilation = true,
   rounding_radius = 0.0)
{
   // Set with_ventilation to true to add ventilation slots in walls.
   // Set rounding_radius to some value greater than zero to round edges

   // width, length, and depth are outer dimensions
   difference()
   {
      if (rounding_radius > 0.0)
         roundedcube([width, length, depth], false, rounding_radius, "zmin");
      else
         cube([width, length, depth]);

      translate([wall_thick, wall_thick, wall_thick])
      {
         if (rounding_radius > 0.0)
            roundedcube([width - wall_thick*2, length - wall_thick*2, depth], false, rounding_radius*.9, "zmin");
         else
            cube([width - wall_thick*2, length - wall_thick*2, depth]);
      }

      if (with_ventilation)
      {
         // Side slots
         rotate([0, -90, 0])
            slots(2.0, 5.0, depth, length);
         translate([width, 0, 0])
         {
            rotate([0, -90, 0])
               slots(2.0, 5.0, depth, length);
         }
         // bottom slots
         top_face_insertion = 9;
         translate([width, top_face_insertion, -depth])
         {
            rotate([0, -90, 90])
               slots(2.0, 5.0, depth*2.0, width);
         }
         translate([width, length, 0])
         {
            rotate([0, -90, 90])
               slots(2.0, 5.0, depth, width);
         }
      }
   }
} // lower_box()

module lid(width, length, depth, wall_thick = 1.0, inset_depth = 3.0, rounding_radius)
{
   // inset_depth is the length of all that slips into lower_box.
   // Set rounding_radius to some value greater than zero to round edges
   
   INTERFERENCE = 0.2 * mm; // Offset to allow interference fit into lower_box.
   
   // width, length, and depth are outer dimensions
   lower_box(width, length, depth, wall_thick*2, false, rounding_radius);

   // Inset
   ins_wall = wall_thick + INTERFERENCE;
   translate([ins_wall, ins_wall, depth])
   {
      difference()
      {
         cube([width-ins_wall*2, length-ins_wall*2, inset_depth]);

         translate([ins_wall, ins_wall, -inset_depth/2])
         {
            cube([width-ins_wall*4, length-ins_wall*4, inset_depth*2]);
         }
      }
   }
} // lid()

module slot(width, length)
{
   DEPTH = 12.0 * mm;

   translate([width/2.0, -width/2.0, 0])
      cube([length-width, width, DEPTH]);
   translate([width/2.0, 0, 0])
      cylinder(h=DEPTH, d=width);
   translate([length-width/2.0, 0, 0])
      cylinder(h=DEPTH, d=width);
   translate([-width/2.0, 0, 0])
      rotate([0, 90, 0])
      cylinder(h=length, d=width);
      
} // slot()

module slots(slot_width, slot_spacing, panel_width, panel_length)
{
   // Laid out in the Y direction
   // Slots in a surface
   slot_length = 0.75*panel_width;
   translate([(panel_width-slot_length)/2.0, (slot_width+slot_spacing)/2.0, -2.5])
   {
      for (y = [0:(panel_length-(slot_width+slot_spacing))/(slot_width+slot_spacing)])
         translate([0, y*(slot_width+slot_spacing), 0])
            slot(slot_width, slot_length);
   }
} // slots()

module rect_standoff(post_dia, screw_dia, support_height, rect_width, rect_len)
{
   // post_dia is the diameter of the standoff
   // screw_dia is the diameter of the screw hole
   // support_height is the height at which the board should be supported
   // rect_width is the support rectangle width
   // rect_len is the support rectangle length
   for (x=[0:1])
   {
      for (y=[0:1])
      {
         translate([x*rect_width, y*rect_len, 0.0])
         {
            standoff(post_dia, screw_dia, support_height);
         }
      }
   }
} // rect_standoff()

module standoff(post_dia, screw_dia, support_height)
{
   // post_dia is the diameter of the standoff
   // screw_dia is the diameter of the screw hole
   // support_height is the height at which the board should be supported
   difference()
   {
      translate([0, 0, support_height/2.0])
      {
         cylinder(h = support_height, d = post_dia, center = true);
//         cylinder(h = support_height/2.0, d1=1.0*post_dia, d2=post_dia);
      }
      translate([0, 0, support_height/2.0-0.1])
      {
         cylinder(h = support_height+0.2, d = screw_dia, center = true);
      }
   }
} // standoff()

// Creates a pass_thru in a wall. A hole is created (pass_thru_subtract()) and then thickness
// around the edge and supports are added (if needed). Primarily designed to provide nice
// pass throughs through vertical walls in 3d printed enclosures.
module pass_thru(X_dim, Z_dim, wall_thick, supports=false)
{
   pass_thru_depth = wall_thick*1.5;
   
   translate([-(pass_thru_depth-wall_thick)/2.0, 0.0, 0.0])
   {
      difference()
      {
         roundedcube([pass_thru_depth, X_dim+wall_thick, Z_dim+wall_thick], false, 1);
         translate([-.5, wall_thick/2.0, wall_thick/2.0])
            roundedcube([pass_thru_depth*1.5, X_dim, Z_dim], false, 1);
      }
   }
} // pass_thru()

// Define data as follows:
PCBData =
[
   34.0, // PCB X dimension
   26.6, // PCB Y dimension
   1.5,  // PCB Thickness
   11.0, // PCB Z offset (length of supports)
   3.0,  // Mounting hole diameter
   26.2, // Mounting pin pitch in X direction
   19.7, // Mounting pin pitch in Y direction
   5.06, // Mounting pin left offset (X)
   3.64, // Mounting pin bottom offset (Y)
   4.6   // Mounting standoff max dia
];

P_PCB_X = 0;  // PCB X dimension
P_PCB_Y = 1;  // PCB Y dimension
P_PCB_Z = 2;  // Thickness
P_PCB_ZO = 3; // PCB Z offset (length of supports)
P_PIN_D = 4;  // Mounting hole diameter
P_PIN_X = 5;  // Mounting pin pitch in X direction
P_PIN_Y = 6;  // Mounting pin pitch in Y direction
P_PIN_XO = 7; // Mounting pin left offset (X)
P_PIN_YO = 8; // Mounting pin bottom offset (Y)
P_SO_MAX = 9; // Mounting standoff max dia

module pcb(data, pcb_color)
{
   if ( $preview )
   {
      color(pcb_color)
      {
         difference()
         {
            cube([data[P_PCB_X], data[P_PCB_Y], data[P_PCB_Z]]);
            translate([0, 0, -data[P_PCB_Z]*2.0])
               pcb_supports(data, data[P_PIN_D], data[P_PCB_Z] * 4.0, 0.0);
         }
      }
   }

   // Standoffs
   pcb_supports(data, data[P_SO_MAX], data[P_PCB_ZO]);
} // pcb()

module pcb_supports(data, standoff_dia, standoff_len, hole_dia=1.6)
{
   translate([data[P_PIN_XO], data[P_PIN_YO], data[P_PCB_Z]])
   {
      rect_standoff(standoff_dia, hole_dia, standoff_len, data[P_PIN_X], data[P_PIN_Y]);
   }

} // display_supports()
