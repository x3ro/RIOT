Tests the in-memory variant of Coffee FS, which does not actually serve a purpose other than testing Coffee on devices that do not have another storage medium. Also useful for demonstration. 

## Caveats

`cfs-coffee-arch.h` is included by the Coffee codebase directly. It contains the configuration parameters for the file system. The options that have sensible default values are defined in `fs/coffee-defaults.h` but can be overwritten locally. The macros that are dependant on the underlying storage medium are defined in a separate header, for in-memory FS `fs/coffee-in-memory.h`.
