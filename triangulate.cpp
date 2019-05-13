#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <iterator>
#include <map>
#include <utility>
#include <vector>
#include <set>
#include <stdexcept>
#include <string>

using std::pair;
using std::vector;
using std::set;

struct point { // Represents a 2D point (x, y)
	double x, y;
	point(double x, double y) : x(x), y(y) {}
	bool operator==(const point& other) const { return x == other.x && y == other.y; }
};
typedef vector<point> polygon; // Polygon in cyclic order of points


double ccw(point a, point b, point c) {
	// Returns a positive number if the points a, b, c are in counterclockwise order
	return (b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y);
}


bool isect(point a, point b, point c, point d) {
	// Check if segments a--b and c--d intersect
	if (std::max(a.x, b.x) < std::min(c.x, d.x) ||
		std::max(c.x, d.x) < std::min(a.x, b.x) ||
		std::max(a.y, b.y) < std::min(c.y, d.y) ||
		std::max(c.y, d.y) < std::min(a.y, b.y))
		return false;
	return ccw(a, c, b) * ccw(a, d, b) <= 0 && ccw(c, a, d) * ccw(c, b, d) <= 0;
}


bool intriangle(point a, point b, point c, point p) {
	// Check if p is strictly within triangle abc
	double x = ccw(p, a, b);
	double y = ccw(p, b, c);
	double z = ccw(p, c, a);
	return x * y > 0 && y * z > 0 && z * x > 0;
}


double area(const polygon& p) {
	// Returns the signed area of p, positive if oriented counterclockwise
	double area = 0.0;
	for (int i = 0; i < p.size(); i++) {
		area += p[i].x * p[(i + 1) % p.size()].y;
		area -= p[i].y * p[(i + 1) % p.size()].x;
	}
	return area / 2;
}


vector<polygon> triangulate_one(polygon p) {
	// Triangulate a single polygon without holes in O(n^2)
	int n = p.size();
	vector<int> pre(n), nxt(n); // polygon as circular DLL
	set<int> candidates; // candidate ear vertices to check
	for (int i = 0; i < n; i++) {
		pre[i] = (i + n - 1) % n;
		nxt[i] = (i + 1) % n;
		if (ccw(p[pre[i]], p[i], p[nxt[i]]) > 0)
			candidates.insert(i);
	}

	vector<polygon> result;
	while (result.size() < n - 2 && candidates.size()) {
		int k = *candidates.begin();
		candidates.erase(k);
		if (ccw(p[pre[k]], p[k], p[nxt[k]]) <= 0)
			continue;
		bool ear = true;
		for (int d = nxt[nxt[k]]; d != pre[k]; d = nxt[d]) {
			if (intriangle(p[pre[k]], p[k], p[nxt[k]], p[d])) {
				ear = false;
				break;
			}
		}
		if (ear) {
			result.push_back({ p[pre[k]], p[k], p[nxt[k]] });
			nxt[pre[k]] = nxt[k];
			pre[nxt[k]] = pre[k];
			candidates.insert(pre[k]);
			candidates.insert(nxt[k]);
		}
	}

	return result;
}


