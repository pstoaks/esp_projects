// More information about roundedcube.scad: 
// https://danielupshaw.com/openscad-rounded-corners/
use <rounded_cube.scad>

//todo Need to move cutout in case up 2 mm relative to screen, or screen
// down 2 mm relative to cutout.

$fn = 80;
in = 25.4;
mm = 1.0;
pin_pitch = 0.1 * in;
round_radius = 2.0; // 2.0 The radius of rounded edges

PCB_DEF = [70 * mm, 50.0 * mm, 1.6 * mm];

display_win_width = 60.0 * mm;
display_win_left_offset = 19.0 * mm-2; // from left edge of PCB
display_thickness = 5.8 * mm;
function display_translation(width, depth, wall_thick, display_y) = 
   [(width - display_win_width)/2.0 - display_win_left_offset, 
   display_y, 
   depth - (display_thickness + wall_thick)];
 
function PIR_xlate(width, depth, wall_thick, y_offset, pir_dia) =
   [width-wall_thick-pir_dia/2.0-1.5,
   y_offset, 
   depth-wall_thick];

knob_dia = 40.0 * mm;
knob_depth = 19.0 * mm;


assembly(knob_dia, knob_depth);
// shaft_attach(10.0 * mm, 10.0);
// knob(knob_dia, knob_depth);
// display();
// littlePIR_sensor();
// PIR_sensor();
// DHT22();
// esp32();
// encoder();
// pcb_assembly();

module assembly(knob_dia, knob_depth)
{
   width = 105.0 * mm;
   length = 135.0 * mm;
   depth = 18.0 * mm;
   wall_thick = 1.2 * mm;

   display_y = length - 60 * mm; // was 56
   PIR_y_offset = length - 70.0 * mm;

   knob_y = 43.0 * mm;
   knob_z_offset = depth-wall_thick-0.4;
   knob_offset = [width/2.0, knob_y, knob_z_offset];

   pcb_z_offset_in_box = 14.0 * mm;

   if ($preview)
   {
%     top_shell(width, length, depth, wall_thick, display_y, knob_offset, knob_dia, PIR_y_offset);
   }
   else
   {
      top_shell(width, length, depth, wall_thick, display_y, knob_offset, knob_dia, PIR_y_offset);
   }
   translate([0, 0, 0])
   {
      // Knob and encoder
      if ($preview)
      {
         translate([knob_offset[0], knob_offset[1], 0])
         {
            translate([0, 0, knob_offset[2]])
               knob(knob_dia, knob_depth);
            translate([0, 0, depth-pcb_z_offset_in_box])
               pcb_assembly();
         }
      }
   }
   if ($preview)
   {
      translate(display_translation(width, depth, wall_thick, display_y))
      {
         display();
      }
   }
   if ($preview)
   {
      translate(PIR_xlate(width, depth, wall_thick, PIR_y_offset, littlePIR_dimensions[DOME_DIA]))
      {
         rotate(0.0)
            littlePIR_sensor();
      }
   }
} //assembly()

module pcb_assembly()
{
   // origin is center of encoder shaft for ease of placement in box
   encoder_pin_offset_from_shaft_center = -3 * pin_pitch;
   encoder_translate = [44, 39, 0]; // Column I, Row 4
   esp32_offset = [8.5, 6.0, 0]; // Column W, Row 17
   pcb_translate = [-encoder_translate[0], -encoder_translate[1], 0];

   translate(pcb_translate)
   {
      pcb(PCB_DEF);
      translate([0, 0, PCB_DEF[2]])
      {
         translate(encoder_translate)
         {
            encoder();
         }
         translate(esp32_offset)
         {
            esp32();
         }
         translate([PCB_DEF[0]-4.0*mm, 19.0 * mm, 0])
         {
            rotate(-90)
               DHT22();
         }
      }
   }

} // pcb_assembly()

module pcb(pcb_def)
{
   color("Green")
      cube(pcb_def);
} // large_pcb()

module encoder()
{
   // Origin is center of shaft
   body_length = 14 * mm;
   body_width = 12.0 * mm;
   body_thick = 6.4 * mm;
   pin_ext = 1.0 * mm;
   pin_width = 6.5 * mm;
   pin_length = 6.85 * mm;
   pin_z_offset = 3.0 * mm;

