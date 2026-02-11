# Seat WorldObject Implementation

## Overview
Seats are now implemented as a standalone WorldObject type (`ObjectType_Seat`), completely separate from the vehicle system. This provides a cleaner, more intuitive implementation.

## Key Changes

### 1. New ObjectType
- Added `ObjectType_Seat = 8` to `WorldObject::ObjectType` enum
- Updated `NUM_OBJECT_TYPES` to 9
- Added string conversion in `objectTypeString()` and `objectTypeForString()`

### 2. SeatTypeData Structure
Added to WorldObject.h:
```cpp
struct SeatTypeData
{
    float seat_position[4];  // Where avatar sits in object space (x, y, z, w)
    float upper_body_rot_angle;
    float upper_leg_rot_angle;  // ~1.57 rad (90°) for sitting
    // ... other pose angles
    float left_hand_hold_point_os[4];  // (x, y, z, w)
    float right_hand_hold_point_os[4]; // (x, y, z, w)
};
```

**Note:** Uses float arrays instead of Vec4f to maintain POD (Plain Old Data) compatibility required for union members.

Added to `TypeData` union:
```cpp
union TypeData
{
    SpotLightTypeData spotlight_data;
    SeatTypeData seat_data;  // NEW
} type_data;
```

### 3. Rendering (GUIClient.cpp)
Seats are rendered when `object_type == ObjectType_Seat`:
- Creates Jolt Physics box shape (0.5m half-extents)
- Uses `getCubeMeshData()` for mesh
- Applies transparent blue-gray material (alpha=0.3)
- Respects object scale for "squashing"

### 4. Interaction
When user presses 'E' on a seat:
- Toggles sit/stand state
- Uses `vehicle_controller_inside` pattern (not vehicle controller for seats)
- Sends `AvatarEnteredVehicle` message to server
- Shows "Press [E] to sit" UI message

## Usage Example

```cpp
// Create a seat
WorldObjectRef seat = new WorldObject();
seat->object_type = WorldObject::ObjectType_Seat;
seat->pos = Vec3d(10, 0, 5);
seat->scale = Vec3f(0.5f, 0.2f, 0.5f); // Squashed cube (flat seat)

// Configure pose (using float arrays now)
seat->type_data.seat_data.seat_position[0] = 0.0f;
seat->type_data.seat_data.seat_position[1] = 0.0f;
seat->type_data.seat_data.seat_position[2] = 0.0f;
seat->type_data.seat_data.seat_position[3] = 1.0f;
seat->type_data.seat_data.upper_leg_rot_angle = 1.5708f; // 90° sitting
```

## Architecture

### Why Separate from Vehicles?
- **Clarity**: Seats aren't vehicles - they don't move
- **Simplicity**: No need for VehicleScript complexity
- **Independence**: Seats work without vehicle system
- **Extensibility**: Easy to add seat-specific features

### Reused Components
Still uses vehicle infrastructure for:
- Avatar anchoring (`PlayerPhysics::enterVehicle/exitVehicle`)
- Sitting pose (`PlayerPhysics::setSittingShape`)
- Network protocol (`AvatarEnteredVehicle` message)

This reuse is practical since "sitting" is essentially being in a stationary "vehicle" with no controls.

## Testing
To test seat functionality:
1. Create a seat object with `object_type = "seat"`
2. Set scale (e.g., `0.5, 0.2, 0.5` for flat seat)
3. Walk up to seat - should see "Press [E] to sit"
4. Press E - avatar should sit down
5. Press E again - avatar should stand up

## Future Work
- Multi-seat support (benches)
- Custom mesh option (not just cubes)
- Configurable materials/colors
- Seat ownership/permissions
