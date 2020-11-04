
#ifndef IN_PROCESS_PHYSICS_C_API_H
#define IN_PROCESS_PHYSICS_C_API_H

#include "PhysicsClientC_API.h"

#ifdef __cplusplus
extern "C"
{
#endif

	///think more about naming. The b3ConnectPhysicsLoopback
	B3_SHARED_API b3PhysicsClientHandle b3CreateInProcessPhysicsServerAndConnect(int argc, char* argv[]);
	B3_SHARED_API b3PhysicsClientHandle b3CreateInProcessPhysicsServerAndConnectMainThread(int argc, char* argv[]);

	B3_SHARED_API b3PhysicsClientHandle b3CreateInProcessPhysicsServerAndConnectSharedMemory(int argc, char* argv[]);
	B3_SHARED_API b3PhysicsClientHandle b3CreateInProcessPhysicsServerAndConnectMainThreadSharedMemory(int argc, char* argv[]);

	B3_SHARED_API b3PhysicsClientHandle b3CreateInProcessGraphicsServerAndConnectSharedMemory(int port);
	B3_SHARED_API b3PhysicsClientHandle b3CreateInProcessGraphicsServerAndConnectMainThreadSharedMemory(int port);
	

	B3_SHARED_API b3PhysicsClientHandle b3CreateInProcessPhysicsServerFromExistingExampleBrowserAndConnect(void* guiHelperPtr);
	//create a shared memory physics server, with a DummyGUIHelper (no graphics)
	B3_SHARED_API b3PhysicsClientHandle b3CreateInProcessPhysicsServerFromExistingExampleBrowserAndConnect2(void* guiHelperPtr);
	//create a shared memory physics server, with a DummyGUIHelper (no graphics) and allow to set shared memory key
	B3_SHARED_API b3PhysicsClientHandle b3CreateInProcessPhysicsServerFromExistingExampleBrowserAndConnect3(void* guiHelperPtr, int sharedMemoryKey);
	//create a shared memory physics server, with a RemoteGUIHelper (connect to remote graphics server) and allow to set shared memory key
	B3_SHARED_API b3PhysicsClientHandle b3CreateInProcessPhysicsServerFromExistingExampleBrowserAndConnect4(void* guiHelperPtr, int sharedMemoryKey);
#ifdef BT_ENABLE_CLSOCKET
	B3_SHARED_API b3PhysicsClientHandle b3CreateInProcessPhysicsServerFromExistingExampleBrowserAndConnectTCP(const char* hostName, int port);
#endif	
	///ignore the following APIs, they are for internal use for example browser
	void b3InProcessRenderSceneInternal(b3PhysicsClientHandle clientHandle);
	void b3InProcessDebugDrawInternal(b3PhysicsClientHandle clientHandle, int debugDrawMode);
	int b3InProcessMouseMoveCallback(b3PhysicsClientHandle clientHandle, float x, float y);
	int b3InProcessMouseButtonCallback(b3PhysicsClientHandle clientHandle, int button, int state, float x, float y);

#ifdef __cplusplus
}
#endif

#endif  //IN_PROCESS_PHYSICS_C_API_H