   translate([-body_length/2.0, -body_width/2.0, 0])
   {
      color("Gray")
         cube([body_length, body_width, body_thick]);
      translate([body_length, (body_width-pin_width)/2.0, pin_z_offset-body_thick])
      {  // pins
         color("Gold")
            cube([pin_ext, pin_width, pin_length]);
      }
      translate([body_length/2.0, body_width/2.0, body_thick])
      {
         encoder_shaft();
      }
   }
} // encoder()

module encoder_shaft()
{
   shaft_dia = 6.0 * mm;
   shaft_length = 21.0 * mm;
   shaft_flat = shaft_dia - 4.5 * mm;

   thread_dia = 6.8 * mm;
   thread_length = 7.0 * mm;

   translate([0, 0, 0])
   {
      difference()
      {
         color("Silver")
            cylinder(d=shaft_dia, h=shaft_length);
         translate([-(shaft_dia*1.1)/2.0, shaft_dia/2.0-shaft_flat, -0.1])
         {
            cube([shaft_dia*1.1, shaft_dia, shaft_length*1.1]);
         }
      }
      color("Silver")
         cylinder(d=thread_dia, h=thread_length);
   }

} // encoder_shaft()

module esp32()
{
   // pin 1 is the origin for ease of positioning on PCB
   width = 26.0 * mm;
   length = 48.5 * mm;
   thick = 13.25 * mm;
   pin_length = 5.75 * mm;
   pin_width = 1.0 * mm;
   pin_pitch = 0.1 * in;
   pin_offset_from_end = pin_pitch/2.0;

   translate([-pin_offset_from_end, width-pin_width/2.0, 0]) rotate(-90.0)
   {
      cube([width, length, thick-pin_length]);
      for (i= [-1, 1])
      {
         translate([width/2.0+i*(width-pin_width*2.0)/2.0, 0, -pin_length])
         {
            translate([-pin_width/2.0, 0, 0])
            {
               color("Gold")
                  cube([1.0, length, pin_length]);
            }
         }
      }

      connector_width = 8.0 * mm;
      connector_thick = 3.0 * mm;
      connector_protrusion = 3.5 * mm;
      top_of_connector_from_bottom_of_pins = 12.4 * mm;
      connector_offset = thick - (connector_thick + top_of_connector_from_bottom_of_pins) + pin_length;
      translate([(width-connector_width)/2.0, -connector_protrusion, connector_offset])
      {
         color("Silver")
            cube([connector_width, connector_protrusion, connector_thick]);
      }
   }
} // esp32()

largePIR_width = 25.0 * mm;
largePIR_depth_below_case = 13.5 * mm;
largePIR_lense_dia = 23.5 * mm;
module largePIR_sensor()
{
   // Origin is at center of sensor dome
   height = 33.0 * mm;
   total_depth = 25.0 * mm;

   translate([0, 0, largePIR_depth_below_case])
      color("White") sphere(d=largePIR_lense_dia);
   
   translate([-largePIR_width/2.0, -height/2.0, 0])
   {
      cube([largePIR_width, height, largePIR_depth_below_case]);
   }
   
} // PIR_sensor()

littlePIR_dimensions = [
   10.5, // hole_diameter
   12.5, // dome diameter
   21.0 // depth below mounting edge
];
HOLE_DIA = 0;
DOME_DIA = 1;
DEPTH = 2;

module littlePIR_sensor_dome()
{
   hole_dia = littlePIR_dimensions[HOLE_DIA];
   dome_dia = littlePIR_dimensions[DOME_DIA];
   mounting_cyl_length = 4.2 * mm;
   color("White")
   {
      difference()
      {
         color("White")
            sphere(d = dome_dia);
         translate([-dome_dia/2.0, -dome_dia/2.0, -dome_dia])
            cube([dome_dia, dome_dia, dome_dia]);
      }
      translate([0, 0, -dome_dia/2.0+mounting_cyl_length/2.0])
         cylinder(d=hole_dia, h=mounting_cyl_length);
   }
}

module littlePIR_sensor()
{
   // XY Origin is at center of sensor dome, Z is at base of dome.
   depth_below_case = littlePIR_dimensions[DEPTH];
   width = 9.0 * mm;
   height = 5.6 * mm;


   littlePIR_sensor_dome();

