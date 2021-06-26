#include "MyNode.h"

template<> Axis fallback<Axis>() { return Axis::X; }
template<> Axis get<Axis>(MDataHandle& element) { return (Axis)element.asShort(); }
template<> void set<Axis>(MDataHandle& element, const Axis& value) {
    element.setShort((short)value);
    element.setClean();
}
template<> MStatus initialize<Axis>(MObject& dst, const char* name) { return enumInitialize(dst, name, {"X", "Y", "Z"}, {0, 1, 2}); }

template<> MStatus initialize<Linear3>(MObject& dst, const char* name, std::vector<MObject>& children) {
    MStatus status;
    MFnCompoundAttribute fn;
    dst = fn.create(name, name, &status); 
    CHECK_MSTATUS_AND_RETURN_IT(status);

    children.emplace_back();
    status = initialize<MDistance>(children[children.size() - 1], "x"); CHECK_MSTATUS_AND_RETURN_IT(status);
    status = fn.addChild(children[children.size() - 1]); CHECK_MSTATUS_AND_RETURN_IT(status);

    children.emplace_back();
    status = initialize<MDistance>(children[children.size() - 1], "y"); CHECK_MSTATUS_AND_RETURN_IT(status);
    status = fn.addChild(children[children.size() - 1]); CHECK_MSTATUS_AND_RETURN_IT(status);

    children.emplace_back();
    status = initialize<MDistance>(children[children.size() - 1], "z"); CHECK_MSTATUS_AND_RETURN_IT(status);
    status = fn.addChild(children[children.size() - 1]); CHECK_MSTATUS_AND_RETURN_IT(status);

    return status;
}
 template<> Linear3 get<Linear3>(MDataHandle& element, const std::vector<MObject>& objects) {
    Linear3 result;
    MDataHandle child;
    child = element.child(objects[0]); result.x = get<MDistance>(child);
    child = element.child(objects[1]); result.y = get<MDistance>(child);
    child = element.child(objects[2]); result.z = get<MDistance>(child);
    return result;
}
template<> void set<Linear3>(MDataHandle& element, const std::vector<MObject>& objects, const Linear3& value) {
    MDataHandle child;
    child = element.child(objects[0]); set<MDistance>(child, value.x);
    child.setClean();

    child = element.child(objects[1]); set<MDistance>(child, value.y);
    child.setClean();

    child = element.child(objects[2]); set<MDistance>(child, value.z);
    child.setClean();

    element.setClean();
}

template<> MStatus initialize<MatrixOnCurve>(MObject& dst, const char* name, std::vector<MObject>& children) {
    MStatus status;
    MFnCompoundAttribute fn;
    dst = fn.create(name, name, &status); 
    CHECK_MSTATUS_AND_RETURN_IT(status);

    children.emplace_back();
    status = initialize<float>(children[children.size() - 1], "parameter"); CHECK_MSTATUS_AND_RETURN_IT(status);
    status = fn.addChild(children[children.size() - 1]); CHECK_MSTATUS_AND_RETURN_IT(status);

    children.emplace_back();
    status = initialize<MPoint>(children[children.size() - 1], "result"); CHECK_MSTATUS_AND_RETURN_IT(status);
    status = fn.addChild(children[children.size() - 1]); CHECK_MSTATUS_AND_RETURN_IT(status);

    return status;
}
 template<> MatrixOnCurve get<MatrixOnCurve>(MDataHandle& element, const std::vector<MObject>& objects) {
    MatrixOnCurve result;
    MDataHandle child;
    child = element.child(objects[0]); result.parameter = get<float>(child);
    child = element.child(objects[1]); result.result = get<MPoint>(child);
    return result;
}
template<> void set<MatrixOnCurve>(MDataHandle& element, const std::vector<MObject>& objects, const MatrixOnCurve& value) {
    MDataHandle child;
    child = element.child(objects[0]); set<float>(child, value.parameter);
    child.setClean();

    child = element.child(objects[1]); set<MPoint>(child, value.result);
    child.setClean();

    element.setClean();
}

MObject MyNode::inputValueAttr;
MObject MyNode::inputArrayAttr;
MObject MyNode::outputValueAttr;
MObject MyNode::outputArrayAttr;
MObject MyNode::enumTestAttr;
MObject MyNode::pointArrayTestAttr;
std::vector<MObject> _MyNode_pointArrayTest_children;

bool MyNode::isInputPlug(const MPlug& p) {
    return (p == inputValueAttr || p == inputArrayAttr || p == enumTestAttr);
}

