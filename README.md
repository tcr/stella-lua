# Stella Lua

Stella debugger with integrated ability to run Lua scripts with access to processor state.

Instructions:

```
lua filename
```

Available globals:

* `cpu()` is a map of registers, e.g. `cpu()['A']` is the accumulator.
* `label(string)` will read a value from the memory map.
* `peek(number)` will read a value from the memory map.

You can also import files, e.g. `require('./json.lua')`.

---

Stella is a multi-platform Atari 2600 VCS emulator which allows you to
play all of your favourite Atari 2600 games on your PC.  You'll find the
Stella Users Manual in the docs subdirectory.  If you'd like to verify
that you have the latest release of Stella, visit the Stella Website at:

  [stella-emu.github.io](https://stella-emu.github.io)

Enjoy,

The Stella Team

# Reporting issues

Please check the list of known issues first (see below) before reporting new ones.
If you encounter any issues, please open a new issue on the project
[issue tracker](https://github.com/stella-emu/stella/issues) and / or in the corresponding
[thread](http://atariage.com/forums/topic/259633-testing-the-new-stella-tia-core/) on
AtariAge.

# Known issues

Please check out the [issue tracker](https://github.com/stella-emu/stella/issues) for
a list of all currently known issues.
