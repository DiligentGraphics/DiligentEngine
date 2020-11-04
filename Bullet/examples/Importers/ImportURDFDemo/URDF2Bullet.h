#ifndef _URDF2BULLET_H
#define _URDF2BULLET_H
#include "LinearMath/btAlignedObjectArray.h"
#include "LinearMath/btTransform.h"
#include <string>
#include "URDFJointTypes.h"  //for UrdfMaterialColor cache

class btVector3;
class btTransform;
class btMultiBodyDynamicsWorld;
class btTransform;

class URDFImporterInterface;
class MultiBodyCreationInterface;



struct UrdfVisualShapeCache
{
	btAlignedObjectArray<UrdfMaterialColor> m_cachedUrdfLinkColors;
	btAlignedObjectArray<int> m_cachedUrdfLinkVisualShapeIndices;
};

void ConvertURDF2Bullet(const URDFImporterInterface& u2b,
						MultiBodyCreationInterface& creationCallback,
						const btTransform& rootTransformInWorldSpace,
						btMultiBodyDynamicsWorld* world,
						bool createMultiBody,
						const char* pathPrefix,
						int flags = 0,
						UrdfVisualShapeCache* cachedLinkGraphicsShapes = 0);

#endif  //_URDF2BULLET_H
