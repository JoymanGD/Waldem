#pragma once
#include "Waldem/ECS/Components/ColliderComponent.h"
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/Math/Math.h"
#include "Waldem/Renderer/AABB.h"
#include "Waldem/Renderer/Model/Plane.h"
#include "Waldem/Renderer/Model/Simplex.h"
#include "Waldem/Renderer/Model/Transform.h"
#include "Waldem/Utils/GeometryUtils.h"

#define EPA_EPSILON 1e-5
#define GJK_MAX_NUM_ITERATIONS 64
#define EPA_MAX_NUM_ITERATIONS 64
#define EPA_MAX_NUM_LOOSE_EDGES 32
#define EPA_MAX_NUM_FACES 64
#define EPA_TOLERANCE 0.0001f

namespace Waldem
{
    class WALDEM_API CollisionSystem : ISystem
    {
    private:
        BVHNode* RootNode;
        int MaxIterations = 50;
        int MaxResolveIterations = 5;
        
        BVHNode* BuildBVH(WArray<AABB>& objects, WArray<String>& names, int start, int end)
        {
            BVHNode* node = new BVHNode();
            int objectCount = end - start;

            AABB box = objects[start];
            for (int i = start + 1; i < end; i++)
            {
                box.Expand(objects[i]);
            }
            node->Box = box;
            node->DebugName = names[start];

            if (objectCount == 1)
            {
                node->ObjectIndex = start;
                node->Left = node->Right = nullptr;
                return node;
            }

            Vector3 size = box.Max - box.Min;
            int axis = 0;
            if (size.y > size.x && size.y > size.z) axis = 1;
            if (size.z > size.x && size.z > size.y) axis = 2;

            int mid = start + objectCount / 2;
            std::nth_element(objects.begin() + start, objects.begin() + mid, objects.begin() + end,
                [axis](const AABB& a, const AABB& b)
                {
                    return a.Min[axis] < b.Min[axis];
                });

            node->Left = BuildBVH(objects, names, start, mid);
            node->Right = BuildBVH(objects, names, mid, end);

            return node;
        }

        void UpdateBVH(BVHNode* node, WArray<AABB>& objects)
        {
            if (node->IsLeaf()) 
            {
                node->Box = objects[node->ObjectIndex];
                return;
            }

            if (node->Left) UpdateBVH(node->Left, objects);
            if (node->Right) UpdateBVH(node->Right, objects);

            node->Box = node->Left ? node->Left->Box : AABB();
            if (node->Right) node->Box.Expand(node->Right->Box);
        }

        void BroadPhaseCollision(BVHNode* node1, BVHNode* node2, WArray<CollisionPair>& collisions)
        {
            if (!node1->Box.Intersects(node2->Box))
            {
                return;
            }

            if (node1->IsLeaf() && node2->IsLeaf())
            {
                if (node1->ObjectIndex != node2->ObjectIndex)
                {
                    int minIndex = std::min(node1->ObjectIndex, node2->ObjectIndex);
                    int maxIndex = std::max(node1->ObjectIndex, node2->ObjectIndex);

                    std::pair<int, int> pair = {minIndex, maxIndex};
                    if (std::find(collisions.begin(), collisions.end(), pair) == collisions.end())
                    {
                        collisions.Add(pair);
                    }
                }
                return;
            }

            if (node1->IsLeaf())
            {
                BroadPhaseCollision(node1, node2->Left, collisions);
                BroadPhaseCollision(node1, node2->Right, collisions);
            }
            else if (node2->IsLeaf())
            {
                BroadPhaseCollision(node1->Left, node2, collisions);
                BroadPhaseCollision(node1->Right, node2, collisions);
            }
            else
            {
                BroadPhaseCollision(node1->Left, node2->Left, collisions);
                BroadPhaseCollision(node1->Left, node2->Right, collisions);
                BroadPhaseCollision(node1->Right, node2->Left, collisions);
                BroadPhaseCollision(node1->Right, node2->Right, collisions);
            }
        }

