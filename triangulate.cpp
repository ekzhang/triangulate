#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <iterator>
#include <map>
#include <utility>
#include <vector>
#include <string>

using std::pair;
using std::vector;

typedef pair<double, double> point; // Represents a 2D point (x, y)
typedef vector<point> polygon; // Polygon in cyclic order of points


double ccw(point a, point b, point c) {
	// Returns a positive number if the points a, b, c are in counterclockwise order
	return (b.first - a.first) * (c.second - a.second)
		- (c.first - a.first) * (b.second - a.second);
}


bool isect(point a, point b, point c, point d) {
	// Check if segments a--b and c--d intersect
	return ccw(a, c, b) * ccw(a, d, b) <= 0 && ccw(b, a, d) * ccw(b, c, d) <= 0;
}


bool intriangle(point a, point b, point c, point p) {
	// Check if p is within triangle abc
	double x = ccw(p, a, b);
	double y = ccw(p, b, c);
	double z = ccw(p, c, a);
	return x * y >= 0 && y * z >= 0 && z * x >= 0;
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


int get_ear(const polygon& p) {
	// Returns the index of an ear of the polygon
	assert(p.size() > 3);
	int n = p.size();
	for (int i = 0; i < n; i++) {
		int j = (i + n - 1) % n;
		int k = (i + 1) % n;
		if (ccw(p[j], p[i], p[k]) < 0)
			continue;
		bool ear = true;
		for (int d = 0; d < n; d++) {
			if (d != i && d != j && d != k && intriangle(p[j], p[i], p[k], p[d])) {
				ear = false;
				break;
			}
		}
		if (ear)
			return i;
	}
	return -1;
}


vector<polygon> triangulate_one(polygon p) {
	// Triangulate a single polygon without holes
	vector<polygon> result;
	while (p.size() > 3) {
		int n = p.size();
		int i = get_ear(p);
		result.push_back({ p[(i + n - 1) % n], p[i], p[(i + 1) % n] });
		p.erase(p.begin() + i);
	}
	result.push_back(p);
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
