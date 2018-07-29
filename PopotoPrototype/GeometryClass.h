#pragma once
class GeometryClass {
public:
	struct Vertex {
		Vertex() {}
		Vertex(
			const Math::Vector3& pos,
			const Math::Vector3& normal,
			const Math::Vector3& tangent,
			const DirectX::XMFLOAT2& uv) : 
			m_position(pos),
			m_normal(normal),
			m_tangent(tangent),
			m_uv(uv){}
		Math::Vector3 m_position;
		Math::Vector3 m_normal;
		Math::Vector3 m_tangent;
		DirectX::XMFLOAT2 m_uv;
	};

	struct Mesh {
		std::vector<Vertex> m_vertices;
		std::vector<UINT32> m_indices;

		void Translate(const Math::Vector3 &trns) {
			for (auto& vert : m_vertices) {
				vert.m_position += trns;
			}
		}
	};

	// Delete functions
	GeometryClass() = delete;
	GeometryClass(GeometryClass const& rhs) = delete;
	GeometryClass& operator=(GeometryClass const& rhs) = delete;

	GeometryClass(GeometryClass&& rhs) = delete;
	GeometryClass& operator=(GeometryClass&& rhs) = delete;

public:
	static Mesh CreateBox(const float width, const float height, const float depth, const UINT numSubdivisions);
	static Mesh CreateSphere(const float radius, const UINT numSubdivisions);

private:
	static void Subdivide(Mesh& meshData);
	static Vertex MidPoint(const Vertex& v0, const Vertex& v1);
};