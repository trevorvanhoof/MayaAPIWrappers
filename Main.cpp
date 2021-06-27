#include "Core.h"

#pragma region array_helpers
unsigned int arraySize(MDataBlock& b, const MObject& o) {
	MStatus status;
	MArrayDataHandle h = b.inputArrayValue(o, &status); CHECK_MSTATUS_AND_RETURN(status, 0);
	unsigned int r = h.elementCount(&status); CHECK_MSTATUS_AND_RETURN(status, 0);
	return r;
}

MDataHandle arrayInputElement(MDataBlock& b, const MObject& o, unsigned int index, MStatus& status) {
	MDataHandle element;
	MArrayDataHandle handle = b.inputArrayValue(o, &status); CHECK_MSTATUS_AND_RETURN(status, element);
	status = handle.jumpToElement(index); CHECK_MSTATUS_AND_RETURN(status, element);
	element = handle.inputValue(&status); CHECK_MSTATUS_AND_RETURN(status, element);
	return element;
}
#pragma endregion

#pragma region attribute_implementations
/// MFnNumericAttribute ///
#define IMPLEMENT_NUMERIC(n, N, S, K) \
template<> n get<n>(MDataHandle& element) { return element.as##N(); } \
template<> void set<n>(MDataHandle& element, const n& value) { element.set##S(value); element.setClean(); } \
template<> MStatus initialize<n>(MObject& dst, const char* name) { MFnNumericAttribute fn; MStatus status; dst = fn.create(name, name, MFnNumericData::K, 0.0, &status); CHECK_MSTATUS_AND_RETURN_IT(status); return status; }

IMPLEMENT_NUMERIC(double, Double, Double, kDouble)
IMPLEMENT_NUMERIC(float, Float, Float, kFloat)
IMPLEMENT_NUMERIC(int, Int, Int, kInt)
IMPLEMENT_NUMERIC(long long, Int64, Int64, kInt64)
IMPLEMENT_NUMERIC(short, Short, Short, kShort)
IMPLEMENT_NUMERIC(bool, Bool, Bool, kBoolean)
// IMPLEMENT_NUMERIC(unsigned char, UChar, kByte) // There is no setter for byte...
IMPLEMENT_NUMERIC(char, Char, Char, kChar)

struct SVec2 { short x = 0; short y = 0; SVec2() {} SVec2(const short2& i) { x = i[0]; y = i[1]; } };
struct IVec2 { int x = 0; int y = 0; IVec2() {} IVec2(const int2& i) { x = i[0]; y = i[1]; } };
struct FVec2 { float x = 0; float y = 0; FVec2() {} FVec2(const float2& i) { x = i[0]; y = i[1]; } };
struct DVec2 { double x = 0; double y = 0; DVec2() {} DVec2(const double2& i) { x = i[0]; y = i[1]; } };
struct SVec3 { short x = 0; short y = 0; short z = 0; SVec3() {} SVec3(const short3& i) { x = i[0]; y = i[1]; z = i[2]; } };
struct IVec3 { int x = 0; int y = 0; int z = 0; IVec3() {} IVec3(const int3& i) { x = i[0]; y = i[1]; z = i[2]; } };
struct FVec3 { float x = 0; float y = 0; float z = 0; FVec3() {} FVec3(const float3& i) { x = i[0]; y = i[1]; z = i[2]; } };
struct DVec3 { double x = 0; double y = 0; double z = 0; DVec3() {} DVec3(const double3& i) { x = i[0]; y = i[1]; z = i[2]; } };
struct DVec4 { double x = 0; double y = 0; double z = 0; double w = 0; DVec4() {} DVec4(const double4& i) { x = i[0]; y = i[1]; z = i[2]; w = i[3]; } };

template<> SVec2 get<SVec2>(MDataHandle& element) { return element.asShort2(); }
template<> void set<SVec2>(MDataHandle& element, const SVec2& value) { element.set2Short(value.x, value.y); element.setClean(); }
template<> MStatus initialize<SVec2>(MObject& dst, const char* name) { MFnNumericAttribute fn; MStatus status; dst = fn.create(name, name, MFnNumericData::k2Short, 0.0, &status); CHECK_MSTATUS_AND_RETURN_IT(status); return status; }

template<> IVec2 get<IVec2>(MDataHandle& element) { return element.asInt2(); }
template<> void set<IVec2>(MDataHandle& element, const IVec2& value) { element.set2Int(value.x, value.y); element.setClean(); }
template<> MStatus initialize<IVec2>(MObject& dst, const char* name) { MFnNumericAttribute fn; MStatus status; dst = fn.create(name, name, MFnNumericData::k2Int, 0.0, &status); CHECK_MSTATUS_AND_RETURN_IT(status); return status; }

template<> FVec2 get<FVec2>(MDataHandle& element) { return element.asFloat2(); }
template<> void set<FVec2>(MDataHandle& element, const FVec2& value) { element.set2Float(value.x, value.y); element.setClean(); }
template<> MStatus initialize<FVec2>(MObject& dst, const char* name) { MFnNumericAttribute fn; MStatus status; dst = fn.create(name, name, MFnNumericData::k2Float, 0.0, &status); CHECK_MSTATUS_AND_RETURN_IT(status); return status; }

template<> DVec2 get<DVec2>(MDataHandle& element) { return element.asDouble2(); }
template<> void set<DVec2>(MDataHandle& element, const DVec2& value) { element.set2Double(value.x, value.y); element.setClean(); }
template<> MStatus initialize<DVec2>(MObject& dst, const char* name) { MFnNumericAttribute fn; MStatus status; dst = fn.create(name, name, MFnNumericData::k2Double, 0.0, &status); CHECK_MSTATUS_AND_RETURN_IT(status); return status; }

template<> SVec3 get<SVec3>(MDataHandle& element) { return element.asShort3(); }
template<> void set<SVec3>(MDataHandle& element, const SVec3& value) { element.set3Short(value.x, value.y, value.z); element.setClean(); }
template<> MStatus initialize<SVec3>(MObject& dst, const char* name) { MFnNumericAttribute fn; MStatus status; dst = fn.create(name, name, MFnNumericData::k3Short, 0.0, &status); CHECK_MSTATUS_AND_RETURN_IT(status); return status; }

template<> IVec3 get<IVec3>(MDataHandle& element) { return element.asInt3(); }
template<> void set<IVec3>(MDataHandle& element, const IVec3& value) { element.set3Int(value.x, value.y, value.z); element.setClean(); }
template<> MStatus initialize<IVec3>(MObject& dst, const char* name) { MFnNumericAttribute fn; MStatus status; dst = fn.create(name, name, MFnNumericData::k3Int, 0.0, &status); CHECK_MSTATUS_AND_RETURN_IT(status); return status; }

template<> FVec3 get<FVec3>(MDataHandle& element) { return element.asFloat3(); }
template<> void set<FVec3>(MDataHandle& element, const FVec3& value) { element.set3Float(value.x, value.y, value.z); element.setClean(); }
template<> MStatus initialize<FVec3>(MObject& dst, const char* name) { MFnNumericAttribute fn; MStatus status; dst = fn.create(name, name, MFnNumericData::k3Float, 0.0, &status); CHECK_MSTATUS_AND_RETURN_IT(status); return status; }

template<> DVec3 get<DVec3>(MDataHandle& element) { return element.asDouble3(); }
template<> void set<DVec3>(MDataHandle& element, const DVec3& value) { element.set3Double(value.x, value.y, value.z); element.setClean(); }
template<> MStatus initialize<DVec3>(MObject& dst, const char* name) { MFnNumericAttribute fn; MStatus status; dst = fn.create(name, name, MFnNumericData::k3Double, 0.0, &status); CHECK_MSTATUS_AND_RETURN_IT(status); return status; }

template<> DVec4 get<DVec4>(MDataHandle& element) { return element.asDouble4(); }
template<> void set<DVec4>(MDataHandle& element, const DVec4& value) { element.set4Double(value.x, value.y, value.z, value.w); element.setClean(); }
template<> MStatus initialize<DVec4>(MObject& dst, const char* name) { MFnNumericAttribute fn; MStatus status; dst = fn.create(name, name, MFnNumericData::k4Double, 0.0, &status); CHECK_MSTATUS_AND_RETURN_IT(status); return status; }

/* There is no setter for addr...
template<> size_t get<size_t>(MDataHandle& element) { void* addr = element.asAddr(); return *reinterpret_cast<size_t*>(&addr); }
template<> void set<size_t>(MDataHandle& element, const size_t& value) { element.setAddr(value); element.setClean(); }
template<> MStatus initialize<size_t>(MObject& dst, const char* name) { MFnNumericAttribute fn; MStatus status; dst = fn.create(name, name, MFnNumericData::kAddr, 0.0, &status); CHECK_MSTATUS_AND_RETURN_IT(status); return status; }*/

/// createPoint / createColor usage
template<> MPoint get<MPoint>(MDataHandle& element) {
	return element.asFloatVector();
}

template<> void set<MPoint>(MDataHandle& element, const MPoint& value) {
	element.set3Float(value.x, value.y, value.z);
	element.setClean();
}

template<> MStatus initialize<MPoint>(MObject& dst, const char* name) {
	MFnNumericAttribute fn;
	MStatus status;
	dst = fn.createPoint(name, name, &status); CHECK_MSTATUS_AND_RETURN_IT(status);
	return status;
}

template<> MVector get<MVector>(MDataHandle& element) {
	return element.asFloatVector();
}

template<> void set<MVector>(MDataHandle& element, const MVector& value) {
	element.set3Float(value.x, value.y, value.z);
	element.setClean();
}

template<> MStatus initialize<MVector>(MObject& dst, const char* name) {
	MFnNumericAttribute fn;
	MStatus status;
	dst = fn.createPoint(name, name, &status); CHECK_MSTATUS_AND_RETURN_IT(status);
	return status;
}

template<> MFloatVector get<MFloatVector>(MDataHandle& element) {
	return element.asFloatVector();
}

template<> void set<MFloatVector>(MDataHandle& element, const MFloatVector& value) {
	element.set3Float(value.x, value.y, value.z);
	element.setClean();
}

template<> MStatus initialize<MFloatVector>(MObject& dst, const char* name) {
	MFnNumericAttribute fn;
	MStatus status;
	dst = fn.createPoint(name, name, &status); CHECK_MSTATUS_AND_RETURN_IT(status);
	return status;
}

template<> MColor get<MColor>(MDataHandle& element) {
	MFloatVector tmp = element.asFloatVector();
	return MColor(tmp.x, tmp.y, tmp.z);
}

template<> void set<MColor>(MDataHandle& element, const MColor& value) {
	element.set3Float(value.r, value.g, value.b);
	element.setClean();
}

template<> MStatus initialize<MColor>(MObject& dst, const char* name) {
	MFnNumericAttribute fn;
	MStatus status;
	dst = fn.createColor(name, name, &status); CHECK_MSTATUS_AND_RETURN_IT(status);
	return status;
}

/// MFnUnitAttribute ///
template<> MDistance get<MDistance>(MDataHandle& element) {
	return element.asDistance();
}

template<> void set<MDistance>(MDataHandle& element, const MDistance& value) {
	element.setMDistance(value);
	element.setClean();
}

template<> MStatus initialize<MDistance>(MObject& dst, const char* name) {
	MFnUnitAttribute fn;
	MStatus status;
	dst = fn.create(name, name, MFnUnitAttribute::kDistance, 0.0, &status); CHECK_MSTATUS_AND_RETURN_IT(status);
	return status;
}

template<> MAngle get<MAngle>(MDataHandle& element) {
	return element.asAngle();
}

template<> void set<MAngle>(MDataHandle& element, const MAngle& value) {
	element.setMAngle(value);
	element.setClean();
}

template<> MStatus initialize<MAngle>(MObject& dst, const char* name) {
	MFnUnitAttribute fn;
	MStatus status;
	dst = fn.create(name, name, MFnUnitAttribute::kAngle, 0.0, &status); CHECK_MSTATUS_AND_RETURN_IT(status);
	return status;
}

template<> MTime get<MTime>(MDataHandle& element) {
	return element.asTime();
}

template<> void set<MTime>(MDataHandle& element, const MTime& value) {
	element.setMTime(value);
	element.setClean();
}

template<> MStatus initialize<MTime>(MObject& dst, const char* name) {
	MFnUnitAttribute fn;
	MStatus status;
	dst = fn.create(name, name, MFnUnitAttribute::kTime, 0.0, &status); CHECK_MSTATUS_AND_RETURN_IT(status);
	return status;
}

/// MFnMatrixAttribute ///
template<> MMatrix get<MMatrix>(MDataHandle& element) {
	return element.asMatrix();
}

template<> void set<MMatrix>(MDataHandle& element, const MMatrix& value) {
	element.setMMatrix(value);
	element.setClean();
}

template<> MStatus initialize<MMatrix>(MObject& dst, const char* name) {
	MFnMatrixAttribute fn;
	MStatus status;
	dst = fn.create(name, name, MFnMatrixAttribute::kDouble, &status); CHECK_MSTATUS_AND_RETURN_IT(status);
	return status;
}


template<> MFloatMatrix get<MFloatMatrix>(MDataHandle& element) {
	return element.asFloatMatrix();
}

template<> void set<MFloatMatrix>(MDataHandle& element, const MFloatMatrix& value) {
	element.setMFloatMatrix(value);
	element.setClean();
}

template<> MStatus initialize<MFloatMatrix>(MObject& dst, const char* name) {
	MFnMatrixAttribute fn;
	MStatus status;
	dst = fn.create(name, name, MFnMatrixAttribute::kFloat, &status); CHECK_MSTATUS_AND_RETURN_IT(status);
	return status;
}

template<> MTransformationMatrix get<MTransformationMatrix>(MDataHandle& element) {
	return element.asMatrix();
}

template<> void set<MTransformationMatrix>(MDataHandle& element, const MTransformationMatrix& value) {
	element.setMMatrix(value.asMatrix());
	element.setClean();
}

template<> MStatus initialize<MTransformationMatrix>(MObject& dst, const char* name) {
	MFnMatrixAttribute fn;
	MStatus status;
	dst = fn.create(name, name, MFnMatrixAttribute::kDouble, &status); CHECK_MSTATUS_AND_RETURN_IT(status);
	return status;
}

template<> MQuaternion get<MQuaternion>(MDataHandle& element) {
	double4& tmp = element.asDouble4();
	return MQuaternion(tmp[0], tmp[1], tmp[2], tmp[3]);
}

template<> void set<MQuaternion>(MDataHandle& element, const MQuaternion& value) {
	element.set4Double(value.x, value.y, value.z, value.w);
	element.setClean();
}

template<> MStatus initialize<MQuaternion>(MObject& dst, const char* name) {
	MFnNumericAttribute fn;
	MStatus status;
	dst = fn.create(name, name, MFnNumericData::k4Double, 0.0, &status); CHECK_MSTATUS_AND_RETURN_IT(status);
	fn.setDefault(0.0, 0.0, 0.0, 1.0);
	return status;
}
#pragma endregion

#pragma region plugin_main
#include "generated.inc"

#ifdef MTYPEID_START
const int pluginStartId = MTYPEID_START;
#else
const int pluginStartId = 0;
#endif
int pluginIdCursor = pluginStartId;

std::vector<MString> registeredCommands;

#define REGISTER_NODE(T) status = fn.registerNode(#T, pluginIdCursor, T::creator, T::initialize); ++pluginIdCursor; CHECK_MSTATUS_AND_RETURN_IT(status);

__declspec(dllexport) MStatus initializePlugin(MObject pluginObj) {
	MStatus status;
	MFnPlugin fn(pluginObj);
	INITIALIZE_PLUGIN
		return MS::kSuccess;
}

__declspec(dllexport) MStatus uninitializePlugin(MObject pluginObj) {
	MFnPlugin fn(pluginObj);
	for (int i = pluginIdCursor - 1; i >= pluginStartId; i--) {
		fn.deregisterNode(i);
	}
	for (const MString& command : registeredCommands) {
		fn.deregisterCommand(command);
	}
	return MS::kSuccess;
}
#pragma endregion