   translate([-width/2.0, -height/2.0, -depth_below_case])
   {
      cube([width, height, depth_below_case]);
   }

} // littlePIR_sensor()

module DHT22()
{
   width = 15.2 * mm;
   length = 20.25 * mm; // Doesn't count pins or tab
   tab_length = 25.4 * mm - length;
   tab_thick = 1.7 * mm;
   pin_length = 4.0 * mm;
   thickness = 7.8 * mm;
   hole_offset = 2.0 * mm;
   hole_dia = 3.0 * mm;
   pins_width = 8.0 * mm;

   translate([0, 0, 0])
   {
      cube([width, length+tab_length, tab_thick]);
      translate([0, 0, 0])
      {
         cube([width, length, thickness]);
      }
      translate([(width/2.0 - pins_width/2.0), -pin_length, 0])
      {
         pins_depth = 3.0 * mm; // offset of top of pins from bottom
         cube([pins_width, pin_length, pins_depth]);
      }
   }
} // DHT22()

module top_shell(width, length, depth, wall_thick, display_y, knob_offset, knob_dia, pir_y, pir_dia)
{
   standoff_height = 4.3 * mm;
   display_xlate = display_translation(width, depth, wall_thick, display_y);

   difference()
   {
      // Box
      translate([width, 0, depth])
      {
         rotate([0, 180, 0])
         {
            lower_box(width, length, depth, wall_thick, true, round_radius);
         }
      }

      // Display Window
      translate([display_xlate[0], display_y, 0])
      {
         translate([0, 0, depth-4.5])
         {
            display_window(5.0);
         }
      }

      // Knob cutout
      translate([knob_offset[0], knob_offset[1], 10.0])
      {
         cylinder(h=10.0, d=knob_dia*1.01);
      }

      // PIR cutout
      translate(PIR_xlate(width, depth, wall_thick, pir_y, littlePIR_dimensions[DOME_DIA]))
      {
         cylinder(h=40.0, d=littlePIR_dimensions[HOLE_DIA]);
      }

      // USB Cutout
      translate([-5, 15.0, 5.0])
      {
         cube([10, pass_thru_width, pass_thru_height]);
      }
   }

   // USB pass-thru
   pass_thru_depth = wall_thick*1.5;
   pass_thru_width = 12.0 * mm;
   pass_thru_height = 9.0 * mm;
   translate([-(pass_thru_depth-wall_thick)/2.0, 15.0, 5.0])
   {
      difference()
      {
         roundedcube([pass_thru_depth, pass_thru_width+wall_thick, pass_thru_height+wall_thick], false, 1);
         translate([-.5, wall_thick/2.0, wall_thick/2.0])
            roundedcube([pass_thru_depth*1.5, pass_thru_width, pass_thru_height], false, 1);
      }
   }

   // Display Standoffs
   translate(display_xlate)
   {
      translate([0, 0, display_thickness - standoff_height])
         display_supports(5.0, standoff_height+0.5);
   }

   // PCB standoffs
   pcb_z_offset_in_box = 14.0 * mm;
   pcb_standoff_length = pcb_z_offset_in_box - PCB_DEF[2] - wall_thick;
   translate([width/2.0, knob_offset[1], depth - wall_thick - pcb_standoff_length])
   {
      pcb_supports(4.0, pcb_standoff_length+0.5);
   }

} // top_shell()

