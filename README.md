##### If you represent Activision and want this fork to be deleted, please contact me first.
___
# Client extension for Call of Duty ([CoD1](https://en.wikipedia.org/wiki/Call_of_Duty_(video_game)))
It works for the multiplayer patch 1.1  
You can get info for this game at [cod.pm](https://cod.pm/)
___
### This adds improvements, such as:
#### Security

- Prevent servers from making you to download non authorized file extensions.
- Prevent servers from making you to change sensitive cvars.

#### Bug fixes

- Fix the CD-Key error issue occurring after joining a modded server.

#### New features

- Sensitivity aim multiplier
  - Multiply you mouse speed when aiming, e.g: `/sensitivityAimMultiply 0.5`
- Fast downloading support
  - If the server enabled HTTP downloading, you will not download using the default capped speed.
- Minimizing and Alt-Tab support
  - You can enter `/minimize` or press Alt-Tab to switch to a window.
___
### Installation instructions

You can build this project for free using [Visual Studio](https://en.wikipedia.org/wiki/Visual_Studio) Community edition.  
You can build with the Release configuration to avoid the debugging setup.

1. Where your *CoDMP* file is located, rename *mss32* to *mss32_original*
2. Put the compiled *mss32* file at this location.

The game will now get hooked with the extension.
___
### Notes

- 11/20/2023: Fork creation, more is planned
