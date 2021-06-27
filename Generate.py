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
    startTokens = ('NODE_BEGIN(', 'ENUM_BEGIN(', 'COMPOUND_BEGIN(')
    endTokens = ('NODE_END', 'ENUM_END', 'COMPOUND_END')
    output = ([], [], [])
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


def processCompound(compoundType):
    name, attrs = compoundType

    code = ["""template<> MStatus initialize<%s>(MObject& dst, const char* name, std::vector<MObject>& children) {
    MStatus status;
    MFnCompoundAttribute fn;
    dst = fn.create(name, name, &status); 
    CHECK_MSTATUS_AND_RETURN_IT(status);
""" % name]
    for attrType, attrName, isArray in attrs:
        extra = ''
        if isArray:
            extra = """\n        status = MFnAttribute(attr).setArray(true); CHECK_MSTATUS_AND_RETURN_IT(status);"""
        code.append("""
    children.emplace_back();
    {
        MObject& attr = children[children.size() - 1];
        status = initialize<%s>(attr, "%s"); CHECK_MSTATUS_AND_RETURN_IT(status);%s
        status = fn.addChild(attr); CHECK_MSTATUS_AND_RETURN_IT(status);
    }
""" % (attrType, attrName, extra))
    code.append("""
    return status;
}""")
    code.append("""
 template<> %s get<%s>(MDataHandle& element, const std::vector<MObject>& objects) {
    %s result;
    MDataHandle child;
""" % (name, name, name))
    for index, (attrType, attrName, isArray) in enumerate(attrs):
        extra = 'child'
        if isArray:
            extra = 'MArrayDataHandle(child)'
        code.append("""    child = element.child(objects[%d]); result.%s = get<%s>(%s);\n""" % (index, attrName, attrType, extra))
    code.append("""    return result;
}""")
    code.append("""
template<> void set<%s>(MDataHandle& element, const std::vector<MObject>& objects, const %s& value) {
    MDataHandle child;
""" % (name, name))
    for index, (attrType, attrName, isArray) in enumerate(attrs):
        extra = 'child'
        if isArray:
            extra = 'MArrayDataHandle(child)'
        code.append("""    child = element.child(objects[%s]); set<%s>(%s, value.%s);\n    child.setClean();\n\n""" % (index, attrType, extra, attrName))
    code.append("""    element.setClean();\n}\n\n""")
    return ''.join(code)


def scanNode(code, compoundTypeNames):
    cursor = code.find('(')
    assert cursor != -1
    tmp = code.find(')', cursor + 1)
    assert tmp != -1
    nodeName = code[cursor + 1:tmp]
    nodeAttrs = []
    tokens = ('INPUT(', 'INOUT(', 'OUTPUT(', 'INPUT_ARRAY(', 'INOUT_ARRAY(', 'OUTPUT_ARRAY(')
    cursor, tokenId = findFirstOf(code, tokens, tmp + 1)
    for stuff in code[cursor:].split(')')[:-1]:
        if not stuff.strip(): continue
        label, args = stuff.split('(', 1)
        isArray = label.endswith('_ARRAY')
        if isArray:
            label = label[:-6]
        isIn = label.endswith('INPUT')
        isOut = label.endswith('OUTPUT')
        if not isIn and not isOut:
            assert label.endswith('INOUT'), label
        args = args.split(',')
        assert len(args) == 2
        args = tuple(arg.strip() for arg in args)
        isCompound = args[0] in compoundTypeNames
        nodeAttrs.append((isIn, isOut, isArray, isCompound, args[0], args[1]))
    return nodeName, nodeAttrs


def processNode(nodeName, nodeAttrs):
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
            code.append(
                '    status = MFnAttribute(%sAttr).setArray(true); CHECK_MSTATUS_AND_RETURN_IT(status);' % attrName)
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
                    code.append(
                        '            status = attributeAffects(obj2, obj); CHECK_MSTATUS_AND_RETURN_IT(status);')
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

    code.append('    return status;')
    code.append('}\n\n')

    for isIn, isOut, isArray, isCompound, attrType, attrName in nodeAttrs:
        compoundArgs = ''
        if isCompound:
            compoundArgs = ', _%s_%s_children' % (nodeName, attrName)
        if isArray:
            code.append('int %s::%sSize(Meta dataBlock) { return arraySize(dataBlock, %sAttr); }' % (nodeName, attrName, attrName))
            if not isOut:
                code.append('%s %s::%s(Meta dataBlock, int index) { return getArray<%s>(dataBlock, %sAttr%s, index); }' % (attrType, nodeName, attrName, attrType, attrName, compoundArgs))
            if not isIn:
                code.append('void %s::%sSet(Meta dataBlock, const std::vector<%s>& value) { setArray<%s>(dataBlock, %sAttr%s, value); }' % (nodeName, attrName, attrType, attrType, attrName, compoundArgs))
        else:
            if not isOut:
                code.append('%s %s::%s(Meta dataBlock) { return getAttr<%s>(dataBlock, %sAttr%s); }' % (attrType, nodeName, attrName, attrType, attrName, compoundArgs))
            if not isIn:
                code.append('void %s::%sSet(Meta dataBlock, const %s& value) { setAttr<%s>(dataBlock, %sAttr%s, value); }' % (nodeName, attrName, attrType, attrType, attrName, compoundArgs))
        code.append('')
    return '\n'.join(code)


def main():
    root = os.path.abspath('.')  # TODO: sys.argv?
    output = 'generated.inc'
    with open(output, 'w') as fh:
        for path in walk(root):
            if os.path.splitext(path)[1].lower() != '.h':
                continue
            if path.lower().replace('/', '\\').endswith('\\core.h'):
                continue
            fh.write('#include "%s"\n\n' % os.path.relpath(path, root))
            nodeCode, enumCode, compoundCode = scan(path)
            enumNames = []
            compoundTypes = []
            for code in enumCode:
                enumName, enumOptions = scanEnum(code)
                enumNames.append(enumName)
                fh.write(processEnum(enumName, enumOptions))
            for code in compoundCode:
                # compounds can reference other compounds, which we need before we can generate any code
                compoundTypes.append(scanCompound(code))
            for compoundType in compoundTypes:
                fh.write(processCompound(compoundType))
            nodeNames = []
            for code in nodeCode:
                nodeName, nodeAttrs = scanNode(code, set(name for (name, _) in compoundTypes))
                nodeNames.append(nodeName)
                fh.write(processNode(nodeName, nodeAttrs))
            if nodeNames:
                fh.write('\n#define INITIALIZE_PLUGIN REGISTER_NODE(%s)' % ') REGISTER_NODE('.join(nodeNames))
            else:
                fh.write('\n#define INITIALIZE_PLUGIN')


if __name__ == '__main__':
    main()
