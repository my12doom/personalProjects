export NDK=/cygdrive/e/ndk7
export PREBUILT=$NDK/toolchains/arm-linux-androideabi-4.4.3/prebuilt
export PLATFORM=$NDK/platforms/android-9/arch-arm
export PREFIX=/cygdrive/e/x264
./configure --prefix=$PREFIX --enable-static --enable-pic --disable-asm --disable-cli --host=arm-linux --cross-prefix=$PREBUILT/windows/bin/arm-linux-androideabi- --sysroot=$PLATFORM