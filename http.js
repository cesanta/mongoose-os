// Copyright (c) 2014 Cesanta Software
// All rights reserved

function parseHttpRequest(str) {
  var ind = str.indexOf('\r\n\r\n');              // Where HTTP headers end
  if (ind < 0 && str.length > 8192) return null;  // Request too big
  if (ind < 14 && ind > 0) return null;           // Request too small
  if (ind > 0) {
    var request = str.substr(0, ind + 1);
    var lines = request.split('\r\n');
    var firstLine = lines[0].split(' ');
    return { request: request, method: firstLine[0], uri: firstLine[1] };
  }
  return {};
};