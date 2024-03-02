# Contributing to Kinoko

The rate at which Kinoko improves depends on how many collaborators are interested in helping out. We appreciate your interest in contributing!

This document outlines how to get started, gives examples of valuable contributions, and explains our coding style.

## Table of Contents

<!-- no toc -->
  - [Getting Started](#getting-started)
  - [Contribution Goals](#contribution-goals)
  - [Coding Style](#coding-style)

## Getting Started

- Since Kinoko needs to match Mario Kart Wii's physics calculations, it's important to leverage [Ghidra](https://ghidra-sre.org/), which can decompile assembly into C++. A lot of relevant code is well-documented in our server.
  - You can gain access to our Ghidra server [here](https://panel.mkw.re/login) by linking your Discord account.
  - In order to decompile PowerPC code, you'll have to download the [Ghidra GameCube Loader](https://github.com/cuyler36/Ghidra-Gamecube-Loader). Follow the installation instructions in the README.
- Get to know the [Decomp Repo](https://github.com/em-eight/mkw).
  - For functions that require a 100% match to the game's source (e.g. floating-point arithmetic), we leverage the infrastructure in this repository to generate decomp.me scratch pads.
- [decomp.me](https://decomp.me/)
  - This platform allows users to collaboratively match a set of assembly instructions with C++ code. We can use the Decomp Repo to generate a scratch with the matching assembly for any function in the game, along with the necessary function declarations context.

## Contribution Goals

It is important to carefully define and follow development goals throughout the development process. Diverging from established goals will inevitably lead to difficulty later in development; though code optimizations lead to performance boosts, this can make bugs significantly harder to track down since the code no longer matches closely with the game. The following goals should be kept in mind:

- Any bugs or concerns with current implementation should first be addressed by opening an Issue on Github.
- Code contributions should strike a balance between readability and similarity to the base game. We aren't striving for perfect matches, but we also want to make it easier to debug by comparing codeflow to the base game.
- Code additions should be ad-hoc. In other words, only add what you need.
  - e.g. If we are presently focused on syncing the race countdown and some function `func1` calls another function `func2` if and only if the countdown has ended, then `func2` should not be implemented yet.
- In order to limit merge conflicts and duplication of work amonst collaborators, pull requests should have a limited scope, such as implementing wheelies or handling offroad.
- Given the ad-hoc nature of the development process, it is worthwhile to communicate what you plan to work on to other collaborators in case the two scopes overlap.
  - e.g. If you are working on the mini-turbo charge calculations while another collaborator is independently working on mini-turbo speed calculations, there is a possibility that the other collaborator has already implemented your scope on their branch.

## Coding Style

Any code contributions should be formatted according to this project's [clang-format](./.clang-format) file. This can be double-checked after pushing by leveraging the [format build job](./.github/workflows/build.yml).

Header files should organize class definitions in the following manner:

```cpp
class MyClass {
public:
    // Structs
    struct MyPublicStruct {
      f32 member1;
      f32 member2;
    };

    // Lifecycle (if not a singleton)
    MyClass();
    ~MyClass();

    // Operators
    MyClass &operator=(const MyClass &rhs) const {
      m_member1 = rhs.member1();
    }

    // Methods
    bool method1() const;
    void setMethod2(bool isSet);
    f32  member1() const;

    // Members
    void *m_somePtr;

    // Static Methods
    static bool staticMethod1();
    static void staticMethod2();

    // Singleton (if applicable)
    static MyClass *CreateInstance();
    static MyClass *Instance();
    static void DestroyInstance();

private:
    // Same as above for any structs, lifecycle (if singleton),
    // operators, methods, members, static methods, and static members

    // Singleton (if applicable)
    static MyClass *s_instance;
};
```

In addition to the style ordering described in the code block above, we also try to adhere to the following style rules:

- If a class has a default constructor/destructor, they should still be declared in the header. In the source, they should be explicitly defined as:

  ```cpp
  MyClass::MyClass() = default;
  MyClass::~MyClass() = default;
  ```

- Operator overloads should be defined in the header.

- Typically a source/header file is named after a particular class. This class should be the first declaration in the header, except when it depends on any structs. In the source file though, that main class should always be defined first. Afterwards, classes should be defined in the same order as everything else in the header.

- Getter/setter functions should use the following format:
  - To get `f32 m_member1`, you should define `f32 member1() const`
  - To set `f32 m_member1`, you should define `void setMember1()`

- It cannot be determined in Ghidra whether a given function argument is a `const&`. For a given function, if a particular pointer is read without a `nullptr` check and the pointer is never written to, then you should always limit the argument to `const&`.

- If a data structure has more than 3 methods, it should probably be a class.

- In source files, static members should be initialized at the bottom of the source file, even if there are other classes defined after the member's class.