// Minifies the phone-side JS bundle in place, after the SDK has built it.
// The source map is carried over, so stack traces still point to the
// sources. Uses the uglify-js of the Pebble SDK.
// Run with: node tools/minify.js <SDK node_modules> <bundle.js> <bundle.js.map>
var fs = require('fs');
var path = require('path');

var uglify = require(path.join(process.argv[2], 'uglify-js'));
var jsPath = process.argv[3];
var mapPath = process.argv[4];

var inMap = JSON.parse(fs.readFileSync(mapPath, 'utf8'));
var result = uglify.minify(
    jsPath, {inSourceMap: inMap, outSourceMap: path.basename(mapPath)});

// uglify-js keeps the mappings to the sources, but not their text
var outMap = JSON.parse(result.map);
if (inMap.sourcesContent) {
  outMap.sourcesContent = outMap.sources.map(function(source) {
    var index = inMap.sources.indexOf(source);
    return index >= 0 ? inMap.sourcesContent[index] : null;
  });
}

fs.writeFileSync(jsPath, result.code);
fs.writeFileSync(mapPath, JSON.stringify(outMap));
