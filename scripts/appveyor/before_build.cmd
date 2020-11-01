git submodule update --init --recursive

mkdir build && cd build

if %platform%==x86 set RELEASE_ARCH=Win32
if %platform%==x64 set RELEASE_ARCH=x64

cmake .. -G"Visual Studio 16 2019" -A"%RELEASE_ARCH%"^
 -DSFIZZ_USE_SNDFILE=OFF^
 -DSFIZZ_JACK=OFF^
 -DSFIZZ_BENCHMARKS=OFF^
 -DSFIZZ_TESTS=ON^
 -DSFIZZ_LV2=ON^
 -DSFIZZ_VST=ON^
 -DSFIZZ_RENDER=OFF^
 -DCMAKE_BUILD_TYPE=Release^
 -DVCPKG_TARGET_TRIPLET=%VCPKG_TRIPLET%^
 -DCMAKE_TOOLCHAIN_FILE=C:/Tools/vcpkg/scripts/buildsystems/vcpkg.cmake