        // Contact EPA(const Simplex& simplex, const ColliderComponent* colliderA, const ColliderComponent* colliderB, Transform* worldTransformA, Transform* worldTransformB)
        // {
        //     Contact contact;
        //     contact.HasCollision = false;
        //
        //     std::vector<Vector3> polytope;
        //     std::vector<Vector3> sourcePointsA;
        //     std::vector<Vector3> sourcePointsB;
        //
        //     for (auto& vertex : simplex)
        //     {
        //         polytope.push_back(vertex.Point);
        //         sourcePointsA.push_back(vertex.SupportA); 
        //         sourcePointsB.push_back(vertex.SupportB);
        //     }
        //
        //     std::vector<size_t> faces = {
        //         0, 1, 2,
        //         0, 3, 1,
        //         0, 2, 3,
        //         1, 3, 2
        //     };
        //
        //     auto [normals, minFace] = GeometryUtils::GetFaceNormals(polytope, faces);
        //
        //     Vector3 minNormal;
        //     float minDistance = FLT_MAX;
        //
        //     // We'll store the final 'face indices'
        //     int finalFaceIdx = -1;
        //
        //     // Store expansions in parallel arrays
        //     // so each polytope vertex has a corresponding A/B source
        //     auto pushSupport = [&](const Vector3& v, const Vector3& a, const Vector3& b)
        //     {
        //         polytope.push_back(v);
        //         sourcePointsA.push_back(a);
        //         sourcePointsB.push_back(b);
        //     };
        //
        //     uint32_t iterations = 0;
        //     
        //     // Iteration loop
        //     while (minDistance == FLT_MAX && iterations < MaxIterations)
        //     {
        //         minNormal   = normals[minFace];
        //         minDistance = normals[minFace].w;
        //         finalFaceIdx = (int)minFace;
        //
        //         // get support from colliders
        //         Vector3 supportA = colliderA->FindFurthestPoint(minNormal, worldTransformA);
        //         Vector3 supportB = colliderB->FindFurthestPoint(-minNormal, worldTransformB);
        //         Vector3 support  = supportA - supportB;
        //
        //         float sDistance = dot(minNormal, support);
        //
        //         // Check if this actually improves
        //         if (fabs(sDistance - minDistance) > 0.0001f) // smaller threshold
        //         {
        //             minDistance = FLT_MAX;
        //
        //             std::vector<std::pair<size_t, size_t>> uniqueEdges;
        //
        //             for (size_t i = 0; i < normals.size(); i++)
        //             {
        //                 if (GeometryUtils::SameDirection(normals[i], support - polytope[faces[i*3]]))
        //                 {
        //                     size_t f = i * 3;
        //
        //                     GeometryUtils::AddIfUniqueEdge(uniqueEdges, faces, f, f + 1);
        //                     GeometryUtils::AddIfUniqueEdge(uniqueEdges, faces, f + 1, f + 2);
        //                     GeometryUtils::AddIfUniqueEdge(uniqueEdges, faces, f + 2, f);
        //
        //                     // remove face i
        //                     faces[f + 2] = faces.back(); faces.pop_back();
        //                     faces[f + 1] = faces.back(); faces.pop_back();
        //                     faces[f]     = faces.back(); faces.pop_back();
        //
        //                     normals[i] = normals.back();
        //                     normals.pop_back();
        //
        //                     i--;
        //                 }
        //             }
        //
        //             // build new faces from uniqueEdges
        //             std::vector<size_t> newFaces;
        //             size_t newVertIndex = polytope.size();
        //
        //             pushSupport(support, supportA, supportB);
        //
        //             for (auto [edgeIndex1, edgeIndex2] : uniqueEdges)
        //             {
        //                 newFaces.push_back(edgeIndex1);
        //                 newFaces.push_back(edgeIndex2);
        //                 newFaces.push_back(newVertIndex);
        //             }
        //
        //             auto [newNormals, newMinFace] = GeometryUtils::GetFaceNormals(polytope, newFaces);
        //
        //             float oldMinDistance = FLT_MAX;
        //             for (size_t i = 0; i < normals.size(); i++)
        //             {
        //                 if (normals[i].w < oldMinDistance)
        //                 {
        //                     oldMinDistance = normals[i].w;
        //                     minFace = i;
        //                 }
        //             }
        //
        //             if (newNormals.empty())
        //                 return contact;
        //
        //             if (newNormals[newMinFace].w < oldMinDistance)
        //             {
        //                 minFace = newMinFace + normals.size();
        //             }
        //
        //             faces.insert(faces.end(), newFaces.begin(), newFaces.end());
        //             normals.insert(normals.end(), newNormals.begin(), newNormals.end());
        //         }
        //
        //         iterations++;
        //     }
        //
        //     // We can compute a better contact point:
        //     contact.HasCollision = true;
        //     
        //     contact.Normal = normalize(minNormal);
        //
        //     // optional margin:
        //     contact.PenetrationDepth = minDistance;
        //
        //     // Let's find the actual face indices
        //     size_t idx0 = faces[finalFaceIdx*3 + 0];
        //     size_t idx1 = faces[finalFaceIdx*3 + 1];
        //     size_t idx2 = faces[finalFaceIdx*3 + 2];
        //
        //     Vector3 v0 = polytope[idx0];
        //     Vector3 v1 = polytope[idx1];
        //     Vector3 v2 = polytope[idx2];
        //
        //     Vector3 a0 = sourcePointsA[idx0];
        //     Vector3 a1 = sourcePointsA[idx1];
        //     Vector3 a2 = sourcePointsA[idx2];
        //
        //     Vector3 b0 = sourcePointsB[idx0];
        //     Vector3 b1 = sourcePointsB[idx1];
        //     Vector3 b2 = sourcePointsB[idx2];
        //
        //     // Solve for barycentric coords alpha0, alpha1, alpha2 such that
        //     // alpha0+alpha1+alpha2=1 and alpha0*v0 + alpha1*v1 + alpha2*v2=0
        //     float alpha0, alpha1, alpha2;
        //     GeometryUtils::ComputeBarycentricForOriginInTriangle(v0, v1, v2, alpha0, alpha1, alpha2);
        //
        //     // Then final contact points
        //     contact.ContactPointA = alpha0 * a0 + alpha1 * a1 + alpha2 * a2;
        //     contact.ContactPointB = alpha0 * b0 + alpha1 * b1 + alpha2 * b2;
        //
        //     return contact;
        // }

