# Seat WorldObject Type

## Overview
The Seat object type allows you to create sittable objects in the KingdomVR metaverse. When users interact with a seat object (by pressing 'E'), they will be anchored to the seat in a sitting position, similar to how vehicles work, but without the ability to drive.

## Features
- **Automatic Visualization**: Seat objects automatically render as transparent, squashed cubes regardless of the model URL specified
- **User Interaction**: Users can press 'E' (or 'A' on gamepad) to sit on the object
- **Custom Sitting Poses**: Configure leg positions, arm positions, and body angles for natural sitting poses
- **Multi-seat Support**: Add multiple seat positions to create benches or group seating
- **Non-drivable**: Unlike vehicles, seats cannot be driven - they are stationary anchoring points

## How to Create a Seat Object

### Method 1: In-Game/Editor
1. Create a new WorldObject
2. Set the object's scale to create a squashed cube appearance (e.g., scale: x=1, y=0.2, z=1)
   - The y-axis typically represents height, so reducing it makes the seat flat
3. Add a seat XML script (see example below) to the object's script field
4. The object will automatically render as a transparent blue-gray cube
5. Position the seat where you want it in the world

### Method 2: XML Configuration
See `docs/seat_object_example.xml` for a complete example with all available options.

## Basic Seat Script

```xml
<?xml version="1.0"?>
<script>
	<seat>
		<seat_position>
			<pos>0 0 0</pos>
			<upper_leg_rot_angle>1.57</upper_leg_rot_angle>
		</seat_position>
	</seat>
</script>
```

## Configuration Options

### Seat Position (`<seat_position>`)
Each `<seat_position>` element defines where an avatar will sit. You can have multiple seat positions for multi-seat objects.

#### Required
- `<pos>`: Position in object/model space (x y z) where the avatar's hips will be anchored

#### Optional Pose Angles (all in radians)
- `<upper_body_rot_angle>`: Upper body rotation (default: 0.0)
- `<upper_leg_rot_angle>`: Thigh rotation, typically ~1.57 (90°) for sitting (default: 0.0)
- `<upper_leg_rot_around_thigh_bone_angle>`: Rotation around the thigh bone axis (default: 0.0)
- `<upper_leg_apart_angle>`: How far apart the legs are (default: 0.0)
- `<lower_leg_rot_angle>`: Lower leg bend angle (default: 0.0)
- `<lower_leg_apart_angle>`: Lower leg spread (default: 0.0)
- `<rotate_foot_out_angle>`: Foot rotation (default: 0.0)
- `<arm_down_angle>`: How far down the arms hang (default: 0.0)
- `<arm_out_angle>`: How far out to the sides the arms are (default: 0.0)
- `<upper_arm_shoulder_lift_angle>`: Shoulder lift angle (default: 0.0)
- `<lower_arm_up_angle>`: Forearm bend angle (default: 0.0)

#### Optional IK Points
- `<left_hand_hold_point_os>`: Position (x y z) for left hand in object space
- `<right_hand_hold_point_os>`: Position (x y z) for right hand in object space

### Model Space Rotations (Advanced)
- `<model_to_y_forwards_rot_1>`: First quaternion rotation (x y z w)
- `<model_to_y_forwards_rot_2>`: Second quaternion rotation (x y z w)

These are typically not needed for simple seats but can be used to orient the seat coordinate system.

## Implementation Details

### Class Structure
- **SeatScript**: Inherits from `VehicleScript`, `isRightable()` returns `false`
- **SeatScriptSettings**: Inherits from `VehicleScriptedSettings`
- Parsed in `Scripting::parseXMLScript()`

### Rendering
Seat objects are rendered with:
- Mesh: Cube mesh from `opengl_engine->getCubeMeshData()`
- Material: Semi-transparent (alpha = 0.3) blue-gray color (RGB: 0.7, 0.7, 0.9)
- Physics: Box shape matching the object's scale

### User Interaction
When a user hovers over a seat object:
- UI shows: "Press [E] to sit" (or "Press [A] on gamepad to sit")
- Pressing E anchors the user to the seat using the vehicle anchoring system
- The user's avatar adopts the configured sitting pose
- Pressing E again exits the seat

## Example Use Cases
- **Chairs**: Single seat position, natural sitting pose
- **Benches**: Multiple seat positions in a row
- **Sofas**: Multiple seat positions with relaxed arm angles
- **Bar Stools**: Higher seat position, legs dangling
- **Floor Cushions**: Low seat position, crossed-leg pose

## Technical Notes
- Seats use the same anchoring system as vehicles, but cannot be driven
- The transparent cube visualization is automatically generated; any `model_url` is ignored
- Physics collision is handled with a box shape scaled to the object's dimensions
- Seats are non-rightable (they don't need to be flipped upright like bikes)
