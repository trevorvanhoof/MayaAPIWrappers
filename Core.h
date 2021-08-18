#pragma once

#pragma comment(lib, "Foundation.lib")
#pragma comment(lib, "OpenMaya.lib")
#pragma comment(lib, "OpenMayaAnim.lib")

#include <vector>

#include <maya/MFnPlugin.h>
#include <maya/MPxNode.h>
#include <maya/MString.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnMatrixAttribute.h>
#include <maya/MFnCompoundAttribute.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MArrayDataBuilder.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MDistance.h>
#include <maya/MAngle.h>
#include <maya/MTime.h>
#include <maya/MFloatVector.h>
#include <maya/MPoint.h>
#include <maya/MVector.h>
#include <maya/MColor.h>
#include <maya/MMatrix.h>
#include <maya/MFloatMatrix.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MQuaternion.h>
#include <maya/MFnMessageAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MPxDeformerNode.h>
#include <maya/MFnMesh.h>
#include <maya/MItMeshVertex.h>
#include <maya/MPointArray.h>
#include <maya/MItGeometry.h>
#include <maya/MGlobal.h>
#include <maya/MEulerRotation.h>

#define DECL_MFN_MOBJECT(N) struct N { MObject obj; N(MObject obj) : obj(obj) {} operator MObject() { return obj; } operator const MObject&() const { return obj; } };

#include "MFnHelpers.inc"

struct Meta {
	MObject node;
	MDataBlock& data;
	unsigned int deformerMultiIndex;
	Meta(MObject node, MDataBlock& data, unsigned int deformerMultiIndex = 0) : node(node), data(data), deformerMultiIndex(deformerMultiIndex) {}
	operator MDataBlock& () { return data; }
};

template<typename T>
class TMPxNode : public MPxNode {
public:
	static void* creator() { return new T; }
protected:
	virtual void compute(Meta b) = 0;
	virtual bool isInputPlug(const MPlug& p) = 0;
private:
	MStatus compute(const MPlug& p, MDataBlock& b) override { if (!isInputPlug(p)) return MS::kUnknownParameter; compute({ thisMObject(), b }); return MS::kSuccess; }
};

template<typename T>
class TMPxDeformer : public MPxDeformerNode {
public:
	static void* creator() { return new T; }
protected:
	static void makePaintable(const char* nodeName) {
		MGlobal::executeCommand(MString("makePaintable -attrType multiFloat -sm deformer ") + nodeName + " weights", true);
	}
	virtual MStatus deform(Meta b, MItGeometry& outputGeometry, const MMatrix& worldMatrix) = 0;
private:
	MStatus deform(MDataBlock& b, MItGeometry& outputGeometry, const MMatrix& worldMatrix, unsigned int multiIndex) override { return deform(Meta(thisMObject(), b, multiIndex), outputGeometry, worldMatrix); }
};

template<typename T, MFn::Type G>
class TTypedMPxDeformer : public TMPxDeformer<T> {
protected:
	MObject inputGeometry(Meta& b) {
		// MItGeometry is a copy of the input that will be written to the output. Therefore we can't write to the output directly, but only to to MItGeometry.
		// TODO: Verify that MItGeometry can't just be ignored if we get a handle to the output plug
		// MItGeometry is geometry-agnostic so we can't use it for polygon-specific deformers. Therefore we will get the input geometry as the right type.
		MStatus status;
		MArrayDataHandle inputGeometryArray = b.data.inputArrayValue(T::input, &status); CHECK_MSTATUS_AND_RETURN(status, MObject::kNullObj);
		status = inputGeometryArray.jumpToElement(b.deformerMultiIndex); CHECK_MSTATUS_AND_RETURN(status, MObject::kNullObj);
		MDataHandle inputGeometryArrayElement = inputGeometryArray.inputValue(&status); CHECK_MSTATUS_AND_RETURN(status, MObject::kNullObj);
		MObject inputShape = inputGeometryArrayElement.child(T::inputGeom).asMesh();
		if (inputShape == MObject::kNullObj || !inputShape.hasFn(G)) {
			MGlobal::displayError("This deformer only works on specific geometry data, input is wrong type of shape.");
			status = MS::kFailure;
			return MObject::kNullObj;
		}
		return inputShape;
	}
};

