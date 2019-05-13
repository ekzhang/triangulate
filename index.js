const $ = document.getElementById.bind(document);

const triangulate = (function() {
	const c_triangulate = Module.cwrap('triangulate', 'number', ['number', 'number', 'number']);

	function makeF64Buf(array) {
		// Allocates and returns a copy of a javascript array or typed array
		// as a C array of doubles
		const offset = Module._malloc(array.length * 8);
		Module.HEAPF64.set(array, offset / 8);
		return {
			data: Module.HEAPF64.subarray(offset / 8, offset / 8 + array.length),
			offset
		};
	}

	function encodePolygons(polygons) {
		// Given an array of polygons, flatten and convert it to a C array
		const data = [];
		for (const polygon of polygons) {
			for (const [x, y] of polygon)
				data.push(x, y);
			data.push(NaN);
		}
		return makeF64Buf(data);
	}

	function triangulate(polygons) {
		// Wrapper around c_triangulate
		const numPoints = polygons.flat().length;
		const data = encodePolygons(polygons);
		const buf = makeF64Buf(new Float64Array(6 * 2 * numPoints));

		const numTriangles = c_triangulate(polygons.length, data.offset, buf.offset);
		const result = [];
		for (let i = 0; i < numTriangles; i++) {
			result.push([
				[buf.data[6 * i], buf.data[6 * i + 1]],
				[buf.data[6 * i + 2], buf.data[6 * i + 3]],
				[buf.data[6 * i + 4], buf.data[6 * i + 5]],
			]);
		}

		Module._free(data.offset);
		Module._free(buf.offset);
		return result;
	}

	return triangulate;
})();


function ccw([x1, y1], [x2, y2], [x3, y3]) {
	return (x2 - x1) * (y3 - y1) - (x3 - x1) * (y2 - y1);
}

function isect(a, b, c, d) {
	if (Math.max(a[0], b[0]) < Math.min(c[0], d[0])
		|| Math.max(c[0], d[0]) < Math.min(a[0], b[0])
		|| Math.max(a[1], b[1]) < Math.min(c[1], d[1])
		|| Math.max(c[1], d[1]) < Math.min(a[1], b[1]))
		return false;
	return ccw(a, c, b) * ccw(a, d, b) <= 0 && ccw(c, a, d) * ccw(c, b, d) <= 0;
}

function iseg(a, b, p) {
	if (p[0] > Math.max(a[0], b[0]) || p[0] < Math.min(a[0], b[0])
		|| p[1] > Math.max(a[1], b[1]) || p[1] < Math.min(a[1], b[1]))
		return false;
	return ccw(a, p, b) == 0;
}

function intriangle(a, b, c, p) {
	const x = ccw(p, a, b);
	const y = ccw(p, b, c);
	const z = ccw(p, c, a);
	return x * y > 0 && y * z > 0 && z * x > 0;
}


