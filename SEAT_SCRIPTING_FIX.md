# Seat Scripting and Anchoring Fix - Complete Implementation

## Issues Fixed

### 1. Script in Wrong Field
**Problem**: XML script was in `content` field instead of `script` field
**Solution**: 
- Changed `MainWindow.cpp` to set `new_world_object->script` 
- Load from `resources/summoned_seat_script.xml` (like bikes/hovercars)

### 2. No XML Parsing for Seats
**Problem**: No parsing logic in `Scripting.cpp` for seat objects
**Solution**:
- Added "static seat" section to `parseXMLScript()` function
- Parses `<seat>` element and populates `type_data.seat_data`
- Reuses existing `parseSeatSettings()` function

### 3. Anchoring Didn't Work
**Problem**: Avatar wasn't anchored to seat position
**Solution**:
- Added seat anchoring logic in `GUIClient.cpp` for client avatar (line ~7822)
- Added seat anchoring logic for other avatars (line ~7946)
- Reads from `type_data.seat_data` and applies to `pose_constraint`

## Implementation Details

### Resource File: `resources/summoned_seat_script.xml`
```xml
<?xml version="1.0" encoding="utf-8"?>
<script>
	<seat>
		<seat_position>0 0 0.1</seat_position>
		<upper_body_rot_angle>0.0</upper_body_rot_angle>
		<upper_leg_rot_angle>1.5708</upper_leg_rot_angle>
		<!-- ... more pose angles ... -->
	</seat>
</script>
```

### Scripting.cpp - Seat Parsing
Added after car section in `parseXMLScript()`:
```cpp
// ----------- static seat -----------
{
	pugi::xml_node seat_elem = root_elem.child("seat");
	if(seat_elem && ob.nonNull() && ob->object_type == WorldObject::ObjectType_Seat)
	{
		SeatSettings seat_settings = parseSeatSettings(seat_elem, default_seat_settings);
		
		// Copy to object's type_data.seat_data
		ob->type_data.seat_data.seat_position[0] = seat_settings.seat_position.x;
		// ... copy all fields ...
	}
}
```

### GUIClient.cpp - Anchoring
For client avatar (checks `current_seat_object`):
```cpp
else if(current_seat_object.nonNull() && current_seat_object->object_type == WorldObject::ObjectType_Seat)
{
	const Matrix4f ob_to_world = obToWorldMatrix(*current_seat_object);
	pose_constraint.sitting = true;
	pose_constraint.seat_to_world = ob_to_world * Matrix4f::translationMatrix(seat_pos_os);
	// ... set all pose angles from type_data.seat_data ...
}
```

For other avatars (checks `avatar->entered_vehicle`):
```cpp
else if(avatar->entered_vehicle.nonNull() && avatar->entered_vehicle->object_type == WorldObject::ObjectType_Seat)
{
	// Same logic using avatar->entered_vehicle instead
}
```

## How It All Works Together

1. **Creation** (MainWindow.cpp):
   - User creates seat via menu
   - Script loaded from `resources/summoned_seat_script.xml`
   - Set to `new_world_object->script` field

2. **Script Loading** (GUIClient.cpp):
   - When object loads, `loadScriptForObject()` is called
   - Calls `Scripting::parseXMLScript()`

3. **Parsing** (Scripting.cpp):
   - Detects `<seat>` element
   - Checks `ob->object_type == ObjectType_Seat`
   - Parses settings and populates `type_data.seat_data`

4. **Sitting** (GUIClient.cpp):
   - User presses E, enters seat
   - Sets `avatar->entered_vehicle` to seat object
   - Sets `current_seat_object` for client

5. **Anchoring** (GUIClient.cpp every frame):
   - Checks if sitting on seat object
   - Reads pose from `type_data.seat_data`
   - Computes `seat_to_world` transform
   - Sets `pose_constraint` fields
   - Avatar rendering uses constraint to position avatar

## Key Differences from Vehicles

| Aspect | Vehicles | Static Seats |
|--------|----------|--------------|
| Script Field | Uses `vehicle_script` Reference | Uses `type_data.seat_data` direct |
| Controller | Has VehiclePhysics controller | No controller (stationary) |
| Tracking (client) | `vehicle_controller_inside` | `current_seat_object` |
| Tracking (avatar) | `avatar->entered_vehicle` | `avatar->entered_vehicle` (same) |
| Model Rotation | Has `model_to_y_forwards_rot_1/2` | Identity (upright) |
| Anchoring Check | `vehicle_controller_inside.nonNull()` | `current_seat_object.nonNull()` |

## Files Modified

1. **resources/summoned_seat_script.xml** (NEW) - Default seat XML
2. **gui_client/Scripting.cpp** - Added seat parsing
3. **gui_client/MainWindow.cpp** - Load script from resources
4. **gui_client/GUIClient.cpp** - Added seat anchoring logic (2 places)

## Testing

To test:
1. Create seat via Edit → Add Seat
2. Walk to seat, press E
3. Avatar should sit with proper pose
4. Avatar should be anchored to seat position
5. Press E again to stand up
