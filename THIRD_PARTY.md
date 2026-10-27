# Third-party components

## Bundled in this repository

| Files | Component | License | Notice |
|---|---|---|---|
| `assets/fonts/Roboto-Regular.ttf`, `Roboto-Bold.ttf`, `RobotoCondensed-Regular.ttf`, `RobotoCondensed-Bold.ttf` | Roboto and Roboto Condensed, Copyright 2011 Google Inc. | Apache License 2.0 | `assets/fonts/Roboto-LICENSE.txt` |
| `assets/fonts/Inconsolata-Medium.ttf` | Inconsolata, Copyright 2006 The Inconsolata Project Authors | SIL Open Font License 1.1 | `assets/fonts/Inconsolata-OFL.txt` |
| `assets/fonts/Jost-Light.ttf`, `Jost-Medium.ttf` | Jost* 3.7, Copyright 2020 The Jost Project Authors | SIL Open Font License 1.1 | `assets/fonts/Jost-OFL.txt` |
| `assets/fonts/ChakraPetch-Light.ttf` | Chakra Petch 1.000, Copyright 2018 The Chakra Petch Project Authors | SIL Open Font License 1.1 | `assets/fonts/ChakraPetch-OFL.txt` |
| `assets/fonts/Barlow-SemiBold.ttf` | Barlow 1.408, Copyright 2017 The Barlow Project Authors | SIL Open Font License 1.1 | `assets/fonts/Barlow-OFL.txt` |
| `assets/fonts/ShareTechMono-Regular.ttf` | Share Tech Mono 1.003, Copyright 2012 Carrois Type Design, Ralph du Carrois; Reserved Font Name: Share | SIL Open Font License 1.1 | `assets/fonts/ShareTechMono-OFL.txt` |
| `assets/fonts/Font_Awesome_7_Free-Solid-900.otf`, `Font_Awesome_7_Brands-Regular-400.otf` | Font Awesome Free 7, Fonticons, Inc. | Fonts: SIL OFL 1.1; icons: CC BY 4.0 | `assets/fonts/FontAwesome-LICENSE.txt` |
| `assets/branding/FusionUI/fonts/Audiowide-Regular.ttf` | Audiowide Regular, Brian J. Bonislawsky / Astigmatic | SIL Open Font License 1.1 | `assets/branding/FusionUI/fonts/Audiowide-OFL.txt` |

Brand icons in Font Awesome Brands are trademarks of their respective owners. The license files
are installed next to the fonts by `fusionui_install_assets`.

Game-style font provenance (unmodified upstream static TrueType files):

- Jost Light and Medium: [indestructible-type/Jost, revision 35f141c970538f1ed0f235789c19156b3ce2a762](https://github.com/indestructible-type/Jost/tree/35f141c970538f1ed0f235789c19156b3ce2a762/fonts/ttf).
  Upstream filenames `Jost-300-Light.ttf` and `Jost-500-Medium.ttf` are shortened locally; internal font names remain unchanged. The OFL comes from the same revision.
- Chakra Petch Light: [google/fonts, revision e7db6ca603fd4862010383ff17ffd6cc4ffac1c0](https://github.com/google/fonts/tree/e7db6ca603fd4862010383ff17ffd6cc4ffac1c0/ofl/chakrapetch).
- Barlow SemiBold: [google/fonts, same revision](https://github.com/google/fonts/tree/e7db6ca603fd4862010383ff17ffd6cc4ffac1c0/ofl/barlow).
- Share Tech Mono Regular: [google/fonts, same revision](https://github.com/google/fonts/tree/e7db6ca603fd4862010383ff17ffd6cc4ffac1c0/ofl/sharetechmono).

Each Google Fonts OFL is copied from its font's directory at that revision. These open fonts
approximate the games' typography; no proprietary game fonts are bundled.

## Fetched at build time (not in this repository)

xmake downloads these from xmake-repo; their licenses apply to the built binaries.

| Package | License |
|---|---|
| Dear ImGui 1.92.7 (`imgui`, DX11 backend) | MIT |
| nlohmann/json (`nlohmann_json`) | MIT |
| stb (`stb`, image loading) | MIT or public domain |
| doctest (`doctest`, tests only) | MIT |

## Supplied by the consuming project

fusionui does not include CommonLibF4 or spdlog; the plugin that uses fusionui provides them (both MIT).
F4SE and Fallout 4 are not part of this project.