MStatus MyNode::initialize() {
    MStatus status;

    status = ::initialize<double>(inputValueAttr, "inputValue"); CHECK_MSTATUS_AND_RETURN_IT(status);
    status = addAttribute(inputValueAttr); CHECK_MSTATUS_AND_RETURN_IT(status);

    status = ::initialize<double>(inputArrayAttr, "inputArray"); CHECK_MSTATUS_AND_RETURN_IT(status);
    status = MFnAttribute(inputArrayAttr).setArray(true); CHECK_MSTATUS_AND_RETURN_IT(status);
    status = addAttribute(inputArrayAttr); CHECK_MSTATUS_AND_RETURN_IT(status);

    status = ::initialize<double>(outputValueAttr, "outputValue"); CHECK_MSTATUS_AND_RETURN_IT(status);
    status = addAttribute(outputValueAttr); CHECK_MSTATUS_AND_RETURN_IT(status);

    status = ::initialize<double>(outputArrayAttr, "outputArray"); CHECK_MSTATUS_AND_RETURN_IT(status);
    status = MFnAttribute(outputArrayAttr).setArray(true); CHECK_MSTATUS_AND_RETURN_IT(status);
    status = addAttribute(outputArrayAttr); CHECK_MSTATUS_AND_RETURN_IT(status);

    status = ::initialize<Axis>(enumTestAttr, "enumTest"); CHECK_MSTATUS_AND_RETURN_IT(status);
    status = addAttribute(enumTestAttr); CHECK_MSTATUS_AND_RETURN_IT(status);

    status = ::initialize<Linear3>(pointArrayTestAttr, "pointArrayTest", _MyNode_pointArrayTest_children); CHECK_MSTATUS_AND_RETURN_IT(status);
    status = MFnAttribute(pointArrayTestAttr).setArray(true); CHECK_MSTATUS_AND_RETURN_IT(status);
    status = addAttribute(pointArrayTestAttr); CHECK_MSTATUS_AND_RETURN_IT(status);

    status = attributeAffects(inputValueAttr, outputValueAttr); CHECK_MSTATUS_AND_RETURN_IT(status);
    status = attributeAffects(inputArrayAttr, outputValueAttr); CHECK_MSTATUS_AND_RETURN_IT(status);
    status = attributeAffects(enumTestAttr, outputValueAttr); CHECK_MSTATUS_AND_RETURN_IT(status);
    status = attributeAffects(pointArrayTestAttr, outputValueAttr); CHECK_MSTATUS_AND_RETURN_IT(status);
    for(const MObject& obj2 : _MyNode_pointArrayTest_children) {
        status = attributeAffects(obj2, outputValueAttr); CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    status = attributeAffects(inputValueAttr, outputArrayAttr); CHECK_MSTATUS_AND_RETURN_IT(status);
    status = attributeAffects(inputArrayAttr, outputArrayAttr); CHECK_MSTATUS_AND_RETURN_IT(status);
    status = attributeAffects(enumTestAttr, outputArrayAttr); CHECK_MSTATUS_AND_RETURN_IT(status);
    status = attributeAffects(pointArrayTestAttr, outputArrayAttr); CHECK_MSTATUS_AND_RETURN_IT(status);
    for(const MObject& obj2 : _MyNode_pointArrayTest_children) {
        status = attributeAffects(obj2, outputArrayAttr); CHECK_MSTATUS_AND_RETURN_IT(status);
    }


    return status;
}


double MyNode::inputValue(MDataBlock& dataBlock) { return getAttr<double>(dataBlock, inputValueAttr); }

int MyNode::inputArraySize(MDataBlock& dataBlock) { return arraySize(dataBlock, inputArrayAttr); }
double MyNode::inputArray(MDataBlock& dataBlock, int index) { return getArray<double>(dataBlock, inputArrayAttr, index); }

void MyNode::outputValueSet(MDataBlock& dataBlock, const double& value) { setAttr<double>(dataBlock, outputValueAttr, value); }

int MyNode::outputArraySize(MDataBlock& dataBlock) { return arraySize(dataBlock, outputArrayAttr); }
void MyNode::outputArraySet(MDataBlock& dataBlock, const std::vector<double>& value) { setArray<double>(dataBlock, outputArrayAttr, value); }

Axis MyNode::enumTest(MDataBlock& dataBlock) { return getAttr<Axis>(dataBlock, enumTestAttr); }

int MyNode::pointArrayTestSize(MDataBlock& dataBlock) { return arraySize(dataBlock, pointArrayTestAttr); }
Linear3 MyNode::pointArrayTest(MDataBlock& dataBlock, int index) { return getArray<Linear3>(dataBlock, pointArrayTestAttr, _MyNode_pointArrayTest_children, index); }
void MyNode::pointArrayTestSet(MDataBlock& dataBlock, const std::vector<Linear3>& value) { setArray<Linear3>(dataBlock, pointArrayTestAttr, _MyNode_pointArrayTest_children, value); }
MObject MatricesOnCurve::inputCurveAttr;
MObject MatricesOnCurve::dataAttr;
std::vector<MObject> _MatricesOnCurve_data_children;

bool MatricesOnCurve::isInputPlug(const MPlug& p) {
    return (p == inputCurveAttr);
}

MStatus MatricesOnCurve::initialize() {
    MStatus status;

    status = ::initialize<NurbsCurve>(inputCurveAttr, "inputCurve"); CHECK_MSTATUS_AND_RETURN_IT(status);
    status = addAttribute(inputCurveAttr); CHECK_MSTATUS_AND_RETURN_IT(status);

    status = ::initialize<MatrixOnCurve>(dataAttr, "data", _MatricesOnCurve_data_children); CHECK_MSTATUS_AND_RETURN_IT(status);
    status = MFnAttribute(dataAttr).setArray(true); CHECK_MSTATUS_AND_RETURN_IT(status);
    status = addAttribute(dataAttr); CHECK_MSTATUS_AND_RETURN_IT(status);


    return status;
}


NurbsCurve MatricesOnCurve::inputCurve(MDataBlock& dataBlock) { return getAttr<NurbsCurve>(dataBlock, inputCurveAttr); }

int MatricesOnCurve::dataSize(MDataBlock& dataBlock) { return arraySize(dataBlock, dataAttr); }
MatrixOnCurve MatricesOnCurve::data(MDataBlock& dataBlock, int index) { return getArray<MatrixOnCurve>(dataBlock, dataAttr, _MatricesOnCurve_data_children, index); }
void MatricesOnCurve::dataSet(MDataBlock& dataBlock, const std::vector<MatrixOnCurve>& value) { setArray<MatrixOnCurve>(dataBlock, dataAttr, _MatricesOnCurve_data_children, value); }

#define INITIALIZE_PLUGIN REGISTER_NODE(MyNode) REGISTER_NODE(MatricesOnCurve)