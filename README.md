# Wagic (battlebowler mod)

This repository is a modernized fork of Wagic, the Homebrew, focused exclusively on a touch-first Wagic experience for Android. This mod has the following features:

- Touch-first UI with more predictable touch behavior
- 16:9 rendering for all of Wagic and it's assets
- Per-profile themes and a dedicated profile manager
- Reworked shop economy with set rarity and pricing by age/size, single-card promo packs
- Booster pack art variants in the shop
- Added foil cards, per-set booster art with historical foil-era rules
- Trophy Room as a collection browser with set completion progress
- Many in-game quality-of-life, targeting cancel, untap mana, user stack zones and cards enlarged for touch

---

### Build Requirements

- Android Studio
- Android SDK
- Android NDK (required for native components)

---

### Build Instructions

1. Open the Android subproject in Android Studio (wagic\projects\mtg\Android)
2. Allow Gradle to sync  
3. Select a device or emulator  
4. Run the `Wagic` module (debug or release)
