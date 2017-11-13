using UnityEngine;
using System;
using System.Collections;
using System.Runtime.InteropServices;

[ExecuteInEditMode]
public class UseRenderingPlugin : MonoBehaviour
{
    // Native plugin rendering events are only called if a plugin is used
    // by some script. This means we have to DllImport at least
    // one function in some active script.
    public RenderTexture ReflectionTexture;
    public Camera ReflectionCamera;

#if (UNITY_IPHONE || UNITY_WEBGL) && !UNITY_EDITOR
    [DllImport ("__Internal")]
#else
    [DllImport("GhostCubePlugin")]
#endif
    private static extern void SetMatrixFromUnity(
        float m00, float m01, float m02, float m03,
        float m10, float m11, float m12, float m13,
        float m20, float m21, float m22, float m23,
        float m30, float m31, float m32, float m33);

#if (UNITY_IPHONE || UNITY_WEBGL) && !UNITY_EDITOR
    [DllImport ("__Internal")]
#else
    [DllImport("GhostCubePlugin")]
#endif
    private static extern void SetTexturesFromUnity(System.IntPtr renderTexture, System.IntPtr depthTexture);

#if (UNITY_IPHONE || UNITY_WEBGL) && !UNITY_EDITOR
    [DllImport ("__Internal")]
#else
    [DllImport("GhostCubePlugin")]
#endif
    private static extern IntPtr GetRenderEventFunc();

#if UNITY_WEBGL && !UNITY_EDITOR
    [DllImport ("__Internal")]
    private static extern void RegisterPlugin();
#endif

    void Start()
    {
#if UNITY_WEBGL && !UNITY_EDITOR
        RegisterPlugin();
#endif
        // Unity may create new native textures for the render texture object
        // The new textures may get the same handles, so we need to 
        // force the plugin to recreate render and depth buffers
        SetTexturesFromUnity(System.IntPtr.Zero, System.IntPtr.Zero);
        if (SystemInfo.graphicsDeviceType == UnityEngine.Rendering.GraphicsDeviceType.Direct3D12)
            Debug.Log("Disregard Unity warnings about d3d12 resource leakage. Diligent Engine keeps references to the render target and depth buffer, but releases them after Unity when plugin event is issued. This makes Unity assertions fail, but there are no leaks");
    }

    void OnRenderObject()
    {
        if (Camera.current.name == "ReflectionCamera")
        {
            Camera camera = ReflectionCamera;
            Matrix4x4 localToWorld = transform.localToWorldMatrix;
            Matrix4x4 view = camera.worldToCameraMatrix;
            Matrix4x4 proj = camera.projectionMatrix;
            proj = GL.GetGPUProjectionMatrix(proj, false);
            Matrix4x4 wvp = localToWorld.transpose * view.transpose * proj.transpose;
            float InverseY =
                (SystemInfo.graphicsDeviceType == UnityEngine.Rendering.GraphicsDeviceType.Direct3D11 ||
                 SystemInfo.graphicsDeviceType == UnityEngine.Rendering.GraphicsDeviceType.Direct3D12) ? -1 : +1;

            SetMatrixFromUnity(wvp.m00, InverseY * wvp.m01, wvp.m02, wvp.m03,
                               wvp.m10, InverseY * wvp.m11, wvp.m12, wvp.m13,
                               wvp.m20, InverseY * wvp.m21, wvp.m22, wvp.m23,
                               wvp.m30, InverseY * wvp.m31, wvp.m32, wvp.m33);

            IntPtr nativeRTHandle = ReflectionTexture.GetNativeTexturePtr();
            IntPtr nativeDepthHandle = ReflectionTexture.GetNativeDepthBufferPtr();
            SetTexturesFromUnity(nativeRTHandle, nativeDepthHandle);
            // Issue a plugin event with arbitrary integer identifier.
            // The plugin can distinguish between different
            // things it needs to do based on this ID.
            // For our simple plugin, it does not matter which ID we pass here.
            GL.IssuePluginEvent(GetRenderEventFunc(), 1);
        }
    }

    public void Update()
    {
        transform.localRotation = Quaternion.AngleAxis(Time.timeSinceLevelLoad*100, new Vector3(0, 1, 0));
    }
}
