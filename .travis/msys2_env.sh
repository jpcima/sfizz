export msys2='cmd //C RefreshEnv.cmd '
export msys2+='& set MSYS=winsymlinks:nativestrict '
export msys2+='& C:\\tools\\msys64\\msys2_shell.cmd -defterm -no-start'
export mingw64="$msys2 -mingw64 -full-path -here -c "\"\$@"\" --"
export mingw32="$msys2 -mingw32 -full-path -here -c "\"\$@"\" --"
export msys2+=" -msys2 -c "\"\$@"\" --"

case "$CI_TARGET" in
  i686-w64-mingw32)
    export PATH="/C/tools/msys64/mingw32/bin:$PATH"
    export mingw="$mingw32"
    ;;
  x86_64-w64-mingw32)
    export PATH="/C/tools/msys64/mingw64/bin:$PATH"
    export mingw="$mingw64"
    ;;
  *)
    echo "Invalid value CI_TARGET for MSYS2 MinGW"
    exit 1
    ;;
esac
