import os


def walk(root):
    for parent, dirs, files in os.walk(root):
        for name in files:
            yield os.path.join(parent, name)


def findFirstOf(data, options, startAt):
    result = len(data)
    resultI = -1
    for i, option in enumerate(options):
        index = data.find(option, startAt)
        if index == -1: continue
        if index < result:
            result = index
            resultI = i
    if resultI != -1:
        return result, resultI
    return None


def scan(path):
    with open(path, 'r') as fh:
        code = fh.read()
    startTokens = ('NODE_BEGIN(', 'ENUM_BEGIN(', 'COMPOUND_BEGIN(', 'DEFORMER_BEGIN(', 'TYPED_DEFORMER_BEGIN(', 'LOCATOR_START(', 'COMPLEX_LOCATOR_START(')
    endTokens = ('NODE_END', 'ENUM_END', 'COMPOUND_END', 'DEFORMER_END', 'DEFORMER_END', 'LOCATOR_END', 'LOCATOR_END')
    output = tuple([] for _ in range(len(startTokens)))
    cursor = 0
    while True:
        result = findFirstOf(code, startTokens, cursor)
        if result is None:
            return output
        cursor, tokenId = result
        endCursor = code.find(endTokens[tokenId], cursor)
        if endCursor == -1:
            raise RuntimeError(
                'Code in "%s" is missing an _END token for "%s" token at %d' % (path, startTokens[i], cursor))
        output[tokenId].append(code[cursor:endCursor])
        cursor = endCursor + len(endTokens[tokenId])


def scanEnum(code):
    cursor = code.find('(')
    assert cursor != -1
    tmp = code.find(')', cursor + 1)
    assert tmp != -1
    enumName = code[cursor + 1:tmp]
    prevValue = -1
    enumOptions = []
    for stuff in code[tmp + 1:].split('ENUM')[1:]:
        a = stuff.find('(')
        b = stuff.find(')')
        assert a != -1 and b != -1
        args = stuff[a + 1:b].split(',')
        if len(args) == 1:
            prevValue += 1
            enumOptions.append((args[0].strip(), prevValue))
        elif len(args) == 2:
            prevValue = int(args[1].strip())
            assert prevValue not in (pair[1] for pair in enumOptions)
            enumOptions.append((args[0].strip(), prevValue))
        else:
            raise ValueError('Error parsing enum element for "%s": "ENUM%s"' % (enumName, stuff))
    return enumName, enumOptions


def processEnum(enumName, enumOptions):
    namesAsStr = '"%s"' % '", "'.join(pair[0] for pair in enumOptions)
    valuesAsStr = ', '.join(str(pair[1]) for pair in enumOptions)
    return """template<> %s fallback<%s>() { return %s::%s; }
template<> %s get<%s>(MDataHandle& element) { return (%s)element.asShort(); }
template<> void set<%s>(MDataHandle& element, const %s& value) {
    element.setShort((short)value);
    element.setClean();
}
template<> MStatus initialize<%s>(MObject& dst, const char* name) { return enumInitialize(dst, name, {%s}, {%s}); }

""" % (
        enumName, enumName, enumName, enumOptions[0][0],  # fallback
        enumName, enumName, enumName,  # get
        enumName, enumName,  # set
        enumName, namesAsStr, valuesAsStr,  # init
    )


def scanCompound(code):
    cursor = code.find('(')
    assert cursor != -1
    tmp = code.find(')', cursor + 1)
    assert tmp != -1
    compoundName = code[cursor + 1:tmp]
    members = []
    for stuff in code[tmp + 1:].split('COMPOUND')[1:]:
        a = stuff.find('(')
        b = stuff.find(')')
        assert a != -1 and b != -1
        isArray = stuff[:a].endswith('_ARRAY')
        if not isArray:
            assert stuff[:a].endswith('_VALUE')
        args = stuff[a + 1:b].split(',')
        assert len(args) == 2
        members.append((args[0].strip(), args[1].strip(), isArray))
    return compoundName, members


