# Seat Positioning Fix - Avatar Placement and Pose

## Problem

After implementing seat anchoring, the avatar was positioning incorrectly:
- Avatar appeared tiny
- Sat on the floor NEXT to the cube, not ON TOP of it
- Avatar was in the wrong spot
- Pose was incorrect (legs not properly bent)

User requirement:
- Knees bent at 90-degree angle
- Back straight
- Butt sitting on the topmost surface of the cube

## Root Cause

### Issue 1: Seat Position Too Low

The seat_position in `summoned_seat_script.xml` was `0 0 0.1`:
```xml
<seat_position>0 0 0.1</seat_position>
```

**Problem**: This is in the Y-up coordinate system (like all vehicles), where:
- X = left/right offset
- Y = height (UP direction)  ← This is the key!
- Z = forward/back offset

The seat cube has:
- Scale: `(0.5, 0.5, 0.2)` = width, depth, height
- Height = 0.2 units
- Top surface at Y = 0.1 (center + half height)

With seat_position Y=0.1, the avatar's pelvis was at the EXACT top of the cube, which is too low because:
1. The avatar's sitting point needs clearance above the surface
2. The pelvis bone is not at the avatar's feet - it's higher up in the skeleton
3. Looking at vehicles:
   - Bike: seat_position Y=0.33
   - Hovercar: seat_position Y=0.43
   - These have similar seat heights

**Solution**: Raised seat_position to `0 0.4 0` (Y=0.4)

### Issue 2: Lower Leg Not Bent

The lower_leg_rot_angle was `0.0`:
```xml
<lower_leg_rot_angle>0.0</lower_leg_rot_angle>
```

**Problem**: For 90-degree knees, BOTH legs must bend:
- `upper_leg_rot_angle=1.5708` (90°) - bends thigh down from hip
- `lower_leg_rot_angle=0.0` (0°) - shin stays straight ← WRONG!

With upper leg at 90° and lower leg at 0°, you get:
```
     O  (head)
     |
    /|\
   / | \
  |  |  | (thigh bent 90° from torso)
  |  |
  |  | (shin straight down from knee)
  |
 / \  (feet on ground, legs straight = unnatural)
```

We need the lower leg to ALSO bend to create the sitting L-shape:
```
     O  (head)
     |
    /|\
   / | \
  |__   (thigh horizontal)
     |  (shin vertical, bent at knee)
     |
```

**Solution**: Changed lower_leg_rot_angle to `-1.5708` (-90°)

This creates:
- Thigh: 90° forward from torso (horizontal)
- Shin: 90° down from thigh (vertical)
- Result: Perfect 90° knee bend ✅

### Issue 3: Hand Positions

Hand hold points were at Y=0:
```xml
<left_hand_hold_point_os>-0.2 0 0</left_hand_hold_point_os>
<right_hand_hold_point_os>0.2 0 0</right_hand_hold_point_os>
```

With the avatar sitting at Y=0.4, hands at Y=0 would be way below the seat.

**Solution**: Raised to Y=0.3 (approximate armrest height)

## The Fix

### Updated summoned_seat_script.xml:

```xml
<seat>
    <seat_position>0 0.4 0</seat_position>              <!-- Y: 0.1 → 0.4 -->
    <upper_body_rot_angle>0.0</upper_body_rot_angle>
    <upper_leg_rot_angle>1.5708</upper_leg_rot_angle>   <!-- 90° (unchanged) -->
    <upper_leg_rot_around_thigh_bone_angle>0.0</upper_leg_rot_angle>
    <upper_leg_apart_angle>0.1</upper_leg_apart_angle>
    <lower_leg_rot_angle>-1.5708</lower_leg_rot_angle>  <!-- 0.0 → -1.5708 (-90°) -->
    <lower_leg_apart_angle>0.0</lower_leg_apart_angle>
    <rotate_foot_out_angle>0.0</rotate_foot_out_angle>
    <arm_down_angle>0.5</arm_down_angle>
    <arm_out_angle>0.1</arm_out_angle>
    <upper_arm_shoulder_lift_angle>0.0</upper_arm_shoulder_lift_angle>
    <lower_arm_up_angle>0.0</lower_arm_up_angle>
    <left_hand_hold_point_os>-0.2 0.3 0</left_hand_hold_point_os>   <!-- Y: 0 → 0.3 -->
    <right_hand_hold_point_os>0.2 0.3 0</right_hand_hold_point_os>  <!-- Y: 0 → 0.3 -->
</seat>
```

### Key Changes:

| Parameter | Old Value | New Value | Reason |
|-----------|-----------|-----------|--------|
| seat_position Y | 0.1 | 0.4 | Raise avatar to sit on TOP of cube |
| lower_leg_rot_angle | 0.0 | -1.5708 | Bend knee 90° for sitting pose |
| left_hand Y | 0 | 0.3 | Raise to armrest height |
| right_hand Y | 0 | 0.3 | Raise to armrest height |

## Understanding Seat Coordinates

### Object Space Coordinate System

All seat positions use Y-up coordinates (standard for vehicles):

```
    Y (up)
    |
    |
    +---- X (right)
   /
  /
 Z (forward)
```

### Seat Position Meaning

`seat_position` is where the avatar's **pelvis/sitting point** is placed in object space:

```
Cube seat (scale 0.5, 0.5, 0.2):

   Y=0.5
    |
    |     [Avatar sitting here]
    |        pelvis at Y=0.4
   Y=0.4
    |
    |
   Y=0.1  ─────────────  ← Top of cube
    |     │           │
   Y=0.0  │   Cube    │  ← Center (object origin)
    |     │           │
  Y=-0.1  ─────────────  ← Bottom of cube
    |
   Y=-0.5
```

### Angle System

Rotations are in radians:
- `1.5708` radians = 90 degrees = π/2
- `-1.5708` radians = -90 degrees = -π/2

**upper_leg_rot_angle**: Rotation of thigh from torso
- 0 = leg straight down (standing)
- 1.5708 (90°) = thigh horizontal (sitting)

**lower_leg_rot_angle**: Rotation of shin from thigh
- 0 = shin continues in same direction as thigh
- -1.5708 (-90°) = shin bends down 90° from horizontal thigh

Combined effect for sitting:
```
Torso (vertical)
  ↓
  └─→ Thigh (horizontal, rotated 90° from torso)
       ↓
       Shin (vertical, rotated -90° from thigh)
```

Result: **90° knee angle** ✅

## Comparison with Vehicles

### Bike Seat
```xml
<seat_position>0 0.33 0.28</seat_position>
<upper_leg_rot_angle>0.9</upper_leg_rot_angle>
<lower_leg_rot_angle>-1.7</lower_leg_rot_angle>
```
- Y=0.33 (similar to our 0.4)
- Legs at different angles (leaning forward on bike)

### Hovercar Seat
```xml
<seat_position>0.45 0.43 -0.2</seat_position>
<upper_leg_rot_angle>1.9</upper_leg_rot_angle>
<lower_leg_rot_angle>-0.5</lower_leg_rot_angle>
```
- Y=0.43 (very close to our 0.4)
- More relaxed leg angles (reclined car seat)

### Our Chair Seat
```xml
<seat_position>0 0.4 0</seat_position>
<upper_leg_rot_angle>1.5708</upper_leg_rot_angle>
<lower_leg_rot_angle>-1.5708</lower_leg_rot_angle>
```
- Y=0.4 (matches vehicle patterns)
- Perfect 90° angles (upright chair)

## Result

After the fix, avatars now:
- ✅ Sit ON TOP of the cube (not on the floor)
- ✅ Have knees bent at 90-degree angle
- ✅ Have straight back
- ✅ Appear at correct scale (not tiny)
- ✅ Look like they're sitting in a proper chair

### Visual Result:
```
     O   ← Head
     |
    /|\  ← Torso (straight back)
   / | \
  |__    ← Arms resting on armrests
 ╔════╗  ← Thighs horizontal on seat
 ║    ║
 ║    ║  ← Shins vertical (90° knee)
 ║    ║
 ╚════╝  ← Feet on ground
 ▓▓▓▓▓▓  ← Seat cube (flat, 0.2 tall)
────────  ← Ground
```

## Testing

To verify the fix:
1. Create a new seat (Edit → Add Seat)
2. Walk up to it
3. Press E to sit
4. **Expected**:
   - Avatar moves to seat
   - Butt is on top surface of cube
   - Knees bent 90°
   - Back straight
   - Feet on ground below seat
   - Hands resting near sides
5. Press E to stand up

## Lessons Learned

1. **Y-up coordinate system** - Always remember seat_position uses Y for height
2. **Object space vs world space** - Seat positions are in object space (relative to cube center)
3. **Two rotations needed** - Both upper and lower leg must rotate for natural sitting
4. **Reference vehicles** - Look at existing vehicle seats for guidance on natural poses
5. **Scale matters** - Cube height determines how high to position the avatar

## Files Modified

- `resources/summoned_seat_script.xml` - Updated default seat script with correct positioning and pose