MStatus enumInitialize(MObject& dst, const MString& enumName, const std::vector<MString>& fieldNames, const std::vector<short> fieldIndices) {
	MFnEnumAttribute fn;
	MStatus status;
	dst = fn.create(enumName, enumName, fieldIndices[0], &status); CHECK_MSTATUS_AND_RETURN_IT(status);
	for (unsigned int i = 0; i < fieldNames.size(); ++i) {
		status = fn.addField(fieldNames[i], fieldIndices[i]);
		CHECK_MSTATUS_AND_RETURN_IT(status);
	}
	return status;
}

unsigned int arraySize(Meta b, const MObject& o);
MDataHandle arrayInputElement(Meta b, const MObject& o, unsigned int index, MStatus& status);

template<typename T> std::vector<T> get(MArrayDataHandle& element) {
	std::vector<T> result;
	result.reserve(element.elementCount());
	for (unsigned int i = 0; i < element.elementCount(); ++i) {
		element.jumpToArrayElement(i);
		result.push_back(get<T>(element.inputValue()));
	}
	return result;
}
template<typename T> T get(MArrayDataHandle& element, const std::vector<MObject>& compoundChildren) {
	std::vector<T> result;
	result.reserve(element.elementCount());
	for (unsigned int i = 0; i < element.elementCount(); ++i) {
		element.jumpToArrayElement(i);
		result.push_back(get<T>(element.inputValue(), compoundChildren));
	}
	return result;
}
template<typename T> void set(MArrayDataHandle& element, const std::vector<T>& value) {
	MStatus status;
	MArrayDataBuilder builder = element.builder(&status); CHECK_MSTATUS(status); if (MStatus::kSuccess != status) return;
	for (unsigned int index = 0; index < (unsigned int)value.size(); ++index) {
		MDataHandle child = builder.addElement(index, &status); CHECK_MSTATUS(status); if (MStatus::kSuccess != status) return;
		set<T>(child, value[index]);
	}
	element.set(builder);
}
template<typename T> void set(MArrayDataHandle& element, const std::vector<MObject>& compoundChildren, const T& value) {
	MStatus status;
	MArrayDataBuilder builder = element.builder(&status); CHECK_MSTATUS(status); if (MStatus::kSuccess != status) return;
	for (unsigned int index = 0; index < (unsigned int)value.size(); ++index) {
		MDataHandle child = builder.addElement(index, &status); CHECK_MSTATUS(status); if (MStatus::kSuccess != status) return;
		set<T>(child, compoundChildren, value[index]);
	}
	element.set(builder);
}

template<typename T> T get(MDataHandle& element);
template<typename T> T get(MDataHandle& element, const std::vector<MObject>& compoundChildren);
template<typename T> void set(MDataHandle& element, const T& value);
template<typename T> void set(MDataHandle& element, const std::vector<MObject>& compoundChildren, const T& value);
template<typename T> T fallback() { return T(); } // fallback value in case of errors

template<typename T> T getAttr(Meta b, const MObject& o) { MDataHandle h = b.data.inputValue(o); return get<T>(h); }
template<typename T> T getAttr(Meta b, const MObject& o, const std::vector<MObject>& compoundChildren) { MDataHandle h = b.data.inputValue(o); return get<T>(h, compoundChildren); }

template<typename T> T getArray(Meta b, const MObject& o, int index) {
	MStatus status;
	MDataHandle element = arrayInputElement(b, o, index, status);
	if (status != MS::kSuccess) // element is not valid
		return fallback<T>();
	return get<T>(element);
}
template<typename T> T getArray(Meta b, const MObject& o, const std::vector<MObject>& compoundChildren, int index) {
	MStatus status;
	MDataHandle element = arrayInputElement(b, o, index, status);
	if (status != MS::kSuccess) // element is not valid
		return fallback<T>();
	return get<T>(element, compoundChildren);
}

