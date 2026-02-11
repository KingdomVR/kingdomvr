# Seat WorldObject Implementation

## Overview
Seats are implemented as a standalone WorldObject type (`ObjectType_Seat`), completely separate and independent from the vehicle system.

## Key Differences from Vehicles

**Seats are NOT vehicles:**
- Seats are stationary (don't move)
- Seats don't use `VehiclePhysics` controllers
- Seats have their own tracking (`current_seat_object` in GUIClient)
- Seats don't call any vehicle-specific methods
- Vehicle code was only used as a reference for how avatars are anchored to objects

## How to Add a Seat

### Via GUI Menu
1. Open the application
2. Go to **Edit** → **Add Seat**
3. A seat will be created at ground level in front of you
4. The seat appears as a transparent blue-gray cube (squashed flat by default)

### Programmatically
See examples in `docs/SEAT_USER_GUIDE.md`

## Key Changes

### 1. New ObjectType
- Added `ObjectType_Seat = 8` to `WorldObject::ObjectType` enum
- Updated `NUM_OBJECT_TYPES` to 9
- Added string conversion in `objectTypeString()` and `objectTypeForString()`

### 2. SeatTypeData Structure (POD-compatible)
```cpp
struct SeatTypeData
{
    float seat_position[4];              // (x, y, z, w) - POD arrays, not Vec4f
    float upper_body_rot_angle;
    float upper_leg_rot_angle;           // ~1.57 rad (90°) for sitting
    // ... other pose angles
    float left_hand_hold_point_os[4];    // POD arrays
    float right_hand_hold_point_os[4];
};
```

**Important:** Uses `float[4]` arrays instead of `Vec4f` to maintain POD (Plain Old Data) compatibility required for union members.

### 3. Rendering (GUIClient.cpp)
Seats are rendered when `object_type == ObjectType_Seat`:
- Creates Jolt Physics box shape (0.5m half-extents)
- Uses `getCubeMeshData()` for mesh
- Applies transparent blue-gray material (alpha=0.3)
- Respects object scale for "squashing"

### 4. Interaction (Completely Independent)
Seat interaction is tracked separately from vehicles:
- Uses `current_seat_object` (WorldObjectRef) to track if sitting
- Does NOT use `vehicle_controller_inside`
- When user presses 'E' on seat:
  - Sets `current_seat_object = seat`
  - Calls `player_physics.setSittingShape()`
  - Sends `AvatarEnteredVehicle` protocol message (for server sync only)
- When user presses 'E' while sitting:
  - Calls `player_physics.setStandingShape()`
  - Clears `current_seat_object`
  - Sends `AvatarExitedVehicle` protocol message

### 5. Menu Integration
- Added "Add Seat" to Edit menu in MainWindow.ui
- Implemented `on_actionAdd_Seat_triggered()` in MainWindow.cpp
- Creates seat with default sitting pose parameters

## Usage Example

```cpp
// Create a seat (done automatically via menu)
WorldObjectRef seat = new WorldObject();
seat->object_type = WorldObject::ObjectType_Seat;
seat->pos = Vec3d(10, 0, 5);
seat->scale = Vec3f(0.5f, 0.2f, 0.5f); // Squashed cube (flat seat)

// Configure pose (using float arrays)
seat->type_data.seat_data.seat_position[0] = 0.0f;
seat->type_data.seat_data.seat_position[1] = 0.0f;
seat->type_data.seat_data.seat_position[2] = 0.0f;
seat->type_data.seat_data.seat_position[3] = 1.0f;
seat->type_data.seat_data.upper_leg_rot_angle = 1.5708f; // 90° sitting
```

## Why Protocol Reuse?

The code uses `Protocol::AvatarEnteredVehicle` and `Protocol::AvatarExitedVehicle` messages, but this is ONLY for:
- Synchronizing avatar position with server
- Using existing network infrastructure
- Avatar state tracking on server side

**This does NOT mean seats are vehicles!** The protocol messages are just a convenient way to tell the server "avatar is now attached to this object" without creating entirely new protocol messages.

## Architecture

### Separation from Vehicles
```
Vehicles:
- Use VehiclePhysics controllers
- Can move/be driven
- Tracked via vehicle_controller_inside
- Have driving controls

Seats:
- No controllers (stationary)
- Cannot move
- Tracked via current_seat_object
- No driving controls
- Just anchor avatar position
```

### Reused Components (Infrastructure Only)
- Avatar anchoring system (common for both)
- Sitting physics shape
- Network protocol messages (convenience)
- These are shared infrastructure, not vehicle-specific features

## Testing
To test seat functionality:
1. Launch gui_client
2. Edit → Add Seat
3. Walk up to seat - should see "Press [E] to sit"
4. Press E - avatar should sit down
5. Press E again - avatar should stand up

## Future Work
- Multi-seat support (benches)
- Custom mesh option (not just cubes)
- Configurable materials/colors
- Seat ownership/permissions