        // Contact GJK(const ColliderComponent* colliderA, const ColliderComponent* colliderB, Transform* worldTransformA, Transform* worldTransformB)
        // {
        //     Contact contact;
        //
        //     Vector3 initialDir = Vector3(1, 0, 0);
        //     
        //     SimplexVertex support = SimplexVertex(colliderA->FindFurthestPoint(initialDir, worldTransformA), colliderB->FindFurthestPoint(-initialDir, worldTransformB));
        //
        //     Simplex points;
        //     points.Add(support);
        //
        //     Vector3 direction = -support.Point;
        //
        //     int iterations = 0;
        //     
        //     while(true)
        //     {
        //         if(iterations > MaxIterations)
        //         {
        //             contact.HasCollision = false;
        //             return contact;
        //         }
        //         
        //         support = SimplexVertex(colliderA->FindFurthestPoint(direction, worldTransformA), colliderB->FindFurthestPoint(-direction, worldTransformB));
        //
        //         if(dot(support.Point, direction) <= 0)
        //         {
        //             contact.HasCollision = false;
        //             return contact;
        //         }
        //
        //         points.Add(support);
        //
        //         if(GeometryUtils::NextSimplex(points, direction))
        //         {
        //             contact = EPA(points, colliderA, colliderB, worldTransformA, worldTransformB);
        //             
        //             return contact;
        //         }
        //
        //         iterations++;
        //     }
        // }

        struct CollisionPoint
        {
            Vector3 p; //Conserve Minkowski Difference
            Vector3 a; //Result coordinate of object A's support function 
            Vector3 b; //Result coordinate of object B's support function 
        };

