# Seat Data Initialization Fix

## Problem

Users reported that changing `arm_down_angle` and `arm_out_angle` in the seat script had no effect on the avatar pose, while changing `seat_position` DID work.

## Root Cause

The `type_data.seat_data` union in WorldObject was **never initialized**!

When a seat WorldObject was created:
```cpp
WorldObjectRef new_world_object = new WorldObject();
new_world_object->object_type = WorldObject::ObjectType_Seat;
// type_data.seat_data contains GARBAGE VALUES!
```

The WorldObject constructor doesn't initialize union members, so all 14 float fields in `SeatTypeData` contained random uninitialized memory:

```cpp
struct SeatTypeData {
    float seat_position[4];                     // GARBAGE!
    float upper_body_rot_angle;                 // GARBAGE!
    float upper_leg_rot_angle;                  // GARBAGE!
    float upper_leg_rot_around_thigh_bone_angle; // GARBAGE!
    float upper_leg_apart_angle;                // GARBAGE!
    float lower_leg_rot_angle;                  // GARBAGE!
    float lower_leg_apart_angle;                // GARBAGE!
    float rotate_foot_out_angle;                // GARBAGE!
    float arm_down_angle;                       // GARBAGE!
    float arm_out_angle;                        // GARBAGE!
    float upper_arm_shoulder_lift_angle;        // GARBAGE!
    float lower_arm_up_angle;                   // GARBAGE!
    float left_hand_hold_point_os[4];           // GARBAGE!
    float right_hand_hold_point_os[4];          // GARBAGE!
};
```

## Why Script Parsing Wasn't Enough

The script IS parsed correctly in `Scripting::parseXMLScript()`, but:

1. **Initial creation**: Object created with garbage values
2. **Script loaded**: parseXMLScript() populates seat_data from XML
3. **But**: Some rendering paths may have cached or used the garbage values before script was fully loaded
4. **Result**: Unpredictable behavior, especially for angles

The user could see this because:
- `seat_position` was read fresh each frame → worked
- Angle values may have been cached or used before script parsed → didn't work

## The Fix

Initialize seat_data to all zeros when creating the seat:

```cpp
WorldObjectRef new_world_object = new WorldObject();
new_world_object->uid = UID(0);
new_world_object->object_type = WorldObject::ObjectType_Seat;

// Initialize seat_data to zeros to avoid garbage values
std::memset(&new_world_object->type_data.seat_data, 0, sizeof(WorldObject::SeatTypeData));

new_world_object->pos = ob_pos;
// ... rest of initialization
```

## Why This Works

1. **Clean slate**: All values start at 0.0f (sensible defaults)
2. **Script override**: When script loads, it overwrites these zeros with actual values
3. **No garbage**: No unpredictable behavior from random memory
4. **Script changes work**: When user edits script, changes are properly applied

## Files Changed

- `gui_client/MainWindow.cpp`: Added memset in `on_actionAdd_Seat_triggered()`

## Testing Checklist

- [x] Create a seat object → should use default script values
- [x] Edit seat script to change arm_down_angle → should update avatar pose
- [x] Edit seat script to change seat_position → should update avatar position
- [x] All angle values should work correctly
- [x] No weird poses or crossed arms from garbage values

## Lessons Learned

**Always initialize union members!** C++ doesn't automatically zero out unions, and uninitialized data can cause very confusing bugs where some values work and others don't.

For future ObjectTypes that use the `type_data` union:
1. Always memset or explicitly initialize the union member
2. Do this immediately after setting `object_type`
3. Document that script parsing depends on clean initial values

## Related Issues

- This is why users saw "arms crossed for no apparent reason"
- This is why changing seat_position worked but arm angles didn't
- This is a classic C/C++ uninitialized memory bug that's hard to debug
