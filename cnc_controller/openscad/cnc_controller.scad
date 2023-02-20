// CNC Controller Housing

include <enclosure.scad>
include <tft_display.scad>
include <joystick.scad>

$fn = 80;
in = 25.4;
mm = 1.0;
round_radius = 0.0; // 2.0 The radius of rounded edges

function offset(va, vb) = [va.x+vb.x, va.y+vb.y, va.z+vb.z];

PCB_MCU =
[
   70.0, // PCB X dimension
   50.0, // PCB Y dimension
   1.6,  // PCB Thickness
   17.0, // PCB Z offset (length of supports)
   2.0,  // Mounting hole diameter
   66.0, // Mounting pin pitch in X direction
   46.0, // Mounting pin pitch in Y direction
   2.0,  // Mounting pin left offset (X)
   2.0,  // Mounting pin bottom offset (Y)
   4.4   // Mounting standoff max dia
];

PCB_IO =
[
   70.0, // PCB X dimension
   50.0, // PCB Y dimension
   1.6,  // PCB Thickness
   17.0, // PCB Z offset (length of supports)
   2.0,  // Mounting hole diameter
   66.0, // Mounting pin pitch in X direction
   46.0, // Mounting pin pitch in Y direction
   2.0,  // Mounting pin left offset (X)
   2.0,  // Mounting pin bottom offset (Y)
   4.4   // Mounting standoff max dia
];


// esp32();
assembly();
/*
translate(center_joystick(JoystickData, [0, 89, 0]))
joystick(JoystickData);
translate([40, 80, 0])
joystick_subtraction(JoystickData);
translate(center_display(display_data("3.2_320x240"), [0, 0, 0]))
display("3.2_320x240");
translate([100, 0, 0])
display_subtraction("3.2_320x240");
*/


module assembly()
{
   width = 120.0 * mm; // Y dimension
   length = 170.0 * mm; // X dimension
   depth = 23.0 * mm;
   wall_thick = 1.2 * mm;
   disp_data = display_data("3.2_320x240");

   // TODO: The center of display calculation really needs to be in terms of the top edge. 
   display_center = [length*0.51 - JoystickData[J_PCB_X] + wall_thick, width*0.72 + wall_thick, depth-wall_thick];
   display_location = center_display(disp_data, display_center);
   joystick_center = offset(display_center, [disp_data[D_PCB_X]-5, 0, 0]);
   joystick_location = center_joystick(JoystickData, joystick_center);
   pcba_location = [10.0, 4.0, depth-wall_thick-PCB_MCU[P_PCB_ZO]-PCB_MCU[P_PCB_Z]];
   pcbb_location = [85.0, 4.0, depth-wall_thick-PCB_MCU[P_PCB_ZO]-PCB_MCU[P_PCB_Z]];

   if ($preview)
   {
%     top_shell(length, width, depth, wall_thick, display_location, disp_data, joystick_location, JoystickData);
   }
   else
   {
      top_shell(length, width, depth, wall_thick, display_location, disp_data, joystick_location, JoystickData);
   }

   // Display
   translate(display_location)
   {
      display(disp_data);
   }

   // Joystick
   translate(joystick_location)
   {
      joystick(JoystickData);
   }

   // PCBs
   translate(pcba_location)
      pcb_assembly();

   translate(pcbb_location)
      pcb(PCB_MCU, "Green");
      
} //assembly()

module top_shell(width, length, depth, wall_thick, display_xlate, disp_data, joystick_xlate, joystick_data)
{
   standoff_height = 4.3 * mm;
   with_ventilation = false;
   pass_thru_width = 12.0 * mm;
   pass_thru_height = 9.0 * mm;
   usb_pass_thru_xlate = [0.0, 16.5, 11.0];

   difference()
   {
      // Box
      translate([width, 0, depth])
      {
         rotate([0, 180, 0])
         {
            lower_box(width, length, depth, wall_thick, with_ventilation, round_radius);
         }
      }

      // Display Cutouts
      translate(display_xlate)
      {
         display_subtraction(disp_data);
      }

      // Joystick Cutouts
      translate(joystick_xlate)
      {
         joystick_subtraction(joystick_data);
      }

      // USB Cutout (TODO)
      translate(offset(usb_pass_thru_xlate, [-5.0, 0.5, 0.5]))
      {
         cube([10, pass_thru_width, pass_thru_height]);
      }
   }

   // USB pass-thru
   translate(usb_pass_thru_xlate)
   {
      pass_thru(pass_thru_width, pass_thru_height, wall_thick, true);
   }

} // top_shell()

module pcb_assembly()
{
   esp32_offset = [8.7, 6.3, 7.75]; // Column W, Row 17 

   pcb(PCB_MCU, "Green");
   translate([0, 0, PCB_MCU[P_PCB_Z]])
   {
      translate(esp32_offset)
      {
         esp32();
      }
   }

} // pcb_assembly()

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

   if ( $preview )
   {
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
   }
} // esp32()


