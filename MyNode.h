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
INOUT_ARRAY(MatrixOnCurve, data)
NODE_END

TYPED_DEFORMER_BEGIN(LaplacianSmooth, Mesh) 
{
	// Get the input mesh
	MObject inputGeometryObj = inputGeometry(b); 
	if (inputGeometryObj != MObject::kNullObj) return MS::kFailure;

	// If the local storage seems to mismatch the geo, rebuild the vertex adjacency info
	lazyInitialize(inputGeometryObj);

	// Laplacian smooth the input geometry
	int iterations = iterationCount(b);
	float weight = smoothAmount(b);
	/* 
	The algorithm works as follows:
	
	For each point, average the adjacent points.
	New point = Lerp between the original point and the averaged point by a certain "smooth amount".
	
	Because we can "overcompensate" when the smooth amount is > 0.5, and we sometimes WANT > 0.5,
	we also have an "iteration count" to repeat the algorithm multiple times, using each
	resulting mesh as input for the next step.
	 */
	
	// Get the original points
	MPointArray originalPoints;
	MFnMesh inputMesh(inputGeometryObj);
	MStatus status = inputMesh.getPoints(originalPoints); CHECK_MSTATUS_AND_RETURN_IT(status);
	// Copy so we have an array to write new points to
	MPointArray newPoints = originalPoints;
	// We want to swap the original & new points on each smooth iteration, so we'll move these into references:
	MPointArray& ping = newPoints;
	MPointArray& pong = originalPoints;
	for (int k = 0; k < iterations; ++k) {
		// For each point, average the adjacent points and lerp towards that point
		unsigned int cursor = 0;
		for (unsigned int i = 0; i < ping.length(); ++i) {
			unsigned int adjacentCount = adjacentVertexCounts[i];
			// no connected points, ignore
			if (!adjacentCount) continue;
			MPoint average;
			for (unsigned int j = 0; j < adjacentCount; ++j) {
				average += pong[adjacentVertexIds[cursor + j]];
			}
			average *= 1.0f / (float)adjacentCount;
			cursor += adjacentCount;
			ping[i] += (average - ping[i]) * weight;
			// if this is the last iteration, we can wriet directly to the output iterator
			if (k == iterations - 1) {
				float w = weightValue(b.data, b.deformerMultiIndex, outputGeometry.index());
				MPoint originalPoint = outputGeometry.position();
				MPoint deformedPoint = ping[i];
				outputGeometry.setPosition((originalPoint - deformedPoint) * w + deformedPoint);
				outputGeometry.next();
			}
		}
		// if this is the last iteration we are done
		if (k == iterations - 1) {
			break;
		}
		// swap the vertex arrays, so the next iteration will average the smoothed points again
		MPointArray& tmp = ping;
		ping = pong;
		pong = tmp;
	}
	return status;
}
INPUT(int, iterationCount)
INPUT(float, smoothAmount)
private:
	// We can add local code like this:
	MIntArray adjacentVertexCounts;
	MIntArray adjacentVertexIds;

	void lazyInitialize(MObject& inputGeometryObj) {
		/* If the local storage seems to mismatch the geo, rebuild the vertex adjacency info. */

		// The algorithm requires every vertex to know every connected vertex.

		// We store this information as a single list "adjacentVertexIds"

		// A separate list "adjacentVertexCounts" will help us iterate
		// the right number of adjacent indices for each vertex later.

		// TODO: In the future, a "adjacentVertexOffsets" can keep a running total
		// so vertices don't need to be evaluated in order (allowing parallelization).
		
		MFnMesh inputGeometryFn(inputGeometryObj);
		unsigned int vertexCount = inputGeometryFn.numVertices();
		// Data out of date?
		if (adjacentVertexCounts.length() != vertexCount) {
			// Allocate what we know we'll need
			adjacentVertexCounts.setLength(vertexCount);
			adjacentVertexIds.clear();
			// Temporary cache for "current" adjcency info
			MIntArray adjacentList;
			// MItMeshVertex can query adjacent vertices for us.
			MItMeshVertex inputGeometryIt(inputGeometryObj);
			// For each vertex
			while(!inputGeometryIt.isDone()) {
				// Get adjacent vertices
				inputGeometryIt.getConnectedVertices(adjacentList);
				// Store how many adjacent entries there will be in adjacentVertexIds for this vertex ID
				int edgeCount = adjacentList.length();
				adjacentVertexCounts[inputGeometryIt.index()] = edgeCount;
				// Make sure we have space to store the new adjacent vertex IDs
				int runningTotal = adjacentVertexIds.length();
				int size = edgeCount * sizeof(int);
				adjacentVertexIds.setLength(runningTotal + edgeCount);
				// Copy over these IDs
				memcpy_s(&adjacentVertexIds[runningTotal], size, &adjacentList[0], size);
				// Continue to the next vertex
				inputGeometryIt.next();
			}
		}
	}
DEFORMER_END



LOCATOR_BEGIN(DemoLocator) {/*empty compute*/}
INPUT_ARRAY(FVec3, points)
INPUT_ARRAY(int, triangles)
LOCATOR_DRAW(DemoLocator)  {
}
LOCATOR_END