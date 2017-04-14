load('api_rpc.js');

RPC.addHandler('Example.Increment', function(args) {
  if (args !== undefined && args.num !== undefined) {
    return {num: args.num + 1};
  } else {
    return {error: 'num is required'};
  }
}, null);

RPC.call(RPC.LOCAL, "Example.Increment", {"num": 100}, function (resp, ud) {
  print("Local callback response:", JSON.stringify(resp));
}, null);
