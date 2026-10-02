# LeapWolf
LeapWolf is a port of id Software's *Wolfenstein 3D* and *Spear of Destiny* to the Leapster2. 

You can find prebuilt binaries in the Releases section.

You must supply your own data files.

## Install
1. Grab the version of Wolfenstein 3D you are going to run from the Releases tab.

| File | Version | Extension |
|---|---|---|
| `wolf.zip` | Wolfenstein 3D Full (v1.4 Activision) | `*.WL6` |
| `wolf1v.zip` | Wolfenstein 3D Shareware (v1.4) | `*.WL1` |
| `sod.zip` | Spear of Destiny (v1.4 Activision) | `*.SOD` |

2. Using a tool like [LFTools](https://github.com/lfhacks/LFTools), mount the device using the command `sudo ./lftools -m 2` or, if on an Admin Windows Command Prompt, `lftools.exe -m 2`. (Alternatively, if your Leapster2 has an SD slot, pop out the SD card and use an SD reader)

3. Create a folder on the root of the SD card named `Wolf3D`. It should sit next to the `Leapster` folder. The directory structure should lay like this:
```
/
├── Leapster
└── Wolf3D
```

4. Extract the ZIP file containing your Wolf3D version and place the folder starting with `LSTR` inside the `/Leapster/Apps` directory. It should now look like this:
```
/
├── Leapster
│   └── Apps
│       ├── LSTR-0x(version)-000000
│       └── ...
└── Wolf3D
```

5. Take your Wolfenstein data files and place them inside the Wolf3D folder. It varies depending on your version, but it should look roughly like this:
```
/
├── Leapster
│   └── Apps
│       ├── LSTR-0x(version)-000000
│       └── ...
└── Wolf3D
    ├── AUDIOHED.WL6
    ├── AUDIOT.WL6
    ├── GAMEMAPS.WL6
    ├── MAPHEAD.WL6
    ├── VGADICT.WL6
    ├── VGAGRAPH.WL6
    ├── VGAHEAD.WL6
    ├── VSWAP.WL6
    └── ...
```

6. Eject your Leapster2's SD card. Or, if using LFTools, run the command `sudo ./lftools -x 2` on Linux or on Windows, `lftools.exe -x 2`, then wait for the Leapster2 to shut down. This will safely flush changes to the device.

7. Turn on your Leapster2, select your username, then click either:
- the arcade cabinet
- the SD card icon

8. Click the icon that corresponds to your downloaded Wolf3D version.
- BJ alone will launch WL6.
- BJ with the number 1 in the corner will launch WL1.
- The Spear starts SOD.

*NOTE: this process will soon be streamlined when doing this through LFTools.*
## Build
To build the game, you need:
- Leapster toolchain, which can be obtained from toadster172's [.DMPSTER](https://github.com/toadster172/.DMPSTER) repository. It must be installed into ~/opt/leapsterSDK (or set SDK=... as an environment variable). You must follow its instructions to build the Leapster toolchain.

If you're building on macOS, install `gmp`, `mpfr`, and `libmpc` with Homebrew, then configure GCC with `--with-system-zlib`, and point it at the Homebrew libraries. `--with-gmp=/opt/homebrew`.
- binToRib, included inside the `tools/` folder. **libelf is a dependency of binToRib.**

Basic build instructions:
```
sh
git clone https://github.com/awes145-neo/LeapWolf.git
cd LeapWolf/tools
make
cd ..
make (target)
```

Refer to the sheet below for the targets. If you need something else, you can configure the Makefile.

| Target| Version | Extension |
|---|---|---|
| `wolf` | Wolfenstein 3D, v1.4 (Activision / GT / id) | `*.WL6` |
| `shareware` | Wolfenstein 3D shareware v1.4 | `*.WL1` |
| `sod` | Spear of Destiny, v1.4 (Activision) | `*.SOD` |

## Controls
| Button | Action | Menu Action |
|---|---|---|
| D-pad | Moving and turning | Move |
| A | Fire | Select / yes |
| B | Use/Run | Select |
| Hint | Strafe | Back |
| Hint + B | Next weapon | |
| Pause | Menu | Back / no |
| Power | End Game, Power Off | Power Off |

<details>
  <summary>DON'T CLICK THIS!</summary>

  This game also has a cheat menu. To access it, hold both **Home** and then **Hint** together.

</details>

## Credits
- toadster172 for the Leapster toolchain and the chorus header.
- DOSBox for their OPL emulator.
- Ripper for the original Wolf4SDL.
- id Software for the great Wolfenstein 3D.

## Licence
This project is licensed under the GPL 3.0 license. The DOSBox OPL emulator and the Leapster toolchain are also licensed. See `LICENSE` for details