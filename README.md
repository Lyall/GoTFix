# Ghost of Tsushima Fix
[![Patreon-Button](https://github.com/Lyall/GoTFix/assets/695941/57085682-1790-4ffe-be46-b9e3c4d5e034)](https://www.patreon.com/Wintermance) [![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/W7W01UAI9)<br />
[![Github All Releases](https://img.shields.io/github/downloads/Lyall/GoTFix/total.svg)](https://github.com/Lyall/GoTFix/releases)

This is a fix for Ghost of Tsushima that can disable letterboxing in cutscenes, adjust FOV to compensate and more.<br />

## Features
- Disable letterboxing in cutscenes/dialogue etc.
- Adjust cutscene FOV to compensate for lack of letterboxing.
- Crop letterboxing from cutscene videos at >16:9s.
- Removes 5.333~ aspect ratio limit.

## Installation
- Grab the latest release of GoTFix from [here.](https://github.com/Lyall/GoTFix/releases)
- Extract the contents of the release zip in to the the game's folder.<br />(e.g. "**steamapps\common\Ghost of Tsushima DIRECTOR'S CUT**" for Steam).

### Steam Deck/Linux Additional Instructions
🚩**You do not need to do this if you are using Windows!**
- Open up the game properties in Steam and add `WINEDLLOVERRIDES="d3d12=n,b" %command%` to the launch options.

## Configuration
- See **GoTFix.ini** to adjust settings for the fix.

## Known Issues
Please report any issues you see.
This list will contain bugs which may or may not be fixed.

## Screenshots

| ![ezgif-2-20eaea88bf](https://github.com/Lyall/GoTFix/assets/695941/99dc3ad0-7168-43b3-8cb5-08d5c435e955) |
|:--:|
| Disabled Letterboxing With Compensated FOV |

| ![ezgif-7-024f28e2eb](https://github.com/Lyall/GoTFix/assets/695941/6dcc330a-155d-422f-8c1f-6a2190445fc7) |
|:--:|
| Letterboxing Cropped in Cutscene Video |

## Credits
Thanks to Ersh and others on the WSGF Discord for testing!<br />
[Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) for ASI loading. <br />
[inipp](https://github.com/mcmtroffaes/inipp) for ini reading. <br />
[spdlog](https://github.com/gabime/spdlog) for logging. <br />
[safetyhook](https://github.com/cursey/safetyhook) for hooking.
