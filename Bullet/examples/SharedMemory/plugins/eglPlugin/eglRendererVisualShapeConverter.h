#ifndef EGL_RENDERER_VISUAL_SHAPE_CONVERTER_H
#define EGL_RENDERER_VISUAL_SHAPE_CONVERTER_H

#include "../../../Importers/ImportURDFDemo/UrdfRenderingInterface.h"

struct EGLRendererVisualShapeConverter : public UrdfRenderingInterface
{
	struct EGLRendererVisualShapeConverterInternalData* m_data;

	EGLRendererVisualShapeConverter();

	virtual ~EGLRendererVisualShapeConverter();

	virtual int convertVisualShapes(int linkIndex, const char* pathPrefix, const btTransform& localInertiaFrame, const UrdfLink* linkPtr, const UrdfModel* model, int orgGraphicsUniqueId, int bodyUniqueId, struct CommonFileIOInterface* fileIO);

	virtual int getNumVisualShapes(int bodyUniqueId);

	virtual int getVisualShapesData(int bodyUniqueId, int shapeIndex, struct b3VisualShapeData* shapeData);

	virtual int registerShapeAndInstance(const b3VisualShapeData& visualShape, const float* vertices, int numvertices, const int* indices, int numIndices, int primitiveType, int textureId, int orgGraphicsUniqueId, int bodyUniqueId, int linkIndex);

	virtual void updateShape(int shapeUniqueId, const btVector3* vertices, int numVertices, const btVector3* normals, int numNormals);

	virtual void changeRGBAColor(int bodyUniqueId, int linkIndex, int shapeIndex, const double rgbaColor[4]);

	virtual void changeInstanceFlags(int bodyUniqueId, int linkIndex, int shapeIndex, int flags);

	virtual void changeShapeTexture(int objectUniqueId, int linkIndex, int shapeIndex, int textureUniqueId);

	virtual void removeVisualShape(int shapeUid);

	virtual void setUpAxis(int axis);

	virtual void resetCamera(float camDist, float yaw, float pitch, float camPosX, float camPosY, float camPosZ);

	virtual void clearBuffers(struct TGAColor& clearColor);

	virtual void resetAll();

	virtual void getWidthAndHeight(int& width, int& height);
	virtual void setWidthAndHeight(int width, int height);
	virtual void setLightDirection(float x, float y, float z);
	virtual void setLightColor(float x, float y, float z);
	virtual void setLightDistance(float dist);
	virtual void setLightAmbientCoeff(float ambientCoeff);
	virtual void setLightDiffuseCoeff(float diffuseCoeff);
	virtual void setLightSpecularCoeff(float specularCoeff);
	virtual void setShadow(bool hasShadow);
	virtual void setFlags(int flags);

	virtual void copyCameraImageData(unsigned char* pixelsRGBA, int rgbaBufferSizeInPixels, float* depthBuffer, int depthBufferSizeInPixels, int* segmentationMaskBuffer, int segmentationMaskSizeInPixels, int startPixelIndex, int* widthPtr, int* heightPtr, int* numPixelsCopied);
	void copyCameraImageDataGL(unsigned char* pixelsRGBA, int rgbaBufferSizeInPixels, float* depthBuffer, int depthBufferSizeInPixels, int* segmentationMaskBuffer, int segmentationMaskSizeInPixels, int startPixelIndex, int* widthPtr, int* heightPtr, int* numPixelsCopied);

	virtual void render();
	virtual void render(const float viewMat[16], const float projMat[16]);

	virtual int loadTextureFile(const char* filename, struct CommonFileIOInterface* fileIO);
	virtual int registerTexture(unsigned char* texels, int width, int height);
	virtual int registerTextureInternal(unsigned char* texels, int width, int height);
	

	virtual void setProjectiveTextureMatrices(const float viewMatrix[16], const float projectionMatrix[16]);
	virtual void setProjectiveTexture(bool useProjectiveTexture);

	virtual void syncTransform(int shapeUid, const class btTransform& worldTransform, const class btVector3& localScaling);

	virtual void mouseMoveCallback(float x, float y);
	virtual void mouseButtonCallback(int button, int state, float x, float y);

	virtual bool getCameraInfo(int* width, int* height, float viewMatrix[16], float projectionMatrix[16], float camUp[3], float camForward[3], float hor[3], float vert[3], float* yaw, float* pitch, float* camDist, float cameraTarget[3]) const;
	

};

#endif  //EGL_RENDERER_VISUAL_SHAPE_CONVERTER_H
