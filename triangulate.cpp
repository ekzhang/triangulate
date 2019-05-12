#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <iterator>
#include <map>
#include <utility>
#include <vector>
#include <set>
#include <string>

using std::pair;
using std::vector;
using std::set;

typedef pair<double, double> point; // Represents a 2D point (x, y)
typedef vector<point> polygon; // Polygon in cyclic order of points


double ccw(point a, point b, point c) {
	// Returns a positive number if the points a, b, c are in counterclockwise order
	return (b.first - a.first) * (c.second - a.second)
		- (c.first - a.first) * (b.second - a.second);
}


bool isect(point a, point b, point c, point d) {
	// Check if segments a--b and c--d intersect
	if (std::max(a.first, b.first) < std::min(c.first, d.first)
		|| std::max(c.first, d.first) < std::min(a.first, b.first)
		|| std::max(a.second, b.second) < std::min(c.second, d.second)
		|| std::max(c.second, d.second) < std::min(a.second, b.second))
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
		area += p[i].first * p[(i + 1) % p.size()].second;
		area -= p[i].second * p[(i + 1) % p.size()].first;
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


vector<polygon> triangulate(vector<polygon> polygons) {
	// TODO support multiple polygons
	// Preprocessing requires a way to place them into a tree structure
	// and also orient them correctly, then add visible connections, etc.

	// vector<polygon> ret;
	// ret.push_back({{2, 3}, {3, 4}, {5, 6}});
	// ret.push_back({{1, 1}, {2, 2}, {3, 3}});
	if (area(polygons[0]) < 0) {
		std::reverse(polygons[0].begin(), polygons[0].end());
	}
	return triangulate_one(polygons[0]);
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
			*result++ = p.first;
			*result++ = p.second;
		}
	}
	return triangles.size();
}
