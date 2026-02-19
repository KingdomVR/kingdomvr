# Standalone Seat Objects

## Overview

Standalone seat objects allow you to create sittable furniture and seating in your world without requiring full vehicle physics. Users can interact with seat objects by hovering over them and pressing the E key (or A on gamepad).

## How to Create a Seat Object

1. **Create or select an object** in the world that you want to make into a seat (e.g., a chair, bench, throne, etc.)

2. **Open the ObjectEditor** for the object

3. **Add a seat script** to the object's Script field. You can use the example script in `docs/seat_script_example.xml` as a starting point.

4. **Configure the seat settings** by adjusting the values in the XML script:
   - `seat_position`: The position in object/model space where the avatar should sit
   - Various angle parameters to control the avatar's pose while sitting
   - Optional hand hold points for IK (inverse kinematics) anchoring

5. **Save the object**

## Using a Seat Object

Once a seat object is configured:

1. **Hover your mouse** over the seat object
2. You should see the tooltip: **"Press [E] to sit"** (or "Press [A] on gamepad to sit")
3. **Press E** (or A on gamepad) to sit on the seat
4. Your avatar will be anchored to the seat with the configured pose
5. **Press F** to stand up and exit the seat

## Configuration Parameters

The seat script supports all the same pose parameters as vehicle seats:

- `seat_position` - Position in object space (Vec3)
- `upper_body_rot_angle` - Torso lean angle
- `upper_leg_rot_angle` - Thigh angle
- `upper_leg_rot_around_thigh_bone_angle` - Thigh rotation
- `upper_leg_apart_angle` - Leg spread
- `lower_leg_rot_angle` - Knee bend
- `lower_leg_apart_angle` - Lower leg spread
- `rotate_foot_out_angle` - Foot rotation
- `arm_down_angle` - Shoulder position
- `arm_out_angle` - Arm spread
- `upper_arm_shoulder_lift_angle` - Shoulder lift
- `lower_arm_up_angle` - Elbow bend
- `left_hand_hold_point_os` - (Optional) Left hand anchor point
- `right_hand_hold_point_os` - (Optional) Right hand anchor point

All angles are in radians.

## Technical Details

Standalone seats use the same SeatSettings structure as vehicles but without requiring vehicle physics or a vehicle controller. The implementation reuses the AvatarEnteredVehicle network message protocol for seats, making them compatible with the existing avatar positioning system.

## Differences from Vehicle Seats

- Seats don't require the object to be dynamic (no physics)
- Seats don't have vehicle controllers
- Seats always have exactly one seat (index 0)
- Seats don't support vehicle-specific features like driving, righting, etc.
- Seats are simpler to configure and use for static furniture

## Example Use Cases

- Chairs in a café or restaurant
- Benches in a park
- Thrones in a castle
- Stadium seating
- Bar stools
- Office chairs
- Any static seating furniture
