// We can't make getters that return MFn*, so intead we use symbolic wrappers around MObject and let the template specialization do it's thing
#include <maya/MFnNurbsCurve.h>
class NurbsCurve : public MObject {};