def processCompound(compoundType, compoundTypeNames):
    name, attrs = compoundType

    code = ["""template<> MStatus initialize<%s>(MObject& dst, const char* name, std::vector<MObject>& children) {
    MStatus status;
    MFnCompoundAttribute fn;
    dst = fn.create(name, name, &status); 
    CHECK_MSTATUS_AND_RETURN_IT(status);
    size_t offset = children.size();
    for(int i = 0; i < %d; ++i) children.emplace_back();
""" % (name, len(attrs))]
    for index, (attrType, attrName, isArray) in enumerate(attrs):
        extra = ''
        if isArray:
            extra = """\n        status = MFnAttribute(attr).setArray(true); CHECK_MSTATUS_AND_RETURN_IT(status);"""
        isCompound = attrType in compoundTypeNames
        code.append("""
    {
        MObject& attr = children[offset + %d];
        MString childName = name;
        childName += "_%s";
        status = initialize<%s>(attr, childName.asChar()%s); CHECK_MSTATUS_AND_RETURN_IT(status);%s
        status = fn.addChild(children[offset + %d]); CHECK_MSTATUS_AND_RETURN_IT(status);
    }
""" % (index, attrName, attrType, ', children' if isCompound else '', extra, index))
    code.append("""
    return status;
}""")
    code.append("""
 template<> %s get<%s>(MDataHandle& element, const MObject* objects) {
    %s result;
    MDataHandle child;
""" % (name, name, name))
    offset = len(attrs)
    sizes = [compoundTypeNames.get(attr[0], 0) for attr in attrs]
    for index, (attrType, attrName, isArray) in enumerate(attrs):
        args = 'child'
        if isArray:
            args = 'handle'
        isCompound = attrType in compoundTypeNames
        if isCompound:
            args += ', objects + %d' % offset
        if isArray:
            code.append("""    child = element.child(objects[%d]); { MArrayDataHandle handle(child); result.%s = get<%s>(%s); }\n""" % (index, attrName, attrType, args))
        else:
            code.append("""    child = element.child(objects[%d]); result.%s = get<%s>(%s);\n""" % (index, attrName, attrType, args))
        offset += sizes[index]
    code.append("""    return result;
}""")
    code.append("""
template<> void set<%s>(MDataHandle& element, const MObject* objects, const %s& value) {
    MDataHandle child;
""" % (name, name))
    offset = len(attrs)
    sizes = [compoundTypeNames.get(attr[0], 0) for attr in attrs]
    for index, (attrType, attrName, isArray) in enumerate(attrs):
        args = 'child'
        if isArray:
            args = 'handle'
        isCompound = attrType in compoundTypeNames
        if isCompound:
            args += ', objects + %d' % offset
        if isArray:
            code.append("""    child = element.child(objects[%s]); { MArrayDataHandle handle(child); set<%s>(%s, value.%s); }\n    child.setClean();\n\n""" % (index, attrType, args, attrName))
        else:
            code.append("""    child = element.child(objects[%s]); set<%s>(%s, value.%s);\n    child.setClean();\n\n""" % (index, attrType, args, attrName))
        offset += sizes[index]
    code.append("""    element.setClean();\n}\n\n""")
    return ''.join(code)


def scanNode(code, compoundTypeNames):
    # get the node name from the "_BEGIN(nodeName)" structure
    cursor = code.find('(')
    assert cursor != -1
    tmp = code.find(')', cursor + 1)
    assert tmp != -1
    nodeName = code[cursor + 1:tmp]
    # for deformers and locators we must strip additional arguments inside the macro
    nodeName = nodeName.split(',')[0].strip()
    
    # find attributes defined in the node block
    nodeAttrs = []
    tokens = ('INPUT(', 'INOUT(', 'OUTPUT(', 'INPUT_ARRAY(', 'INOUT_ARRAY(', 'OUTPUT_ARRAY(')
    for ln in code.splitlines():
        ln = ln.strip()
        for token in tokens: 
            if ln.startswith(token):
                break
        else: 
            continue # There might be some more code inlined
        assert ln.endswith(')')
        ln = ln.rstrip(')')
        isArray = token.endswith('_ARRAY(')
        label, args = ln.split('(', 1)
        if isArray:
            label = label[:-6]
        isIn = label == 'INPUT'
        isOut = label == 'OUTPUT'
        if not isIn and not isOut:
            assert label == 'INOUT', label
        args = args.split(',')
        assert len(args) == 2
        args = tuple(arg.strip() for arg in args)
        isCompound = args[0] in compoundTypeNames
        nodeAttrs.append((isIn, isOut, isArray, isCompound, args[0], args[1]))
    return nodeName, nodeAttrs