template<typename T> MStatus setAttr(Meta b, const MObject& o, const T& value) {
	MStatus status;
	MDataHandle element = b.data.outputValue(o, &status); CHECK_MSTATUS_AND_RETURN_IT(status);
	set<T>(element, value);
	return status;
}

template<typename T> MStatus setAttr(Meta b, const MObject& o, const std::vector<MObject>& compoundChildren, const T& value) {
	MStatus status;
	MDataHandle element = b.data.outputValue(o, &status); CHECK_MSTATUS_AND_RETURN_IT(status);
	set<T>(element, compoundChildren, value);
	return status;
}

template<typename T> MStatus setArray(Meta b, const MObject& o, const std::vector<T>& value) {
	MStatus status;
	MDataHandle element;
	MArrayDataHandle handle = b.data.outputArrayValue(o, &status); CHECK_MSTATUS_AND_RETURN_IT(status);
	MArrayDataBuilder builder = handle.builder(&status); CHECK_MSTATUS_AND_RETURN_IT(status);
	for (unsigned int index = 0; index < (unsigned int)value.size(); ++index) {
		element = builder.addElement(index, &status); CHECK_MSTATUS_AND_RETURN_IT(status);
		set<T>(element, value[index]);
	}
	handle.set(builder);
	return status;
}

template<typename T> MStatus setArray(Meta b, const MObject& o, const std::vector<MObject>& compoundChildren, const std::vector<T>& value) {
	MStatus status;
	MDataHandle element;
	MArrayDataHandle handle = b.data.outputArrayValue(o, &status); CHECK_MSTATUS_AND_RETURN_IT(status);
	MArrayDataBuilder builder = handle.builder(&status); CHECK_MSTATUS_AND_RETURN_IT(status);
	for (unsigned int index = 0; index < (unsigned int)value.size(); ++index) {
		element = builder.addElement(index, &status); CHECK_MSTATUS_AND_RETURN_IT(status);
		set<T>(element, compoundChildren, value[index]);
	}
	handle.set(builder);
	return status;
}

template<typename T> MStatus initialize(MObject& dst, const char* name);
template<typename T> MStatus initialize(MObject& dst, const char* name, std::vector<MObject>& dstChildren);

#define TYPED_DEFORMER_BEGIN(N, TYPE) class N : public TTypedMPxDeformer<N, MFn::k##TYPE> { public: static MStatus initialize(); virtual MStatus deform(Meta b, MItGeometry& outputGeometry, const MMatrix& worldMatrix) override
#define DEFORMER_BEGIN(N)             class N : public TMPxDeformer<N>                    { public: static MStatus initialize(); virtual MStatus deform(Meta b, MItGeometry& outputGeometry, const MMatrix& worldMatrix) override
#define DEFORMER_END };

#define NODE_BEGIN(N) class N : public TMPxNode<N> { public: static MStatus initialize(); protected: virtual bool isInputPlug(const MPlug& p) override; virtual void compute(Meta b) override
#define NODE_END };

#define INPUT(T, N) static MObject N##Attr; T N(Meta dataBlock);
#define OUTPUT(T, N) static MObject N##Attr; void N##Set(Meta dataBlock, const T& value);
#define INOUT(T, N) static MObject N##Attr; T N(Meta dataBlock); void N##Set(Meta dataBlock, const T& value);
#define INPUT_ARRAY(T, N) static MObject N##Attr; int N##Size(Meta dataBlock); T N(Meta dataBlock, int index);
#define OUTPUT_ARRAY(T, N) static MObject N##Attr; int N##Size(Meta dataBlock); void N##Set(Meta dataBlock, const std::vector<T>& value);
#define INOUT_ARRAY(T, N) static MObject N##Attr; int N##Size(Meta dataBlock); T N(Meta dataBlock, int index); void N##Set(Meta dataBlock, const std::vector<T>& value);

#define ENUM_BEGIN(N) enum class N {
#define ENUM(N) N,
#define ENUM_VALUE(N, V) N = V,
#define ENUM_END };

#define COMPOUND_BEGIN(N) struct N {
#define COMPOUND_VALUE(T, N) T N;
#define COMPOUND_ARRAY(T, N) std::vector<T> N;
#define COMPOUND_END };