Module.onRuntimeInitialized = function() {
	$('loading').style.display = 'none';
	$('main').style.display = 'block';
	const canvas = $('canvas');
	const ctx = canvas.getContext('2d');

	function selectPath(polygon, closePath=true) {
		ctx.beginPath();
		ctx.moveTo(...polygon[0]);
		for (let i = 1; i < polygon.length; i++)
			ctx.lineTo(...polygon[i]);
		if (closePath)
			ctx.closePath();
	}

	function draw(polygon, fill) {
		selectPath(polygon);
		if (fill) {
			ctx.fillStyle = fill;
			ctx.fill();
		}
		ctx.stroke();
	}

	let mode = 'draw';
	let currentPolygon = null;
	let completedPolygons = [];
	let triangles = null;
	let mouse = [0, 0];

	canvas.addEventListener('mousemove', event => {
		const rect = canvas.getBoundingClientRect();
		mouse = [event.clientX - rect.left, event.clientY - rect.top];
	});

	canvas.addEventListener('click', event => {
		event.preventDefault();
		if (mode === 'draw') {
			let ok = true;
			for (const p of completedPolygons) {
				for (let i = 0; i < p.length; i++) {
					if (iseg(p[i], p[(i + 1) % p.length], mouse))
						ok = false;
				}
			}

			if (ok) {
				if (!currentPolygon)
					currentPolygon = [mouse];
				else {
					if (mouse[0] === currentPolygon[0][0] && mouse[1] === currentPolygon[0][1]
						|| currentPolygon.length >= 2 && iseg(...currentPolygon.slice(-2), mouse))
						ok = false;
					const last = currentPolygon[currentPolygon.length - 1];
					for (let i = 0; i < currentPolygon.length - 2; i++) {
						if (isect(currentPolygon[i], currentPolygon[i + 1], mouse, last))
							ok = false;
					}
					for (const p of completedPolygons) {
						for (let i = 0; i < p.length; i++) {
							if (isect(p[i], p[(i + 1) % p.length], mouse, last))
								ok = false;
						}
					}
					if (ok)
						currentPolygon.push(mouse);
				}
			}
		}
	});

	canvas.addEventListener('contextmenu', event => {
		event.preventDefault();
		if (mode === 'draw') {
			if (currentPolygon && currentPolygon.length >= 3) {
				let ok = true;
				const p = currentPolygon[0], q = currentPolygon[currentPolygon.length - 1];
				if (iseg(p, q, currentPolygon[1])
					|| iseg(p, q, currentPolygon[currentPolygon.length - 2]))
					ok = false;
				for (let i = 1; i < currentPolygon.length - 2; i++) {
					if (isect(currentPolygon[i], currentPolygon[i + 1], p, q))
						ok = false;
				}
				for (const poly of completedPolygons) {
					for (let i = 0; i < poly.length; i++) {
						if (isect(poly[i], poly[(i + 1) % poly.length], p, q))
							ok = false;
					}
				}

				if (ok) {
					completedPolygons.push(currentPolygon);
					currentPolygon = null;
				}
			}
		}
	});

	$('triangulate-btn').addEventListener('click', () => {
		if (!triangles && completedPolygons.length) {
			triangles = triangulate(completedPolygons);
			mode = 'display';
		}
	});

	$('reset-btn').addEventListener('click', () => {
		completedPolygons = [];
		currentPolygon = null;
		triangles = null;
		mode = 'draw';
	});

	const helpText = [
		"Click to add points, right-click to complete the polygon.",
		"Try adding holes and nested polygons as well!",
		"When you're ready, press the \"Triangulate\" button below.",
	];

	requestAnimationFrame(function paint() {
		requestAnimationFrame(paint);
		ctx.clearRect(0, 0, canvas.width, canvas.height);
		if (mode === 'draw') {
			if (completedPolygons.length === 0 && !currentPolygon) {
				// Show help text
				ctx.font = '24px sans-serif';
				ctx.textAlign = 'center';
				ctx.fillStyle = 'black';
				ctx.fillText(helpText[0], canvas.width / 2, canvas.height / 2 - 40);
				ctx.fillText(helpText[1], canvas.width / 2, canvas.height / 2);
				ctx.fillText(helpText[2], canvas.width / 2, canvas.height / 2 + 40);
			}
			for (const polygon of completedPolygons)
				draw(polygon);
			if (currentPolygon) {
				selectPath(currentPolygon, false);
				ctx.stroke();
				ctx.setLineDash([2, 3]);
				ctx.lineTo(...mouse);
				ctx.stroke();
				ctx.setLineDash([0, 0]);
			}
		}
		else {
			for (let i = 0; i < triangles.length; i++) {
				let hue = Date.now() / 100;
				let lightness = 30 + 40 * ((0.618 * i + 0.5) % 1);
				if (intriangle(...triangles[i], mouse))
					hue -= 180, lightness = 50;
				draw(triangles[i], `hsl(${hue % 360}, 100%, ${lightness}%)`);
			}
		}
	});
}
