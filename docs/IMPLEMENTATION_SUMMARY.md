# Standalone Seat Implementation - Technical Summary

## Problem Statement
Users wanted to create standalone seat objects (chairs, benches, etc.) that they could sit on, similar to vehicles, but without requiring vehicle physics or dynamics. The existing implementation only supported seats as part of vehicles (cars, bikes, boats).

## Solution Overview
Implemented a new `SeatScript` system that allows any object to become a sittable seat by adding a simple XML script. The implementation reuses much of the existing vehicle seat infrastructure but simplifies it for static furniture.

## Technical Implementation

### 1. New Classes (Scripting.h)
- `SeatScriptSettings`: Container for seat configuration, holds a single `SeatSettings` object
- `SeatScript`: Main script class for standalone seats, similar to VehicleScript but simpler

### 2. WorldObject Extension (WorldObject.h)
- Added `seat_script` member to store reference to SeatScript
- Added forward declaration for SeatScript namespace

### 3. Script Parsing (Scripting.cpp)
- Extended `parseXMLScript()` to accept `seat_script_out` parameter
- Added parsing for `<seat>` XML element
- Reuses existing `parseSeatSettings()` function

### 4. User Interaction (GUIClient.cpp)

#### Tooltip Display (~line 13956)
```cpp
if(ob->seat_script.nonNull() && vehicle_controller_inside.isNull())
{
    ob_info_ui.showMessage("Press [E] to sit", cursor_gl_coords);
    show_mouseover_info_ui = true;
}
```

#### E Key Handler (~line 15455)
When user presses E on a seat object:
1. Check if not already in vehicle
2. Set `cur_seat_index = 0` (seats always have one seat)
3. Set sitting physics shape
4. Store reference to seat object in avatar's `entered_vehicle` field
5. Send `AvatarEnteredVehicle` message to server (reusing vehicle protocol)

#### Avatar Pose Application
- **Player Avatar** (~line 7755): Applies pose constraints when `vehicle_controller_inside` is null but `entered_vehicle->seat_script` is not null
- **Other Avatars** (~line 7820): Similar logic for remote players

Pose constraint includes:
- Seat position transformation
- Body angles (upper body, legs, arms, feet)
- Hand hold points for IK anchoring

### 5. Documentation
- `docs/seat_script_example.xml`: Complete example with all parameters explained
- `docs/standalone_seats.md`: User guide and technical reference

## Key Design Decisions

### 1. Reuse Vehicle Infrastructure
- Uses existing `SeatSettings` structure
- Reuses `AvatarEnteredVehicle` network protocol
- Follows same pose constraint system
- **Rationale**: Minimizes code changes, maintains consistency, reuses well-tested code

### 2. No Vehicle Controller
- Unlike vehicles, seats don't create a VehiclePhysics controller
- Pose is applied directly from SeatScript settings
- **Rationale**: Seats don't need physics simulation, keeping them simpler and more performant

### 3. Single Seat Per Object
- Each SeatScript contains exactly one SeatSettings (not an array)
- Always uses seat index 0
- **Rationale**: Simplifies the common case; users can place multiple seat objects for multiple seats

### 4. Code Duplication
- Pose application code appears in three places (player avatar, other avatars, vehicles)
- Kept separate rather than extracted to helper function
- **Rationale**: Subtle differences in transformation calculations and state management make extraction complex

## Network Protocol
Reuses existing `AvatarEnteredVehicle` message:
- Object UID: The seat object
- Seat Index: Always 0 for standalone seats
- Flags: Standard flags

This ensures compatibility with existing server code and avatar state management.

## Testing Recommendations

1. **Basic Functionality**
   - Create a simple cube object
   - Add seat script with default parameters
   - Hover over it - should show "Press E to sit"
   - Press E - should sit on the object
   - Press F - should stand up

2. **Pose Configuration**
   - Test various angle parameters
   - Verify avatar posture matches configuration
   - Test hand hold points with actual geometry

3. **Multi-User**
   - Have two users in same world
   - One sits on seat
   - Verify other user sees them sitting correctly

4. **Edge Cases**
   - Try to sit while already sitting
   - Try to sit while in a vehicle
   - Test with scaled objects
   - Test with rotated objects

## Potential Issues & Future Enhancements

### Potential Issues
1. **Object Transform**: Assumes object transform is properly set; may need validation
2. **Concurrent Sitting**: No explicit prevention of multiple users sitting on same seat (but likely handled by existing code)
3. **Dynamic Objects**: What happens if a seat object is made dynamic? May need validation

### Future Enhancements
1. **Multiple Seats**: Extend to support array of seats like vehicles
2. **Exit Position**: Configurable exit position (currently uses generic logic)
3. **Animation**: Custom sitting/standing animations
4. **Seat State**: Visual feedback when seat is occupied
5. **UI Editor**: GUI for configuring seat parameters instead of XML

## Files Modified
- `gui_client/GUIClient.cpp`: +119 lines (tooltip, E key handler, pose application)
- `gui_client/Scripting.cpp`: +17 lines (XML parsing)
- `gui_client/Scripting.h`: +20 lines (new classes)
- `gui_client/ShaderEditorDialog.cpp`: +3 lines (function signature update)
- `shared/WorldObject.h`: +2 lines (seat_script member)
- `docs/seat_script_example.xml`: +55 lines (example)
- `docs/standalone_seats.md`: +73 lines (documentation)

**Total**: 289 lines added, 7 lines modified

## Security Considerations
- No new network messages (reuses existing protocol)
- No file I/O beyond existing XML parsing
- No user input validation needed (all parameters are numeric)
- CodeQL scan passed with no issues

## Conclusion
The implementation successfully adds standalone seat functionality with minimal code changes by intelligently reusing the existing vehicle seat infrastructure. The solution is clean, well-documented, and follows the existing code patterns in the project.
