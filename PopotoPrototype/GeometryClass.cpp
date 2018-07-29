#include "stdafx.h"
#include "GeometryClass.h"

using namespace Math;

GeometryClass::Mesh GeometryClass::CreateBox(
	const float width,
	const float height,
	const float depth,
	const UINT numSubdivisions) {

	Mesh meshData{};

	{ // Generate Vertices
		std::array<Vertex, 24> v;
		const float w2 = 0.5f * width;
		const float h2 = 0.5f * height;
		const float d2 = 0.5f * depth;

		// Front face
		v[0] = Vertex{ { -w2, -h2, -d2 },{ 0.0f, 0.0f, -1.0f },{ 1.0f, 0.0f, 0.0f},{ 0.0f, 1.0f } };
		v[1] = Vertex{ { -w2, +h2, -d2 },{ 0.0f, 0.0f, -1.0f },{ 1.0f, 0.0f, 0.0f},{ 0.0f, 0.0f } };
		v[2] = Vertex{ { +w2, +h2, -d2 },{ 0.0f, 0.0f, -1.0f },{ 1.0f, 0.0f, 0.0f},{ 1.0f, 0.0f } };
		v[3] = Vertex{ { +w2, -h2, -d2 },{ 0.0f, 0.0f, -1.0f },{ 1.0f, 0.0f, 0.0f},{ 1.0f, 1.0f } };

		// Back face										   					 
		v[4] = Vertex{ { -w2, -h2, +d2 },{ 0.0f, 0.0f, 1.0f }, {-1.0f, 0.0f, 0.0f},{ 1.0f, 1.0f } };
		v[5] = Vertex{ { +w2, -h2, +d2 },{ 0.0f, 0.0f, 1.0f }, {-1.0f, 0.0f, 0.0f},{ 0.0f, 1.0f } };
		v[6] = Vertex{ { +w2, +h2, +d2 },{ 0.0f, 0.0f, 1.0f }, {-1.0f, 0.0f, 0.0f},{ 0.0f, 0.0f } };
		v[7] = Vertex{ { -w2, +h2, +d2 },{ 0.0f, 0.0f, 1.0f }, {-1.0f, 0.0f, 0.0f},{ 1.0f, 0.0f } };

		// Top face											   					 
		v[8] = Vertex{ { -w2, +h2, -d2 },{ 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f},{ 0.0f, 1.0f } };
		v[9] = Vertex{ { -w2, +h2, +d2 },{ 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f},{ 0.0f, 0.0f } };
		v[10] = Vertex{ { +w2, +h2, +d2 },{ 0.0f, 1.0f, 0.0f },{ 1.0f, 0.0f, 0.0f},{ 1.0f, 0.0f } };
		v[11] = Vertex{ { +w2, +h2, -d2 },{ 0.0f, 1.0f, 0.0f },{ 1.0f, 0.0f, 0.0f},{ 1.0f, 1.0f } };

		// Bottom face
		v[12] = Vertex{ { -w2, -h2, -d2 },{ 0.0f, -1.0f, 0.0f },{-1.0f, 0.0f, 0.0f,}, { 1.0f, 1.0f } };
		v[13] = Vertex{ { +w2, -h2, -d2 },{ 0.0f, -1.0f, 0.0f },{-1.0f, 0.0f, 0.0f,}, { 0.0f, 1.0f } };
		v[14] = Vertex{ { +w2, -h2, +d2 },{ 0.0f, -1.0f, 0.0f },{-1.0f, 0.0f, 0.0f,}, { 0.0f, 0.0f } };
		v[15] = Vertex{ { -w2, -h2, +d2 },{ 0.0f, -1.0f, 0.0f },{-1.0f, 0.0f, 0.0f,}, { 1.0f, 0.0f } };

		// Left face											
		v[16] = Vertex{ { -w2, -h2, +d2 },{ -1.0f, 0.0f, 0.0f },{0.0f, 0.0f, -1.0f,}, { 0.0f, 1.0f } };
		v[17] = Vertex{ { -w2, +h2, +d2 },{ -1.0f, 0.0f, 0.0f },{0.0f, 0.0f, -1.0f,}, { 0.0f, 0.0f } };
		v[18] = Vertex{ { -w2, +h2, -d2 },{ -1.0f, 0.0f, 0.0f },{0.0f, 0.0f, -1.0f,}, { 1.0f, 0.0f } };
		v[19] = Vertex{ { -w2, -h2, -d2 },{ -1.0f, 0.0f, 0.0f },{0.0f, 0.0f, -1.0f,}, { 1.0f, 1.0f } };

		// Right face											 
		v[20] = Vertex{ { +w2, -h2, -d2 },{ 1.0f, 0.0f, 0.0f }, {0.0f, 0.0f, 1.0f, }, { 0.0f, 1.0f } };
		v[21] = Vertex{ { +w2, +h2, -d2 },{ 1.0f, 0.0f, 0.0f }, {0.0f, 0.0f, 1.0f, }, { 0.0f, 0.0f } };
		v[22] = Vertex{ { +w2, +h2, +d2 },{ 1.0f, 0.0f, 0.0f }, {0.0f, 0.0f, 1.0f, }, { 1.0f, 0.0f } };
		v[23] = Vertex{ { +w2, -h2, +d2 },{ 1.0f, 0.0f, 0.0f }, {0.0f, 0.0f, 1.0f, }, { 1.0f, 1.0f } };

		for (auto& vert : v) {
			vert.m_uv.x = (vert.m_uv.x == 0.0f) ? 1.0f : 0.0f;
		}

		meshData.m_vertices.assign(v.begin(), v.end());
	}

	{// Generate Indices
		std::array<UINT32, 36> k;

		// Front face
		k[0] = 0; k[1] = 1; k[2] = 2;
		k[3] = 0; k[4] = 2; k[5] = 3;

		// Back face
		k[6] = 4; k[7] = 5; k[8] = 6;
		k[9] = 4; k[10] = 6; k[11] = 7;

		// Top face
		k[12] = 8; k[13] = 9; k[14] = 10;
		k[15] = 8; k[16] = 10; k[17] = 11;

		// Bottom face
		k[18] = 12; k[19] = 13; k[20] = 14;
		k[21] = 12; k[22] = 14; k[23] = 15;

		// Left face
		k[24] = 16; k[25] = 17; k[26] = 18;
		k[27] = 16; k[28] = 18; k[29] = 19;

		// Right face
		k[30] = 20; k[31] = 21; k[32] = 22;
		k[33] = 20; k[34] = 22; k[35] = 23;

		// Convert to left-handed
		for (auto i = 0; i < k.size(); i += 3) {
			auto rest = k[i + 1];
			k[i + 1] = k[i + 2];
			k[i + 2] = rest;
		}

		meshData.m_indices.assign(k.begin(), k.end());
	}

	for (auto i = 0u; i < (numSubdivisions < 6u ? numSubdivisions : 6u); ++i) {
		Subdivide(meshData);
	}

	return meshData;
}

