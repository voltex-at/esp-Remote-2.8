#pragma once
#include <stdint.h>

enum class ThemeMode : uint8_t { Dark = 0, Light = 1 };

namespace ThemeSettings {
constexpr ThemeMode normalize(int value) {
    return value == 1 ? ThemeMode::Light : ThemeMode::Dark;
}
// Keep schema1's 264-byte Settings blob intact. Older firmware ignores this
// independent NVS key; an upgrade without the key keeps the existing dark UI.
template<class Store> ThemeMode load(Store& store) {
    return normalize(store.getUChar("theme", 0));
}
template<class Store> bool save(Store& store, ThemeMode mode) {
    return store.putUChar("theme", static_cast<uint8_t>(normalize(int(mode)))) == 1;
}
}
