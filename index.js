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
	const buf = makeF64Buf(new Float64Array(6 * numPoints));
	
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

Module.onRuntimeInitialized = function() {
	document.getElementById('loading').style.display = 'none';
	document.getElementsByTagName('main')[0].style.display = 'block';
	const canvas = document.getElementById('canvas');
	const ctx = canvas.getContext('2d');

	function selectPath(polygon) {
		ctx.beginPath();
		ctx.moveTo(...polygon[0]);
		for (let i = 1; i < polygon.length; i++)
			ctx.lineTo(...polygon[i]);
		ctx.closePath();
	}

	function draw(polygon) {
		selectPath(polygon);
		ctx.fillStyle = `hsl(220, 100%, ${25 + Math.random() * 50}%)`;
		ctx.stroke();
		ctx.fill();
	}

	const pentagon = [[100, 100], [100, 200], [200, 300], [300, 200], [300, 100]];
	const triangles = triangulate([pentagon]);
	for (const triangle of triangles)
		draw(triangle);
	// console.log(JSON.stringify(triangulate([pentagon])));
}
