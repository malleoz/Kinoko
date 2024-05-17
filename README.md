<div align="center">
  <img src="https://github.com/vabold/Kinoko/assets/16770560/0e4ef63a-409a-40e4-8801-d30a27d7f7b5" width=100>
  <div style="font-size: 2em; margin: .67em 0">
    <b>Kinoko</b>
  </div>

  [![Discord](https://img.shields.io/badge/Discord-grey?logo=discord)](https://discord.gg/qjt66ptQde)
  [![Doxygen](https://img.shields.io/badge/📚%20Documentation-grey)](https://malleoz.github.io/Kinoko/docs/html/index.html)
</div>
<hr/><br/>



Kinoko is a reimplementation of Mario Kart Wii's physics engine in C++. This project is a spiritual continuation of [stblr/Hanachan](https://github.com/stblr/Hanachan). Like Hanachan, the goal of this reimplementation is to reach perfectly accurate ghost replay.


<details>
  <summary>Test Cases</summary>

  [![Luke's rMC3 RTA WR](<https://vabold.github.io/Kinoko/Luke rMC3 RTA WR.svg>)](https://www.youtube.com/watch?v=6H6UnSDPPdI)
  [![Kasey's LC RTA WR](<https://vabold.github.io/Kinoko/Kasey LC RTA WR.svg>)](https://www.youtube.com/watch?v=HPcvNS8QFVI)
  [![Luke's rBC RTA WR](<https://vabold.github.io/Kinoko/Luke rBC RTA WR.svg>)](https://www.youtube.com/watch?v=twZes-RI6Sc)
  [![Kasey's rSGB RTA WR](<https://vabold.github.io/Kinoko/Kasey rSGB RTA WR.svg>)](https://www.youtube.com/watch?v=SjXUPXT8n8g)
  [![Vincent's rMR RTA WR](<https://vabold.github.io/Kinoko/Vincent rMR RTA WR.svg>)](https://www.youtube.com/watch?v=y7t4_xXuD2A)
  [![Sosis's rSL RTA WR](<https://vabold.github.io/Kinoko/Sosis rSL RTA WR.svg>)](https://www.youtube.com/watch?v=ahNGAaUzm6s)
  [![Logan's MH RTA WR](<https://vabold.github.io/Kinoko/Logan MH RTA WR.svg>)](https://www.youtube.com/watch?v=CellUlOYgnc)
  [![Logan's rBC3 RTA WR](<https://vabold.github.io/Kinoko/Logan rBC3 RTA WR.svg>)](https://www.youtube.com/watch?v=vSbSADDEzEs)
</details>

<br/>

## Building

### Dependencies

- g++ 13.1 (C++23)
- ninja
- Python 3.6

### Process

Generate the ninja file and test case binary:

```bash
./configure.py
```

Execute it:

```bash
ninja
```

Run Kinoko:

```bash
cd out
./kinoko -s testCases.bin
```

## Creating New Test Cases

Currently, Kinoko runs by iterating over a set of test cases defined in `testCases.json`.

### Create KRKG

Test cases rely on a custom `.krkg` file format, which stores some header information followed by data points that we want to check against on each frame. This file is generated using a custom version of MKW-SP.

1. Navigate to the [workflows](https://github.com/vabold/mkw-sp/actions?query=branch%3Akrkg) page for the KRKG branch.
2. Download the `mkw-sp-test` artifact at the bottom of the page.
3. Extract `boot.dol` to a folder of your choosing.
4. In Dolphin, navigate to Config > Paths and set `Default ISO` to the path of your Mario Kart Wii PAL ISO.
5. In Dolphin, click Open and select the extracted `boot.dol`.
6. You should see some debug information printed on-screen and Mario Kart Wii should boot soon after.
7. Watch a ghost replay of your choosing. After the ghost finishes and the screen fades to black, navigate to `[DolphinDir]\User\Wii\title\00010004\524d4345\data`. You should see a new file called `test.krkg`.

### Add Test Case Params

Test cases are defined in `testCases.json` in the following format:

```json
"testCaseName": {
    "rkgPath": "pathTo.rkg",
    "krkgPath": "pathTo.krkg",
    "targetFrame": 0
}
```

`rkgPath` is the path to the game's native ghost file. `krkgPath` points to the krkg file for this ghost generated in the previous section. `targetFrame` is the number of frames to attempt sync for this ghost. Kinoko will throw an error if it is larger than the framecount stored in the krkg. If Kinoko reaches `targetFrame` while maintaing sync, then Kinoko passes the test case.

To update the test binary, be sure to re-run:

```bash
./configure.py
```

## Interfacing

While a GUI is not planned for the project at this time, contributors are welcome to add a graphics frontend under three conditions. The license must not change, it does not interfere with the CLI, and most importantly, it does **not** distribute any in-game assets.

## Contributing

The codebase uses C++ for the engine and Python for any external scripts.

Pull requests resolving an issue or element of a tracking issue should reference the issue or item in the description.

Any commits should be formatted using the project's clang-format file.

## Resources

- [Kinoko Discord](https://discord.gg/qjt66ptQde) - Discuss development, offer suggestions, and more!
- [MKW-SP Discord](https://discord.gg/TPSKtyKgqD) - Request Ghidra server access and chat with contributors.
- [Matching Decompilation](https://github.com/riidefi/mkw) - A repository that compiles back into the game's original assembly.
- [Tockdom](http://wiki.tockdom.com/wiki/Main_Page) - A wiki maintained by multiple community members, most notably used for file formats.