GeometryClass::Mesh GeometryClass::CreateSphere(
	const float radius, 
	const UINT numSubdivisions) {

	Mesh meshData{};

	const auto X = 0.525731f;
	const auto Z = 0.850651f;

	std::array<Vector3, 12> pos{
		Vector3{ -X, 0.0f, Z }, Vector3{ X, 0.0f, Z },
		Vector3{ -X, 0.0f, -Z }, Vector3{ X, 0.0f, -Z },
		Vector3{ 0.0f, Z, X }, Vector3{ 0.0f, Z, -X },
		Vector3{ 0.0f, -Z, X }, Vector3{ 0.0f, -Z, -X },
		Vector3{ Z, X, 0.0f }, Vector3{ -Z, X, 0.0f },
		Vector3{ Z, -X, 0.0f }, Vector3{ -Z, -X, 0.0f }
	};

	std::array<Vertex, pos.size()> verts;
	for (auto i = 0; i < pos.size(); ++i) {
		verts[i] = Vertex{ pos[i], {}, {}, {} };
	}

	std::array<UINT32, 60> k{
		1,4,0,  4,9,0,  4,5,9,  8,5,4,  1,8,4,
		1,10,8, 10,3,8, 8,3,5,  3,2,5,  3,7,2,
		3,10,7, 10,6,7, 6,11,7, 6,0,11, 6,1,0,
		10,1,6, 11,0,9, 2,11,9, 5,2,9,  11,2,7
	};

	// Convert to left-handed
	for (auto i = 0; i < k.size(); i += 3) {
		auto rest = k[i + 1];
		k[i + 1] = k[i + 2];
		k[i + 2] = rest;
	}

	meshData.m_vertices.assign(verts.begin(), verts.end());
	meshData.m_indices.assign(k.begin(), k.end());


	for (auto i = 0u; i < (numSubdivisions < 6u ? numSubdivisions : 6u); ++i) {
		Subdivide(meshData);
	}

	for (size_t i = 0; i < meshData.m_vertices.size(); ++i) {
		auto& vertex = meshData.m_vertices[i];

		vertex.m_normal = Normalize(vertex.m_position);
		vertex.m_position = vertex.m_normal * radius;

		auto theta = atan2f(vertex.m_position.GetZ(), vertex.m_position.GetX());
		theta = theta < 0.0f ? theta + XM_2PI : theta;

		float phi = acosf(vertex.m_position.GetY() / radius);
		vertex.m_uv.x = 1.0f - (theta / XM_2PI);
		vertex.m_uv.y = phi / XM_PI;

		vertex.m_tangent = {
			-radius*sinf(phi)*sinf(theta),
			0.0f,
			+radius*sinf(phi)*cosf(theta)};
		vertex.m_tangent = Math::Normalize(vertex.m_tangent);
	}

	return meshData;
}

