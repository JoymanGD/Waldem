#pragma once

namespace Waldem
{
	namespace Math
	{
		void ScreenBoxOfTri(const Vector3& v0, const Vector3& v1, const Vector3& v2, Vector2& topLeft, Vector2& bottomRight);

		int ScreenAreaOfTri(const Vector3 &a, const Vector3 &b, const Vector3 & c);
		float FloatAreaOfTri(const Vector3 &a, const Vector3 &b, const Vector3 & c);

		float CrossAreaOfTri(const Vector3 &a, const Vector3 &b, const Vector3 & c);

		// Given segment ab and point c, computes closest point d on ab. 
		// Also returns d, d(t)=a+ t*(b - a)
		void ClosestPtPointSegment(const Vector3& c, const Vector3& a, const Vector3& b, Vector3& d);

		// Computes closest points C1 and C2 of S1(s)=P1+s*(Q1-P1) and 
		// S2(t)=P2+t*(Q2-P2), returning s and t. Function result is squared 
		// distance between between S1(s) and S2(t)
		float ClosestPtSegmentSegment(const Vector3& p1, const Vector3& q1, const Vector3& p2, const Vector3& q2, Vector3& c1, Vector3& c2);

		// Returns the squared distance between point c and segment ab
		float SqDistPointSegment(const Vector3& a, const Vector3& b, const Vector3& c);

		float DistPointSegment(const Vector3& a, const Vector3& b, const Vector3& c);

		float LineToLineDistance(const Vector3& a1, const Vector3& a2, const Vector3& b1, const Vector3& b2);
		float LineToLineDistance(const Vector3& a1, const Vector3& a2, const Vector3& b1, const Vector3& b2, Vector3& a, Vector3& b);
		float PointToLineDistance(const Vector3& p1, const Vector3& p2, const Vector3& p);

		void Barycentric(const Vector3 & a, const Vector3 & b, const Vector3 & c, const Vector3 & p, float& u, float& v, float& w);
	}
}