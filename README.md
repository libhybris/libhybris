# What is libhybris

libhybris is a way to load drivers compiled for Android from "regular linux
processes".
In other words it allows you to load drivers that link against the bionic c
library inside processes whose native c library is e.g. glibc, musl, ...

This allows us to load graphics drivers, or any other driver that's sitting
inside an Android "so" file and utilize them in a non-Android-based
Linux distribution.

# How to use libhybris

In general Android is patched to remove TLS accesses in bionic c library which
usually cause problems with glibc TLS slots. Android is also patched to remove
zygote, surfaceflinger, bootanim and other processes, that are not needed when
running a regular Linux distribution or would cause problems,
from Android init.
The resulting stripped down Android system is then launched as a systemd
service or container by the Linux distributions init in order to provide
hardware adaptation services that are mandatory to be able to use
Android drivers.

For reference:
* https://github.com/mer-hybris/hybris-patches
* https://github.com/halium/hybris-patches

For building libhybris you need android-headers to compile against the
Android version you are targeting.

See also
* https://github.com/libhybris/libhybris/blob/master/utils/extract-headers.sh
* https://github.com/mer-hybris/droid-hal-device/blob/master/helpers/extract-headers.sh

There are projects which make this process easier documented here:
* SailfishOS Hardware Adaptation Development Kit:
https://docs.sailfishos.org/Develop/HADK which documents how to port the
Linux distribution SailfishOS on top of Android.
* Halium: https://github.com/Halium which provides a standardized & containerized
abstraction between the Android system and the used Linux distribution, which
is used by Droidian, FuriPhone, Ubuntu Touch and others.

Since Android uses hardware composer for graphics this needs to be accounted
for by using for example a plugin to the Qt compositor if Qt is being used:
https://github.com/mer-hybris/qt5-qpa-hwcomposer-plugin

Similarly for other drivers, e.g. audio, wrappers need to be developed that
bridge the Android functionality into the corresponding Linux distribution,
e.g.: https://github.com/mer-hybris/pulseaudio-modules-droid

# Additional notes

Since Android 8, many of the drivers, except those which require the highest
possible performance like graphic drivers have been implemented via binder-IPC.

For these kinds of drivers it's sometimes easier to use binder-IPC directly
from a native context. For these purposes there exists a glib based binder
implementation here: https://github.com/mer-hybris/libgbinder and some example
how to use it can be found here: https://github.com/mer-hybris/bluebinder

