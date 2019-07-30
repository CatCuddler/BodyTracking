#pragma once

#include "pch.h"
#include "MeshObject.h"

using namespace Kore;

// A plane is defined as the plane's normal and the distance of the plane to the origin
class PlaneCollider {
public:
	
	// Distance
	float d;
	
	// Normal
	vec3 normal;
};

// A box collider
class BoxCollider {
public:
	PlaneCollider posX;
	PlaneCollider negX;
	PlaneCollider posY;
	PlaneCollider negY;
	PlaneCollider posZ;
	PlaneCollider negZ;
	
	BoxCollider(Kore::vec3 center, Kore::vec3 fullExtents) {
		posX.normal = vec3(1, 0, 0);
		posX.d = center.x() + fullExtents.x() * 0.5f;
		posX.d *= -1.0f;
		negX.normal = vec3(-1, 0, 0);
		negX.d = center.x() - fullExtents.x() * 0.5f;
		posY.normal = vec3(0, 1, 0);
		posY.d = center.y() + fullExtents.y() * 0.5f;
		posY.d *= -1.0f;
		negY.normal = vec3(0, -1, 0);
		negY.d = center.y() - fullExtents.y() * 0.5f;
		posZ.normal = vec3(0, 0, 1);
		posZ.d = center.z() + fullExtents.z() * 0.5f;
		posZ.d *= -1.0f;
		negZ.normal = vec3(0, 0, -1);
		negZ.d = center.z() - fullExtents.z() * 0.5f;
	}
};

class TriangleCollider {
	void LoadVector(vec3& v, int index, float* vb) {
		v.set(vb[index*8 + 0], vb[index*8 + 1], vb[index*8 + 2]);
	}
	
public:
	vec3 A;
	vec3 B;
	vec3 C;
	
	float Area() {
		return 0.5f * ((B-A).cross(C-A)).getLength();
	}
	
	void LoadFromBuffers(int index, int* ib, float* vb) {
		LoadVector(A, ib[index * 3], vb);
		LoadVector(B, ib[index * 3 + 1], vb);
		LoadVector(C, ib[index * 3 + 2], vb);
	}
	
	vec3 GetNormal() {
		vec3 n = (B-A).cross(C-A);
		n.normalize();
		return n;
	}
	
	PlaneCollider GetPlane() {
		vec3 n = GetNormal();
		float d = n.dot(A);
		
		PlaneCollider p;
		p.d = -d;
		p.normal = n;
		return p;
	}
};

class TriangleMeshCollider {
public:
	MeshObject* mesh;
	
	int lastCollision;
};

// A sphere is defined by a radius and a center.
class SphereCollider {
public:
	vec3 center;
	float radius;
	
	SphereCollider(Kore::vec3 center, float radius) : center(center), radius(radius) {
		
	}
	
	// Return true iff there is an intersection with the other sphere
	bool IntersectsWith(const SphereCollider& other) {
		float distance = (other.center - center).getLength();
		return distance < other.radius + radius;
	}
	
	// Collision normal is the normal vector pointing towards the other sphere
	vec3 GetCollisionNormal(const SphereCollider& other) {
		vec3 n = other.center - center;
		n = n.normalize();
		return n;
	}
	
	// The penetration depth
	float PenetrationDepth(const SphereCollider& other) {
		return other.radius + radius - (other.center - center).getLength();
	}
	
	
	vec3 GetCollisionNormal(const PlaneCollider& other) {
		return other.normal;
	}
	
	float PenetrationDepth(const PlaneCollider &other) {
		return other.normal.dot(center) + other.d - radius;
	}
	
	float Distance(const PlaneCollider& other) {
		return other.normal.dot(center) + other.d;
	}
	
	bool IntersectsWith(const PlaneCollider& other) {
		return Kore::abs(Distance(other)) <= radius;
	}
	
	bool IsInside(const PlaneCollider& other) {
		return Distance(other) < radius;
	}
	
	bool IsOutside(const PlaneCollider& other) {
		return Distance(other) > radius;
	}
	
