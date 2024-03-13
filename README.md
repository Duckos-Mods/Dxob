
---

# What is Dxob?
Dxob is a format made to store [height maps](https://en.wikipedia.org/wiki/Heightmap) of terrain in a smaller space than using a 16-bit TIFF file. It achieves this by using bit packing for all the height values. If you would like to learn more, visit the [specs](https://www.github.com/duckos-mods/Dxob/blob/master/Specs.MD) to understand how it works or to see how to write your own parser!

## Docs
##### Note: All of these examples assume you have included `Dxob.hpp`.
---
### Writing Height Map Data to a File
```cpp
// This assumes you already have a HeightDataAccessor named `hda`.
Dxob::Writer writer;
Dxob::BinaryStream output;
writer.Write(hda, output);
```
This operation stores the data in `output` in the Dxob file format. Simply write this data to disk. You can later read it using a reader to retrieve the height map data.

---
### Reading Height Map Data from a File
```cpp
// This assumes you have a BinaryStream containing data named `rawHeightData`.
Dxob::Reader reader;
if (!reader.IsDxob(rawHeightData))
    std::cout << "Data isn't in the Dxob format!\n";
Dxob::HeightDataAccessor hda = reader.Process(rawHeightData);
```
This function call returns a `HeightDataAccessor` object containing the unpacked height data.

#### Alternatively, you can read from a file directly:
```cpp
Dxob::Reader reader;
Dxob::HeightDataAccessor hda = reader.Process("Path/To/Dxob");
```
This approach reads the Dxob file from the specified path and returns a `HeightDataAccessor` object containing the unpacked height data.

---
### HeightDataAccessor
The `HeightDataAccessor` class (referred to as HDA hereafter) is the main class used by Dxob to read and write height data. This class provides a wrapper over a 2D array of `uint16_t` or `u16` (referred to as u16 from now on). It includes a `HeightAt` function, which takes an `x` and a `y` coordinate and returns a `u16` representing the height at the specified location. By default, it utilizes a `PositionTranslationFunction` to perform this operation, where the default `PositionTranslationFunction` is `y * width + x`. However, this can be altered by invoking `SetPositionTranslationFunction` with a function pointer to a function that takes a `u16` along with two `u64`s for the x and y coordinates.

The `SetHeightAt` function mirrors `HeightAt`, but accepts a `u16` as a third parameter, which serves as the value for the specified position. Additionally, this class stores the settings for the Dxob file, which will be discussed in the following section.

---
### BinaryStream
The `BinaryStream` class (referred to as BS hereafter) is utilized for storing binary data that can be sequentially parsed. To instantiate one, you have two options: `BinaryStream(someUint8_tPointer, SomeSize)` or `BinaryStream(pointerToAnyData, SomeSize)`. Then, you can pass this instance to the `Process` or `Write` functions.

The `BinaryStream` class includes a default constructor `BinaryStream(bufferSize)`, where by default `bufferInitSize` is set to 1024 bytes. However, this might not be sufficient for all cases, so it's advisable to adjust the allocated amount accordingly.

---
### FileSettings
The file settings class stores data about the settings of the dxob file. The only settings you should touch are `width` and `height` which are used when writing and reading.
---

### Examples
If you still need some help understanding it, why not look at the [test](https://github.com/Duckos-Mods/Dxob/blob/master/test/main.cpp) file? It shows all uses of `Read` and `Write`.