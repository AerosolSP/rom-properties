# ROM Properties Page shell extension

This shell extension adds a few nice features to file browsers for managing
video game ROM and disc images.

[![License: GPL v2](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html)<br>
[![Travis Build Status](https://travis-ci.org/GerbilSoft/rom-properties.svg?branch=master)](https://travis-ci.org/GerbilSoft/rom-properties)
[![AppVeyor Build status](https://ci.appveyor.com/api/projects/status/5lk15ct43jtmhejs/branch/master?svg=true)](https://ci.appveyor.com/project/GerbilSoft/rom-properties/branch/master)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/10146/badge.svg)](https://scan.coverity.com/projects/10146)

## v1.2 - The Dreamcast Release

This release adds, among other things, previewing for Dreamcast disc images. ISO (2048-byte sector) and BIN (2352-byte sector) raw tracks are supported, as well as GDI cue sheets.

![Dreamcast disc images on KDE5](doc/img/rp.kde5.Dreamcast.GDI.png)

Along with this, general support for Sega PVR and GVR textures has been added, as well as DirectDraw Surfaces. Note that BC6 and BC7 compression isn't supported yet.

_TODO: Add image showing PVR and GVR textures._

VMU save files have been supported since v0.9-beta2.

_TODO: Add image showing VMU save files._

See [CHANGES.md](CHANGES.md) for a full list of changes in v1.2.

## Feedback

This is a work in progress; feedback is encouraged. To leave feedback, you
can file an issue on GitHub, or visit the Gens/GS IRC channel:
[irc://irc.badnik.zone/GensGS](irc://irc.badnik.zone/GensGS)

Or use the Mibbit Web IRC client: http://mibbit.com/?server=irc.badnik.zone&channel=#GensGS

## Installation

Currently, the ROM Properties Page shell extension is compatible with the
following platforms:
* KDE 4.x
* KDE Frameworks 5.x
* XFCE (GTK+ 2.x)
* GNOME and Unity (GTK+ 3.x)
* Windows XP (and later)

For instructions on compiling from source, see doc/COMPILING.md .

### Linux

Install the relevant .deb package, depending on what desktop environment you
are using and what your CPU is. Note that you may want to install the KDE4
package even if using KDE5, since many KDE programs still use the 4.x libraries.

After installing, the plugin needs to be enabled in the Dolphin file browser:
* Close all instances of Dolphin.
* Start Dolphin.
* Click Control, Configure Dolphin.
* Click the "General" section, then the "Preview" tab.
* Check the "ROM Properties Page" item, then click OK.
* Enable previews in a directory containing a supported file type.

If installed correctly, thumbnails should be generated for the supported
file type. You can also right-click a file, select Properties, then click
the "ROM Properties" tab to view more information about the ROM image.

### Windows

Extract the ZIP archive to a directory, then run install.exe. The installer
requires administrator access, so click "Yes" if requested. In the installer,
click the "Install" button to register the ROM Properties Page DLL.

Note that this will hard-code the location of the DLL files in the registry,
so you may want to place the DLLs in a common location.

To uninstall the plugin, run install.exe again, then click the "Uninstall"
button.

## Current ROM Feature Support Level

|             System            | Properties Tab | Internal Images | External Scans |
|:-----------------------------:|:--------------:|:---------------:|:--------------:|
| Sega Mega Drive               |       Yes      |       N/A       |       No       |
| Sega Dreamcast Saves          |       Yes      |   Icon, Banner  |       No       |
| Nintendo DS(i)                |       Yes      |       Icon      |  Covers, Box   |
| Nintendo GameCube             |       Yes      |      Banner     |      Disc      |
| Nintendo GameCube Saves       |       Yes      |       Icon      |       N/A      |
| Nintendo Wii                  |       Yes      |        No       |  Disc, Covers  |
| Nintendo Game Boy (Color)     |       Yes      |       N/A       |       No       |
| Nintendo Game Boy Advance     |       Yes      |       N/A       |       No       |
| Nintendo 64                   |       Yes      |       N/A       |       No       |
| Nintendo Virtual Boy          |       Yes      |       N/A       |       No       |
| Sony PlayStation Saves        |       Yes      |       Icon      |       N/A      |
| Nintendo amiibo               |       Yes      |        No       |      Media     |
| Nintendo Entertainment System |       Yes      |       N/A       |       No       |
| Windows/DOS Executables       |       Yes      |        No       |       N/A      |
| Nintendo Wii U                |       Yes      |        No       |  Disc, Covers  |
| Nintendo 3DS                  |       Yes      |       Icon      |  Covers, Box   |
| Sega 8-bit (SMS, GG)          |       Yes      |       N/A       |       No       |
| Sega PVR Textures             |       Yes      |      Image      |       N/A      |
| Microsoft DirectDraw Surface  |       Yes      |      Image      |       N/A      |
| Nintendo Badge Arcade         |       Yes      |      Image      |       N/A      |
| Sega Dreamcast                |       Yes      |      Media      |       No       |
| Sega Saturn                   |       Yes      |       N/A       |       No       |
| Atari Lynx                    |       Yes      |       N/A       |       No       |

Notes:
* Internal image refers to artwork contained within the ROM and/or disc image.
  These images are typically displayed on the system's main menu prior to
  starting the game.
  * "N/A" here means the ROM or disc image doesn't have this feature.
  * "No" indicates that the feature is present but not currently implemented.
* External scans refers to scans from an external database, such as GameTDB.com
  for GameCube and Wii.
  * "No" indicates no database is currently available for this system.
  * Anything else indicates what types of images are available.
  * For amiibo, "media" refers to the amiibo object, which may be a figurine,
    a card, or a plush.
* Windows executables may contain multiple icon sizes. Support for Windows icons
  will be added once support for multiple image sizes is added.
* Sega 8-bit only supports ROM images with a "TMR SEGA" header.

An initial configuration program is included with the Windows version of
rom-propreties 1.0. This allows you to configure which images will be used for
thumbnails on each system. The functionality is available on Linux as well, but
the UI hasn't been ported over yet. See `doc/rom-properties.conf.example` for
an example configuration file, which can be placed in `~/.config/rom-properties`.

## File Types Supported

* Sega Mega Drive: Plain binary (\*.gen, \*.bin), Super Magic Drive (\*.smd)
* Sega Dreamcast: Save files (\*.vmi, \*.vms, \*.dci)
* Nintendo DS(i): Plain binary (\*.nds, \*.dsi, \*.srl)
* Nintendo GameCube: 1:1 disc image (\*.iso, \*.gcm) [including DiscEx-shrunken images],
  CISO disc image (\*.ciso), TGC embedded disc image (\*.tgc), save files (\*.gci, \*.gcs, \*.sav)
* Nintendo Wii: 1:1 disc image (\*.iso, \*.gcm), WBFS disc image (\*.wbfs),
  CISO disc image (\*.ciso), WIA disc image (\*.wia)
* Nintendo Game Boy: Plain binary (\*.gb, \*.gbc, \*.sgb)
* Nintendo Game Boy Advance: Plain binary (\*.gba, \*.agb, \*.mb, \*.srl)
* Nintendo 64: Various byteswap formats (\*.z64, \*.n64, \*.v64)
* Nintendo Virtual Boy: Plain binary (\*.vb)
* Sony PlayStation: Save files (\*.psv, \*.mcb, \*.mcx, \*.pda, \*.psx, \*.mcs, \*.ps1)
* Nintendo amiibo: Plain binary (\*.bin, \*.nfc, \*.nfp)
* Nintendo Entertainment System: iNES dumps (\*.nes), FDS dumps (\*.fds, \*.qd),
  3DS Virtual Console dumps (\*.tds)
* Windows/DOS: Executables (\*.exe, \*.dll, others)
* Nintendo Wii U: 1:1 disc image (\*.wud)
* Nintendo 3DS: Icon files (\*.smdh), homebrew (\*.3dsx), cartridge images
  (\*.3ds, \*.cci), importable archives (\*.cia), eMMC dumps (\*.bin),
  title contents (\*.ncch, \*.app), and firmware binaries (\*.firm, \*.bin)
  * Encryption keys are needed for encrypted cartridge images, importable
    archives, and title contents.
* Sega 8-bit: Sega Master System and Game Gear ROM images (\*.sms, \*.gg)
* Sega PVR Textures: Dreamcast PVR (\*.pvr), GameCube GVR (\*.gvr)
* Microsoft DirectDraw Surface: DDS files (\*.dds)
  * Currently supports uncompressed RGB, DXTn, BC4, and BC5.
* Nintendo Badge Arcade: Badge files (\*.prb), badge set files (\*.cab)
* Sega Dreamcast: Raw disc image files (\*.bin, \*.iso), GD-ROM cuesheets
  (\*.gdi)
* Sega Saturn: Raw disc image files (\*.bin, \*.iso)
* Atari Lynx: Headered binary (\*.lnx)

## External Media Downloads

Certain parsers support the use of external media scans through an online
database, e.g. GameTDB.com. The current release of the ROM Properties Page
shell extension will always attempt to download images from GameTDB.com if
thumbnail preview is enabled in the file browser and a supported file is
present in the current directory. An option to disable automatic downloads
will be added in a future version.

Downloaded images are cached to the following directory:
* Linux: `~/.cache/rom-properties/`
* Windows: `%LOCALAPPDATA%\rom-properties\cache`

The directory structure matches the source site, so e.g. a disc image of
Super Smash Bros. Brawl would be downloaded to
`~/.cache/rom-properties/wii/disc/US/RSBE01.png`. Note that if the download
fails for any reason, a 0-byte dummy file will be placed in the cache,
which tells the shell extension not to attempt to download the file again.
[FIXME: If the download fails due to no network connectivity, it shouldn't
do this.]

If you have an offline copy of the GameTDB image database, you can copy
it to the ROM Properties Page cache directory to allow the extension to
use the pre-downloaded version instead of downloading images as needed.

## Decryption Keys

Some newer formats, including Wii disc images, have encrypted sections. The
shell extension includes decryption code for handling these images, but the
keys are not included. To install the keys, create a text file called
`keys.conf` in the rom-properties configuration directory:

* Linux: `~/.config/rom-properties/keys.conf`
* Windows: `%APPDATA%\rom-properties\keys.conf`

The `keys.conf` file uses INI format. An example file, `keys.conf.example`,
is included with the shell extension. This file has a list of all supported
keys, with placeholders instead of the actual key data. For example, a
`keys.conf` file with the supported keys for Wii looks like this:

```
[Keys]
rvl-common=[Wii common key]
rvl-korean=[Wii Korean key]
```

Replace the key placeholders with hexadecimal strings representing the key.
In this example, both keys are AES-128, so the hexadecimal strings should be
32 characters long.

NOTE: If a key is incorrect, any properties dialog that uses the key to
decrypt data will show an error message instead of the data in question.

## Credits

### Developers

* @GerbilSoft: Main developer.
* @DankRank: Contributor, bug tester.
* @CheatFreak: Bug tester, amiibo support.

### Websites

* [GBATEK](http://problemkaputt.de/gbatek.htm): Game Boy Advance, Nintendo DS,
  and Nintendo DSi technical information. Used for ROM format information for
  those systems.
* [WiiBrew](http://wiibrew.org/wiki/Main_Page): Wii homebrew and reverse
  engineering. Used for Wii and GameCube disc format information.
* [GameTDB](http://www.gametdb.com/): Database of games for various game
  consoles. Used for automatic downloading of disc scans for Wii and GameCube.
* [Pan Docs](http://problemkaputt.de/pandocs.htm): Game Boy, Game Boy Color and
  Super Game Boy technical information. Used for ROM format information for
  those systems.
* [Virtual Boy Programmers Manual](http://www.goliathindustries.com/vb/download/vbprog.pdf):
  Virtual Boy technical information. Used for ROM format information for that
  system.
* [Sega Retro](http://www.segaretro.org/Main_Page): Sega Mega Drive technical
  information, plus information for other Sega systems that will be supported
  in a future release.
* [PS3 Developer wiki](http://www.psdevwiki.com/ps3/) for information on the
  "PS1 on PS3" save file format.
* [Nocash PSX Specification Reference](http://problemkaputt.de/psx-spx.htm)
  for more information on PS1 save files.
* [amiibo.life](http://amiibo.life): Database of Nintendo amiibo figurines,
  cards, and plushes. Used for automatic downloading of amiibo images.
* [3dbrew](https://www.3dbrew.org): Nintendo 3DS homebrew and reverse
  engineering. Used for Nintendo 3DS file format information.
* [SMS Power](http://www.smspower.org): Sega 8-bit technical information.
  Used for ROM format information for Sega Master System and Game Gear.
* [Puyo Tools](https://github.com/nickworonekin/puyotools): Information on
  Sega's PVR, GVR, and related texture formats.
* [Badge Arcade Tool](https://github.com/CaitSith2/BadgeArcadeTool): Information
  on Nintendo Badge Arcade files.
* [Advanced Badge Editor](https://github.com/TheMachinumps/Advanced-badge-editor):
  Information on Nintendo Badge Arcade files.
* [HandyBug Documentation](http://handy.cvs.sourceforge.net/viewvc/handy/win32src/public/handybug/dvreadme.txt):
  Information on Atari Lynx cartridge format.
