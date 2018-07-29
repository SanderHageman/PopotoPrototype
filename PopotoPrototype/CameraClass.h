#pragma once

using namespace Math;

class CameraClass
{
public:
	CameraClass(float vFovRad, float aspectRatio, float nearClip, float farClip);
	void Update();

	// Functions for controlling the camera
	void SetPositionAndTarget(Vector3 pos, Vector3 target, Vector3 up);
	void SetPosition(Vector3 pos);
	void SetDirection(Vector3 forward, Vector3 up);
	void SetRotation(Quaternion basisRotation);
	void SetTransform(const AffineTransform& xform);

	// Accessors for reading the matrices and vectors
	const Matrix4& GetViewMatrix() const { return m_viewMat; }
	const Matrix4& GetProjMatrix() const { return m_projMat; }
	const Matrix4& GetViewProjMatrix() const { return m_viewProjMat; }

	const Vector3 GetPosition() const { return m_CameraToWorld.GetTranslation(); }
	const Vector3 GetRight() const { return m_Basis.GetX(); }
	const Vector3 GetUp() const { return m_Basis.GetY(); }
	const Vector3 GetForward() const { return -m_Basis.GetZ(); }

	// Delete functions
	CameraClass(CameraClass const& rhs) = delete;
	CameraClass& operator=(CameraClass const& rhs) = delete;

	CameraClass(CameraClass&& rhs) = delete;
	CameraClass& operator=(CameraClass&& rhs) = delete;
private:

	void UpdateProjectionMatrix();

	OrthogonalTransform m_CameraToWorld;
	Matrix3 m_Basis;
	Matrix4 m_viewMat;
	Matrix4 m_projMat;
	Matrix4 m_viewProjMat;

	float m_vFov;
	float m_aspectRatio;
	float m_nearClip;
	float m_farClip;
};

