# Changes

## v0.9-beta2 (unreleased)

* New features:
  * New XFCE frontend. Currently only implements the property page.
    Thumbnailing will be implemented before release.

* New systems supported:
  * Nintendo 64 ROM images: Z64, V64, swap2, and LE32 byteswap formats.
    Currently only supports text fields.
  * Super NES ROM images: Headered and non-headered images.
    Currently only supports text fields.
  * Sega Dreamcast save files: .VMS, .VMI, .VMS+.VMI, and .DCI formats.
    Supports text fields, icons, and the graphic eyecatch (as the banner).
    ICONDATA_VMS files are also supported.
  * Nintendo Virtual Boy ROM images: .VB format.
  * Nintendo amiibo NFC dumps: 540-byte .bin files. (On Windows, the .bin
    extension is not currently registered; alternatives are .nfc and .nfp)

* (Windows) Fixed anti-aliasing issues with monospaced fonts on the
  properties page.

* libpng is now used on all platforms. Previously, GDI+'s PNG loader was
  used on Windows, and it had some odd quirks like loading grayscale images
  as 32-bit ARGB instead of paletted.

* (Windows) zlib and libpng are now compiled as DLLs instead of being
  statically linked.

* pngcheck: Fixed a race condition that could result in crashes if more
  than one thread attempted to load a PNG image at the same time.

* Various optimizations and bug fixes.

## v0.8.1 (Windows only) (released 2016/10/24)

* RP_ExtractIcon: Disabled icon caching. This was causing issues where
  all icons for supported ROM images were showing up as whichever file
  was the first to be processed.

## v0.8-winfix (Windows only) (released 2016/10/23)

* Fixed the working directory in the install.cmd and uninstall.cmd
  scripts. Windows switches to C:\\Windows\\System32 by default when
  running a program or batch file as Administrator, which prevented
  the installation scripts from finding the DLLs.

## v0.8-beta1 (released 2016/10/23)

* First beta release.

* Host platforms: Windows XP and later, KDE 4.x, KDE 5.x

* Target systems: Mega Drive (and add-ons), Nintendo DS(i), Nintendo
  GameCube (discs and save files), Nintendo Wii, Game Boy (Color),
  Game Boy Advance.

* Internal images supported for GameCube discs and save files, and
  Nintendo DS ROMs.

* Animated icons supported for GameCube save files and Nintendo DSi
  ROMs. Note that file browsers generally don't support animated icons,
  so they're only animated on the file properties page.

* External image downloads supported for GameCube and Wii discs
  via [GameTDB.com](http://www.gametdb.com/).

* Decryption subsystem for Wii discs. Currently only used to determine
  the System Update version if a Wii disc has a System Update partition.
  Keys are not included; you must manually add the encryption keys to
  ```keys.conf```. (See [README.md](README.md) for more information.)