        void CalculateSearchPoint(CollisionPoint& point, Vector3& search_dir, ColliderComponent* coll1, ColliderComponent* coll2, Transform* worldTransformA, Transform* worldTransformB)
        {
            point.b = coll2->FindFurthestPoint(search_dir, worldTransformA);
            point.a = coll1->FindFurthestPoint(-search_dir, worldTransformB);
            point.p = point.b - point.a;
        }

        void update_simplex3(CollisionPoint& a, CollisionPoint& b, CollisionPoint& c, CollisionPoint& d, int& simp_dim, Vector3& search_dir)
        {
	        /* Required winding order:
	           //  b
	           //  | \
	           //  |   \
	           //  |    a
	           //  |   /
	           //  | /
	           //  c
	           */
	        Vector3 n = cross(b.p - a.p, c.p - a.p); //triangle's normal
	        Vector3 AO = -a.p; //direction to origin

	        //Determine which feature is closest to origin, make that the new simplex

	        simp_dim = 2;
	        if (dot(cross(b.p - a.p, n), AO) > 0) { //Closest to edge AB
		        c = a;
		        //simp_dim = 2;
		        search_dir = cross(cross(b.p - a.p, AO), b.p - a.p);
		        return;
	        }
	        if (dot(cross(n, c.p - a.p), AO) > 0) { //Closest to edge AC
		        b = a;
		        //simp_dim = 2;
		        search_dir = cross(cross(c.p - a.p, AO), c.p - a.p);
		        return;
	        }

	        simp_dim = 3;
	        if (dot(n, AO) > 0) { //Above triangle
		        d = c;
		        c = b;
		        b = a;
		        //simp_dim = 3;
		        search_dir = n;
		        return;
	        }
	        //else //Below triangle
	        d = b;
	        b = a;
	        //simp_dim = 3;
	        search_dir = -n;
	        return;
        }

        bool update_simplex4(CollisionPoint& a, CollisionPoint& b, CollisionPoint& c, CollisionPoint& d, int& simp_dim, Vector3& search_dir)
        {
	        // a is peak/tip of pyramid, BCD is the base (counterclockwise winding order)
	        //We know a priori that origin is above BCD and below a

	        //Get normals of three new faces
	        Vector3 ABC = cross(b.p - a.p, c.p - a.p);
	        Vector3 ACD = cross(c.p - a.p, d.p - a.p);
	        Vector3 ADB = cross(d.p - a.p, b.p - a.p);

	        Vector3 AO = -a.p; //dir to origin
	        simp_dim = 3; //hoisting this just cause

	        //Plane-test origin with 3 faces
	        /*
	        // Note: Kind of primitive approach used here; If origin is in front of a face, just use it as the new simplex.
	        // We just go through the faces sequentially and exit at the first one which satisfies dot product. Not sure this
	        // is optimal or if edges should be considered as possible simplices? Thinking this through in my head I feel like
	        // this method is good enough. Makes no difference for AABBS, should test with more complex colliders.
	        */
	        if (dot(ABC, AO) > 0) { //In front of ABC
		        d = c;
		        c = b;
		        b = a;
		        search_dir = ABC;
		        return false;
	        }

	        if (dot(ACD, AO) > 0) { //In front of ACD
		        b = a;
		        search_dir = ACD;
		        return false;
	        }
	        if (dot(ADB, AO) > 0) { //In front of ADB
		        c = d;
		        d = b;
		        b = a;
		        search_dir = ADB;
		        return false;
	        }

	        //else inside tetrahedron; enclosed!
	        return true;
        }