	bool IsInside(const BoxCollider& other) {
		if (!IsInside(other.posX))  { return false; }
		if (!IsInside(other.negX))  { return false; }
		if (!IsInside(other.posY))  { return false; }
		if (!IsInside(other.negY))  { return false; }
		if (!IsInside(other.posZ))  { return false; }
		if (!IsInside(other.negZ))  { return false; }
		
		return true;
	}
	
	bool IntersectsWithSides(const BoxCollider& other) {
		bool in_posX = !IsOutside(other.posX);
		bool in_negX = !IsOutside(other.negX);
		bool in_posY = !IsOutside(other.posY);
		bool in_negY = !IsOutside(other.negY);
		bool in_posZ = !IsOutside(other.posZ);
		bool in_negZ = !IsOutside(other.negZ);
		
		if (IntersectsWith(other.posZ) &&
			in_posX && in_negX && in_posY && in_negY) {
			return true;
		}
		
		if (IntersectsWith(other.negZ) &&
			in_posX && in_negX && in_posY && in_negY) {
			return true;
		}
		
		if (IntersectsWith(other.posX) &&
			in_posZ && in_negZ && in_posY && in_negY) {
			return true;
		}
		
		if (IntersectsWith(other.negX) &&
			in_posZ && in_negZ && in_posY && in_negY) {
			return true;
		}
		
		if (IntersectsWith(other.posY) &&
			in_posZ && in_negZ && in_posX && in_negX) {
			return true;
		}
		
		if (IntersectsWith(other.negY) &&
			in_posZ && in_negZ && in_posX && in_negX) {
			return true;
		}
		
		return false;
	}
	
	bool IntersectsWith(const BoxCollider& other) {
		return IsInside(other) || IntersectsWithSides(other);
	}
	
	vec3 GetCollisionNormal(const TriangleCollider& other) {
		// Use the normal of the triangle plane
		vec3 n = (other.B - other.A).cross(other.C - other.A);
		n.normalize();
		return n;
	}
	
	float PenetrationDepth(const TriangleCollider& other) {
		// Use the triangle plane
		// The assumption is that we have a mesh and will not collide only on a vertex or an edge
		
		return 0.0f;
	}
	
	bool IntersectsWith(TriangleMeshCollider& other) {
		TriangleCollider coll;
		int* current = other.mesh->meshes[0]->indices;
		float* currentVertex = other.mesh->meshes[0]->vertices;
		for (int i = 0; i < other.mesh->meshes[0]->numFaces; i++) {
			coll.LoadFromBuffers(i, current, currentVertex);
			if (coll.Area() < 0.1f) continue;
			if (IntersectsWith(coll)) {
				other.lastCollision = i;
				vec3 normal;
				if (coll.GetNormal().x() < -0.8f)
					normal = coll.GetNormal();
				// Kore::log(Warning, "Intersected with triangle: %f, %f, %f", coll.GetNormal().x(), coll.GetNormal().y(), coll.GetNormal().z());
				return true;
			}
		}
		return false;
	}
	
	vec3 GetCollisionNormal(const TriangleMeshCollider& other) {
		TriangleCollider coll;
		coll.LoadFromBuffers(other.lastCollision, other.mesh->meshes[0]->indices, other.mesh->meshes[0]->vertices);
		return coll.GetNormal();
	}
	
	float PenetrationDepth(const TriangleMeshCollider& other) {
		// Get a collider for the plane of the triangle
		TriangleCollider coll;
		coll.LoadFromBuffers(other.lastCollision, other.mesh->meshes[0]->indices, other.mesh->meshes[0]->vertices);
		PlaneCollider plane = coll.GetPlane();
		
		return PenetrationDepth(plane);
	}
	
	
	// Find the point where the sphere collided with the triangle mesh
	vec3 GetCollisionPoint(const TriangleMeshCollider& other) {
		// We take the point on the sphere that is closest to the triangle
		vec3 result = center - GetCollisionNormal(other) * radius;
		return result;
	}
	
