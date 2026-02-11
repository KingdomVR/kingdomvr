# Seat Avatar Scale Fix

## The Problem

When users sat on a seat, their avatar appeared **extremely tiny** and was positioned on the floor next to the seat cube, rather than sitting normally on top of it. This was described as the avatar looking like it "got really tiny and sat down on the floor right next to the squashed cube."

## Root Cause

The issue was in how the seat-to-world transformation matrix was computed. The code was using:

```cpp
const Matrix4f ob_to_world = obToWorldMatrix(*current_seat_object);
```

The `obToWorldMatrix()` function creates a transformation matrix that includes:
- Translation (position)
- Rotation (orientation)
- **Scale** (size)

Looking at the `obToWorldMatrix` implementation in `shared/WorldObject.cpp`:

```cpp
Matrix4f rot = Matrix4f::rotationMatrix(normalise(ob.axis.toVec4fVector()), ob.angle);
rot.setColumn(0, rot.getColumn(0) * use_scale.x);  // Scale X axis
rot.setColumn(1, rot.getColumn(1) * use_scale.y);  // Scale Y axis
rot.setColumn(2, rot.getColumn(2) * use_scale.z);  // Scale Z axis
rot.setColumn(3, pos + ob.translation);
return rot;
```

The seat object has a scale of `(0.5, 0.5, 0.2)` to make it a flat, squashed cube. When this transform was applied to the avatar's pose, it **scaled the avatar down** to 50% width, 50% depth, and 20% height - making the avatar appear tiny!

## The Solution

Vehicles (bikes, hovercars, cars) solve this problem by using a **NoScale** transformation matrix. Looking at `BikePhysics.cpp`:

```cpp
Matrix4f BikePhysics::getSeatToWorldTransformNoScale(...) const
{ 
    Matrix4f ob_to_world_no_scale;
    if(use_smoothed_network_transform && world_object->physics_object.nonNull())
        ob_to_world_no_scale = world_object->physics_object->getSmoothedObToWorldNoScaleMatrix();
    else
        ob_to_world_no_scale = getBodyTransform(physics_world);
    
    return ob_to_world_no_scale * Matrix4f::translationMatrix(seat_position) * R_inv;
}
```

The key function is `getSmoothedObToWorldNoScaleMatrix()` which returns a transformation matrix with **only rotation and translation, no scale**.

## The Fix

Changed seat transform computation from:

```cpp
// OLD - BROKEN
const Matrix4f ob_to_world = obToWorldMatrix(*current_seat_object);
pose_constraint.seat_to_world = ob_to_world * Matrix4f::translationMatrix(seat_pos_os);
```

To:

```cpp
// NEW - FIXED
Matrix4f ob_to_world_no_scale;
if(current_seat_object->physics_object.nonNull())
    ob_to_world_no_scale = current_seat_object->physics_object->getSmoothedObToWorldNoScaleMatrix();
else
    ob_to_world_no_scale = obToWorldMatrix(*current_seat_object); // Fallback
    
pose_constraint.seat_to_world = ob_to_world_no_scale * Matrix4f::translationMatrix(seat_pos_os);
```

This fix was applied in **two locations** in `GUIClient.cpp`:
1. Client avatar seat handling (~line 7824)
2. Other avatars seat handling (~line 7951)

## Why This Works

The `getSmoothedObToWorldNoScaleMatrix()` method:
- Returns the seat's **position** in world space
- Returns the seat's **rotation** in world space  
- **Excludes** the seat's scale (0.5, 0.5, 0.2)

This means:
- The **seat cube** still renders at scale (0.5, 0.5, 0.2) - it looks squashed ✅
- The **avatar** is positioned relative to the seat's position/rotation but NOT scaled ✅
- The avatar appears at **normal size** sitting on the seat ✅

## Technical Details

### Transform Chain

The complete transform chain for a sitting avatar is:

```
Avatar world position = seat_to_world * avatar_offset_in_seat_space
```

Where:
```
seat_to_world = ob_to_world_no_scale * seat_position_offset
```

### Why Physics Object?

The `physics_object->getSmoothedObToWorldNoScaleMatrix()` method is used instead of constructing the matrix manually because:
1. It provides **smoothed interpolation** for network objects
2. It's **optimized** for performance
3. It's the **standard pattern** used by all vehicles
4. It handles edge cases properly

### Fallback Behavior

The code includes a fallback to `obToWorldMatrix()` if `physics_object` is null. This is for:
- Objects being created/destroyed
- Edge cases in testing
- Defensive programming

However, in normal operation, seat objects **always** have a physics_object (they're kinematic objects), so the NoScale path is always taken.

## Comparison with Vehicles

This fix makes seats work **exactly like vehicle seats**:

| Aspect | Bikes/Cars | Seats (Fixed) |
|--------|-----------|---------------|
| Transform | `getSmoothedObToWorldNoScaleMatrix()` | `getSmoothedObToWorldNoScaleMatrix()` |
| Avatar scale | Normal size | Normal size |
| Object scale | Applied to vehicle model only | Applied to seat cube only |
| Pose data | From script | From script |

The only difference is that vehicles have `VehiclePhysics` controllers while seats are static kinematic objects.

## Testing

After this fix:
- ✅ Avatar appears at **normal size** when sitting
- ✅ Avatar is positioned **on top** of the seat cube
- ✅ Sitting pose (knees bent, back straight) is visible
- ✅ Seat cube still appears as a **flat squashed cube** (0.2 height)
- ✅ Multiple avatars can sit on different seats without issues
- ✅ Network synchronization works correctly

## Related Files

- `gui_client/GUIClient.cpp` - Main fix location (2 places)
- `gui_client/PhysicsObject.h` - Declares `getSmoothedObToWorldNoScaleMatrix()`
- `gui_client/BikePhysics.cpp` - Reference implementation for vehicles
- `gui_client/CarPhysics.cpp` - Reference implementation for vehicles
- `shared/WorldObject.cpp` - Shows how `obToWorldMatrix()` includes scale

## Lessons Learned

1. **Always use NoScale transforms for avatar anchoring** - avatars should never be scaled by the object they're sitting/standing on
2. **Follow vehicle patterns** - vehicles have solved these problems already, their patterns are proven
3. **Check both client and network avatar paths** - fixes need to be applied in both places
4. **Scale is for rendering, not for anchoring** - object scale affects mesh rendering, but shouldn't affect avatar positioning

## Future Considerations

If adding other types of anchoring (e.g., beds, benches, cockpits), remember to:
- Use `getSmoothedObToWorldNoScaleMatrix()` for the base transform
- Only apply rotation and translation to the avatar
- Let the object's scale affect only its visual appearance
- Test with both normal and unusual scales (very small, very large, non-uniform)