        void EPA(CollisionPoint& a, CollisionPoint& b, CollisionPoint& c, CollisionPoint& d, ColliderComponent* coll1, ColliderComponent* coll2, Transform* worldTransformA, Transform* worldTransformB, Contact& collisionInfo)
		{
			CollisionPoint faces[EPA_MAX_NUM_FACES][4]; //Array of faces, each with 3 verts and a normal

			Vector3 VertexA[3];
			Vector3 VertexB[3];

			//Init with final simplex from GJK
			faces[0][0] = a;
			faces[0][1] = b;
			faces[0][2] = c;
			faces[0][3].p = normalize(cross(b.p - a.p, c.p - a.p)); //ABC
			faces[1][0] = a;
			faces[1][1] = c;
			faces[1][2] = d;
			faces[1][3].p = normalize(cross(c.p - a.p, d.p - a.p)); //ACD
			faces[2][0] = a;
			faces[2][1] = d;
			faces[2][2] = b;
			faces[2][3].p = normalize(cross(d.p - a.p, b.p - a.p)); //ADB
			faces[3][0] = b;
			faces[3][1] = d;
			faces[3][2] = c;
			faces[3][3].p = normalize(cross(d.p - b.p, c.p - b.p)); //BDC

			int num_faces = 4;
			int closest_face;

			for (int iterations = 0; iterations < EPA_MAX_NUM_ITERATIONS; iterations++)
			{
				//Find face that's closest to origin
				float min_dist = dot(faces[0][0].p, faces[0][3].p);
				closest_face = 0;
				for (int i = 1; i < num_faces; i++)
				{
					float dist = dot(faces[i][0].p, faces[i][3].p);
					if (dist < min_dist)
					{
						min_dist = dist;
						closest_face = i;
					}
				}

				//search normal to face that's closest to origin
				Vector3 search_dir = faces[closest_face][3].p;

				CollisionPoint p;
				CalculateSearchPoint(p, search_dir, coll1, coll2, worldTransformA, worldTransformB);

				if (dot(p.p, search_dir) - min_dist < EPA_TOLERANCE) {

				/*Core of calculating collision information*/
					Plane closestPlane = Plane::PlaneFromTri(faces[closest_face][0].p, faces[closest_face][1].p, faces[closest_face][2].p); //plane of closest triangle face
					Vector3 projectionPoint = closestPlane.ProjectPointOntoPlane(Vector3(0, 0, 0)); //projecting the origin onto the triangle(both are in Minkowski space)
					float u, v, w;
					Math::Barycentric(faces[closest_face][0].p, faces[closest_face][1].p, faces[closest_face][2].p,
						projectionPoint, u, v, w); //finding the barycentric coordinate of this projection point to the triangle

					//The contact points just have the same barycentric coordinate in their own triangles which  are composed by result coordinates of support function 
					Vector3 localA = faces[closest_face][0].a * u + faces[closest_face][1].a * v + faces[closest_face][2].a * w;
					Vector3 localB = faces[closest_face][0].b * u + faces[closest_face][1].b * v + faces[closest_face][2].b * w;
					float penetration = length(localA - localB);
					Vector3 normal = normalize(localA - localB);

					//Convergence (new point is not significantly further from origin)
					localA -= worldTransformA->Position;
					localB -= worldTransformB->Position;

					collisionInfo.AddContactPoint(localA, localB, normal, penetration);
				/*Core of calculating collision information*/

					return;
				}

				CollisionPoint loose_edges[EPA_MAX_NUM_LOOSE_EDGES][2]; //keep track of edges we need to fix after removing faces
				int num_loose_edges = 0;

				//Find all triangles that are facing p
				for (int i = 0; i < num_faces; i++)
				{
					if (dot(faces[i][3].p, p.p - faces[i][0].p) > 0) //triangle i faces p, remove it
					{
						//Add removed triangle's edges to loose edge list.
						//If it's already there, remove it (both triangles it belonged to are gone)
						for (int j = 0; j < 3; j++) //Three edges per face
						{
							CollisionPoint current_edge[2] = { faces[i][j], faces[i][(j + 1) % 3] };
							bool found_edge = false;
							for (int k = 0; k < num_loose_edges; k++) //Check if current edge is already in list
							{
								if (loose_edges[k][1].p == current_edge[0].p && loose_edges[k][0].p == current_edge[1].p) {
									loose_edges[k][0] = loose_edges[num_loose_edges - 1][0]; //Overwrite current edge
									loose_edges[k][1] = loose_edges[num_loose_edges - 1][1]; //with last edge in list
									num_loose_edges--;
									found_edge = true;
									k = num_loose_edges; //exit loop because edge can only be shared once
								}
							}//endfor loose_edges

							if (!found_edge) { //add current edge to list
								// assert(num_loose_edges<EPA_MAX_NUM_LOOSE_EDGES);
								if (num_loose_edges >= EPA_MAX_NUM_LOOSE_EDGES) break;
								loose_edges[num_loose_edges][0] = current_edge[0];
								loose_edges[num_loose_edges][1] = current_edge[1];
								num_loose_edges++;
							}
						}

						//Remove triangle i from list
						faces[i][0] = faces[num_faces - 1][0];
						faces[i][1] = faces[num_faces - 1][1];
						faces[i][2] = faces[num_faces - 1][2];
						faces[i][3] = faces[num_faces - 1][3];
						num_faces--;
						i--;
					}//endif p can see triangle i
				}//endfor num_faces

				//Reconstruct polytope with p added
				for (int i = 0; i < num_loose_edges; i++)
				{
					// assert(num_faces<EPA_MAX_NUM_FACES);
					if (num_faces >= EPA_MAX_NUM_FACES) break;
					faces[num_faces][0] = loose_edges[i][0];
					faces[num_faces][1] = loose_edges[i][1];
					faces[num_faces][2] = p;
					faces[num_faces][3].p = normalize(cross(loose_edges[i][0].p - loose_edges[i][1].p, loose_edges[i][0].p - p.p));

					//Check for wrong normal to maintain CCW winding
					float bias = 0.000001f; //in case dot result is only slightly < 0 (because origin is on face)
					if (dot(faces[num_faces][0].p, faces[num_faces][3].p) + bias < 0)
					{
						CollisionPoint temp = faces[num_faces][0];
						faces[num_faces][0] = faces[num_faces][1];
						faces[num_faces][1] = temp;
						faces[num_faces][3].p = -faces[num_faces][3].p;
					}
					num_faces++;
				}
			} //End for iterations
			printf("EPA did not converge\n");
			//Return most recent closest point
			Vector3 search_dir = faces[closest_face][3].p;

			CollisionPoint p;
			CalculateSearchPoint(p, search_dir, coll1, coll2, worldTransformA, worldTransformB);

			Plane closestPlane = Plane::PlaneFromTri(faces[closest_face][0].p, faces[closest_face][1].p, faces[closest_face][2].p);
			Vector3 projectionPoint = closestPlane.ProjectPointOntoPlane(Vector3(0, 0, 0));
			float u, v, w;
			Math::Barycentric(faces[closest_face][0].p, faces[closest_face][1].p, faces[closest_face][2].p, projectionPoint, u, v, w);
			Vector3 localA = faces[closest_face][0].a * u + faces[closest_face][1].a * v + faces[closest_face][2].a * w;
			Vector3 localB = faces[closest_face][0].b * u + faces[closest_face][1].b * v + faces[closest_face][2].b * w;
			float penetration = length(localA - localB);
			Vector3 normal = normalize(localA - localB);

			collisionInfo.AddContactPoint(localA, localB, normal, penetration);
		}

