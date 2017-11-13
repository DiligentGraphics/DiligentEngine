using System.Collections;
using System.Collections.Generic;
using UnityEngine;

[ExecuteInEditMode]
public class ReflectionCamera : MonoBehaviour {

    public GameObject Mirror;

	// Use this for initialization
	void Start () {
		
	}
	
	// Update is called once per frame
	void Update () {
		
	}
    
    static Matrix4x4 PerspectiveOffCenter(float left, float right, float bottom, float top, float near, float far)
    {
        float x = 2.0F * near / (right - left);
        float y = 2.0F * near / (top - bottom);
        float a = (right + left) / (right - left);
        float b = (top + bottom) / (top - bottom);
        float c = -(far + near) / (far - near);
        float d = -(2.0F * far * near) / (far - near);
        float e = -1.0F;
        Matrix4x4 m = new Matrix4x4();
        m[0, 0] = x;
        m[0, 1] = 0;
        m[0, 2] = a;
        m[0, 3] = 0;

        m[1, 0] = 0;
        m[1, 1] = y;
        m[1, 2] = b;
        m[1, 3] = 0;

        m[2, 0] = 0;
        m[2, 1] = 0;
        m[2, 2] = c;
        m[2, 3] = d;

        m[3, 0] = 0;
        m[3, 1] = 0;
        m[3, 2] = e;
        m[3, 3] = 0;
        return m;
    }

    void LateUpdate() {
        Camera mainCamera = Camera.main;
        Camera camera = GetComponent<Camera>();
        Transform mirrorTransform = Mirror.transform;
        Vector3 reflectionPos = mirrorTransform.position;
        Vector3 reflectionDir = mirrorTransform.forward;
        Vector3 mainCameraPos = mainCamera.transform.position;
        Vector3 reflectedCameraPos = mainCameraPos - 2 * Vector3.Dot(reflectionDir, mainCameraPos - reflectionPos) * reflectionDir;
        transform.position = reflectedCameraPos;

        // https://en.wikibooks.org/wiki/Cg_Programming/Unity/Projection_for_Virtual_Reality
        // lower left corner in world coordinates
        Vector3 pa = Mirror.transform.TransformPoint( new Vector3(-0.5f, -0.5f, 0.0f) );
        // lower right corner
        Vector3 pb = Mirror.transform.TransformPoint( new Vector3( 0.5f, -0.5f, 0.0f) );
        // upper left corner
        Vector3 pc = Mirror.transform.TransformPoint( new Vector3(-0.5f,  0.5f, 0.0f) );

        // eye position
        Vector3 pe = reflectedCameraPos;

        // distance of near clipping plane
        float n = camera.nearClipPlane;
        // distance of far clipping plane
        float f = camera.farClipPlane;

        Vector3 va = pa - pe; // from pe to pa
        Vector3 vb = pb - pe; // from pe to pb
        Vector3 vc = pc - pe; // from pe to pc
        Vector3 vr = pb - pa; // right axis of screen
        Vector3 vu = pc - pa; // up axis of screen
        Vector3 vn; // normal vector of screen

        float l; // distance to left screen edge
        float r; // distance to right screen edge
        float b; // distance to bottom screen edge
        float t; // distance to top screen edge
        float d; // distance from eye to screen

        // are we looking at the backface of the plane object?
        if (Vector3.Dot(-Vector3.Cross(va, vc), vb) < 0.0)
        {
            // mirror points along the z axis (most users 
            // probably expect the x axis to stay fixed)
            vu = -vu;
            pa = pc;
            pb = pa + vr;
            pc = pa + vu;
            va = pa - pe;
            vb = pb - pe;
            vc = pc - pe;
        }

        vr.Normalize();
        vu.Normalize();
        vn = -Vector3.Cross(vr, vu);
        // we need the minus sign because Unity 
        // uses a left-handed coordinate system
        vn.Normalize();

        d = -Vector3.Dot(va, vn);
        //if (setNearClipPlane)
        //{
        //    n = d + nearClipDistanceOffset;
        //    cameraComponent.nearClipPlane = n;
        //}
        l = Vector3.Dot(vr, va) * n / d;
        r = Vector3.Dot(vr, vb) * n / d;
        b = Vector3.Dot(vu, va) * n / d;
        t = Vector3.Dot(vu, vc) * n / d;

        // Flip top and bottom 
        camera.projectionMatrix = PerspectiveOffCenter(l,r, b,t, n, f);

        Matrix4x4 rm = new Matrix4x4(); // rotation matrix;
        rm[0, 0] = vr.x;
        rm[0, 1] = vr.y;
        rm[0, 2] = vr.z;
        rm[0, 3] = 0.0f;

        rm[1, 0] = vu.x;
        rm[1, 1] = vu.y;
        rm[1, 2] = vu.z;
        rm[1, 3] = 0.0f;

        rm[2, 0] = vn.x;
        rm[2, 1] = vn.y;
        rm[2, 2] = vn.z;
        rm[2, 3] = 0.0f;

        rm[3, 0] = 0.0f;
        rm[3, 1] = 0.0f;
        rm[3, 2] = 0.0f;
        rm[3, 3] = 1.0f;

        Matrix4x4 tm = new Matrix4x4(); // translation matrix;
        tm[0, 0] = 1.0f;
        tm[0, 1] = 0.0f;
        tm[0, 2] = 0.0f;
        tm[0, 3] = -pe.x;

        tm[1, 0] = 0.0f;
        tm[1, 1] = 1.0f;
        tm[1, 2] = 0.0f;
        tm[1, 3] = -pe.y;

        tm[2, 0] = 0.0f;
        tm[2, 1] = 0.0f;
        tm[2, 2] = 1.0f;
        tm[2, 3] = -pe.z;

        tm[3, 0] = 0.0f;
        tm[3, 1] = 0.0f;
        tm[3, 2] = 0.0f;
        tm[3, 3] = 1.0f;

        // set matrices
        camera.worldToCameraMatrix = rm * tm;
        // The original paper puts everything into the projection 
        // matrix (i.e. sets it to p * rm * tm and the other 
        // matrix to the identity), but this doesn't appear to 
        // work with Unity's shadow maps.

        bool estimateViewFrustum = true;
        if (estimateViewFrustum)
        {
            // rotate camera to screen for culling to work
            Quaternion q = new Quaternion();
            q.SetLookRotation((0.5f * (pb + pc) - pe), vu);
            // look at center of screen
            camera.transform.rotation = q;

            // set fieldOfView to a conservative estimate 
            // to make frustum tall enough
            if (camera.aspect >= 1.0)
            {
                camera.fieldOfView = Mathf.Rad2Deg * Mathf.Atan(((pb - pa).magnitude + (pc - pa).magnitude) / va.magnitude);
            }
            else
            {
                // take the camera aspect into account to 
                // make the frustum wide enough 
                camera.fieldOfView = Mathf.Rad2Deg / camera.aspect * Mathf.Atan(((pb - pa).magnitude + (pc - pa).magnitude) / va.magnitude);
            }
        }
    }
}