def processNode(nodeName, nodeAttrs, isDeformer=False, isLocator=False):
    code = []
    inputNames = []
    outputNames = []
    inoutNames = []
    for isIn, isOut, isArray, isCompound, attrType, attrName in nodeAttrs:
        code.append('MObject %s::%sAttr;' % (nodeName, attrName))
        if isIn:
            inputNames.append(attrName)
        elif isOut:
            outputNames.append(attrName)
        else:
            inoutNames.append(attrName)
        if isCompound:
            code.append('std::vector<MObject> _%s_%s_children;' % (nodeName, attrName))

    if not isDeformer and not isLocator:
        if not inputNames:
            code.append("""
bool %s::isInputPlug(const MPlug& p) {
    return false;
}
""" % nodeName)
        else:
            code.append("""
bool %s::isInputPlug(const MPlug& p) {
    return (p == %sAttr);
}
""" % (nodeName, 'Attr || p == '.join(inputNames)))

    code.append('MStatus %s::initialize() {' % nodeName)
    code.append('    MStatus status;\n')
    for isIn, isOut, isArray, isCompound, attrType, attrName in nodeAttrs:
        args = ''
        if isCompound:
            args = ', _%s_%s_children' % (nodeName, attrName)
        code.append('    status = ::initialize<%s>(%sAttr, "%s"%s); CHECK_MSTATUS_AND_RETURN_IT(status);' % (
        attrType, attrName, attrName, args))
        if isArray:
            code.append('    status = MFnAttribute(%sAttr).setArray(true); CHECK_MSTATUS_AND_RETURN_IT(status);' % attrName)
        code.append('    status = addAttribute(%sAttr); CHECK_MSTATUS_AND_RETURN_IT(status);\n' % attrName)

    for isIn, isOut, isArray, isCompound, attrType, attrName in nodeAttrs:
        if isIn:
            continue
        # this is an output or inout attribute, make it affected by all inputs
        for isIn2, isOut2, isArray2, isCompound2, attrType2, attrName2 in nodeAttrs:
            if not isOut:
                # only inputs affect inouts
                if not isIn:
                    continue
            else:
                # inputs and inouts affect outputs
                if isOut2:
                    continue

            code.append('    status = attributeAffects(%sAttr, %sAttr); CHECK_MSTATUS_AND_RETURN_IT(status);' % (
            attrName2, attrName))

            if isCompound:
                # The input affects each of the output's chidlren
                code.append('    for(const MObject& obj : _%s_%s_children) {' % (nodeName, attrName))
                code.append(
                    '        status = attributeAffects(%sAttr, obj); CHECK_MSTATUS_AND_RETURN_IT(status);' % attrName2)
                # Each of the input's children affect each of the output's children
                if isCompound2:
                    code.append('        for(const MObject& obj2 : _%s_%s_children) {' % (nodeName, attrName2))
                    code.append('            status = attributeAffects(obj2, obj); CHECK_MSTATUS_AND_RETURN_IT(status);')
                    code.append('        }')
                code.append('    }')

            if isCompound2:
                # Each of the input's children affect each the output
                code.append('    for(const MObject& obj2 : _%s_%s_children) {' % (nodeName, attrName2))
                code.append('        status = attributeAffects(obj2, %sAttr); CHECK_MSTATUS_AND_RETURN_IT(status);' % attrName)
                # Each of the input's children affect each of the output's children'
                # if isCompound:
                #    code.append('    for(const MObject& obj : _%s_%s_children) {' % (nodeName, attrName))
                #    code.append('            status = attributeAffects(obj2, obj); CHECK_MSTATUS_AND_RETURN_IT(status);')
                #    code.append('        }')
                code.append('    }')

        code.append('')

    if isDeformer:
        code.append('    makePaintable("%s");' % nodeName)
    code.append('    return status;')
    code.append('}\n\n')

    for isIn, isOut, isArray, isCompound, attrType, attrName in nodeAttrs:
        compoundArgs = ''
        if isCompound:
            compoundArgs = ', _%s_%s_children' % (nodeName, attrName)
        if isArray:
            code.append('int %s::%sSize(Meta dataBlock) { return arraySize(dataBlock, %sAttr); }' % (nodeName, attrName, attrName))
            if not isOut:  # NOTE: We set "False, False" for INOUT so we must check the opposite, NOT OUT means IN
                code.append('%s %s::%s(Meta dataBlock, int index) { return getArray<%s>(dataBlock, %sAttr%s, index); }' % (attrType, nodeName, attrName, attrType, attrName, compoundArgs))
            if not isIn:
                code.append('void %s::%sSet(Meta dataBlock, const std::vector<%s>& value) { setArray<%s>(dataBlock, %sAttr%s, value); }' % (nodeName, attrName, attrType, attrType, attrName, compoundArgs))
        else:
            if not isOut:
                code.append('%s %s::%s(Meta dataBlock) { return getAttr<%s>(dataBlock, %sAttr%s); }' % (attrType, nodeName, attrName, attrType, attrName, compoundArgs))
            if not isIn:
                code.append('void %s::%sSet(Meta dataBlock, const %s& value) { setAttr<%s>(dataBlock, %sAttr%s, value); }' % (nodeName, attrName, attrType, attrType, attrName, compoundArgs))
        code.append('')

    if isLocator:
        # Generate MUserData
        code.append('\nclass %sUserData {' % nodeName)
        for isIn, isOut, isArray, isCompound, attrType, attrName in nodeAttrs:
            if not isOut:  # NOTE: We set "False, False" for INOUT so we must check the opposite, NOT OUT means IN
                code.append('\t%s %s;' % (attrType, attrName))
        code.append('}\n')
        # Generate copyInputs
        code.append('template<> MUserData* TMPxLocator<%s, %sUserData>::compute(Meta b, %sUserData& dst) {' % (nodeName, nodeName, nodeName))
        for isIn, isOut, isArray, isCompound, attrType, attrName in nodeAttrs:
            code.append('\tdst.%s = %s(b);')
    return '\n'.join(code)