void GeometryClass::Subdivide(Mesh& meshData) {
	const auto copy = meshData;

	meshData.m_vertices.resize(0);
	meshData.m_indices.resize(0);

	const UINT32 numTris = static_cast<UINT32>(copy.m_indices.size() / 3);

	for (UINT32 i = 0; i < numTris; ++i) {
		Vertex v0 = copy.m_vertices[copy.m_indices[i * 3 + 0]];
		Vertex v1 = copy.m_vertices[copy.m_indices[i * 3 + 1]];
		Vertex v2 = copy.m_vertices[copy.m_indices[i * 3 + 2]];

		Vertex m0 = MidPoint(v0, v1);
		Vertex m1 = MidPoint(v1, v2);
		Vertex m2 = MidPoint(v0, v2);

		meshData.m_vertices.push_back(v0);
		meshData.m_vertices.push_back(v1);
		meshData.m_vertices.push_back(v2);
		meshData.m_vertices.push_back(m0);
		meshData.m_vertices.push_back(m1);
		meshData.m_vertices.push_back(m2);

		const UINT32 i6 = i * 6ui32;
		meshData.m_indices.push_back(i6 + 0ui32);
		meshData.m_indices.push_back(i6 + 3ui32);
		meshData.m_indices.push_back(i6 + 5ui32);

		meshData.m_indices.push_back(i6 + 3ui32);
		meshData.m_indices.push_back(i6 + 4ui32);
		meshData.m_indices.push_back(i6 + 5ui32);
										   
		meshData.m_indices.push_back(i6 + 5ui32);
		meshData.m_indices.push_back(i6 + 4ui32);
		meshData.m_indices.push_back(i6 + 2ui32);
										   
		meshData.m_indices.push_back(i6 + 3ui32);
		meshData.m_indices.push_back(i6 + 1ui32);
		meshData.m_indices.push_back(i6 + 4ui32);
	}
}

GeometryClass::Vertex GeometryClass::MidPoint(const Vertex& v0, const Vertex& v1) {
	DirectX::XMVECTOR tex0 = DirectX::XMLoadFloat2(&v0.m_uv);
	DirectX::XMVECTOR tex1 = DirectX::XMLoadFloat2(&v1.m_uv);

	// Compute the midpoints of all the attributes.  Vectors need to be normalized
	// since linear interpolating can make them not unit length.
	Vertex v{};
	v.m_position = 0.5f * (v0.m_position + v1.m_position);
	v.m_normal = Normalize(0.5f * (v0.m_normal + v1.m_normal));
	DirectX::XMStoreFloat2(&v.m_uv, DirectX::XMVectorMultiply(DirectX::XMVectorAdd(tex0, tex1), { 0.5f, 0.5f, 0.5f }));

	return v;
}