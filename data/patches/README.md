Place IDA-exported `.dif` files for use by `binpatch` in subdirectories of this
directory. Each `.dif` file must be in a subdirectory named after the full
symbol table version string. For example, for DF version 51.05, you would use
these subdirectories:

- "v0.51.05 linux64 CLASSIC"
- "v0.51.05 linux64 ITCH"
- "v0.51.05 linux64 STEAM"
- "v0.51.05 win64 CLASSIC"
- "v0.51.05 win64 ITCH"
- "v0.51.05 win64 STEAM"

See https://docs.dfhack.org/en/stable/docs/dev/Binpatches.html for more details.