def main():
    root = os.path.abspath('.')  # TODO: sys.argv?
    output = 'generated.inc'
    with open(output, 'w') as fh:
        registerPlugin = []
        enumNames = []
        compoundTypes = []
        codeBlocks = []
        for path in walk(root):
            if os.path.splitext(path)[1].lower() != '.h':
                continue
            if path.lower().replace('/', '\\').endswith('\\core.h'):
                continue
            fh.write('#include "%s"\n\n' % os.path.relpath(path, root))
            nodeCode, enumCode, compoundCode, deformerCode, typedDeformerCode, locatorCode, complexLocatorCode = scan(path)
            for code in enumCode:
                enumName, enumOptions = scanEnum(code)
                enumNames.append(enumName)
                fh.write(processEnum(enumName, enumOptions))
            for code in compoundCode:
                # compounds can reference other compounds, which we need before we can generate any code
                compoundTypes.append(scanCompound(code))
            codeBlocks.append((nodeCode, deformerCode, typedDeformerCode, locatorCode, complexLocatorCode))
        compoundTypeNames = {name: len(members) for (name, members) in compoundTypes}
        for compoundType in compoundTypes:
            fh.write(processCompound(compoundType, compoundTypeNames))
        for codeBlock in codeBlocks:
            for grpId, grp in enumerate(codeBlock):
                isDeformer = grpId in (1, 2)
                isLocator = grpId in (3, 4)
                for code in grp:
                    nodeName, nodeAttrs = scanNode(code, compoundTypeNames)
                    if isLocator:
                        registerPlugin.append('REGISTER_LOCATOR(%s)' % nodeName)
                    elif isDeformer:
                        registerPlugin.append('REGISTER_DEFORMER(%s)' % nodeName)
                    else:
                        registerPlugin.append('REGISTER_NODE(%s)' % nodeName)
                    fh.write(processNode(nodeName, nodeAttrs, isDeformer, isLocator))
        if registerPlugin:
            fh.write('\n#define INITIALIZE_PLUGIN %s' % ' '.join(registerPlugin))


if __name__ == '__main__':
    main()