        bool GJK(ColliderComponent* colliderA, ColliderComponent* colliderB, Transform* worldTransformA, Transform* worldTransformB, Contact& outContact)
        {
            Contact contact;
            contact.ColliderA = colliderA;
            contact.ColliderB = colliderB;

            Vector3* mtv;

            Vector3 coll1Pos = worldTransformA->Position;
            Vector3 coll2Pos = worldTransformB->Position;
            
            CollisionPoint a, b, c, d;
            
            Vector3 search_dir = coll1Pos - coll2Pos;

            CalculateSearchPoint(c, search_dir, colliderA, colliderB, worldTransformA, worldTransformB);
            search_dir = -c.p;
            
            CalculateSearchPoint(b, search_dir, colliderA, colliderB, worldTransformA, worldTransformB);

            if (dot(b.p, search_dir) < 0)
            {
                return false;
            }//we didn't reach the origin, won't enclose it

            search_dir = cross(cross(c.p - b.p, -b.p), c.p - b.p);

            if (search_dir == Vector3(0)) //origin is on this line segment
            {
                //Apparently any normal search vector will do?
                search_dir = cross(c.p - b.p, Vector3(1, 0, 0)); //normal with x-axis

                if (search_dir == Vector3(0, 0, 0))
                {
                    search_dir = cross(c.p - b.p, Vector3(0, 0, -1)); //normal with z-axis
                }
            }

            int simp_dim = 2;

            for (int iterations = 0; iterations < GJK_MAX_NUM_ITERATIONS; iterations++)
            {
                //Point a;
                CalculateSearchPoint(a, search_dir, colliderA, colliderB, worldTransformA, worldTransformB);

                if (dot(a.p, search_dir) < 0)
                {
                    return false;
                }//we didn't reach the origin, won't enclose it

                simp_dim++;
                if (simp_dim == 3)
                {
                    update_simplex3(a, b, c, d, simp_dim, search_dir);
                }
                else if (update_simplex4(a, b, c, d, simp_dim, search_dir))
                {
                    EPA(a, b, c, d, colliderA, colliderB, worldTransformA, worldTransformB, outContact);
                    return true;
                }
            }

            return false;
        }

