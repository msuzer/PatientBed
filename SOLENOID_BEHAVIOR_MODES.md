# Solenoid Behavior Modes

This note summarizes the current solenoid drive modes and possible future modes for hydraulic-style actuator control.

## Current Modes

### Classic Pairs

Architecture:
- 8 complementary pairs.
- Each pair directly drives one actuator direction.
- Common main pump enable follows any active pair.

Behavior:
- Pair `N` forward energizes its forward channel and turns off its backward channel.
- Pair `N` backward energizes its backward channel and turns off its forward channel.
- Releasing the accepted key turns that pair off.

Useful when:
- Each actuator has its own directional valve pair.
- Actuators may be operated independently.

### Shared Direction

Architecture:
- 7 mirrored work pairs.
- 1 complementary direction pair.
- Common main pump enable follows any active pair.

Behavior:
- Work pairs select which actuator path is open.
- The direction pair selects the shared forward/backward hydraulic direction.
- Multiple work pairs can share the same active direction.
- Opposite-direction requests are rejected while the direction pair is active.
- The direction pair turns off after the last work pair releases.

Useful when:
- The hydraulic circuit has one common directional valve and multiple selection valves.
- Simultaneous movement in the same direction is acceptable.

### Future Scenario

Architecture:
- Currently configured like classic pairs.
- Behavior implementation is intentionally a scaffold.

Useful when:
- A third hardware or control policy needs to be added later without disturbing current modes.

## Candidate Future Modes

### Single-Actuator Exclusive

Architecture:
- Same hardware mapping as classic pairs.
- Common main pump enable.

Behavior:
- Only one pair may be active at a time.
- Any press while another pair is active returns a no-op.

Useful when:
- Pump capacity or safety policy allows only one actuator movement at once.
- Simultaneous motion could cause unwanted bed movement or pressure drop.

Suggested enum name:
```cpp
SOL_BEHAVIOR_EXCLUSIVE_PAIR
```

### Shared Direction Exclusive

Architecture:
- Same hardware mapping as shared direction.
- 7 mirrored work pairs.
- 1 complementary direction pair.
- Common main pump enable.

Behavior:
- Only one work pair may be active at a time.
- The direction pair is set before the work pair activates.
- The direction pair turns off when the selected work pair releases.

Useful when:
- The system has one common directional valve and multiple selection valves.
- Only one actuator should move at a time.
- This is often safer than allowing several work pairs to move together.

Suggested enum name:
```cpp
SOL_BEHAVIOR_SHARED_DIRECTION_EXCLUSIVE
```

### Pump Precharge

Architecture:
- Can be combined with classic or shared-style valve layouts.
- Requires timed behavior rather than immediate press/release only.

Behavior:
- On press, the pump starts first.
- After a short precharge delay, the valve pair activates.
- On release, valves turn off first.
- Pump may remain on briefly after valve release.

Useful when:
- The hydraulic circuit benefits from pressure stabilization before valve actuation.
- Motion should start more smoothly.

Suggested enum name:
```cpp
SOL_BEHAVIOR_PUMP_PRECHARGE
```

### Float Capable

Architecture:
- Depends heavily on the actual valve plumbing.
- May require an additional `SolenoidPairState`, such as `FLOAT`.

Behavior:
- Adds a state where a cylinder can float or release pressure.
- The actual output pattern depends on valve type.

Useful when:
- A hydraulic function needs free movement, gravity lowering, or pressure relief.

Suggested enum name:
```cpp
SOL_BEHAVIOR_FLOAT_CAPABLE
```

### Group Interlock

Architecture:
- Can be layered on classic or shared layouts.
- Pair indexes are assigned to compatibility groups.

Behavior:
- Some groups may operate together.
- Other group combinations are rejected.
- Example: head and leg may move together, but height and tilt may be exclusive.

Useful when:
- The hydraulic/mechanical design allows only certain simultaneous motions.
- Safety policy is more nuanced than single-actuator exclusive.

Suggested enum name:
```cpp
SOL_BEHAVIOR_GROUP_INTERLOCK
```

## Practical Next Candidate

The most practical next mode is probably `SOL_BEHAVIOR_SHARED_DIRECTION_EXCLUSIVE`.

It is close to the current shared-direction implementation, but safer:
- Reject new work-pair presses while any other work pair is active.
- Keep the direction pair policy local to the shared behavior module.
- Reuse the same pair configuration as shared direction.
