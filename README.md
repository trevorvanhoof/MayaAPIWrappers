# WIP

---

This is a work in progress, the following documentation may be more complete than the actual product.

#### TODO

- MPxDeformer: 
- MPxLocator: 
- MPxCommand: This needs the whole API design to be done
- MFnTypedAttribute: Attributes of type kPlugin & kPluginGeometry are not supported yet
- Nested compounds are not supported yet: this may be as simple as passing in the right object array in Generate.py

---

# Maya API Wrappers

Plugins Made Easy

---

## License & Attribution

Released under the MIT License.

Copyright 2021 Trevor van Hoof

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

---

Big thanks to [Perry Leijten](https://www.perryleijten.com) for beta-testing and mocking up challenging nodes to get to work reliably.

## About

The few files in this project make creating Maya plugins easier than ever. 

With minimal setup, you write only the code that is relevant to your implementation, and Generate.py will output all irrelevant code to make it happen.

## Getting started

### Caveats
It is currently not possible to use this system alongside other plugin code, as it completely generates the initializePlugin function.

See the TODO section above for features that are planned. Intentionally omitted features are reported below in the HOWTO section relevant to them.

### Project setup
To use this in your project, you will need to add `Generate.py`, `MFnHelpers.inc`,  `Core.h` and `Main.cpp`.

You will also need to set up a pre-build event in the project's properties (See the Configuration Properties/Build Events/Pre-Build Event/Command Line subsection). The pre-build event needs to call python, which Maya conveniently already includes. The full command line can look like this (when installing Maya2020 in the default location this works verbatim):

> "C:\Program Files\Autodesk\Maya2020\bin\mayapy.exe" Generate.py

Like any Maya project, setting up your maya include/ and lib/ paths, as well as outputting to an .mll file are also required.

### First node
All your code will become header-only, so to start creating a node, just add a new header file `MyFirstNode.h` and add the following:

```c++
NODE_BEGIN(MyFirstNode) { 
	outputValueSet(b, inputValue(b)); 
}
INPUT(double, inputValue)
OUTPUT(double, outputValue)
NODE_END
```
_This code is elaborated in the Howtos & Examples below._

### Type IDs
Maya plugins use MTypeId's to distinguish nodes from each other.
This plugin will put all nodes on consecutive IDs starting at 0.

If you want to start at a different number, simply add this to 1 of your header file:

`#define MTYPEID_START 0x8000`

_This starts counting at 0x8000 for example_

### Closing notes
Any header file is automatically included in Main.cpp, no need to edit it by hand.
This system makes sure header files are included only once. If you can't guarantee that to be the case
for you, you may remove the code after `NODE_BEGIN(...)`, and put a semicolon `;` instead of the opening brace `{`.
Then you can add a .cpp file and implement the compute method like this:

```c++
void MyFirstNode::compute(MDataBlock& b) { 
	outputValueSet(b, inputValue(b)); 
}
```

## Howtos & Examples

### Simple nodes

```C++
/*
MyNode
	in: double inputValue
	out: double outputValue

Basic node that copies an input plug to an output plug.
*/
NODE_BEGIN(MyNode) // This is the name of your node when creating it in Maya (and also the name of the C++ class that implemetns it).
{ // This is your compute() method, it has already checked that you are not dealing with an invalid parameter.
	// "b" is the data block, and the only Maya-thing left for you to deal with. We kept it short to make it faster to write.
	double value = inputValue(b); // Input attributes automatically generate a getter that returns the expected data type.
	outputValueSet(b, value); // Output attributes automatically generate a setter that takes the expected data type and cleans the relevant plug.
}
INPUT(double, inputValue) // These are the direction (input/output/inot), type (double) and name of the plug on your node, and declares the above getter/setter functions!
OUTPUT(double, outputValue) // Attribute affects is handled based on the direction of the input. Also, short names no longer exist, your scripts are now forced to be readable.
NODE_END // This is just a marker for Generate.py, but it'll generate compiler errors to make sure you don't forget to add it.
```

### Array attributes

```C++
/*
SumNumbers
	in: double[] inputValue
	out: double outputValue
	out: double[] example

Sums the values of each input plug and sets the total result as output.
*/
NODE_BEGIN(SumNumbers)
{
	double sum = 0.0; // We will accumulate the inputs here.
	for (int index = 0; index < inputArraySize(b); ++index) { // <attrName>Size methods are automatically generated.
		sum += inputArray(b, index); // Array getters require an index to get the right element.
	}
	outputValueSet(b, sum); // Set the output.

	// To demonstrate how output arrays work, here is an example of copying an input array into an output array.
	std::vector<double> arr(inputArraySize(b)); // We first allocate an array of the right type and size/
	for (unsigned int index = 0; index < arr.size(); ++index) { // For each input element index:
		arr.push_back(inputArray(b, index)); // Add the input value to the output array at the same index.
	}
	outputArraySet(b, arr); // Finally we set and clean the output plug as a whole.
}
INPUT_ARRAY(double, inputArray) // To make an attribute an array attribute, we simply suffix the attribute type with _ARRAY
OUTPUT(double, outputValue)
OUTPUT_ARRAY(double, example) // Output arrays always use data builders, and due to current limitations they must be set in 1 go.
NODE_END
```

### Enum attributes

```C++
ENUM_BEGIN(Axis) // Enums must be declared using our macros
ENUM(X)
ENUM(Y)
ENUM_VALUE(Z, 2) // It is possible to add explicit indices, subsequent ENUM() usage will continue counting from this value (2). Duplicate values are not allowed.
ENUM_END

NODE_BEGIN(EnumDemo) // This is just to quickly show how an Enum automatically acts like any other type of data.
{
	Axis axis = enumTest(b); // Getters are generated as usual, and return the expected data type.
}
INPUT(Axis, enumTest) // This works the same for OUTPUT, INOUT, _ARRAY macros, enums are no different from other numbers.
NODE_END
```

### Compound attributes

Compound attribute values are represented as C++ structs.
This way getting the attribute is staight forward, and returns an instance with data acessible exactly how you would expect.

```C++
COMPOUND_BEGIN(Linear3) // The type name to use in your code.
COMPOUND_VALUE(MDistance, x) // The child attribute types and names.
COMPOUND_VALUE(MDistance, y)
COMPOUND_VALUE(MDistance, z)
COMPOUND_END // A marker for the code generation logic that you should not forget!
```

Usage is then straight forward:

```
NODE_BEGIN(CompoundDemo) // This is just to quickly show how a Compound automatically acts like any other type of data.
{
	Linear3 data = compoundTest(b); // Getters are generated as usual, and return the expected data type.
}
INPUT(Linear3, compoundTest) // This works the same for OUTPUT, INOUT, _ARRAY macros, compounds are no different from other data.
NODE_END
```

### Builtin data types

#### Caveats
Maya provides convenience functions createPoint and createColor, internally those are stored as 32-bit floats.

We also use createPoint for MVector & MPoint attributes, technically this results in data loss. To avoid this
either use DVec3/DVec4 or create your own compound types to have child-plugs (DVec3 won't provide those).

Calling setters on output attributes of type message is not permitted.

#### Omissions
We have omitted 2 number types: `byte` and `addr`, they can not be set using MDataHandle.
We have omitted types that would normally use: MFnGenericAttribute, MFnLightDataAttribute.
We have omitted kDynArrayAttrs as it's just a dictionary of typed array attributes and you can achieve the same thing more safely by using multiple attributes instead.

#### Renames
We have aliases for the numeric array types:

| Maya's name | Your attribute type |
| ----------- | ------------------- |
| short2      | SVec2               |
| short3      | SVec3               |
| int2        | IVec2               |
| int3        | IVec3               |
| float2      | FVec2               |
| float3      | FVec3               |
| double2     | DVec2               |
| double3     | DVec3               |
| double4     | DVec4               |

_`long` is another name for `int` so for `long2` etc you can also use `IVec2`_

#### DIY

We may not have full coverage of builtin data types, so here is hwo to add them:

**NOTE:** if you want to deal with MFn* objects, be sure to read the section after this too!

To support a new data type, we have to implement a bunch of template functions. You can do this in any .cpp file in your project, be sure the #include "Core.h" in it and you're good to go.

```c++
template<> NEW_TYPE fallback<NEW_TYPE>(); // optional, default implementation uses default constructor (errors if unavailable)
template<> NEW_TYPE get<NEW_TYPE>(MDataHandle& element);
template<> MStatus set<NEW_TYPE>(MDataHandle& element, const NEW_TYPE& value);
template<> MStatus initialize<NEW_TYPE>(MObject& dst, const char* name);
```

In the case of compounds and enums, Python will try and generate these functions for you.

This example implements double:

```c++
template<> double fallback<double>() { 
	return 0.0; 
}

template<> double get<double>(MDataHandle& element) {
	return element.asDouble();
}

template<> void set<double>(MDataHandle& element, const double& value) {
	element.setDouble(value);
	element.setClean();
}

template<> MStatus initialize<double>(MObject& dst, const char* name) {
	MFnNumericAttribute fn;
	MStatus status;
	dst = fn.create(name, name, MFnNumericData::kDouble, 0.0, &status); CHECK_MSTATUS_AND_RETURN_IT(status);
	return status;
}
```

### Complex data types

It is not possible to pass around MFn* subclasses, so we can not support attributes in the form of "INPUT(MFnMesh, myMesh)".

Instead, we add subclasses of MObject to unambiguously define which type of object we are dealing with.

Not all data types may be covered, so feel free to expand MFnHelpers.inc as needed.

Example: adding MFnNurbsCurve to MFnHelpers.inc:
```C++
#include <maya/MFnNurbsCurve.h> // You may have to include the data types you're representing here!
DECL_MFN_MOBJECT(NurbsCurve) // This macro allows us to detect which types have to be resolved in a special way.
```

Then you can use this like any normal attribute, and pretend that the return type is MObject instead of your more specific type:
```
NODE_BEGIN(FnDemo) // This is just to quickly show how an MFN_MOBJECT automatically acts like any other type of attribute of type MObject.
{
	NurbsCurve obj = curveTest(b); // Getters are generated as usual, and return the expected data type.
	MFnNurbsCurve curveTestFn(obj); // Thanks to polymorphism a NurbsCurve automatically convertrs to an MObject and correctly constructs the MFn.
}
INPUT(NurbsCurve, curveTest) // This works the same for OUTPUT, INOUT, _ARRAY macros, compounds are no different from other data.
NODE_END
```
