import pybullet as p
import time
import math
import pybullet_data


def getRayFromTo(mouseX, mouseY):
  width, height, viewMat, projMat, cameraUp, camForward, horizon, vertical, _, _, dist, camTarget = p.getDebugVisualizerCamera(
  )
  camPos = [
      camTarget[0] - dist * camForward[0], camTarget[1] - dist * camForward[1],
      camTarget[2] - dist * camForward[2]
  ]
  farPlane = 10000
  rayForward = [(camTarget[0] - camPos[0]), (camTarget[1] - camPos[1]), (camTarget[2] - camPos[2])]
  invLen = farPlane * 1. / (math.sqrt(rayForward[0] * rayForward[0] + rayForward[1] *
                                      rayForward[1] + rayForward[2] * rayForward[2]))
  rayForward = [invLen * rayForward[0], invLen * rayForward[1], invLen * rayForward[2]]
  rayFrom = camPos
  oneOverWidth = float(1) / float(width)
  oneOverHeight = float(1) / float(height)
  dHor = [horizon[0] * oneOverWidth, horizon[1] * oneOverWidth, horizon[2] * oneOverWidth]
  dVer = [vertical[0] * oneOverHeight, vertical[1] * oneOverHeight, vertical[2] * oneOverHeight]
  rayToCenter = [
      rayFrom[0] + rayForward[0], rayFrom[1] + rayForward[1], rayFrom[2] + rayForward[2]
  ]
  rayTo = [
      rayFrom[0] + rayForward[0] - 0.5 * horizon[0] + 0.5 * vertical[0] + float(mouseX) * dHor[0] -
      float(mouseY) * dVer[0], rayFrom[1] + rayForward[1] - 0.5 * horizon[1] + 0.5 * vertical[1] +
      float(mouseX) * dHor[1] - float(mouseY) * dVer[1], rayFrom[2] + rayForward[2] -
      0.5 * horizon[2] + 0.5 * vertical[2] + float(mouseX) * dHor[2] - float(mouseY) * dVer[2]
  ]
  return rayFrom, rayTo


cid = p.connect(p.SHARED_MEMORY)
if (cid < 0):
  p.connect(p.GUI)
p.setAdditionalSearchPath(pybullet_data.getDataPath())  
p.setPhysicsEngineParameter(numSolverIterations=10)
p.setTimeStep(1. / 120.)
logId = p.startStateLogging(p.STATE_LOGGING_PROFILE_TIMINGS, "visualShapeBench.json")
#useMaximalCoordinates is much faster then the default reduced coordinates (Featherstone)
p.loadURDF("plane100.urdf", useMaximalCoordinates=True)
#disable rendering during creation.
p.configureDebugVisualizer(p.COV_ENABLE_RENDERING, 0)
p.configureDebugVisualizer(p.COV_ENABLE_GUI, 0)
#disable tinyrenderer, software (CPU) renderer, we don't use it here
p.configureDebugVisualizer(p.COV_ENABLE_TINY_RENDERER, 0)

shift = [0, -0.02, 0]
meshScale = [0.1, 0.1, 0.1]

vertices = [[-1.000000, -1.000000, 1.000000], [1.000000, -1.000000, 1.000000],
            [1.000000, 1.000000, 1.000000], [-1.000000, 1.000000, 1.000000],
            [-1.000000, -1.000000, -1.000000], [1.000000, -1.000000, -1.000000],
            [1.000000, 1.000000, -1.000000], [-1.000000, 1.000000, -1.000000],
            [-1.000000, -1.000000, -1.000000], [-1.000000, 1.000000, -1.000000],
            [-1.000000, 1.000000, 1.000000], [-1.000000, -1.000000, 1.000000],
            [1.000000, -1.000000, -1.000000], [1.000000, 1.000000, -1.000000],
            [1.000000, 1.000000, 1.000000], [1.000000, -1.000000, 1.000000],
            [-1.000000, -1.000000, -1.000000], [-1.000000, -1.000000, 1.000000],
            [1.000000, -1.000000, 1.000000], [1.000000, -1.000000, -1.000000],
            [-1.000000, 1.000000, -1.000000], [-1.000000, 1.000000, 1.000000],
            [1.000000, 1.000000, 1.000000], [1.000000, 1.000000, -1.000000]]