module lower_box(width, length, depth, wall_thick = 1.2, with_ventilation = true,
   rounding_radius = 0.0)
{
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
         // top_slots
         // slots(2.0, 5.0, width, length);

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
   INTERFERENCE = 0.2 * mm;
   
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


module display()
{
   pcb_width = 86 * mm;
   pcb_thick = 1.5 * mm;
   pcb_height = 50.0 * mm;

   display_left_offset = 9.0 * mm-2; // from left edge of PCB
   display_thick = 5.8 * mm - pcb_thick;
   display_gross_width = 70.0 * mm;
   display_gross_height = pcb_height;

   mounting_hole_dia = 3.0 * mm;

   // pcb
   difference()
   {
      translate([0, 0, 0])
      {
         color("Red")
            cube([pcb_width, pcb_height, pcb_thick]);
      }
      translate([0, 0, -pcb_thick])
         display_supports(mounting_hole_dia, pcb_thick * 3.0);
   }

   // display
   translate([display_left_offset, 0.0, pcb_thick])
   {
      color("Gray")
         cube([display_gross_width, display_gross_height, display_thick]);
   }
   translate([0, 0, pcb_thick+display_thick])
      color("Black")
         display_window(0.1);

   // pins
   pin_width = 4.0 * mm;
   pin_height = 10.0 * mm - pcb_thick;
   pin_row_y = 36.0 * mm;

   translate([0, (pcb_height-pin_row_y)/2.0, -pin_height])
   {
      color("Yellow")
         cube([pin_width, pin_row_y, pin_height]);
   }

} // display()

module display_window(thick)
{
   display_win_height = 45.0 * mm;
   display_win_bottom_offset = (2.8) * mm; // 3.3

   // display window
   translate([display_win_left_offset, display_win_bottom_offset, 0])
   {
      color("Black")
         cube([display_win_width, display_win_height, thick]);
   }
} // display_window()

module display_supports(post_dia, standoff_height)
{
   y_pitch = 44.0 * mm;
   x_pitch = 76.0 * mm;
   screw_dia = 1.6 * mm;
   hole_depth = 4.0 * mm;

   left_offset = 6.8 * mm;
   bottom_offset = (2.5) * mm; // 3.8

   translate([left_offset, bottom_offset, 0])
   {
      rect_standoff(post_dia, screw_dia, hole_depth, standoff_height, x_pitch, y_pitch);
   }

} // display_supports()

module pcb_supports(post_dia, standoff_height)
{
   // Origin is center
   y_pitch = 46.0 * mm;
   x_pitch = 66.0 * mm;
   screw_dia = 1.6 * mm;
   hole_depth = 8.0 * mm;

   left_offset = 2.0 * mm;
   bottom_offset = 2.0 * mm;

   // The following copied from pcb_assembly()
   encoder_translate = [44, 39, 0]; // Column I, Row 4
   pcb_translate = [-encoder_translate[0], -encoder_translate[1], 0];

   translate(pcb_translate)
   {
      translate([left_offset, bottom_offset, 0])
      {
         rect_standoff(post_dia, screw_dia, hole_depth, standoff_height, x_pitch, y_pitch);
      }
   }
} // pcb_supports()

module rect_standoff(post_dia, screw_dia, hole_depth, support_height, rect_width, rect_len)
{
   // post_dia is the diameter of the standoff
   // screw_dia is the diameter of the screw hole
   // hole_depth is the depth of the screw hole
   // support_height is the height at which the board should be supported
   // rect_width is the support rectangle width
   // rect_len is the support rectangle length
   for (x=[0:1])
   {
      for (y=[0:1])
      {
         translate([x*rect_width, y*rect_len, 0.0])
         {
            standoff(post_dia, screw_dia, hole_depth, support_height);
         }
      }
   }
} // support_pins()

module standoff(post_dia, screw_dia, hole_depth, support_height)
{
   // post_dia is the diameter of the standoff
   // screw_dia is the diameter of the screw hole
   // hole_depth is the depth of the screw hole
   // support_height is the height at which the board should be supported
   difference()
   {
      translate([0, 0, support_height/2.0])
      {
         cylinder(h = support_height, d = post_dia, center = true);
      }
      translate([0, 0, support_height - hole_depth])
      {
         cylinder(h = hole_depth+0.1, d = screw_dia, center = true);
      }
   }
} // support_pin()

module knob(knob_dia, knob_depth)
{
   shaft_flat_length = 9.0 * mm;
   shaft_attach_len = shaft_flat_length+1.0;
   shaft_attach_dia = 10.0 * mm;

   difference()
   {
      cylinder(h=knob_depth, d1=knob_dia, d2=knob_dia*0.9, center=false);

      translate([0, 0, -2])
      {
         scale([0.9, 0.9, 0.9])
            cylinder(h=knob_depth+2, d1=knob_dia, d2=knob_dia*0.9, center=false);
      }
   }
   translate([0, 0, knob_depth-shaft_attach_len-1.0])
      shaft_attach(shaft_attach_dia, shaft_attach_len);
} // knob()

module shaft_attach(dia, length)
{
   difference()
   {
      cylinder(h=length, d=dia);
      translate([0, 0, -11])
      {
         scale([1.07, 1.07, 1.0])
            encoder_shaft();
      }
   }
} // shaft_attach()
