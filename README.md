# Thread pool

Make thread pool

## Requirements

- c++23: it uses `std::move_only_function` which you can implement yourself for old standards and compilers
- cmake
- gtest: added as external project downloaded via cmake

## Build

```
cmake --build . --target install --config Debug
```

## Dev notes

To address the issue of executing `Halt()` between evaluating `halt_` variable in this cv's predicate and cv going to sleep (see 2 #references) which leads to undesired block on `cv.wait` despite halting I use `wait_for` with timeout around `100ms`.  


# References

1. [C++ compiler support](https://runebook.dev/en/docs/cpp/compiler_support#C.2B.2B23_library_features)
2. [Multithreading by example: C++ dev blog](https://dev.to/glpuga/multithreading-by-example-the-stuff-they-didn-t-tell-you-4ed8)
