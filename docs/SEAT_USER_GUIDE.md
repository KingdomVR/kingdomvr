# Seat WorldObject - User Guide

## What are Seats?

Seats are a new type of WorldObject (`ObjectType_Seat`) that users can sit on. They appear as transparent blue-gray cubes that can be "squashed" into various shapes using the scale property.

## Quick Start

### Creating a Seat

**Method 1: Using the GUI Menu (Easiest)**
1. Launch the application
2. Go to **Edit** → **Add Seat**
3. A seat will appear at ground level in front of you
4. The seat is a transparent blue-gray cube (flat/squashed shape)
5. You can then edit its position, rotation, and scale as needed

**Method 2: In the Editor**
   - Create a new object
   - Set `object_type` to `"seat"` (or `ObjectType_Seat` in code)
   - Set scale to desired dimensions (e.g., `x=0.5, y=0.2, z=0.5` for a flat chair)
   - Position it where you want

**Method 3: In Code**
   ```cpp
   WorldObjectRef seat = new WorldObject();
   seat->object_type = WorldObject::ObjectType_Seat;
   seat->pos = Vec3d(10, 0, 5);
   seat->scale = Vec3f(0.5f, 0.2f, 0.5f); // Width, Height, Depth
   
   // Optional: Configure sitting pose (using float arrays)
   seat->type_data.seat_data.seat_position[0] = 0.0f;
   seat->type_data.seat_data.seat_position[1] = 0.0f;
   seat->type_data.seat_data.seat_position[2] = 0.0f;
   seat->type_data.seat_data.seat_position[3] = 1.0f;
   seat->type_data.seat_data.upper_leg_rot_angle = 1.5708f; // 90 degrees
   ```

### Using a Seat

1. Walk up to the seat
2. You'll see: "Press [E] to sit"
3. Press `E` to sit down
4. Press `E` again to stand up

## Customizing Seats

### Dimensions (via scale)
- **Chair**: `scale = (0.5, 0.5, 0.5)` - regular cube
- **Flat Seat**: `scale = (0.5, 0.2, 0.5)` - squashed for bench/chair
- **Bench**: `scale = (2.0, 0.2, 0.5)` - long and flat
- **Floor Cushion**: `scale = (0.6, 0.1, 0.6)` - very flat

### Sitting Pose
Configure the avatar's pose via `type_data.seat_data`:

```cpp
// Default sitting pose
seat->type_data.seat_data.upper_leg_rot_angle = 1.5708f;  // 90° (legs bent)
seat->type_data.seat_data.arm_down_angle = 0.5f;          // Arms relaxed
seat->type_data.seat_data.upper_body_rot_angle = 0.0f;    // Upright

// For a relaxed lounge chair
seat->type_data.seat_data.upper_leg_rot_angle = 2.0f;     // Legs more extended
seat->type_data.seat_data.upper_body_rot_angle = -0.3f;   // Lean back

// For a floor cushion (cross-legged)
seat->type_data.seat_data.upper_leg_rot_angle = 2.0f;     
seat->type_data.seat_data.upper_leg_apart_angle = 0.5f;   // Legs apart
```

## Properties Reference

### SeatTypeData Fields

All angles are in **radians** (π radians = 180 degrees, π/2 radians = 90 degrees):

| Field | Type | Description | Typical Value |
|-------|------|-------------|---------------|
| `seat_position` | float[4] | Where avatar sits (object space) | `(0, 0, 0, 1)` |
| `upper_body_rot_angle` | float | Torso lean (forward/back) | `0.0` (upright) |
| `upper_leg_rot_angle` | float | Thigh rotation | `1.5708` (90° sitting) |
| `upper_leg_rot_around_thigh_bone_angle` | float | Thigh twist | `0.0` |
| `upper_leg_apart_angle` | float | How far legs are apart | `0.1` |
| `lower_leg_rot_angle` | float | Knee bend | `0.0` |
| `lower_leg_apart_angle` | float | Lower leg spread | `0.0` |
| `rotate_foot_out_angle` | float | Foot rotation | `0.0` |
| `arm_down_angle` | float | Shoulder drop | `0.5` |
| `arm_out_angle` | float | Arms away from body | `0.1` |
| `upper_arm_shoulder_lift_angle` | float | Shoulder raise | `0.0` |
| `lower_arm_up_angle` | float | Elbow bend | `0.0` |
| `left_hand_hold_point_os` | float[4] | Left hand IK target | `(0, 0, 0, 1)` |
| `right_hand_hold_point_os` | float[4] | Right hand IK target | `(0, 0, 0, 1)` |

## Examples

