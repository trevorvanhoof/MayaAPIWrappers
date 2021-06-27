// We can't make getters that return MFn*, so intead we use symbolic wrappers around MObject and let the template specialization do it's thing
#include <maya/MFnNurbsCurve.h>
DECL_MFN_MOBJECT(NurbsCurve)
DECL_MFN_MOBJECT(Mesh)
DECL_MFN_MOBJECT(NurbsSurface)
DECL_MFN_MOBJECT(Lattice)
DECL_MFN_MOBJECT(Sphere)
DECL_MFN_MOBJECT(ComponentList)
DECL_MFN_MOBJECT(DynArrayAttrs)
DECL_MFN_MOBJECT(DynSweptGeometry)
DECL_MFN_MOBJECT(SubdSurface)
DECL_MFN_MOBJECT(NObject)
DECL_MFN_MOBJECT(NId)

// For clarity we add aliases for specific attribute types:
// Message return values of type MPlug
typedef MPlug Message;
// Addr is just a void* but not named as such in documentation
typedef void* Addr;