	// Find a set of basis vectors such that the provided collision normal x is the first column of the basis matrix,
	// and the other two vectors are perpendicular to the collision normal
	mat3 GetCollisonBasis(vec3 x) {
		x.normalize();
		mat3 basis;
		basis.Set(0, 0, x.x());
		basis.Set(1, 0, x.y());
		basis.Set(2, 0, x.z());
		
		// Find a y-vector
		// x will often be the global y-axis, so don't use this -> use global z instead
		vec3 y(0, 0, 1);
		y = x.cross(y);
		y = y.normalize();
		
		basis.Set(0, 1, y.x());
		basis.Set(1, 1, y.y());
		basis.Set(2, 1, y.z());
		
		vec3 z = x.cross(y);
		z.normalize();
		
		basis.Set(0, 2, z.x());
		basis.Set(1, 2, z.y());
		basis.Set(2, 2, z.z());
		return basis;
	}
	
	bool IsSeparatedByVertexA(const TriangleCollider& other)
	{
		float distance = (other.A - center).getLength();
		bool distanceCheck = distance > radius;
		
		bool isSameDirectionB = (other.B - other.A).dot(other.A - center) > 0.0f;
		bool isSameDirectionC = (other.C - other.A).dot(other.A - center) > 0.0f;
		return distanceCheck & isSameDirectionB & isSameDirectionC;
	}
	
	bool IsSeparatedByVertexB(const TriangleCollider& other)
	{
		float distance = (other.B - center).getLength();
		bool distanceCheck = distance > radius;
		
		bool isSameDirectionA = (other.A - other.B).dot(other.B - center) > 0.0f;
		bool isSameDirectionC = (other.C - other.B).dot(other.B - center) > 0.0f;
		return distanceCheck & isSameDirectionA & isSameDirectionC;
	}
	
	bool IsSeparatedByVertexC(const TriangleCollider& other)
	{
		float distance = (other.C - center).getLength();
		bool distanceCheck = distance > radius;
		
		bool isSameDirectionA = (other.A - other.C).dot(other.C - center) > 0.0f;
		bool isSameDirectionB = (other.B - other.C).dot(other.C - center) > 0.0f;
		return distanceCheck & isSameDirectionA & isSameDirectionB;
	}
	
	bool IntersectsWith(const TriangleCollider& other) {
		// Separating Axes Test
		// 1 Face
		// 3 Vertices
		// 3 Edges
		// No overlap if all of the resulting axes are not touching
		
		vec3 A = other.A - center;
		vec3 B = other.B - center;
		vec3 C = other.C - center;
		float rr = radius * radius;
		vec3 V = (B - A).cross(C - A);
		float d = A.dot(V);
		float e = V.dot(V);
		bool sepPlane = d * d > rr * e;
		float aa = A.dot(A);
		float ab = A.dot(B);
		float ac = A.dot(C);
		float bb = B.dot(B);
		float bc = B.dot(C);
		float cc = C.dot(C);
		vec3 AB = B - A;
		vec3 BC = C - B;
		vec3 CA = A - C;
		float d1 = ab - aa;
		float d2 = bc - bb;
		float d3 = ac - cc;
		float e1 = AB.dot(AB);
		float e2 = BC.dot(BC);
		float e3 = CA.dot(CA);
		vec3 Q1 = A * e1 - d1 * AB;
		vec3 Q2 = B * e2 - d2 * BC;
		vec3 Q3 = C * e3 - d3 * CA;
		vec3 QC = C * e1 - Q1;
		vec3 QA = A * e2 - Q2;
		vec3 QB = B * e3 - Q3;
		bool sepEdge1 = (Q1.dot(Q1) > rr * e1 * e1) & (Q1.dot(QC) > 0);
		bool sepEdge2 = (Q2.dot(Q2) > rr * e2 * e2) & (Q2.dot(QA) > 0);
		bool sepEdge3 = (Q3.dot(Q3) > rr * e3 * e3) & (Q3.dot(QB) > 0);
		
		bool sepVertexA = IsSeparatedByVertexA(other);
		bool sepVertexB = IsSeparatedByVertexB(other);
		bool sepVertexC = IsSeparatedByVertexC(other);
		
		bool separated = sepPlane | sepVertexA | sepVertexB | sepVertexC | sepEdge1 | sepEdge2 | sepEdge3;
		
		return !separated;
	}
};
