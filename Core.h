#pragma once

#pragma comment(lib, "Foundation.lib")
#pragma comment(lib, "OpenMaya.lib")

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

#define DECL_MFN_MOBJECT(N) class N : public MObject {};

#include "MFnHelpers.inc"

template<typename T>
class TMPxNode : public MPxNode {
public:
	static void* creator() { return new T; }
protected:
	virtual void compute(MDataBlock& b) = 0;
	virtual bool isInputPlug(const MPlug& p) = 0;
private:
	MStatus compute(const MPlug& p, MDataBlock& b) override { if (!isInputPlug(p)) return MS::kUnknownParameter; compute(b); return MS::kSuccess; }
};

MStatus enumInitialize(MObject& dst, const MString& enumName, const std::vector<MString>& fieldNames, const std::vector<short> fieldIndices) {
	MFnEnumAttribute fn;
	MStatus status;
	dst = fn.create(enumName, enumName, fieldIndices[0], &status); CHECK_MSTATUS_AND_RETURN_IT(status);
	for (int i = 0; i < fieldNames.size(); ++i) {
		status = fn.addField(fieldNames[i], fieldIndices[i]); 
		CHECK_MSTATUS_AND_RETURN_IT(status);
	}
	return status;
}

unsigned int arraySize(MDataBlock& b, const MObject& o);
MDataHandle arrayInputElement(MDataBlock& b, const MObject& o, unsigned int index, MStatus& status);

template<typename T> T get(MDataHandle& element);
template<typename T> T get(MDataHandle& element, const std::vector<MObject>& compoundChildren);
template<typename T> void set(MDataHandle& element, const T& value);
template<typename T> void set(MDataHandle& element, const std::vector<MObject>& compoundChildren, const T& value);
template<typename T> T fallback() { return T(); } // fallback value in case of errors

template<typename T> T getAttr(MDataBlock& b, const MObject& o) { MDataHandle h = b.inputValue(o); return get<T>(h); }
template<typename T> T getAttr(MDataBlock& b, const MObject& o, const std::vector<MObject>& compoundChildren) { MDataHandle h = b.inputValue(o); return get<T>(h, compoundChildren); }

template<typename T> T getArray(MDataBlock& b, const MObject& o, int index) {
	MStatus status;
	MDataHandle element = arrayInputElement(b, o, index, status);
	if (status != MS::kSuccess) // element is not valid
		return fallback<T>();
	return get<T>(element);
}
template<typename T> T getArray(MDataBlock& b, const MObject& o, const std::vector<MObject>& compoundChildren, int index) {
	MStatus status;
	MDataHandle element = arrayInputElement(b, o, index, status);
	if (status != MS::kSuccess) // element is not valid
		return fallback<T>();
	return get<T>(element, compoundChildren);
}

template<typename T> MStatus setAttr(MDataBlock& b, const MObject& o, const T& value) {
	MStatus status;
	MDataHandle element = b.outputValue(o, &status); CHECK_MSTATUS_AND_RETURN_IT(status);
	set<T>(element, value);
	return status;
}

template<typename T> MStatus setAttr(MDataBlock& b, const MObject& o, const std::vector<MObject>& compoundChildren, const T& value) {
	MStatus status;
	MDataHandle element = b.outputValue(o, &status); CHECK_MSTATUS_AND_RETURN_IT(status);
	set<T>(element, compoundChildren, value);
	return status;
}

template<typename T> MStatus setArray(MDataBlock& b, const MObject& o, const std::vector<T>& value) {
	MStatus status;
	MDataHandle element;
	MArrayDataHandle handle = b.outputArrayValue(o, &status); CHECK_MSTATUS_AND_RETURN_IT(status);
	MArrayDataBuilder builder = handle.builder(&status); CHECK_MSTATUS_AND_RETURN_IT(status);
	for (unsigned int index = 0; index < (unsigned int)value.size(); ++index) {
		element = builder.addElement(index, &status); CHECK_MSTATUS_AND_RETURN_IT(status);
		set<T>(element, value[index]);
	}
	handle.set(builder);
	return status;
}

template<typename T> MStatus setArray(MDataBlock& b, const MObject& o, const std::vector<MObject>& compoundChildren, const std::vector<T>& value) {
	MStatus status;
	MDataHandle element;
	MArrayDataHandle handle = b.outputArrayValue(o, &status); CHECK_MSTATUS_AND_RETURN_IT(status);
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

#define NODE_BEGIN(N) class N : public TMPxNode<N> { public: static MStatus initialize(); protected: virtual bool isInputPlug(const MPlug& p) override; virtual void compute(MDataBlock& b) override
#define INPUT(T, N) static MObject N##Attr; T N(MDataBlock& dataBlock);
#define OUTPUT(T, N) static MObject N##Attr; void N##Set(MDataBlock& dataBlock, const T& value);
#define INOUT(T, N) static MObject N##Attr; T N(MDataBlock& dataBlock); void N##Set(MDataBlock& dataBlock, const T& value);
#define INPUT_ARRAY(T, N) static MObject N##Attr; int N##Size(MDataBlock& dataBlock); T N(MDataBlock& dataBlock, int index);
#define OUTPUT_ARRAY(T, N) static MObject N##Attr; int N##Size(MDataBlock& dataBlock); void N##Set(MDataBlock& dataBlock, const std::vector<T>& value);
#define INOUT_ARRAY(T, N) static MObject N##Attr; int N##Size(MDataBlock& dataBlock); T N(MDataBlock& dataBlock, int index); void N##Set(MDataBlock& dataBlock, const std::vector<T>& value);
#define NODE_END };

#define ENUM_BEGIN(N) enum class N {
#define ENUM(N) N,
#define ENUM_VALUE(N, V) N = V,
#define ENUM_END };

#define COMPOUND_BEGIN(N) struct N {
#define COMPOUND_VALUE(T, N) T N;
#define COMPOUND_ARRAY(T, N) std::vector<T> N;
#define COMPOUND_END };