normals = [[0.000000, 0.000000, 1.000000], [0.000000, 0.000000, 1.000000],
           [0.000000, 0.000000, 1.000000], [0.000000, 0.000000, 1.000000],
           [0.000000, 0.000000, -1.000000], [0.000000, 0.000000, -1.000000],
           [0.000000, 0.000000, -1.000000], [0.000000, 0.000000, -1.000000],
           [-1.000000, 0.000000, 0.000000], [-1.000000, 0.000000, 0.000000],
           [-1.000000, 0.000000, 0.000000], [-1.000000, 0.000000, 0.000000],
           [1.000000, 0.000000, 0.000000], [1.000000, 0.000000, 0.000000],
           [1.000000, 0.000000, 0.000000], [1.000000, 0.000000, 0.000000],
           [0.000000, -1.000000, 0.000000], [0.000000, -1.000000, 0.000000],
           [0.000000, -1.000000, 0.000000], [0.000000, -1.000000, 0.000000],
           [0.000000, 1.000000, 0.000000], [0.000000, 1.000000, 0.000000],
           [0.000000, 1.000000, 0.000000], [0.000000, 1.000000, 0.000000]]

uvs = [[0.750000, 0.250000], [1.000000, 0.250000], [1.000000, 0.000000], [0.750000, 0.000000],
       [0.500000, 0.250000], [0.250000, 0.250000], [0.250000, 0.000000], [0.500000, 0.000000],
       [0.500000, 0.000000], [0.750000, 0.000000], [0.750000, 0.250000], [0.500000, 0.250000],
       [0.250000, 0.500000], [0.250000, 0.250000], [0.000000, 0.250000], [0.000000, 0.500000],
       [0.250000, 0.500000], [0.250000, 0.250000], [0.500000, 0.250000], [0.500000, 0.500000],
       [0.000000, 0.000000], [0.000000, 0.250000], [0.250000, 0.250000], [0.250000, 0.000000]]
indices = [
    0,
    1,
    2,
    0,
    2,
    3,  #//ground face
    6,
    5,
    4,
    7,
    6,
    4,  #//top face
    10,
    9,
    8,
    11,
    10,
    8,
    12,
    13,
    14,
    12,
    14,
    15,
    18,
    17,
    16,
    19,
    18,
    16,
    20,
    21,
    22,
    20,
    22,
    23
]

#the visual shape and collision shape can be re-used by all createMultiBody instances (instancing)
visualShapeId = p.createVisualShape(shapeType=p.GEOM_MESH,
                                    rgbaColor=[1, 1, 1, 1],
                                    specularColor=[0.4, .4, 0],
                                    visualFramePosition=shift,
                                    meshScale=meshScale,
                                    vertices=vertices,
                                    indices=indices,
                                    uvs=uvs,
                                    normals=normals)
#visualShapeId = p.createVisualShape(shapeType=p.GEOM_BOX,rgbaColor=[1,1,1,1], halfExtents=[0.5,0.5,0.5],specularColor=[0.4,.4,0], visualFramePosition=shift, meshScale=[1,1,1], vertices=vertices, indices=indices)

#visualShapeId = p.createVisualShape(shapeType=p.GEOM_MESH,rgbaColor=[1,1,1,1], specularColor=[0.4,.4,0], visualFramePosition=shift, meshScale=meshScale, fileName="duck.obj")
collisionShapeId = p.createCollisionShape(shapeType=p.GEOM_MESH,
                                          vertices=vertices,
                                          collisionFramePosition=shift,
                                          meshScale=meshScale)

texUid = p.loadTexture("tex256.png")

rangex = 1
rangey = 1
for i in range(rangex):
  for j in range(rangey):
    bodyUid = p.createMultiBody(baseMass=1,
                                baseInertialFramePosition=[0, 0, 0],
                                baseCollisionShapeIndex=collisionShapeId,
                                baseVisualShapeIndex=visualShapeId,
                                basePosition=[((-rangex / 2) + i) * meshScale[0] * 2,
                                              (-rangey / 2 + j) * meshScale[1] * 2, 1],
                                useMaximalCoordinates=True)
    p.changeVisualShape(bodyUid, -1, textureUniqueId=texUid)
p.configureDebugVisualizer(p.COV_ENABLE_RENDERING, 1)
p.stopStateLogging(logId)
p.setGravity(0, 0, -10)
p.setRealTimeSimulation(1)

colors = [[1, 0, 0, 1], [0, 1, 0, 1], [0, 0, 1, 1], [1, 1, 1, 1]]
currentColor = 0

while (1):
  p.getCameraImage(320, 200)
  mouseEvents = p.getMouseEvents()
  for e in mouseEvents:
    if ((e[0] == 2) and (e[3] == 0) and (e[4] & p.KEY_WAS_TRIGGERED)):
      mouseX = e[1]
      mouseY = e[2]
      rayFrom, rayTo = getRayFromTo(mouseX, mouseY)
      rayInfo = p.rayTest(rayFrom, rayTo)
      #p.addUserDebugLine(rayFrom,rayTo,[1,0,0],3)
      for l in range(len(rayInfo)):
        hit = rayInfo[l]
        objectUid = hit[0]
        if (objectUid >= 1):
          #p.removeBody(objectUid)
          p.changeVisualShape(objectUid, -1, rgbaColor=colors[currentColor])
          currentColor += 1
          if (currentColor >= len(colors)):
            currentColor = 0
