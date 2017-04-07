load('api_rpc.js');

RPC.addHandler('Example.Increment', function(args) {
  if (args !== undefined && args.num !== undefined) {
    return {num: args.num + 1};
  } else {
    return {error: 'num is required'};
  }
}, null);