### Simple Chair
```cpp
seat->scale = Vec3f(0.5f, 0.5f, 0.5f);
seat->type_data.seat_data.seat_position[0] = 0.0f;
seat->type_data.seat_data.seat_position[1] = 0.25f; // Raised a bit
seat->type_data.seat_data.seat_position[2] = 0.0f;
seat->type_data.seat_data.seat_position[3] = 1.0f;
seat->type_data.seat_data.upper_leg_rot_angle = 1.5708f;
```

### Park Bench
```cpp
seat->scale = Vec3f(2.0f, 0.3f, 0.6f); // Long, slightly thick
seat->type_data.seat_data.seat_position[0] = 0.0f;
seat->type_data.seat_data.seat_position[1] = 0.0f;
seat->type_data.seat_data.seat_position[2] = 0.0f;
seat->type_data.seat_data.seat_position[3] = 1.0f;
seat->type_data.seat_data.upper_leg_rot_angle = 1.5708f;
seat->type_data.seat_data.arm_down_angle = 0.3f; // Arms more relaxed
```

### Gaming Chair (with armrests)
```cpp
seat->scale = Vec3f(0.6f, 0.6f, 0.6f);
seat->type_data.seat_data.seat_position[0] = 0.0f;
seat->type_data.seat_data.seat_position[1] = 0.2f;
seat->type_data.seat_data.seat_position[2] = 0.0f;
seat->type_data.seat_data.seat_position[3] = 1.0f;
seat->type_data.seat_data.upper_leg_rot_angle = 1.5708f;
seat->type_data.seat_data.arm_down_angle = 1.2f; // Arms on armrests
seat->type_data.seat_data.lower_arm_up_angle = 1.3f; // Forearms bent
seat->type_data.seat_data.left_hand_hold_point_os[0] = -0.25f;
seat->type_data.seat_data.left_hand_hold_point_os[1] = 0.1f;
seat->type_data.seat_data.left_hand_hold_point_os[2] = 0.0f;
seat->type_data.seat_data.left_hand_hold_point_os[3] = 1.0f;
seat->type_data.seat_data.right_hand_hold_point_os[0] = 0.25f;
seat->type_data.seat_data.right_hand_hold_point_os[1] = 0.1f;
seat->type_data.seat_data.right_hand_hold_point_os[2] = 0.0f;
seat->type_data.seat_data.right_hand_hold_point_os[3] = 1.0f;
```

### Meditation Cushion
```cpp
seat->scale = Vec3f(0.6f, 0.08f, 0.6f); // Very flat
seat->type_data.seat_data.seat_position[0] = 0.0f;
seat->type_data.seat_data.seat_position[1] = 0.04f; // Very low
seat->type_data.seat_data.seat_position[2] = 0.0f;
seat->type_data.seat_data.seat_position[3] = 1.0f;
seat->type_data.seat_data.upper_leg_rot_angle = 2.1f; // Legs crossed
seat->type_data.seat_data.upper_leg_apart_angle = 0.6f; // Legs wide apart
seat->type_data.seat_data.upper_body_rot_angle = 0.0f; // Upright/meditative
```

## Technical Details

### Physics
- Seats use kinematic physics (don't move)
- Box shape with 0.5m half-extents (scaled by object scale)
- Can be collidable or non-collidable via `collidable` flag

### Rendering
- Always renders as transparent cube mesh
- Material: Blue-gray (RGB: 0.7, 0.7, 0.9), Alpha: 0.3
- Affected by scale property for dimensions
- No custom models supported (by design)

### Network
- Uses existing `AvatarEnteredVehicle` protocol
- Always uses `seat_index = 0`
- Server tracks who is sitting where

### Limitations
- Currently single-occupancy (one person per seat)
- No custom mesh support (always a cube)
- No material customization (always transparent blue-gray)
- Pose configuration requires code (not in UI yet)

## Troubleshooting

**Seat doesn't appear:**
- Check `object_type` is set to `"seat"` or `ObjectType_Seat`
- Verify object is in world and loaded

**Can't sit down:**
- Make sure object has physics enabled
- Check you're not already in a vehicle/seat
- Verify seat has been added to physics world

**Avatar pose looks wrong:**
- Adjust angle values (remember: radians, not degrees)
- Check `seat_position` is reasonable (near origin of object)
- Try default values first, then tweak

**Seat is too small/large:**
- Adjust `scale` property
- Remember scale multiplies the 1-meter cube dimensions

## Future Features
- Multi-seat support for benches
- Custom mesh option
- Configurable colors/materials
- UI for pose editing
- Seat ownership/permissions
