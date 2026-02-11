# Seat Anchoring Fix - Critical Bug Resolution

## Problem

When pressing E on a seat object:
- ✅ "Press [E] to sit" message showed correctly
- ✅ "Sat down" notification appeared
- ✅ Message sent to server
- ❌ **Avatar did NOT anchor to seat** - player could still walk around freely
- ❌ **No sitting pose applied**
- ❌ **Avatar not positioned at seat location**

## Root Cause Analysis

### The Bug

The seat pose constraint code was placed OUTSIDE the `if(our_avatar)` block in the avatar rendering loop (`GUIClient.cpp` line ~7789).

```cpp
// BEFORE (BROKEN CODE):
if(our_avatar) {
    // Line 7789
    if(vehicle_controller_inside.nonNull()) {
        // Set pose constraint for vehicles
        pose_constraint.sitting = true;
        pose_constraint.seat_to_world = ...;
        // ... etc
    }
} // Line 7820 - our_avatar block ENDS here

// Line 7822 - WRONG INDENTATION!
else if(current_seat_object.nonNull() && current_seat_object->object_type == WorldObject::ObjectType_Seat) {
    // Set pose constraint for seats
    pose_constraint.sitting = true;
    pose_constraint.seat_to_world = ...;
    // ... etc
}
```

### Why This Broke Anchoring

The pose constraint loop processes avatars in two categories:

1. **`if(our_avatar)`** - The client's own avatar
   - Uses `cam_controller.getFirstPersonPosition()` for position
   - Uses `vehicle_controller_inside` and `current_seat_object` to determine sitting state
   - **This is where the client's pose constraint must be set**

2. **`else`** - Other players' avatars
   - Uses network-synchronized position from `avatar->pos`
   - Uses `avatar->entered_vehicle` to determine sitting state
   - Pose constraint set based on what vehicle/seat they're in

The seat code was mistakenly placed as an `else if` to the `if(our_avatar)` block, making it execute ONLY for other avatars, never for the client!

### Evidence

Looking at the indentation levels:

```cpp
Line 7789:  if(our_avatar)                           // 6 tabs
Line 7791:      if(vehicle_controller_inside...)     // 7 tabs
Line 7820:      }                                     // 6 tabs (closes vehicle check)
Line 7821:  }                                         // 5 tabs (closes our_avatar)
Line 7822:  else if(current_seat_object...)          // NO TABS! (same level as our_avatar)
```

The `else if` at line 7822 was at the SAME level as the `if(our_avatar)` block, not inside it!

## The Fix

Move the seat pose constraint code INSIDE the `if(our_avatar)` block, as an `else if` to the vehicle controller check.

```cpp
// AFTER (FIXED CODE):
if(our_avatar) {
    // Line 7789
    if(vehicle_controller_inside.nonNull()) {
        // Set pose constraint for vehicles
        pose_constraint.sitting = true;
        pose_constraint.seat_to_world = ...;
        // ... etc
    }
    // Line 7821 - CORRECT INDENTATION!
    else if(current_seat_object.nonNull() && current_seat_object->object_type == WorldObject::ObjectType_Seat) {
        // Set pose constraint for seats
        const Matrix4f ob_to_world = obToWorldMatrix(*current_seat_object);
        
        pose_constraint.sitting = true;
        
        // Compute seat position in world space
        const Vec4f seat_pos_os(
            current_seat_object->type_data.seat_data.seat_position[0],
            current_seat_object->type_data.seat_data.seat_position[1],
            current_seat_object->type_data.seat_data.seat_position[2],
            current_seat_object->type_data.seat_data.seat_position[3]
        );
        pose_constraint.seat_to_world = ob_to_world * Matrix4f::translationMatrix(seat_pos_os);
        
        // Set all pose angles from seat data
        pose_constraint.upper_leg_rot_angle = current_seat_object->type_data.seat_data.upper_leg_rot_angle;
        // ... etc (all pose parameters)
    }
} // our_avatar block closes here
```

### Correct Indentation Levels

