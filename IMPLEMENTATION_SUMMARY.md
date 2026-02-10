# Seat WorldObject Implementation Summary

## Overview
This implementation adds a new "seat" WorldObject type to KingdomVR that allows users to create interactive seating objects. When users press 'E' on a seat object, they become anchored to it in a sitting position, similar to how vehicle entry works, but without the ability to drive.

## Files Modified

### 1. gui_client/Scripting.h
**Added:**
- `struct SeatScriptSettings : public VehicleScriptedSettings` - Settings container for seat scripts
- `class SeatScript : public VehicleScript` - Main seat script class with `isRightable()` returning `false`

**Purpose:** Define the seat script type within the existing vehicle script infrastructure.

### 2. gui_client/Scripting.cpp  
**Added:**
- XML parsing for `<seat>` elements in `parseXMLScript()` function (lines ~410-435)
- Parses `<seat_position>` sub-elements for multi-seat support
- Uses existing `parseSeatSettings()` function to read pose configuration
- Validates that at least one seat position is defined

**Purpose:** Enable creation of seat objects via XML scripts.

### 3. gui_client/GUIClient.cpp
**Added (around line 3200):**
- Special handling when `SeatScript` is loaded
- Automatically creates transparent cube visualization:
  - Removes any existing OpenGL object
  - Creates cube mesh using `opengl_engine->getCubeMeshData()`
  - Sets semi-transparent material (alpha=0.3, RGB=0.7,0.7,0.9)
  - Creates box physics shape matching object dimensions

**Added (around line 13948):**
- Custom UI message for seats: "Press [E] to sit"
- Type check: `vehicle_script.isType<Scripting::SeatScript>()`

**Purpose:** Provide automatic visualization and user-friendly interaction.

### 4. .gitignore
**Added:**
- `build/` directory to prevent build artifacts from being committed

### 5. docs/SEAT_OBJECTS.md (NEW)
**Contains:**
- Complete user guide for creating and configuring seat objects
- List of all configuration options with descriptions
- Example use cases (chairs, benches, sofas, etc.)
- Technical implementation notes

### 6. docs/seat_object_example.xml (NEW)
**Contains:**
- Fully annotated example seat XML script
- All available options with explanations
- Comments explaining each setting's purpose

## Key Design Decisions

### 1. Reuse Vehicle Infrastructure
**Decision:** Inherit from `VehicleScript` rather than creating a new base class.

**Rationale:**
- Seats need the same anchoring behavior as vehicles
- Seats use the same seat position and pose configuration
- Minimal code changes required
- Consistent with existing codebase patterns

**Trade-offs:**
- Seats appear in vehicle-related code paths
- `isRightable()` must be checked to differentiate seats from drivable vehicles

### 2. Automatic Cube Visualization
**Decision:** Override any model_url and always render seats as transparent cubes.

**Rationale:**
- Meets requirement: "display a seat object as a transparent squashed cube"
- Provides consistent, recognizable appearance for all seats
- Users don't need to find/provide model files
- Transparency clearly indicates interactivity

**Trade-offs:**
- Cannot use custom 3D models for seats
- All seats look the same (except for scale)

**Alternative Considered:** Allow custom models but provide default cube - rejected because problem statement explicitly says "display... as a transparent squashed cube"

### 3. "Squashed" via Scale
**Decision:** Rely on WorldObject scale property to make cubes "squashed".

**Rationale:**
- No code changes needed - scale already supported
- Users can make any dimension smaller (typically y-axis for flat seats)
- Flexible - different seat types can have different proportions

**Example:** Scale (1, 0.2, 1) creates a flat chair seat

### 4. Message: "Press [E] to sit"
**Decision:** Show seat-specific message rather than generic "enter vehicle".

**Rationale:**
- More intuitive for users
- Clearly communicates the object's purpose
- Distinguishes seats from actual vehicles

## How It Works

### Creation Flow
1. User creates WorldObject in editor/game
2. User sets object scale (e.g., y=0.2 for flat seat)
3. User adds XML script with `<seat>` element
4. `loadScriptForObject()` called → `parseXMLScript()` creates `SeatScript`
5. Special handling detects `SeatScript` type:
   - Removes any existing OpenGL mesh
   - Creates cube mesh with transparent material
   - Creates box physics shape

### Interaction Flow
1. User aims cursor at seat object
2. Physics raycast hits seat's box shape
3. `vehicle_script.isType<SeatScript>()` → shows "Press [E] to sit"
4. User presses E → existing vehicle entry code runs:
   - `avatar->entered_vehicle` set to seat object
   - `avatar->vehicle_seat_index` set (usually 0)
   - Avatar anchored to seat position
   - Avatar pose updated based on seat settings
5. User presses E again → exits seat

### Rendering
- **Mesh:** Cube from `opengl_engine->getCubeMeshData()`
- **Material:**
  - `albedo_linear_rgb`: Light blue-gray (0.7, 0.7, 0.9) in linear RGB
  - `alpha`: 0.3 (30% opaque, 70% transparent)
  - `transparent`: true
- **Physics:** Box shape scaled to match WorldObject dimensions

## Configuration Example

```xml
<script>
  <seat>
    <seat_position>
      <pos>0 0 0</pos>
      <upper_leg_rot_angle>1.5708</upper_leg_rot_angle>  <!-- 90°, legs bent -->
    </seat_position>
  </seat>
</script>
```

With object scale: x=0.5, y=0.15, z=0.5 (small, flat seat)

## Testing Recommendations

### Manual Testing
1. **Basic Chair:**
   - Create object with scale (0.5, 0.2, 0.5)
   - Add basic seat script
   - Verify transparent cube renders
   - Press E and verify sitting works

2. **Bench (Multi-seat):**
   - Create object with scale (2, 0.2, 0.5)  
   - Add script with 3 seat positions at different x offsets
   - Verify multiple users can sit

3. **Different Poses:**
   - Test various leg and arm angles
   - Verify avatar pose matches configuration

4. **Physics:**
   - Verify collision detection works
   - Test with collidable and non-collidable settings

### Edge Cases
- Empty seat_settings array → should throw exception ✓
- Very small scale → ensure visible
- Very large scale → ensure reasonable
- Dynamic vs kinematic physics

## Future Enhancements

Possible improvements (not in current scope):
1. **Custom models option:** Allow users to override cube with custom mesh
2. **Material customization:** Let users set seat color/texture
3. **Animation:** Entry/exit animations for sitting down
4. **Sound effects:** Chair creak on sit/stand
5. **Ownership:** Restrict who can use certain seats
6. **Stand command:** Separate key to stand up (currently uses same E key)

## Code Quality

### Review Feedback Addressed
- ✅ Improved error message clarity: "Seat script must define at least one <seat_position> element"
- ✅ Better comment placement: Physics scale comment moved to assignment line
- ✅ Accurate constants: Changed 1.57 to 1.5708 radians (true π/2)

### Security
- ✅ No vulnerabilities detected by CodeQL
- ✅ No new dependencies added
- ✅ Reuses existing security-validated code paths

### Documentation
- ✅ Comprehensive user guide (SEAT_OBJECTS.md)
- ✅ Fully annotated example (seat_object_example.xml)
- ✅ Code comments explain non-obvious behavior

## Conclusion

This implementation successfully adds seat objects to KingdomVR by:
- Leveraging existing vehicle infrastructure
- Providing automatic transparent cube visualization  
- Offering intuitive "Press [E] to sit" interaction
- Supporting flexible configuration via XML
- Maintaining code quality and security standards

The minimal changes required (3 files modified, 2 docs added) demonstrate good architectural design in the existing codebase, allowing feature extension with minimal risk.
