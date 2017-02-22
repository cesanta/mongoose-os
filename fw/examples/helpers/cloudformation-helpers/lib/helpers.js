var Promise = require('bluebird');

// Translates from raw JSON into DynamoDB-formatted JSON. This is more than a
// convenience thing: the original iteration of this accepted DyanamoDB-JSON Items,
// to avoid complications in translation. But when CloudFormation passes the parameters
// throught the event model, all non-string values get wrapped in quotes. For Booleans
// specifically, this is a problem - because DynamoDB does not allow a string (even if
// it is 'true' or 'false') as a 'BOOL' value. So some translation was needed, and
// it seemed best to then simplify things for the client by accepting raw JSON.
exports.formatForDynamo = function(value, topLevel) {
  var result = undefined;
  if (value == 'true' || value == 'false') {
    result = {'BOOL': value == 'true'}
  } else if (!isNaN(value) && value.trim() != '') {
    result = {'N': value}
  } else if (Array.isArray(value)) {
    var arr = [];
    for (var i = 0; i < value.length; i++) {
      arr.push(exports.formatForDynamo(value[i], false));
    }
    result = {'L': arr};
  } else if (typeof value  === "object") {
    var map = {};
    Object.keys(value).forEach(function(key) {
      map[key] = exports.formatForDynamo(value[key], false)
    });
    if (topLevel) result = map;
    else result = {'M': map}
  } else {
    result = {'S': value}
  }
  return result;
}

exports.formatFromDynamo = function(value) {
  var result = undefined;
  if (typeof value === "string" || typeof value === 'boolean' || typeof value === 'number') {
    result = value;
  } else if (Array.isArray(value)) {
    var arr = [];
    for (var i = 0; i < value.length; i++) {
      arr.push(exports.formatFromDynamo(value[i]));
    }
    result = arr;
  } else if (typeof value  === "object") {
    var map = {};
    Object.keys(value).forEach(function(key) {
      var v = exports.formatFromDynamo(value[key]);
      switch (key) {
        case 'B':
          throw "Unsupported Mongo type [B]";
        case 'BOOL':
          result = (v == 'true');
          break;
        case 'BS':
          throw "Unsupported Mongo type [BS]";
        case 'L':
          result = v;
          break;
        case 'M':
          result = v;
          break;
        case 'N':
          result = Number(v);
          break;
        case 'NS':
          result = [];
          for (var i = 0; i < v.length; i++) {
            result.push(Number(value[i]));
          };
          break;
        case 'S':
          result = v;
          break;
        case 'SS':
          result = v;
          break;
        default:
          map[key] = v;
      }
      if (result === undefined)
        result = map;
    });
  } else {
    throw "Unrecognized type [" + (typeof value) + "]";
  }
  return result;
}

if (!String.prototype.endsWith) {
  String.prototype.endsWith = function(searchString, position) {
      var subjectString = this.toString();
      if (typeof position !== 'number' || !isFinite(position) || Math.floor(position) !== position || position > subjectString.length) {
        position = subjectString.length;
      }
      position -= searchString.length;
      var lastIndex = subjectString.indexOf(searchString, position);
      return lastIndex !== -1 && lastIndex === position;
  };
}