void merge_hole(polygon& p, const polygon& hole) {
	// Finds and connects mutually-visible vertices to merge a hole into polygon p
	auto rightmost = hole.begin();
	for (auto it = hole.begin(); it != hole.end(); it++) {
		if (it->x > rightmost->x)
			rightmost = it;
	}

	point m = *rightmost;
	auto visible = p.end();
	double minx = INFINITY;
	for (int i = 0, j = p.size() - 1; i < p.size(); j = i++) {
		if ((p[i].y - m.y) * (p[j].y - m.y) < 0) { // intersects a segment
			double x = p[i].x +
				(p[j].x - p[i].x) * (m.y - p[i].y) / (p[j].y - p[i].y);
			if (x > m.x && x < minx) {
				minx = x;
				visible = p.begin() + (p[i].x > p[j].x ? i : j);
			}
		}
		if (p[i].y == m.y && p[i].x > m.x && p[i].x < minx) { // intersects a point
			minx = p[i].x;
			visible = p.begin() + i;
		}
	}

	if (visible->y != m.y) {
		point r(minx, m.y);
		point q = *visible;
		double sgn = ccw(r, m, q);
		for (auto it = p.begin(); it != p.end(); it++) {
			if (intriangle(m, r, q, *it) || (ccw(m, q, *it) == 0 && it->x > m.x)) {
				double prod = ccw(*it, m, *visible);
				if (sgn * prod > 0 || (prod == 0 && it->x < visible->x))
					visible = it;
			}
		}
	}

	// If visible is repeated from a past merge, find the proper index for it
	for (int i = 0; i < p.size(); i++) {
		if (p[i] == *visible) {
			point t = p[(i + 1) % p.size()];
			if (t.x > visible->x || ccw(m, *visible, t) > 0) {
				visible = p.begin() + i;
				break;
			}
		}
	}

	polygon inside;
	inside.reserve(hole.size() + 2);
	inside.push_back(*visible);
	inside.insert(inside.end(), rightmost, hole.end());
	inside.insert(inside.end(), hole.begin(), rightmost + 1);
	p.insert(visible, inside.begin(), inside.end());
}


vector<polygon> triangulate_with_holes(polygon p, vector<polygon> holes) {
	// Triangulates a polygon with some holes
	vector<pair<double, int>> rightmost;
	for (int i = 0; i < holes.size(); i++) {
		double mx = holes[i][0].x;
		for (const point& q : holes[i])
			mx = std::max(mx, q.x);
		rightmost.emplace_back(mx, i);
	}
	std::sort(rightmost.begin(), rightmost.end());
	for (auto it = rightmost.rbegin(); it != rightmost.rend(); it++) {
		merge_hole(p, holes[it->second]);
	}
	return triangulate_one(p);
}


bool inpolygon(const polygon& p, point q) {
	// Check if point q is in polygon p (undetermined if exactly on the edge)
	int n = p.size();
	bool result = false;
	for (int i = 0, j = n - 1; i < n; j = i++) {
		if ((p[i].y > q.y) != (p[j].y > q.y) &&
			q.x < p[i].x + (p[j].x - p[i].x) * (q.y - p[i].y) / (p[j].y - p[i].y))
			result = !result;
	}
	return result;
}


vector<polygon> triangulate(vector<polygon> polygons) {
	// TODO validate input by ensuring no intersections or self-intersections

	int m = polygons.size();
	vector<vector<int>> subtree(m);
	vector<int> depth(m);
	for (int i = 0; i < m; i++) {
		for (int j = 0; j < m; j++) {
			if (i != j && inpolygon(polygons[i], polygons[j][0])) {
				subtree[i].push_back(j);
				depth[j]++;
			}
		}
	}

	for (int i = 0; i < m; i++) {
		if ((area(polygons[i]) < 0) != (depth[i] % 2)) // outer CCW, inner CW
			std::reverse(polygons[i].begin(), polygons[i].end());
	}

	vector<polygon> triangles;
	for (int i = 0; i < m; i++) {
		if (depth[i] % 2 == 0) { // outer polygon
			vector<polygon> holes;
			for (int j : subtree[i]) {
				if (depth[j] == depth[i] + 1)
					holes.push_back(std::move(polygons[j]));
			}
			auto result = triangulate_with_holes(std::move(polygons[i]), std::move(holes));
			triangles.insert(triangles.end(), result.begin(), result.end());
		}
	}
	return triangles;
}


extern "C" // Endpoint for emscripten
int triangulate(int num_polygons, double* data, double* result) {
	vector<polygon> polygons;
	for (int i = 0; i < num_polygons; i++) {
		polygon cur_polygon;
		while (!std::isnan(*data)) {
			double x = *data++;
			double y = *data++;
			cur_polygon.emplace_back(x, y);
		}
		data++;
		polygons.push_back(cur_polygon);
	}
	auto triangles = triangulate(polygons);
	for (const polygon& triangle : triangles) {
		for (const point& p : triangle) {
			*result++ = p.x;
			*result++ = p.y;
		}
	}
	return triangles.size();
}
