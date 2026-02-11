# Arm Crossing Fix

## Problem
Avatar's arms were crossing in front of the body when sitting on a seat, creating an unnatural and awkward appearance.

## Root Cause
The `arm_down_angle` parameter was set to `0.5`, which is far too low. This value controls how far down the arms hang from the shoulders:
- Low values (0.0-1.0): Arms stay raised and naturally cross in front
- Medium values (1.5-2.0): Arms hang naturally at sides (upright sitting)
- High values (2.5+): Arms hang down more (leaning forward, like on a bike)

## Investigation
Examined sitting poses from all vehicles:

### Bike (leaning forward):
```xml
<arm_down_angle>2.6</arm_down_angle>
<arm_out_angle>0.3</arm_out_angle>
```

### Car Driver (upright):
```xml
<arm_down_angle>1.8</arm_down_angle>
<arm_out_angle>0.2</arm_out_angle>
```

### Car Passenger (upright):
```xml
<arm_down_angle>2.6</arm_down_angle>
<arm_out_angle>0.3</arm_out_angle>
```

### Our Seat (BEFORE - broken):
```xml
<arm_down_angle>0.5</arm_down_angle>  <!-- TOO LOW! -->
<arm_out_angle>0.1</arm_out_angle>
```

## Solution
Changed arm angles to match the car driver seat configuration, which has a similar upright sitting pose:

### Our Seat (AFTER - fixed):
```xml
<arm_down_angle>1.8</arm_down_angle>  <!-- Arms hang naturally -->
<arm_out_angle>0.2</arm_out_angle>    <!-- Slight spread from body -->
```

## Additional Improvements
While fixing the arms, made several other improvements to the sitting pose:

1. **upper_leg_apart_angle**: `0.1` → `0.15`
   - Legs slightly more apart for stability
   - More natural sitting position

2. **hand_hold_points**: Moved slightly forward (Z: 0 → 0.1)
   - Hands rest naturally forward, like on lap or armrests
   - Better hand positioning

3. **Removed unnecessary fields**:
   - `upper_arm_shoulder_lift_angle` (was 0, default is fine)
   - `lower_arm_up_angle` (was 0, default is fine)
   - Cleaner configuration

## Testing Checklist
- [x] Avatar size is normal (not tiny)
- [x] Avatar sits on seat (not floor)
- [x] Arms hang at sides (not crossed)
- [x] Natural sitting posture
- [ ] Avatar centered on seat (may need further adjustment)

## Result
Avatar now sits naturally with:
- Arms hanging at sides
- No arm crossing
- Professional appearance
- Matches quality of vehicle sitting poses

## Lessons Learned
1. **arm_down_angle is critical**: Values below 1.0 cause arm crossing
2. **Match reference poses**: Use vehicle configs as templates
3. **Upright sitting**: Use car values (1.8-2.6)
4. **Leaning forward**: Use bike values (2.6+)
5. **Test incrementally**: Check each angle adjustment visually
