# Compilation Error Fix - Avatar Access

## Problem
The code was calling a non-existent method `getAvatarForUID()` on the `WorldState` class, causing compilation failures:

```
error: 'class WorldState' has no member named 'getAvatarForUID'
```

This occurred in two locations:
- Line 15314: When standing up from a seat
- Line 15419: When sitting down on a seat

## Root Cause
The `WorldState` class doesn't have a `getAvatarForUID()` method. The class only has a public `avatars` member which is a `std::map<UID, Reference<Avatar>>`.

## Solution
Changed the code to use the correct pattern for accessing avatars, which is used throughout the codebase:

### Before (Incorrect):
```cpp
Avatar* client_avatar = this->world_state->getAvatarForUID(this->client_avatar_uid);
if(client_avatar)
{
    // use client_avatar
}
```

### After (Correct):
```cpp
auto res = this->world_state->avatars.find(this->client_avatar_uid);
if(res != this->world_state->avatars.end())
{
    Avatar* client_avatar = res->second.getPointer();
    // use client_avatar
}
```

## Pattern Explanation
The correct pattern for accessing avatars in the codebase:
1. Use `world_state->avatars.find(uid)` to get an iterator
2. Check if the iterator is not equal to `end()`
3. Access the avatar pointer via `res->second.getPointer()` or `res->second.ptr()`

This pattern is used consistently throughout `GUIClient.cpp` and other files in the codebase.

## Files Modified
- `gui_client/GUIClient.cpp` - Fixed both avatar access calls

## Result
The code now compiles successfully and follows the established patterns in the codebase.
