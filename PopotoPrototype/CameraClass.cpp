#include "stdafx.h"
#include "CameraClass.h"

CameraClass::CameraClass(float vFovRad, float aspectRatio, float nearClip, float farClip ):
	m_vFov(vFovRad), m_aspectRatio(aspectRatio), m_nearClip(nearClip), m_farClip(farClip),
	m_CameraToWorld(kIdentity), m_Basis(kIdentity) {

	// Set camerastate
	//SetPositionAndTarget(Vector3(0.0f), Vector3(kZUnitVector), Vector3(kYUnitVector));
	SetPositionAndTarget(Vector3{ -0.283602f, -0.198372f, -0.245529f }, Vector3(kXUnitVector), Vector3(kYUnitVector));
	UpdateProjectionMatrix();
	Update();
}

void CameraClass::Update() {
	m_viewMat = Matrix4(~m_CameraToWorld);
	m_viewProjMat = m_projMat * m_viewMat;
}

void CameraClass::SetPositionAndTarget(Vector3 pos, Vector3 target, Vector3 up) {
	SetDirection(target - pos, up);
	SetPosition(pos);
}

void CameraClass::SetPosition(Vector3 pos) {
	m_CameraToWorld.SetTranslation(pos);
}

void CameraClass::SetDirection(Vector3 forward, Vector3 up) {
	// Ensure normalization
	Scalar forwardLenSq = LengthSquare(forward);
	forward = Select(forward * RecipSqrt(forwardLenSq), -Vector3(kZUnitVector), forwardLenSq < Scalar(0.000001f));

	// Create right vector
	Vector3 right = Cross(forward, up);
	Scalar rightLenSq = LengthSquare(right);
	right = Select(right * RecipSqrt(rightLenSq), Quaternion(Vector3(kYUnitVector), -XM_PIDIV2) * forward, rightLenSq < Scalar(0.000001f));

	// Create up vector
	up = Cross(right, forward);

	// Construct Basis
	m_Basis = Matrix3(right, up, -forward);
	m_CameraToWorld.SetRotation(Quaternion(m_Basis));
}

void CameraClass::SetRotation(Quaternion basisRotation) {
	m_CameraToWorld.SetRotation(Normalize(basisRotation));
	m_Basis = Matrix3(m_CameraToWorld.GetRotation());
}

void CameraClass::SetTransform(const AffineTransform& xform) {
	SetDirection(-xform.GetZ(), xform.GetY());
	SetPosition(xform.GetTranslation());
}

void CameraClass::UpdateProjectionMatrix() {
	m_projMat = Matrix4{ XMMatrixPerspectiveFovRH(m_vFov, m_aspectRatio, m_nearClip, m_farClip) };
}