        void ResolveCollision(Transform* transformA, Transform* transformB, RigidBody* rigidBodyA, RigidBody* rigidBodyB, const Contact& contact)
        {
            if (!contact.HasCollision)
                return;

            // 1. Positional correction (same approach)
            {
                const float percent = 0.3f;  // typically 20–80% correction
                const float slop = 0.01f;    // penetration slop

                // penetration depth - slop
                float penDepth = std::max(contact.PenetrationDepth - slop, 0.0f);
                Vector3 correction = contact.Normal * (penDepth / (rigidBodyA->InvMass + rigidBodyB->InvMass)) * percent;

                transformA->Position -= correction * rigidBodyA->InvMass;
                transformB->Position += correction * rigidBodyB->InvMass;
            }

            // 2. Early-out if objects are separating along the contact normal
            //    We'll do a more accurate velocityAlongNormal by considering relative angular velocity too
            Vector3 ra = (contact.ContactPointA - transformA->Position); // radius from A's center of mass to contact
            Vector3 rb = (contact.ContactPointB - transformB->Position); // radius from B's center of mass to contact
            float check = dot(contact.ContactPointB - contact.ContactPointA, contact.Normal);
            WD_CORE_INFO("Check: {0}, PDepth: {1}", check, contact.PenetrationDepth);

            // linear velocity at contact point from each body
            Vector3 velA = rigidBodyA->Velocity + cross(rigidBodyA->AngularVelocity, ra);
            Vector3 velB = rigidBodyB->Velocity + cross(rigidBodyB->AngularVelocity, rb);

            Vector3 relativeVelocity = velB - velA;
            float velocityAlongNormal = dot(relativeVelocity, contact.Normal);

            if (velocityAlongNormal > 0.0f)
                return; // they're moving apart, no impulse needed

            // 3. Compute the impulse scalar 'j'
            //    For 3D rigid bodies, the effective mass is more complicated than just (1/mA + 1/mB).
            //    We have to factor in inertia for angular response.

            // Restitution (bounciness)
            float e = std::min(rigidBodyA->Bounciness, rigidBodyB->Bounciness);

            // Inverse mass sum along normal:
            // 1) linear: InvMassA + InvMassB
            // 2) angular: (rA x n)^T * InvInertiaA * (rA x n) + same for B
            Vector3 rA_x_n = cross(ra, contact.Normal);
            Vector3 rB_x_n = cross(rb, contact.Normal);

            // (I^-1) is the inverse inertia tensor in world space (3x3). For a simple diag, keep it in matrix form or do direct math
            // We'll denote them as invInertiaWorldA and invInertiaWorldB

            Vector3 angularFactorA = rigidBodyA->InvInertiaTensor * rA_x_n;
            float angularTermA = dot(rA_x_n, angularFactorA);

            Vector3 angularFactorB = rigidBodyB->InvInertiaTensor * rB_x_n;
            float angularTermB = dot(rB_x_n, angularFactorB);

            float invMassSum = rigidBodyA->InvMass + rigidBodyB->InvMass + angularTermA + angularTermB;

            float j = -(1.0f + e) * velocityAlongNormal;
            j /= invMassSum;

            // 4. Compute actual impulse vector
            Vector3 impulse = j * contact.Normal;

            // 5. Apply impulse — both linear and angular
            // For linear:
            rigidBodyA->Velocity -= impulse * rigidBodyA->InvMass;
            rigidBodyB->Velocity += impulse * rigidBodyB->InvMass;

            // For angular:
            // angularVelocity += InvInertia * (r x impulse)
            Vector3 angularImpulseA = rigidBodyA->InvInertiaTensor * cross(ra, -impulse);
            Vector3 angularImpulseB = rigidBodyB->InvInertiaTensor * cross(rb,  impulse);

            rigidBodyA->AngularVelocity += angularImpulseA;
            rigidBodyB->AngularVelocity += angularImpulseB;

            // If you store inertia in local space, you'd convert the cross(...) result to local coords
            // and then multiply by invInertiaLocal, or you keep a world-space inverse inertia matrix.
        }

