#include "input_router.h"

#include "solenoid_behavior.h"

namespace {

// ---------------------------------------------------------
// Key → solenoid mapping
// ---------------------------------------------------------
constexpr uint8_t KEY_ID_SOL1_F = 93;
constexpr uint8_t KEY_ID_SOL1_B = 39;
constexpr uint8_t KEY_ID_SOL2_F = 83;
constexpr uint8_t KEY_ID_SOL2_B = 38;
constexpr uint8_t KEY_ID_SOL3_F = 73;
constexpr uint8_t KEY_ID_SOL3_B = 37;
constexpr uint8_t KEY_ID_SOL4_F = 63;
constexpr uint8_t KEY_ID_SOL4_B = 36;
constexpr uint8_t KEY_ID_SOL5_F = 53;
constexpr uint8_t KEY_ID_SOL5_B = 35;
constexpr uint8_t KEY_ID_SOL6_F = 43;
constexpr uint8_t KEY_ID_SOL6_B = 34;

constexpr uint8_t KEY_ID_SOL7_F = 48;
constexpr uint8_t KEY_ID_SOL7_B = 84;
constexpr uint8_t KEY_ID_SOL8_F = 47;
constexpr uint8_t KEY_ID_SOL8_B = 74;

struct KeyBinding {
    uint8_t keyId;
    uint8_t pairIndex;
    SolenoidPairState state;
};

KeyBinding keyBindings[] = {
    {KEY_ID_SOL1_F, 0, SolenoidPairState::FORWARD}, {KEY_ID_SOL1_B, 0, SolenoidPairState::BACKWARD},
    {KEY_ID_SOL2_F, 1, SolenoidPairState::FORWARD}, {KEY_ID_SOL2_B, 1, SolenoidPairState::BACKWARD},
    {KEY_ID_SOL3_F, 2, SolenoidPairState::FORWARD}, {KEY_ID_SOL3_B, 2, SolenoidPairState::BACKWARD},
    {KEY_ID_SOL4_F, 3, SolenoidPairState::FORWARD}, {KEY_ID_SOL4_B, 3, SolenoidPairState::BACKWARD},
    {KEY_ID_SOL5_F, 4, SolenoidPairState::FORWARD}, {KEY_ID_SOL5_B, 4, SolenoidPairState::BACKWARD},
    {KEY_ID_SOL6_F, 5, SolenoidPairState::FORWARD}, {KEY_ID_SOL6_B, 5, SolenoidPairState::BACKWARD},
    {KEY_ID_SOL7_F, 6, SolenoidPairState::FORWARD}, {KEY_ID_SOL7_B, 6, SolenoidPairState::BACKWARD},
    {KEY_ID_SOL8_F, 7, SolenoidPairState::FORWARD}, {KEY_ID_SOL8_B, 7, SolenoidPairState::BACKWARD},
};

constexpr uint8_t KEY_BINDING_COUNT = sizeof(keyBindings) / sizeof(keyBindings[0]);
bool keyAccepted[KEY_BINDING_COUNT] = {false};

int find_binding_for_key(uint8_t keyId) {
    for (uint8_t i = 0; i < KEY_BINDING_COUNT; ++i) {
        if (keyBindings[i].keyId == keyId) {
            return i;
        }
    }
    return -1;
}

void clear_event(InputRouterEvent *event, bool pressed) {
    if (!event) {
        return;
    }

    event->recognized = false;
    event->accepted = false;
    event->pairIndex = 0;
    event->state = SolenoidPairState::OFF;
    event->pressed = pressed;
}

Result handle_binding_press(uint8_t bindIndex, InputRouterEvent *event) {
    KeyBinding &binding = keyBindings[bindIndex];

    event->recognized = true;
    event->pairIndex = binding.pairIndex;
    event->state = binding.state;

    if (binding.pairIndex >= SOLENOID_PAIR_COUNT) {
        keyAccepted[bindIndex] = false;
        return RES_PARAM;
    }

    Result result = solenoid_behavior_press(binding.pairIndex, binding.state);
    if (result == RES_NOOP) {
        keyAccepted[bindIndex] = false;
        return RES_NOOP;
    }

    if (result != RES_OK) {
        keyAccepted[bindIndex] = false;
        return result;
    }

    keyAccepted[bindIndex] = true;
    event->accepted = true;
    return RES_OK;
}

Result handle_binding_release(uint8_t bindIndex, InputRouterEvent *event) {
    KeyBinding &binding = keyBindings[bindIndex];

    event->recognized = true;
    event->pairIndex = binding.pairIndex;
    event->state = binding.state;

    if (!keyAccepted[bindIndex]) {
        return RES_NOOP;
    }

    if (binding.pairIndex >= SOLENOID_PAIR_COUNT) {
        return RES_PARAM;
    }

    Result result = solenoid_behavior_release(binding.pairIndex, binding.state);
    if (result == RES_NOOP) {
        keyAccepted[bindIndex] = false;
        return RES_NOOP;
    }

    if (result != RES_OK) {
        return result;
    }

    keyAccepted[bindIndex] = false;
    event->accepted = true;
    return RES_OK;
}

} // namespace

bool input_router_is_bound_key(uint8_t keyId) {
    return find_binding_for_key(keyId) >= 0;
}

Result input_router_handle_key_event(uint8_t keyId, bool pressed, InputRouterEvent *event) {
    clear_event(event, pressed);

    int bindIndex = find_binding_for_key(keyId);
    if (bindIndex < 0) {
        return RES_NOOP;
    }

    if (pressed) {
        return handle_binding_press((uint8_t)bindIndex, event);
    }

    return handle_binding_release((uint8_t)bindIndex, event);
}