```cpp
Line 7789:  if(our_avatar)                           // 6 tabs
Line 7791:      if(vehicle_controller_inside...)     // 7 tabs
Line 7820:      }                                     // 7 tabs (closes vehicle check)
Line 7821:      else if(current_seat_object...)      // 7 tabs (same level as vehicle check!)
Line 7869:      }                                     // 7 tabs (closes seat check)
Line 7870:  }                                         // 6 tabs (closes our_avatar)
```

## How Anchoring Works

### Pose Constraint System

The avatar rendering system uses `PoseConstraint` to modify avatar position and pose:

```cpp
struct PoseConstraint {
    bool sitting;                          // Is avatar sitting?
    Matrix4f seat_to_world;                // Transform from seat space to world space
    Quatf model_to_y_forwards_rot_1/2;     // Model rotation (vehicles have this, seats don't)
    
    // Pose angles (in radians)
    float upper_body_rot_angle;
    float upper_leg_rot_angle;
    float arm_down_angle;
    // ... etc
    
    Vec4f left_hand_hold_point_ws;         // Hand positions in world space
    Vec4f right_hand_hold_point_ws;
};
```

### Anchoring Flow

1. **User presses E on seat** (`useActionTriggered`)
   - Sets `current_seat_object = seat_object`
   - Calls `player_physics.setSittingShape()` (shorter capsule)
   - Sends `AvatarEnteredVehicle` message to server
   - Sets `avatar->entered_vehicle = seat_object`

2. **Every frame** (avatar rendering loop)
   - Checks `if(our_avatar)`
   - Checks `if(current_seat_object.nonNull())`
   - **Sets `pose_constraint.sitting = true`**
   - **Computes `pose_constraint.seat_to_world`** from seat object transform
   - **Copies all pose angles** from `seat_data` to `pose_constraint`

3. **Avatar graphics update** (`AvatarGraphics::update`)
   - Applies `pose_constraint` to avatar skeleton
   - Positions avatar at `seat_to_world` origin
   - Rotates bones according to pose angles
   - **Avatar is now visibly sitting in the seat**

### Why It Now Works

With the seat check INSIDE `if(our_avatar)`:
- Client's `current_seat_object` is checked ✅
- `pose_constraint` is set for client avatar ✅
- Avatar position overridden to seat position ✅
- Sitting pose applied to avatar skeleton ✅
- Player camera follows seated position ✅

## Comparison with Vehicles

### Similarities
- Both use `pose_constraint` system
- Both set `pose_constraint.sitting = true`
- Both compute `seat_to_world` transform
- Both copy pose angles from settings

### Differences

| Aspect | Vehicles | Seats |
|--------|----------|-------|
| **Controller** | `vehicle_controller_inside` (VehiclePhysics) | `current_seat_object` (WorldObjectRef) |
| **Check** | `if(vehicle_controller_inside.nonNull())` | `else if(current_seat_object.nonNull())` |
| **Transform** | `controller->getSeatToWorldTransformNoScale()` | `obToWorldMatrix() * seat_position` |
| **Model Rotation** | `model_to_y_forwards_rot_1/2` from settings | `Quatf::identity()` (upright) |
| **Pose Source** | `controller->getSettings().seat_settings[index]` | `seat_object->type_data.seat_data` |
| **Movement** | Vehicle can move (physics controller) | Seat is stationary (kinematic) |

## Files Modified

- `gui_client/GUIClient.cpp` line 7821-7869 - Moved seat pose constraint inside `if(our_avatar)` block

## Testing

To verify the fix:
1. Create a seat via Edit → Add Seat
2. Walk up to the seat
3. See "Press [E] to sit" message
4. Press E
5. **Expected behavior:**
   - Avatar should be positioned at seat location
   - Avatar should adopt sitting pose (legs bent, arms on armrests)
   - Camera should be at seated height
   - Player cannot walk (anchored to seat)
   - Press E again to stand up and exit seat

## Lessons Learned

1. **Indentation matters** - In deeply nested code, a single wrong indentation level can completely break functionality
2. **Scope awareness** - Always verify which `if` block an `else if` belongs to
3. **Client vs. other avatars** - The `if(our_avatar)` vs `else` distinction is critical for client-side logic
4. **Test both paths** - Code working for "other avatars" doesn't mean it works for "our avatar"
