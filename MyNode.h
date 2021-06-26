#pragma once

#include "Core.h"

#include <maya/MDistance.h>
#include <maya/MMatrix.h>
#include <maya/MPoint.h>

ENUM_BEGIN(Axis)
ENUM(X)
ENUM(Y)
ENUM(Z)
ENUM_END

COMPOUND_BEGIN(Linear3)
COMPOUND_VALUE(MDistance, x)
COMPOUND_VALUE(MDistance, y)
COMPOUND_VALUE(MDistance, z)
COMPOUND_END

NODE_BEGIN(MyNode)
{
	Axis axis = enumTest(b);
	double totalMetersInAxis = 0.0;
	for (int index = 0; index < pointArrayTestSize(b); ++index) {
		Linear3 point = pointArrayTest(b, index);
		MDistance element;
		if(axis == Axis::X) element = point.x;
		else if(axis == Axis::Y) element = point.y;
		else /*if(axis == Axis::Z)*/ element = point.z;
		totalMetersInAxis += element.asMeters();
	}

	double value = inputValue(b);
	outputValueSet(b, value + totalMetersInAxis);

	std::vector<double> arr(inputArraySize(b));
	for (unsigned int index = 0; index < arr.size(); ++index) {
		arr.push_back(inputArray(b, index));
	}
	outputArraySet(b, arr);
}
INPUT(double, inputValue)
INPUT_ARRAY(double, inputArray)
OUTPUT(double, outputValue)
OUTPUT_ARRAY(double, outputArray)
INPUT(Axis, enumTest)
INOUT_ARRAY(Linear3, pointArrayTest)
NODE_END

COMPOUND_BEGIN(MatrixOnCurve)
COMPOUND_VALUE(float, parameter)
COMPOUND_VALUE(MPoint, result)
COMPOUND_END

NODE_BEGIN(MatricesOnCurve)
{
	MFnNurbsCurve curve(inputCurve(b));
	unsigned int size = dataSize(b);
	std::vector<MatrixOnCurve> inoutValue(size);
	for (unsigned int index = 0; index < size; ++index) {
		inoutValue[index] = data(b, index);
		curve.getPointAtParam(inoutValue[index].parameter, inoutValue[index].result);
	}
	dataSet(b, inoutValue);
}
INPUT(NurbsCurve, inputCurve)
INOUT_ARRAY(MatrixOnCurve, data) // IMPORTANT: when generating an input attribute for a compound, Python should generate the required child attribute MObject definitions.
NODE_END