        void NarrowPhaseCollision(WArray<CollisionPair>& collisions, WArray<ColliderComponent*>& colliders, WArray<RigidBody*>& rigidBodies, WArray<Transform*>& transforms)
        {
            for (auto collision : collisions)
            {
                ColliderComponent* collider1 = colliders[collision.first];
                ColliderComponent* collider2 = colliders[collision.second];
                Transform* worldTransformA = transforms[collision.first];
                Transform* worldTransformB = transforms[collision.second];
                RigidBody* rigidBodyA = rigidBodies[collision.first];
                RigidBody* rigidBodyB = rigidBodies[collision.second];

                if(rigidBodyA->IsKinematic && rigidBodyB->IsKinematic)
                {
                    continue;
                }

            	Contact contact;
                
                if (GJK(collider1, collider2, worldTransformA, worldTransformB, contact))
                {
                    collider1->IsColliding = true;
                    collider2->IsColliding = true;

                    // ResolveCollision(worldTransformA, worldTransformB, rigidBodyA, rigidBodyB, contact);
                }
            }
        }
        
    public:
        CollisionSystem(ecs::Manager* eCSManager) : ISystem(eCSManager) {}
        
        void Initialize(SceneData* sceneData, InputManager* inputManager, ResourceManager* resourceManager) override
        {
            WArray<AABB> boundingBoxes;
            WArray<String> names;

            for (auto [transformEntity, transform, collider, meshComponent] : ECSManager->EntitiesWith<Transform, ColliderComponent, MeshComponent>())
            {
                boundingBoxes.Add(meshComponent.Mesh->BBox.GetTransformed(transform));
                names.Add(meshComponent.Mesh->Name);
            }

            if(boundingBoxes.IsEmpty())
            {
                return;
            }
            
            RootNode = BuildBVH(boundingBoxes, names, 0, boundingBoxes.Num());
        }

        void Update(float deltaTime) override
        {
            WArray<AABB> boundingBoxes;
            WArray<Transform*> transforms;
            WArray<ColliderComponent*> colliders;
            WArray<RigidBody*> rigidBodies;

            for (auto [transformEntity, transform, collider, rigidBody, meshComponent] : ECSManager->EntitiesWith<Transform, ColliderComponent, RigidBody, MeshComponent>())
            {
                boundingBoxes.Add(meshComponent.Mesh->BBox.GetTransformed(transform));
                collider.IsColliding = false;
                colliders.Add(&collider);
                rigidBodies.Add(&rigidBody);
                transforms.Add(&transform);
            }

            if(boundingBoxes.IsEmpty())
            {
                return;
            }

            UpdateBVH(RootNode, boundingBoxes);
            
            WArray<CollisionPair> collisions;

            BroadPhaseCollision(RootNode, RootNode, collisions);
            NarrowPhaseCollision(collisions, colliders, rigidBodies, transforms);
        }
    };
}
